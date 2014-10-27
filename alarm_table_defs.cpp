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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "alarm_table_defs.hpp"

AlarmTableDefs AlarmTableDefs::_instance;

// Translate ituAlarmPerceivedSeverity to alarmModelState based upon mapping
// defined in section 5.4 of RFC 3877.
unsigned int AlarmTableDef::state()
{
  static const unsigned int severity_to_state[] = {2, 1, 2, 6, 5, 4, 3};

  unsigned int idx = severity();

  if ((idx < AlarmDef::CLEARED) || (idx > AlarmDef::WARNING))
  {
    idx = 0;
  }

  return severity_to_state[idx];
}

bool AlarmTableDefKey::operator<(const AlarmTableDefKey& rhs) const
{
  return  (_index  < rhs._index) ||
         ((_index == rhs._index) && (_severity < rhs._severity));
}

bool AlarmTableDefs::initialize(const std::vector<AlarmDef::AlarmDefinition>& alarm_definitions)
{
  std::map<unsigned int, unsigned int> dup_check;
  bool init_ok = true;

  _key_to_def.clear();

  std::vector<AlarmDef::AlarmDefinition>::const_iterator a_it;
  for (a_it = alarm_definitions.begin(); a_it != alarm_definitions.end(); a_it++)
  {
    bool has_non_cleared = false;
    bool has_cleared = false;

    if (dup_check.count(a_it->_index))
    {
      snmp_log(LOG_ERR, "alarm %d.*: is multiply defined", a_it->_index); 
      init_ok = false;
      continue;
    }
    else
    {
      dup_check[a_it->_index] = 1;
    }

    std::vector<AlarmDef::SeverityDetails>::const_iterator s_it;
    for (s_it = a_it->_severity_details.begin(); s_it != a_it->_severity_details.end(); s_it++)
    {
      if (s_it->_description.size() > AlarmTableDef::MIB_STRING_LEN)
      {
        snmp_log(LOG_ERR, "alarm %d.%d: 'description' exceeds %d char limit", a_it->_index, s_it->_severity, 
                                                                              AlarmTableDef::MIB_STRING_LEN);
        init_ok = false;
        continue;
      }

      if (s_it->_details.size() > AlarmTableDef::MIB_STRING_LEN)
      {
        snmp_log(LOG_ERR, "alarm %d.%d: 'details' exceeds %d char limit", a_it->_index, s_it->_severity,
                                                                          AlarmTableDef::MIB_STRING_LEN);
        init_ok = false;
        continue;
      }

      has_non_cleared |= (s_it->_severity != AlarmDef::CLEARED);
      has_cleared     |= (s_it->_severity == AlarmDef::CLEARED);

      AlarmTableDef def(&(*a_it), &(*s_it)); 
      AlarmTableDefKey key(a_it->_index, s_it->_severity);
      _key_to_def[key] = def;
    }

    if (!has_non_cleared)
    {
      snmp_log(LOG_ERR, "alarm %d.*: must define at least one non-CLEARED severity", a_it->_index);
      init_ok = false;
    }

    if (!has_cleared)
    {
      snmp_log(LOG_ERR, "alarm %d.*: must define a CLEARED severity", a_it->_index);
      init_ok = false;
    }
  }

  return init_ok;
}

AlarmTableDef& AlarmTableDefs::get_definition(unsigned int index, 
                                              unsigned int severity)
{
  AlarmTableDefKey key(index, severity);

  std::map<AlarmTableDefKey, AlarmTableDef>::iterator it = _key_to_def.find(key);

  return (it != _key_to_def.end()) ? it->second : _invalid_def;
}

