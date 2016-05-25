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
  _alarm_table_defs(alarm_table_defs),
  _alarm_trap_sender(new AlarmTrapSender(this))
{
  for (AlarmTableDefsIterator it = _alarm_table_defs->begin();
                              it != _alarm_table_defs->end();
                              it++)
  {
    AlarmStateMap::iterator index = _alarm_state.find(it->alarm_index());

    if (index == _alarm_state.end())
    {
      AlarmInHeap* alarm_in_heap = new AlarmInHeap(*it);
      _alarm_heap.insert(alarm_in_heap);
      AlarmInfo* alarm_info = new AlarmInfo(alarm_in_heap,
                                            AlarmDef::Severity::UNDEFINED_SEVERITY,
                                            AlarmDef::Severity::UNDEFINED_SEVERITY);
      _alarm_state.insert(std::pair<unsigned int, AlarmInfo*>(it->alarm_index(),
                                                              alarm_info));
    }
  }

  // TODO - Create heap thread
}

AlarmHeap::~AlarmHeap()
{
  delete _alarm_trap_sender; _alarm_trap_sender = NULL;
  _alarm_heap.clear();
  for (AlarmStateMap::iterator it = _alarm_state.begin();
                               it != _alarm_state.end();
                               it++)
  {
    delete (*it).second;
  }
}

void AlarmHeap::handle_failed_alarm(AlarmTableDef& alarm_table_def)
{
  AlarmStateMap::iterator alarm = _alarm_state.find(alarm_table_def.alarm_index());
  if (alarm != _alarm_state.end())
  {
    if ((*alarm).second->get_state() == alarm_table_def.severity())
    {
      _alarm_trap_sender->send_trap(alarm_table_def);
    }
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
    AlarmDef::Severity severity_as_enum = (AlarmDef::Severity)severity;

    // If the alarm to be issued exists in the ObservedAlarms mapping at the
    // same severity then we disregard the alarm as it has not changed state.
    AlarmStateMap::iterator alarm = _alarm_state.find(index);
    if (alarm != _alarm_state.end())
    {
      if ((*alarm).second->add_alarm_to_heap(severity_as_enum))
      {
        // TODO - move this to the heap thread
        // Update the alarm state map
        (*alarm).second->set_state(severity_as_enum);

        if (!AlarmFilter::get_instance().alarm_filtered(index, severity))
        {
          alarmActiveTable_trap_handler(alarm_table_def);
          _alarm_trap_sender->send_trap(alarm_table_def);
        }
      }
    }
  }
  else
  {
    TRC_ERROR("Unknown alarm definition: %s", identifier.c_str());
  }
}

void AlarmHeap::sync_alarms()
{
  TRC_STATUS("Resyncing alarms");

  for (AlarmStateMap::iterator it = _alarm_state.begin();
                               it != _alarm_state.end();
                               it++)
  {
    AlarmDef::Severity current_state = (*it).second->get_state();

    if (current_state != AlarmDef::Severity::UNDEFINED_SEVERITY)
    {
      AlarmTableDef& alarm_table_def = _alarm_table_defs->get_definition((*it).first, current_state);
      _alarm_trap_sender->send_trap(alarm_table_def);
    }
  }
}
