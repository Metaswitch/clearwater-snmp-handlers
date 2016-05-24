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

bool ObservedAlarms::update(const AlarmTableDef& alarm_table_def, const std::string& issuer)
{
  bool updated = false;
  if (!is_active(alarm_table_def))
  {
    _index_to_entry[alarm_table_def.alarm_index()] = AlarmListEntry(alarm_table_def, issuer);
    updated = true;
  }
  return updated;
}

bool ObservedAlarms::is_active(const AlarmTableDef& alarm_table_def)
{
  std::map<unsigned int, AlarmListEntry>::iterator it = _index_to_entry.find(alarm_table_def.alarm_index());

  if ((it == _index_to_entry.end()) || (alarm_table_def.severity() != it->second.alarm_table_def().severity()))
  {
    // Either the current alarm doesn't exist in the ObservedAlarms mapping
    // or there is an entry for the current alarm in the mapping but at a
    // different severity to the one we are currently raising the alarm with.
    if (it == _index_to_entry.end())
    {
      TRC_DEBUG("Alarm was not known at any severity");
    }
    else
    {
      TRC_DEBUG("Alarm is active at %d severity which does not match the given severity (%d)",
                it->second.alarm_table_def().severity(),
                alarm_table_def.severity());
    }
    return false;
  }
  else
  {
    TRC_DEBUG("Alarm is active at the given severity (%d)",
              alarm_table_def.severity());
    return true;
  }
}

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

void AlarmHeap::handle_failed_alarm(AlarmTableDef& alarm_table_def)
{
  if (_observed_alarms.is_active(alarm_table_def))
  {
    // Alarm is still active, attempt to re-transmit
    _alarm_trap_sender->send_trap(alarm_table_def);
  }
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
    // If the alarm to be issued exists in the ObservedAlarms mapping at the
    // same severity then we disregard the alarm as it has not changed state.
    if (_observed_alarms.update(alarm_table_def, issuer))
    {
      if (!AlarmFilter::get_instance().alarm_filtered(index, severity))
      {
        alarmActiveTable_trap_handler(alarm_table_def);
        _alarm_trap_sender->send_trap(alarm_table_def);
      }
    }
  }
  else
  {
    TRC_ERROR("unknown alarm definition: %s", identifier.c_str());
  }
}

void AlarmHeap::sync_alarms()
{
  for (ObservedAlarmsIterator it = _observed_alarms.begin(); it != _observed_alarms.end(); it++)
  {
    _alarm_trap_sender->send_trap(it->alarm_table_def());
  }
}
