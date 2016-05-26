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

#ifndef ALARM_HEAP_HPP
#define ALARM_HEAP_HPP

#include <time.h>

#include <string>
#include <map>
#include <cstdint>

#include "alarm_table_defs.hpp"
#include "alarm_trap_sender.hpp"
#include "timer_heap.h"
#include "utils.h"
#include "cond_var.h"

enum AlarmSeverityChange
{
  NO_CHANGE,
  INCREASED_SEVERITY,
  REDUCED_SEVERITY,
};


class AlarmInHeap : public HeapableTimer
{
public:
  AlarmInHeap(unsigned int index) :
    _pop_time(UINT_MAX),
    _severity(AlarmDef::Severity::UNDEFINED_SEVERITY),
    _index(index)
 {}

  const unsigned int index() { return _index; }
  AlarmDef::Severity severity() { return _severity; }
  void set_severity(AlarmDef::Severity severity) { _severity = severity; }

  void update_pop_time(uint64_t time_to_add)
  {
    _pop_time = Utils::get_time() + time_to_add;
    _heap->rebalance(this);
  }

  void set_max_pop_time()
  {
    _pop_time = UINT_MAX;
    _heap->rebalance(this);
  }

  uint64_t get_pop_time() const { return _pop_time; }

private:
  uint64_t _pop_time;
  AlarmDef::Severity _severity;
  unsigned int _index;
};

class AlarmInfo
{
public:
  AlarmInfo(AlarmInHeap* alarm_in_heap,
            AlarmDef::Severity severity) :
    _alarm_in_heap(alarm_in_heap),
    _severity(severity)
  {}

  ~AlarmInfo();

  // Compares a new severity with the current severity for this alarm,
  // and returns the severity change
  // @params severity - the new severity
  // @returns         - the severity change
  AlarmSeverityChange new_alarm_severity(AlarmDef::Severity severity);

  // Updates the alarm in the heap's severity, and it's time to pop
  void update_alarm_in_heap(AlarmDef::Severity new_severity,
                            uint64_t time_to_delay);

  AlarmInHeap* alarm_in_heap() { return _alarm_in_heap; }
  AlarmDef::Severity get_severity() { return _severity; }
  void set_severity(AlarmTableDef& alarm_table_def);

  // Determines whether this alarm should be resent after failure
  bool should_resend_alarm(AlarmDef::Severity severity)
  {
    // Resend the alarm if its severity and the current severity match,
    // and there isn't already an alarm in the heap waiting to update
    // the severity.
    return ((_severity == severity) &&
            (_alarm_in_heap->severity() ==
             AlarmDef::Severity::UNDEFINED_SEVERITY));
  }

private:
  AlarmInHeap* _alarm_in_heap;
  AlarmDef::Severity _severity;
};

typedef std::map<unsigned int, AlarmInfo*> AllAlarmsStateMap;

// This class accepts alarm triggers, checks that they correspond to valid
// alarms, and passes valid alarms to the alarm trap sender to send as SNMP
// INFORMs. It also filters the alarms.
class AlarmHeap
{
public:
  // Times to send an alarms. If we're reducing the severity of an alarm we
  // add a delay of 30secs (to protect against flicker). Increased severity
  // alarms or resyncing alarms are sent immediately.
  const static uint64_t ALARM_REDUCED = 30000;
  const static uint64_t ALARM_RETRY_DELAY = 30000;
  const static uint64_t ALARM_RESYNC_DELAY = 0;
  const static uint64_t ALARM_INCREASED = 0;

  // Constructor/Destructor
  AlarmHeap(AlarmTableDefs* alarm_table_defs);
  ~AlarmHeap();

  // Generates an alarmActiveState inform if the identified alarm is not
  // of CLEARED severity and not already active (or subject to filtering).
  // Generates an alarmClearState inform if the identified alarm is of a
  // CLEARED severity and an associated alarm is active (unless subject
  // to filtering).
  void issue_alarm(const std::string& issuer,
                   const std::string& identifier);

  static void* heap_sender_function(void* data);

  void heap_sender();

  // Runs through all currently active alarms and adds them to the alarm heap
  // to send immediately.
  void sync_alarms();

  // Handles the case where an INFORM sent by the trap sender timed out.
  // @params alarm_table_def - Definition of the alarm (index/severity) we
  //                           tried to send.
  void handle_failed_alarm(AlarmTableDef& alarm_table_def);

  // Whether the program has terminated
  volatile bool _terminated;

private:
  AlarmTableDefs* _alarm_table_defs;
  AlarmTrapSender* _alarm_trap_sender;
  TimerHeap _alarm_heap;

  // Map holding all alarms. This maps the index of an alarm to information
  // about the alarm (its current severity and its representation in the heap).
  AllAlarmsStateMap _all_alarms_state;

  pthread_mutex_t _lock;
  CondVar* _cond;
  pthread_t _heap_sender_thread;
};

#endif
