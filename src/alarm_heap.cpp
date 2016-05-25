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

AlarmFilter AlarmFilter::_instance;

bool AlarmFilter::alarm_filtered(unsigned int index, unsigned int severity)
{
  unsigned long now = current_time_ms();

  if (now > _clean_time)
  {
    _clean_time = now + CLEAN_FILTER_TIME;

    std::map<AlarmFilterKey, unsigned long>::iterator it = _issue_times.begin();
    while (it != _issue_times.end())
    {
      if (now > (it->second + ALARM_FILTER_TIME))
      {
        _issue_times.erase(it++);
      }
      else
      {
        ++it;
      }
    }
  }

  AlarmFilterKey key(index, severity);
  std::map<AlarmFilterKey, unsigned long>::iterator it = _issue_times.find(key);

  bool filtered = false;

  if (it != _issue_times.end())
  {
    if (now > (it->second + ALARM_FILTER_TIME))
    {
      _issue_times[key] = now;
    }
    else
    {
      filtered = true;
    }
  }
  else
  {
    _issue_times[key] = now;
  }

  return filtered;
}

bool AlarmFilter::AlarmFilterKey::operator<(const AlarmFilter::AlarmFilterKey& rhs) const
{
  return  (_index  < rhs._index) ||
         ((_index == rhs._index) && (_severity < rhs._severity));
}

unsigned long AlarmFilter::current_time_ms()
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

bool AlarmInfo::add_alarm_to_heap(AlarmDef::Severity severity)
{
  return (severity != _state);
}

AlarmHeap::AlarmHeap(AlarmTableDefs* alarm_table_defs) :
  _terminated(false),
  _alarm_table_defs(alarm_table_defs),
  _alarm_trap_sender(new AlarmTrapSender(this))
{
  pthread_mutex_init(&_lock, NULL);
  _cond = new CondVar(&_lock);

  for (AlarmTableDefsIterator it = _alarm_table_defs->begin();
                              it != _alarm_table_defs->end();
                              it++)
  {
    AlarmStateMap::iterator index = _alarm_state.find(it->alarm_index());

    if (index == _alarm_state.end())
    {
      AlarmInHeap* alarm_in_heap = new AlarmInHeap(it->alarm_index());
      _alarm_heap.insert(alarm_in_heap);
      AlarmInfo* alarm_info = new AlarmInfo(alarm_in_heap,
                                            AlarmDef::Severity::UNDEFINED_SEVERITY);
      _alarm_state.insert(std::pair<unsigned int, AlarmInfo*>(it->alarm_index(),
                                                              alarm_info));
    }
  }

  int rc = pthread_create(&_heap_pop_thread, NULL, heap_pop_function, this);

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
  pthread_join(_heap_pop_thread, NULL);
  delete _cond; _cond = NULL;
  pthread_mutex_destroy(&_lock);

  delete _alarm_trap_sender; _alarm_trap_sender = NULL;
  _alarm_heap.clear();

  for (AlarmStateMap::iterator it = _alarm_state.begin();
                               it != _alarm_state.end();
                               it++)
  {
    delete (*it).second;
  }
}

void* AlarmHeap::heap_pop_function(void* data)
{
  ((AlarmHeap*)data)->heap_pop();
  return NULL;
}

void AlarmHeap::heap_pop()
{
  pthread_mutex_lock(&_lock);

  while (!_terminated)
  {
    uint64_t time_now_in_ms = Utils::get_time();
    AlarmInHeap* alarm_in_heap = (AlarmInHeap*)_alarm_heap.get_next_timer();

    while (alarm_in_heap->get_pop_time() <= time_now_in_ms)
    {
      AlarmTableDef& alarm_table_def =
              _alarm_table_defs->get_definition(alarm_in_heap->index(),
                                                alarm_in_heap->severity());

      if (alarm_table_def.is_valid())
      {
        _alarm_trap_sender->send_trap(alarm_table_def);

        AlarmStateMap::iterator alarm = _alarm_state.find(alarm_in_heap->index());
        if (alarm != _alarm_state.end())
        {
          // Update the alarm state map
          (*alarm).second->set_state(alarm_in_heap->severity());
        }
      }

      // Now set the alarm in the heap to ages away, and rebalance the heap
      alarm_in_heap->set_pop_time_to_max();
      alarm_in_heap = (AlarmInHeap*)_alarm_heap.get_next_timer();
    }

    struct timespec time;
    time.tv_sec = time_now_in_ms / 1000;
    time.tv_nsec = (time_now_in_ms % 1000) * 1000000;
    _cond->timedwait(&time);
  }

  pthread_mutex_unlock(&_lock);
}

void AlarmHeap::handle_failed_alarm(AlarmTableDef& alarm_table_def)
{
  pthread_mutex_lock(&_lock);
  AlarmStateMap::iterator alarm = _alarm_state.find(alarm_table_def.alarm_index());
  if (alarm != _alarm_state.end())
  {
    if ((*alarm).second->get_state() == alarm_table_def.severity())
    {
      (*alarm).second->alarm_in_heap()->set_severity(alarm_table_def.severity()); // TODO this needs more checking
      (*alarm).second->alarm_in_heap()->update_pop_time(0);
      _cond->signal();
    }
  }
  pthread_mutex_unlock(&_lock);
}

void AlarmHeap::issue_alarm(const std::string& issuer, const std::string& identifier)
{
  unsigned int index;
  unsigned int severity;
  if (sscanf(identifier.c_str(), "%u.%u", &index, &severity) != 2)
  {
    TRC_ERROR("malformed alarm identifier: %s", identifier.c_str());
    return;
  }

  AlarmTableDef& alarm_table_def = _alarm_table_defs->get_definition(index, severity);

  if (alarm_table_def.is_valid())
  {
    AlarmDef::Severity severity_as_enum = (AlarmDef::Severity)severity;
    // If the alarm to be issued exists in the ObservedAlarms mapping at the
    // same severity then we disregard the alarm as it has not changed state.
    pthread_mutex_lock(&_lock);
    AlarmStateMap::iterator alarm = _alarm_state.find(index);

    if (alarm != _alarm_state.end())
    {
      if ((*alarm).second->add_alarm_to_heap(severity_as_enum))
      {
        if (!AlarmFilter::get_instance().alarm_filtered(index, severity))
        {
          alarmActiveTable_trap_handler(alarm_table_def); // TODO - move to alarm_state_map
          (*alarm).second->alarm_in_heap()->set_severity(severity_as_enum);
          (*alarm).second->alarm_in_heap()->update_pop_time(0);
          _cond->signal();
        }
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
  pthread_mutex_lock(&_lock);
  TRC_STATUS("Resyncing alarms");

  for (AlarmStateMap::iterator it = _alarm_state.begin();
                               it != _alarm_state.end();
                               it++)
  {
    AlarmDef::Severity current_state = (*it).second->get_state();

    if (current_state != AlarmDef::Severity::UNDEFINED_SEVERITY)
    {
      (*it).second->alarm_in_heap()->update_pop_time(0);
    }
  }

  _cond->signal();
  pthread_mutex_unlock(&_lock);
}
