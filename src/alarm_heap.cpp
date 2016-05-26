/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2016 Metaswitch Networks Ltd
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version, along with the "Special Exception" for use of
* the program along with SSL, set forth below. This program is distributed
* in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more
* details. You should have received a copy of the GNU General Public
* License along with this program. If not, see
* <http://www.gnu.org/licenses/>.
*
* The author can be reached by email at clearwater@metaswitch.com or by
* post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
*
* Special Exception
* Metaswitch Networks Ltd grants you permission to copy, modify,
* propagate, and distribute a work formed by combining OpenSSL with The
* Software, or a work derivative of such a combination, even if such
* copying, modification, propagation, or distribution would otherwise
* violate the terms of the GPL. You must comply with the GPL in all
* respects for all of the code used other than OpenSSL.
* "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
* Project and licensed under the OpenSSL Licenses, or a work based on such
* software and licensed under the OpenSSL Licenses.
* "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
* under which the OpenSSL Project distributes the OpenSSL toolkit software,
* as those licenses appear in the file LICENSE-OPENSSL.
*/

#include <stdexcept>
#include <time.h>
#include <cmath>
#include <arpa/inet.h>

#include "log.h"
#include "alarm_heap.hpp"
#include "itu_alarm_table.hpp"
#include "alarm_active_table.hpp"

AlarmInfo::~AlarmInfo()
{
  delete _alarm_in_heap; _alarm_in_heap = NULL;
}

AlarmSeverityChange AlarmInfo::new_alarm_severity(AlarmDef::Severity severity)
{
  // Order the severities so we can do a simple comparison to determine any
  // severity change.
  unsigned int ordered_severities[] = {0, 1, 2, 6, 5, 4, 3};
  int old_severity = ordered_severities[_severity];
  int new_severity = ordered_severities[severity];

  if (old_severity == new_severity)
  {
    return AlarmSeverityChange::NO_CHANGE;
  }
  else if (old_severity > new_severity)
  {
    return AlarmSeverityChange::REDUCED_SEVERITY;
  }
  else
  {
    return AlarmSeverityChange::INCREASED_SEVERITY;
  }
}

void AlarmInfo::set_severity(AlarmTableDef& alarm_table_def)
{
  // Update the severity in the map.
  _severity = alarm_table_def.severity();

  // Update the ActiveAlarmTable.
  alarmActiveTable_trap_handler(alarm_table_def);
}

void AlarmInfo::update_alarm_in_heap(AlarmDef::Severity new_severity,
                                     uint64_t time_to_delay)
{
  // Update the alarm in the heap's severity, and update its time to pop
  // (which also rebalances the heap).
  if (new_severity != _alarm_in_heap->severity())
  {
    _alarm_in_heap->set_severity(new_severity);
    _alarm_in_heap->update_pop_time(time_to_delay);
  }
}

AlarmHeap::AlarmHeap(AlarmTableDefs* alarm_table_defs) :
  _terminated(false),
  _alarm_table_defs(alarm_table_defs),
  _alarm_trap_sender(new AlarmTrapSender(this))
{
  // Create the lock/condition variables.
  pthread_mutex_init(&_lock, NULL);
  _cond = new CondVar(&_lock);

  // Populate the alarm state map
  for (AlarmTableDefsIterator it = _alarm_table_defs->begin();
                              it != _alarm_table_defs->end();
                              it++)
  {
    AllAlarmsStateMap::iterator index = _all_alarms_state.find(it->alarm_index());

    if (index == _all_alarms_state.end())
    {
      // We only have one entry for each alarm index (not one per severity).
      AlarmInHeap* alarm_in_heap = new AlarmInHeap(it->alarm_index());
      AlarmInfo* alarm_info = new AlarmInfo(alarm_in_heap,
                                            AlarmDef::Severity::UNDEFINED_SEVERITY);
      _all_alarms_state.insert(std::pair<unsigned int, AlarmInfo*>(it->alarm_index(),
                                                                   alarm_info));
    }
  }

  // Finally, create the heap thread. This covers getting any alarms to send
  // from the heap and passing them to the trap sender to actually send.
  int rc = pthread_create(&_heap_sender_thread, NULL, heap_sender_function, this);

  if (rc < 0)
  {
    // LCOV_EXCL_START
    printf("Failed to start heap pop thread: %s", strerror(errno));
    exit(2);
    // LCOV_EXCL_STOP
  }
}

AlarmHeap::~AlarmHeap()
{
  pthread_mutex_lock(&_lock);
  _terminated = true;
  _cond->signal();
  pthread_mutex_unlock(&_lock);

  pthread_join(_heap_sender_thread, NULL);
  delete _cond; _cond = NULL;

  pthread_mutex_destroy(&_lock);

  delete _alarm_trap_sender; _alarm_trap_sender = NULL;
  _alarm_heap.clear();

  for (AllAlarmsStateMap::iterator it = _all_alarms_state.begin();
                               it != _all_alarms_state.end();
                               it++)
  {
    delete (*it).second;
  }
}

void* AlarmHeap::heap_sender_function(void* data)
{
  ((AlarmHeap*)data)->heap_sender();
  return NULL;
}

