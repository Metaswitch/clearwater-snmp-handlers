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

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "json_parse_utils.h"

#include <fstream>
#include <sys/stat.h>

AlarmTableDefs AlarmTableDefs::_instance;

// Translate ituAlarmPerceivedSeverity to alarmModelState based upon mapping
// defined in section 5.4 of RFC 3877.
unsigned int AlarmTableDef::state()
{
  static const unsigned int severity_to_state[] = {2, 1, 2, 6, 5, 4, 3};

  unsigned int idx = severity();

  if ((idx < AlarmDef::CLEARED) || (idx > AlarmDef::WARNING))
  {
    idx = 0; // LCOV_EXCL_LINE (during rework)
  }

  return severity_to_state[idx];
}

bool AlarmTableDefKey::operator<(const AlarmTableDefKey& rhs) const
{
  return  (_index  < rhs._index) ||
         ((_index == rhs._index) && (_severity < rhs._severity));
}

bool AlarmTableDefs::initialize(std::string& path,
                                const std::vector<AlarmDef::AlarmDefinition>& alarm_definitions)
{
  std::map<unsigned int, unsigned int> dup_check;
  bool init_ok = true;

  _key_to_def.clear();

  // The intent is that we'll loop over any JSON files in a known
  // location to get the definition for all the alarms. During the
  // rework, we take all the alarms from AlarmDef::AlarmDefinition, and
  // only check whether there's a single extra file.
  init_ok = populate_map(alarm_definitions, dup_check);

  // Load any local alarm definitions, and add them to the definition map
  if (init_ok)
  {
    std::vector<AlarmDef::AlarmDefinition> local_alarms;
    init_ok = parse_local_alarms_from_file(path, local_alarms);
    init_ok &= populate_map(local_alarms, dup_check);
  }

  return init_ok;
}

bool AlarmTableDefs::populate_map(const std::vector<AlarmDef::AlarmDefinition>& alarm_definitions,
                                  std::map<unsigned int, unsigned int>& dup_check)
{
  bool init_ok = true;

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

      AlarmTableDef def(*a_it, *s_it);
      AlarmTableDefKey key(a_it->_index, s_it->_severity);
      _key_to_def.emplace(key, def);
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

bool AlarmTableDefs::parse_local_alarms_from_file(std::string& path,
                                                  std::vector<AlarmDef::AlarmDefinition>& local_alarms)
{
  // Check whether the file exists.
  struct stat s;
  if ((stat(path.c_str(), &s) != 0) &&
      (errno == ENOENT))
  {
    snmp_log(LOG_DEBUG, "No such file: %s", path.c_str());
    return true;
  }

  // Read from the file
  std::ifstream fs(path.c_str());
  std::string local_alarms_str((std::istreambuf_iterator<char>(fs)),
                                std::istreambuf_iterator<char>());

  if (local_alarms_str == "")
  {
    // LCOV_EXCL_START - Not tested in UT
    snmp_log(LOG_ERR, "Empty file: %s", path.c_str());
    return false; 
    // LCOV_EXCL_STOP
  }

  // Now parse the document
  rapidjson::Document doc;
  doc.Parse<0>(local_alarms_str.c_str());

  if (doc.HasParseError())
  {
    snmp_log(LOG_ERR, "Invalid JSON file. Error: %s", 
             rapidjson::GetParseError_En(doc.GetParseError()));
    return false; 
  }

  try
  {
    // Parse the JSON file
    JSON_ASSERT_CONTAINS(doc, "local_alarms");
    JSON_ASSERT_ARRAY(doc["local_alarms"]);
    const rapidjson::Value& alarms_arr = doc["local_alarms"];

    for (rapidjson::Value::ConstValueIterator alarms_it = alarms_arr.Begin();
         alarms_it != alarms_arr.End();
         ++alarms_it)
    {
      int index;
      int cause;
      JSON_GET_INT_MEMBER(*alarms_it, "index", index);
      JSON_GET_INT_MEMBER(*alarms_it, "cause", cause);

      JSON_ASSERT_CONTAINS(*alarms_it, "alarms");
      JSON_ASSERT_ARRAY((*alarms_it)["alarms"]);
      const rapidjson::Value& alarms_def_arr = (*alarms_it)["alarms"];

      std::vector<AlarmDef::SeverityDetails> severity_vec;
      for (rapidjson::Value::ConstValueIterator alarms_def_it = alarms_def_arr.Begin();
           alarms_def_it != alarms_def_arr.End();
           ++alarms_def_it)
      {
        int severity;
        std::string details;
        std::string description;
        JSON_GET_INT_MEMBER(*alarms_def_it, "severity", severity);
        JSON_GET_STRING_MEMBER(*alarms_def_it, "details", details);
        JSON_GET_STRING_MEMBER(*alarms_def_it, "description", description);
        AlarmDef::SeverityDetails sd = {(AlarmDef::Severity)severity, description, details};
        severity_vec.push_back(sd);     
      }

      AlarmDef::AlarmDefinition ad = {(AlarmDef::Index)index,
                                      (AlarmDef::Cause)cause,
                                      severity_vec};
      local_alarms.push_back(ad);
    }
  }
  catch (JsonFormatError err)
  {
    snmp_log(LOG_ERR, "Invalid JSON file (hit error at %s:%d)",
             err._file, err._line);
    return false; 
  }

  return true;
}

AlarmTableDef& AlarmTableDefs::get_definition(unsigned int index, 
                                              unsigned int severity)
{
  AlarmTableDefKey key(index, severity);

  std::map<AlarmTableDefKey, AlarmTableDef>::iterator it = _key_to_def.find(key);

  return (it != _key_to_def.end()) ? it->second : _invalid_def;
}

