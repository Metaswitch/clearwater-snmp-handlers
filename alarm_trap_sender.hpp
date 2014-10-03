/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2014 Metaswitch Networks Ltd
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

#ifndef ALARM_TRAPS_HPP
#define ALARM_TRAPS_HPP

#include <time.h>

#include <string>
#include <map>

#include "alarm_defs.hpp"

// Definition of an entry in the active alarm list. 

class AlarmListEntry
{
public:
  AlarmListEntry() : _alarm_def(NULL) {}
  AlarmListEntry(AlarmDef* alarm_def, const std::string& issuer) :
    _alarm_def(alarm_def), _issuer(issuer) {}

  AlarmDef* alarm_def() {return _alarm_def;}
  std::string& issuer() {return _issuer;}

private:
  AlarmDef* _alarm_def;
  std::string _issuer;
};

// Iterator for enumerating entries in the active alarm list. Subclassed
// from map iterator to hide pair template. Only operations defined are
// supported.

class ActiveAlarmIterator : public std::map<unsigned int, AlarmListEntry>::iterator
{
public:
  ActiveAlarmIterator(std::map<unsigned int, AlarmListEntry>::iterator iter) : std::map<unsigned int, AlarmListEntry>::iterator(iter) {}

  AlarmListEntry& operator*()  {return  (std::map<unsigned int, AlarmListEntry>::iterator::operator*().second);}
  AlarmListEntry* operator->() {return &(std::map<unsigned int, AlarmListEntry>::iterator::operator*().second);}
};

// Container for all currently active alarms. Currently indexed by alarm
// model index (may need to be extended to include a resource id going
// forward). 

class ActiveAlarmList
{
public:
  ActiveAlarmList() {}

  // Adds a non CLEARED severity alarm to the list if it does not already
  // exist. For a CLEARED severity update, removes an associated alarm of
  // the same alarm model index if one exists. Returns true if a change
  // was made to the list.
  bool update(AlarmDef* alarm_def, const std::string& issuer);

  // Removes an entry from the list obtained via iteration over the list.
  void remove(ActiveAlarmIterator& it);

  ActiveAlarmIterator begin() {return _idx_to_entry.begin();}
  ActiveAlarmIterator end() {return _idx_to_entry.end();}

private:
  std::map<unsigned int, AlarmListEntry> _idx_to_entry;
};

// Singleton helper used to filter inform notifications (as per their
// descriptions in section 4.2 of RFC 3877). 

class AlarmFilter
{
public:
  // Intened to be called before generating an inform notification to
  // determine if the inform should be filtered (i.e. dropped because
  // it has already been issued within the filter period). This must
  // be tracked based on a <alarm model index, severity> basis.
  bool alarm_filtered(unsigned int index, AlarmDef::Severity severity);

  static AlarmFilter& get_instance() {return _instance;}

private:
  enum 
  { 
    ALARM_FILTER_TIME = 5000,
    CLEAN_FILTER_TIME = 60000
  };

  AlarmFilter() : _clean_time(0) {}

  unsigned long current_time_ms();

  std::map<unsigned int, unsigned long> _issue_times;

  unsigned long _clean_time;

  static AlarmFilter _instance;
};

// Singleton class providing methods for generating alarmActiveState and
// alarmClearState inform notifications. It maintains an active alarm 
// list to support filtering and synchronization. 

class AlarmTrapSender
{
public:
  // Generates an alarmActiveState inform if the identified alarm is not 
  // of CLEARED severity and not already active (or subject to filtering).
  // Generates an alarmClearState inform if the identified alarm is of a
  // CLEARED severity and an associated alarm is active (unless subject
  // to filtering).
  void issue_alarm(const std::string& issuer, const std::string& identifier);

  // Generates alarmClearState informs corresponding to each of the active 
  // alarms previously initiated by issuer. 
  void clear_alarms(const std::string& issuer);

  // Generates alarmClearState informs corresponding to each of the alarms
  // defined for this node; followed by alarmActiveState informs for each
  // currently active alarm.
  void sync_alarms();

  static AlarmTrapSender& get_instance() {return _instance;}

private:
  AlarmTrapSender() {}

  void send_trap(AlarmDef* alarm);

  ActiveAlarmList _active_alarms;

  static AlarmTrapSender _instance;
};

#endif

