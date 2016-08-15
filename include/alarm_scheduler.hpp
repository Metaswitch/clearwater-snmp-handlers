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

#ifndef ALARM_SCHEDULER_HPP
#define ALARM_SCHEDULER_HPP

#include <time.h>

#include <string>
#include <map>
#include <cstdint>

#include "alarm_table_defs.hpp"
#include "alarm_trap_sender.hpp"
#include "timer_heap.h"
#include "utils.h"
#ifdef UNIT_TEST
#include "pthread_cond_var_helper.h"
#else
#include "cond_var.h"
#endif

/// AlarmTimer. This holds information about an alarm that's scheduled to be
/// sent, in a format that can be placed into the heap.
class AlarmTimer : public HeapableTimer
{
public:
  AlarmTimer(unsigned int index) :
    _pop_time(UINT64_MAX),
    _severity(AlarmDef::Severity::UNDEFINED_SEVERITY),
    _index(index)
 {}

  // Updates the pop time of the AlarmTimer, and rebalances the heap to
  // account for the changed pop time
  void update_pop_time(uint64_t time_to_add)
  {
    _pop_time = Utils::get_time() + time_to_add;
    _heap->rebalance(this);
  }

  // Removes the AlarmTimer from the heap, and sets the pop time/severity
  // to clearly invalid values (that won't cause INFORMS to be sent in error
  // cases)
  void remove_from_heap()
  {
    _pop_time = UINT64_MAX;
    _severity = AlarmDef::Severity::UNDEFINED_SEVERITY;
    _heap->remove(this);
  }

  const unsigned int index() { return _index; }
  AlarmDef::Severity severity() { return _severity; }
  void set_severity(AlarmDef::Severity severity) { _severity = severity; }
  bool in_heap() { return (_heap != nullptr); }
  uint64_t get_pop_time() const { return _pop_time; }

private:
  uint64_t _pop_time;
  AlarmDef::Severity _severity;
  unsigned int _index;
};

/// SingleAlarmManager. This class manages a single alarm. It holds a pointer
/// to the alarm that's been scheduled to be sent to the NMS, the severity
/// that we last sent to the NMS, and holds the logic for comparing these
/// and deciding if we resend/reschedule the alarm.
class SingleAlarmManager
{
public:
  SingleAlarmManager(AlarmTimer* alarm_timer,
                     AlarmDef::Severity severity) :
    _alarm_timer(alarm_timer),
    _severity(severity)
  {}

  ~SingleAlarmManager();

  // Updates the alarm in the heap's severity, and its time to pop
  void change_schedule(AlarmDef::Severity new_severity,
                       uint64_t time_to_delay_in_ms);

  AlarmTimer* alarm_timer() { return _alarm_timer; }
  AlarmDef::Severity severity() { return _severity; }

  // Updates the alarm in the heap and the Active Alarm Table
  void update_alarm_state(AlarmTableDef& alarm_table_def);

  // Determines whether this alarm should be resent after failure
  bool should_resend_alarm(AlarmDef::Severity severity)
  {
    // Resend the alarm if its severity and the current severity match,
    // and there isn't already an alarm in the heap waiting to update
    // the severity.
    return ((_severity == severity) && (!_alarm_timer->in_heap()));
  }

private:
  AlarmTimer* _alarm_timer;
  AlarmDef::Severity _severity;
};

typedef unsigned int AlarmIndex;

// This class accepts alarm triggers, checks that they correspond to valid
// alarms, and passes schedules sending alarms to the alarm trap sender to
// send as SNMP INFORMs.
class AlarmScheduler
{
public:
  // Times to send an alarms. If we're reducing the severity of an alarm we
  // add a delay of 30secs (to protect against flicker). Increased severity
  // alarms or resyncing alarms are sent immediately.
  const static uint64_t ALARM_REDUCED_DELAY = 30000;
  const static uint64_t ALARM_RETRY_DELAY = 30000;
  const static uint64_t ALARM_RESYNC_DELAY = 0;
  const static uint64_t ALARM_INCREASED_DELAY = 0;

  // Constructor/Destructor
  AlarmScheduler(AlarmTableDefs* alarm_table_defs, 
                 std::set<NotificationType> snmp_notifications);
  virtual ~AlarmScheduler();

  // Generates an alarmActiveState inform if the identified alarm is not
  // of CLEARED severity and not already active (or subject to filtering).
  // Generates an alarmClearState inform if the identified alarm is of a
  // CLEARED severity and an associated alarm is active (unless subject
  // to filtering).
  virtual void issue_alarm(const std::string& issuer,
                           const std::string& identifier);

  static void* heap_sender_function(void* data);

  void heap_sender();

  // Runs through all currently active alarms and adds them to the alarm heap
  // to send immediately.
  virtual void sync_alarms();

  // Handles the case where an INFORM sent by the trap sender timed out.
  // @params alarm_table_def - Definition of the alarm (index/severity) we
  //                           tried to send.
  void handle_failed_alarm(AlarmTableDef& alarm_table_def);

  // Whether the program has terminated
  volatile bool _terminated;

private:
  void change_schedule_for_alarm(SingleAlarmManager* single_alarm_manager,
                                 AlarmDef::Severity severity,
                                 uint64_t alarm_pop_time_ms);

  void remove_outdated_alarm_from_heap(SingleAlarmManager* single_alarm_manager);

  bool validate_alarm_trigger(std::string identifier,
                              AlarmIndex& index,
                              unsigned int& severity);

  AlarmTableDefs* _alarm_table_defs;
  TimerHeap _alarm_heap;

  // Map holding all alarms. This maps the index of an alarm to information
  // about the alarm (its current severity (the severity we last tried to send
  // to the NMS) and its representation in the heap).
  std::map<unsigned AlarmIndex, SingleAlarmManager*> _all_alarms_state;

  // This lock protects access to the _all_alarms_state map and the _alarm_heap,
  // and should be taken whenever reading/writing to these structures
  pthread_mutex_t _lock;
#ifdef UNIT_TEST
  MockPThreadCondVar* _cond;
#else
  CondVar* _cond;
#endif
  pthread_t _heap_sender_thread;
};

#endif
