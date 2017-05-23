/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include <time.h>

#include "log.h"
#include "alarm_scheduler.hpp"
#include "itu_alarm_table.hpp"
#include "alarm_active_table.hpp"

SingleAlarmManager::~SingleAlarmManager()
{
  delete _alarm_timer; _alarm_timer = NULL;
}

void SingleAlarmManager::update_alarm_state(AlarmTableDef& alarm_table_def)
{
  // Update the severity in the map.
  _severity = alarm_table_def.severity();

  // Update the ActiveAlarmTable.
  alarmActiveTable_trap_handler(alarm_table_def);
}

void SingleAlarmManager::change_schedule(AlarmDef::Severity new_severity,
                                uint64_t time_to_delay_in_ms)
{
  // Update the alarm in the heap's severity, and update its time to pop
  // (which also rebalances the heap).
  if (new_severity != _alarm_timer->severity())
  {
    _alarm_timer->set_severity(new_severity);
    _alarm_timer->update_pop_time(time_to_delay_in_ms);
  }
}

AlarmScheduler::AlarmScheduler(AlarmTableDefs* alarm_table_defs, 
                               std::set<NotificationType> snmp_notifications,
                               std::string hostname) :
  _terminated(false),
  _alarm_table_defs(alarm_table_defs)
{
  AlarmTrapSender::get_instance().initialise(this, snmp_notifications, hostname);

  // Create the lock/condition variables.
  pthread_mutex_init(&_lock, NULL);
#ifdef UNIT_TEST
  _cond = new MockPThreadCondVar(&_lock);
#else
  _cond = new CondVar(&_lock);
#endif

  // Populate the alarm state map
  for (AlarmTableDefsIterator it = _alarm_table_defs->begin();
                              it != _alarm_table_defs->end();
                              it++)
  {
    std::map<unsigned AlarmIndex, SingleAlarmManager*>::iterator index =
                                      _all_alarms_state.find(it->alarm_index());
    if (index == _all_alarms_state.end())
    {
      // We only have one entry for each alarm index (not one per severity).
      AlarmTimer* alarm_timer = new AlarmTimer(it->alarm_index());
      SingleAlarmManager* single_alarm_manager =
                 new SingleAlarmManager(alarm_timer,
                                        AlarmDef::Severity::UNDEFINED_SEVERITY);
      _all_alarms_state.insert(std::make_pair(it->alarm_index(),
                                              single_alarm_manager));
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

AlarmScheduler::~AlarmScheduler()
{
  pthread_mutex_lock(&_lock);
  _terminated = true;
  _cond->signal();
  pthread_mutex_unlock(&_lock);

  pthread_join(_heap_sender_thread, NULL);
  delete _cond; _cond = NULL;

  pthread_mutex_destroy(&_lock);

  _alarm_heap.clear();

  for (std::pair<unsigned AlarmIndex, SingleAlarmManager*> pair : _all_alarms_state)
  {
    delete pair.second;
  }
}

void* AlarmScheduler::heap_sender_function(void* data)
{
  ((AlarmScheduler*)data)->heap_sender();
  return NULL;
}

void AlarmScheduler::heap_sender()
{
  pthread_mutex_lock(&_lock);

  while (!_terminated)
  {
    uint64_t time_now_in_ms = Utils::get_time();
    AlarmTimer* alarm_timer = (AlarmTimer*)_alarm_heap.get_next_timer();

    while ((alarm_timer) &&
           (alarm_timer->get_pop_time() <= time_now_in_ms))
    {
      // For each alarm in the heap that's due to be sent, we:
      //  - Pull out the alarm definition for its index and severity.
      //  - Pass the alarm definition to the trap sender (which is responsible
      //    for actually sending any INFORMs).
      //  - Update the master view of the current severity for this alarm (by
      //    updating the _all_alarms_state map).
      //  - Finally, we then remove the alarm from the heap.
      AlarmTableDef& alarm_table_def =
              _alarm_table_defs->get_definition(alarm_timer->index(),
                                                alarm_timer->severity());

      if (alarm_table_def.is_valid())
      {
        AlarmTrapSender::get_instance().send_trap(alarm_table_def);
        std::map<unsigned AlarmIndex, SingleAlarmManager*>::iterator alarm =
                                 _all_alarms_state.find(alarm_timer->index());

        if (alarm != _all_alarms_state.end())
        {
          alarm->second->update_alarm_state(alarm_table_def);
        }
        else
        {
          // LCOV_EXCL_START - logic error
          TRC_ERROR("Logic error - unable to handle alarm");
          // LCOV_EXCL_STOP
        }
      }

      alarm_timer->remove_from_heap();
      alarm_timer = (AlarmTimer*)_alarm_heap.get_next_timer();
    }

    if (alarm_timer)
    {
      // The next alarm in the heap isn't due to pop yet. Wait until it's due.
      struct timespec time;
      time.tv_sec = alarm_timer->get_pop_time() / 1000;
      time.tv_nsec = (alarm_timer->get_pop_time() % 1000) * 1000000;
      _cond->timedwait(&time);
    }
    else
    {
      // There are no alarms in the heap. Wait until we're signalled.
      _cond->wait();
    }
  }

  pthread_mutex_unlock(&_lock);
}

void AlarmScheduler::remove_outdated_alarm_from_heap(
                                       SingleAlarmManager* single_alarm_manager)
{
  if (single_alarm_manager->severity() !=
      single_alarm_manager->alarm_timer()->severity())
  {
    _alarm_heap.remove(single_alarm_manager->alarm_timer());
    _cond->signal();
  }
}

void AlarmScheduler::change_schedule_for_alarm(
                                       SingleAlarmManager* single_alarm_manager,
                                       AlarmDef::Severity severity,
                                       uint64_t alarm_pop_time_ms)
{
  _alarm_heap.insert(single_alarm_manager->alarm_timer());
  single_alarm_manager->change_schedule(severity, alarm_pop_time_ms);
}

bool AlarmScheduler::validate_alarm_trigger(std::string identifier,
                                            AlarmIndex& index,
                                            unsigned int& severity)
{
  if (sscanf(identifier.c_str(), "%u.%u", &index, &severity) != 2)
  {
    TRC_ERROR("malformed alarm identifier: %s", identifier.c_str());
    return false;
  }

  AlarmTableDef& alarm_table_def = _alarm_table_defs->get_definition(index,
                                                                     severity);

  return alarm_table_def.is_valid();
}

void AlarmScheduler::issue_alarm(const std::string& issuer,
                                 const std::string& identifier)
{
  unsigned int index;
  unsigned int severity;

  if (validate_alarm_trigger(identifier, index, severity))
  {
    pthread_mutex_lock(&_lock);

    std::map<unsigned AlarmIndex, SingleAlarmManager*>::iterator alarm =
                                                  _all_alarms_state.find(index);

    if (alarm != _all_alarms_state.end())
    {
      AlarmDef::Severity severity_as_enum = (AlarmDef::Severity)severity;

      if (severity_as_enum == alarm->second->severity())
      {
        // The severity of the alarm hasn't changed. If there's an alarm in the
        // heap with a different severity remove it.
        TRC_DEBUG("Severity of the alarm %u hasn't changed", index);
        remove_outdated_alarm_from_heap(alarm->second);
      }
      else if (severity_as_enum > alarm->second->severity())
      {
        TRC_DEBUG("Severity of the alarm %u has increased", index);
        change_schedule_for_alarm(alarm->second,
                                  severity_as_enum,
                                  ALARM_INCREASED_DELAY);
        _cond->signal();
      }
      else
      {
        TRC_DEBUG("Severity of the alarm %u has reduced", index);
        change_schedule_for_alarm(alarm->second,
                                  severity_as_enum,
                                  ALARM_REDUCED_DELAY);
        _cond->signal();
      }
    }
    else
    {
      // LCOV_EXCL_START - logic error
      TRC_ERROR("Logic error - unable to handle alarm");
      // LCOV_EXCL_STOP
    }

    pthread_mutex_unlock(&_lock);
  }
  else
  {
    TRC_ERROR("Unknown alarm definition: %s", identifier.c_str());
  }
}

void AlarmScheduler::sync_alarms()
{
  TRC_STATUS("Resyncing all alarms");

  pthread_mutex_lock(&_lock);

  // For all alarms that we know a state for (so we've tried to send an alarm
  // state to the NMS at least once), resend the current state.
  for (std::pair<unsigned AlarmIndex, SingleAlarmManager*> pair : _all_alarms_state)
  {
    AlarmDef::Severity current_severity = pair.second->severity();

    if (current_severity != AlarmDef::Severity::UNDEFINED_SEVERITY)
    {
      change_schedule_for_alarm(pair.second,
                                current_severity,
                                ALARM_RESYNC_DELAY);
    }
  }

  _cond->signal();
  pthread_mutex_unlock(&_lock);
}

void AlarmScheduler::handle_failed_alarm(AlarmTableDef& alarm_table_def)
{
  TRC_DEBUG("Handling an alarm (%u) the NMS didn't respond to",
            alarm_table_def.alarm_index());

  pthread_mutex_lock(&_lock);

  std::map<unsigned AlarmIndex, SingleAlarmManager*>::iterator alarm =
                          _all_alarms_state.find(alarm_table_def.alarm_index());

  if (alarm != _all_alarms_state.end())
  {
    // If the alarm status hasn't changed, reschedule the alarm
    if (alarm->second->should_resend_alarm(alarm_table_def.severity()))
    {
      change_schedule_for_alarm(alarm->second,
                                alarm_table_def.severity(),
                                ALARM_RETRY_DELAY);
      _cond->signal();
    }
  }
  else
  {
    // LCOV_EXCL_START - logic error
    TRC_ERROR("Logic error - unable to handle alarm callback");
    // LCOV_EXCL_STOP
  }

  pthread_mutex_unlock(&_lock);
}
