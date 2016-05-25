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

// Container for all alarms we have seen being raised (either at CLEARED or
// non-CLEARED severity). Maps the index of an alarm to the latest severity
// with which it was raised. Currently indexed by alarm model index (may need
// to be extended to include a resource id going forward).

// Singleton helper used to filter inform notifications (as per their
// descriptions in section 4.2 of RFC 3877).

class AlarmFilter
{
public:
  enum
  {
    ALARM_FILTER_TIME = 5000,
    CLEAN_FILTER_TIME = 60000
  };

  // Intened to be called before generating an inform notification to
  // determine if the inform should be filtered (i.e. dropped because
  // it has already been issued within the filter period). This must
  // be tracked based on a <alarm model index, severity> basis.
  bool alarm_filtered(unsigned int index, unsigned int severity);

  static AlarmFilter& get_instance() {return _instance;}

private:
  class AlarmFilterKey
  {
  public:
    AlarmFilterKey(unsigned int index,
                   unsigned int severity) :
      _index(index),
      _severity(severity) {}

    bool operator<(const AlarmFilterKey& rhs) const;

  private:
    unsigned int _index;
    unsigned int _severity;
  };

  AlarmFilter() : _clean_time(0) {}

  unsigned long current_time_ms();

  std::map<AlarmFilterKey, unsigned long> _issue_times;

  unsigned long _clean_time;

  static AlarmFilter _instance;
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
  void set_severity(AlarmDef::Severity severity) { _severity = severity; }
  AlarmDef::Severity severity() { return _severity; }

  void update_pop_time(uint64_t time_to_add)
  {
    _pop_time = Utils::get_time() + time_to_add;
    _heap->rebalance(this);
  }

  void set_pop_time_to_max()
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
            AlarmDef::Severity state) :
    _alarm_in_heap(alarm_in_heap),
    _state(state)
  {}

  ~AlarmInfo()
  {
    delete _alarm_in_heap; _alarm_in_heap = NULL;
  }

  AlarmInHeap* alarm_in_heap() { return _alarm_in_heap; }
  bool add_alarm_to_heap(AlarmDef::Severity severity);
  AlarmDef::Severity get_state() { return _state; }
  void set_state(AlarmDef::Severity state) { _state = state; }
  void set_alarm_in_heap(AlarmDef::Severity severity, uint64_t time_to_send) { _alarm_in_heap->set_severity(severity); _alarm_in_heap->update_pop_time(time_to_send); }

private:
  AlarmInHeap* _alarm_in_heap;
  AlarmDef::Severity _state;
};

typedef std::map<unsigned int, AlarmInfo*> AlarmStateMap;

// Singleton class providing methods for generating alarmActiveState and
// alarmClearState inform notifications. It maintains an observed alarm
// list to support filtering and synchronization.
class AlarmHeap
{
public:
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

  void handle_failed_alarm(AlarmTableDef& alarm_table_def);

  static void* heap_pop_function(void* data);

  void heap_pop();

  // Generates alarmClearState INFORMs corresponding to each of the currently
  // cleared alarms and alarmActiveState INFORMs for each currently active
  // alarm.
  void sync_alarms();

  // Whether the program has terminated
  volatile bool _terminated;

private:
  AlarmTableDefs* _alarm_table_defs;
  AlarmTrapSender* _alarm_trap_sender;
  TimerHeap _alarm_heap;
  AlarmStateMap _alarm_state;

  pthread_mutex_t _lock;
  CondVar* _cond;
  pthread_t _heap_pop_thread;
};

#endif