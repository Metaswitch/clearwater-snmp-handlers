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

#include "log.h"
#include "alarm_table_defs.hpp"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <boost/filesystem.hpp>

#include "json_parse_utils.h"
#include "json_alarms.h"

#include <fstream>
#include <sys/stat.h>
#include <dirent.h>

AlarmTableDefs AlarmTableDefs::_instance;

// Translate ituAlarmPerceivedSeverity to alarmModelState based upon mapping
// defined in section 5.4 of RFC 3877.
// https://tools.ietf.org/html/rfc3877#section-5.4
unsigned int AlarmTableDef::state() const
{
  static const unsigned int severity_to_state[] = {2, 1, 2, 6, 5, 4, 3};
  unsigned int idx = severity();
  return severity_to_state[idx];
}

bool AlarmTableDefKey::operator<(const AlarmTableDefKey& rhs) const
{
  return  (_index  < rhs._index) ||
         ((_index == rhs._index) && (_severity < rhs._severity));
}

bool AlarmTableDefs::initialize(std::string& path)
{
  std::map<unsigned int, unsigned int> dup_check;
  _key_to_def.clear();

  bool rc = true;
  boost::filesystem::path p(path);

  if ((boost::filesystem::exists(p)) && (boost::filesystem::is_directory(p)))
  {
    boost::filesystem::directory_iterator dir_it(p);
    boost::filesystem::directory_iterator end_it;

    while ((dir_it != end_it) && (rc))
    {
      if ((boost::filesystem::is_regular_file(dir_it->status())) &&
          (dir_it->path().extension() == ".json"))
      {
        rc = populate_map(dir_it->path().c_str(), dup_check);
      }

      dir_it++;
    }
  }
  else
  {
    TRC_ERROR("Unable to open directory at %s", path.c_str());
    rc = false;
  }

  return rc;
}

bool AlarmTableDefs::populate_map(std::string path,
                                  std::map<unsigned int, unsigned int>& dup_check)
{
  std::string error;
  std::vector<AlarmDef::AlarmDefinition> alarm_definitions;
  bool rc = JSONAlarms::validate_alarms_from_json(path, error, alarm_definitions);

  if (!rc)
  {
    TRC_ERROR("%s", error.c_str());
    return rc;
  }

  std::vector<AlarmDef::AlarmDefinition>::const_iterator a_it;
  for (a_it = alarm_definitions.begin(); a_it != alarm_definitions.end(); a_it++)
  {
    if (dup_check.count(a_it->_index))
    {
      TRC_ERROR("alarm %d.*: is multiply defined", a_it->_index);
      rc = false;
      break;
    }
    else
    {
      dup_check[a_it->_index] = 1;
    }

    std::vector<AlarmDef::SeverityDetails>::const_iterator s_it;

    for (s_it = a_it->_severity_details.begin(); s_it != a_it->_severity_details.end(); s_it++)
    {
      AlarmTableDef def(*a_it, *s_it);
      AlarmTableDefs::insert_def(def);
    }
  }
  return rc;
}

void AlarmTableDefs::insert_def(AlarmTableDef def)
{
  AlarmTableDefKey key(def.alarm_index(), def.severity());
  _key_to_def.emplace(key, def);
}

AlarmTableDef& AlarmTableDefs::get_definition(unsigned int index, 
                                              unsigned int severity)
{
  AlarmTableDefKey key(index, severity);

  std::map<AlarmTableDefKey, AlarmTableDef>::iterator it = _key_to_def.find(key);

  return (it != _key_to_def.end()) ? it->second : _invalid_def;
}