void AlarmHeap::heap_sender()
{
  pthread_mutex_lock(&_lock);

  while (!_terminated)
  {
    uint64_t time_now_in_ms = Utils::get_time();
    AlarmInHeap* alarm_in_heap = (AlarmInHeap*)_alarm_heap.get_next_timer();

    while ((alarm_in_heap) &&
           (alarm_in_heap->get_pop_time() <= time_now_in_ms))
    {
      // While the next alarm in the heap is due to be sent, we:
      //  - Pull out the alarm definition for its index and severity.
      //  - Pass the alarm definition to the trap sender (which is responsible
      //    for actually sending any INFORMs).
      //  - Update the master view of the current severity for this alarm (by
      //    updating the _all_alarms_state map).
      //  - Finally, we then remove the alarm from the heap.
      AlarmTableDef& alarm_table_def =
              _alarm_table_defs->get_definition(alarm_in_heap->index(),
                                                alarm_in_heap->severity());

      if (alarm_table_def.is_valid())
      {
        _alarm_trap_sender->send_trap(alarm_table_def);
        AllAlarmsStateMap::iterator alarm = _all_alarms_state.find(alarm_in_heap->index());

        if (alarm != _all_alarms_state.end())
        {
          (*alarm).second->set_severity(alarm_table_def);
        }
      }

      alarm_in_heap->remove_from_heap();
      alarm_in_heap = (AlarmInHeap*)_alarm_heap.get_next_timer();
    }

    // The next alarm in the heap isn't due to pop yet. Wait until it's due.
    if (alarm_in_heap)
    {
      // The next alarm in the heap isn't due to pop yet. Wait until it's due.
      struct timespec time;
      time.tv_sec = alarm_in_heap->get_pop_time() / 1000;
      time.tv_nsec = (alarm_in_heap->get_pop_time() % 1000) * 1000000;
      _cond->timedwait(&time);
    }
    else
    {
      // There's no alarms in the heap. Wait until we're signalled.
      _cond->wait();
    }
  }

  pthread_mutex_unlock(&_lock);
}

void AlarmHeap::update_alarm_in_heap(AlarmInfo* alarm_info,
                                     AlarmDef::Severity severity,
                                     uint64_t alarm_pop_time)
{
  _alarm_heap.insert(alarm_info->alarm_in_heap());
  alarm_info->update_alarm_in_heap(severity, alarm_pop_time);
}

void AlarmHeap::issue_alarm(const std::string& issuer,
                            const std::string& identifier)
{
  unsigned int index;
  unsigned int severity;
  if (sscanf(identifier.c_str(), "%u.%u", &index, &severity) != 2)
  {
    TRC_ERROR("malformed alarm identifier: %s", identifier.c_str());
    return;
  }

  AlarmTableDef& alarm_table_def = _alarm_table_defs->get_definition(index,
                                                                     severity);

  if (alarm_table_def.is_valid())
  {
    pthread_mutex_lock(&_lock);

    AllAlarmsStateMap::iterator alarm = _all_alarms_state.find(index);

    if (alarm != _all_alarms_state.end())
    {
      AlarmDef::Severity severity_as_enum = (AlarmDef::Severity)severity;
      AlarmSeverityChange severity_change =
                  (*alarm).second->new_alarm_severity(severity_as_enum);

      if (severity_change == AlarmSeverityChange::INCREASED_SEVERITY)
      {
        TRC_DEBUG("Severity of the alarm %lu has increased", index);
        update_alarm_in_heap((*alarm).second, severity_as_enum, ALARM_INCREASED);
        _cond->signal();
      }
      else if (severity_change == AlarmSeverityChange::REDUCED_SEVERITY)
      {
        TRC_DEBUG("Severity of the alarm %lu has reduced", index);
        _alarm_heap.insert((*alarm).second->alarm_in_heap());
        update_alarm_in_heap((*alarm).second, severity_as_enum, ALARM_REDUCED);
        _cond->signal();
      }
      else
      {
        // The severity of the alarm hasn't changed. Remove it from the heap
        // (safe to call even if it's already not in the heap).
        _alarm_heap.remove((*alarm).second->alarm_in_heap());
      }
    }

    pthread_mutex_unlock(&_lock);
  }
  else
  {
    TRC_ERROR("Unknown alarm definition: %s", identifier.c_str());
  }
}

void AlarmHeap::sync_alarms()
{
  TRC_STATUS("Resyncing all alarms");

  pthread_mutex_lock(&_lock);

  for (AllAlarmsStateMap::iterator it = _all_alarms_state.begin();
                               it != _all_alarms_state.end();
                               it++)
  {
    AlarmDef::Severity current_severity = (*it).second->get_severity();

    if (current_severity != AlarmDef::Severity::UNDEFINED_SEVERITY)
    {
      update_alarm_in_heap((*it).second, current_severity, ALARM_RESYNC_DELAY);
    }
  }

  _cond->signal();
  pthread_mutex_unlock(&_lock);
}

void AlarmHeap::handle_failed_alarm(AlarmTableDef& alarm_table_def)
{
  TRC_DEBUG("Handling a failed callback for the alarm: %lu",
            alarm_table_def.alarm_index());

  pthread_mutex_lock(&_lock);

  AllAlarmsStateMap::iterator alarm =
                          _all_alarms_state.find(alarm_table_def.alarm_index());

  if (alarm != _all_alarms_state.end())
  {
    if ((*alarm).second->should_resend_alarm(alarm_table_def.severity()))
    {
      update_alarm_in_heap((*alarm).second,
                           alarm_table_def.severity(),
                           ALARM_RETRY_DELAY);
      _cond->signal();
    }
  }

  pthread_mutex_unlock(&_lock);
}
