/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
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
