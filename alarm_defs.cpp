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

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include <boost/algorithm/string/predicate.hpp>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <fstream>
#include <stdexcept>

// Override rapidjson assert macro to ensure that an unexpected error will
// not cause snmpd to terminate.

#define  RAPIDJSON_ASSERT(x)                          \
           do {                                       \
             if (!(x)) {                              \
               throw std::domain_error("rapidjson");  \
             }                                        \
           } while (0) 

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include "alarm_defs.hpp"

std::vector<AlarmDefFile> AlarmDefFile::_def_files;

AlarmDefs AlarmDefs::_instance;

// Translate ituAlarmPerceivedSeverity to alarmModelState based upon mapping
// defined in section 5.4 of RFC 3877.
unsigned int AlarmDef::state()
{
  static const unsigned int severity_to_state[] = {2, 1, 2, 6, 5, 4, 3};

  unsigned int idx = severity();

  if ((idx < 1) || (idx > 6))
  {
    idx = 0;
  }

  return severity_to_state[idx];
}

bool AlarmDefFile::parse(std::list<AlarmDef>& defs)
{
  try
  {
    rapidjson::Document doc;

    if (!parse_doc(doc))
    {
      return false;
    }

    // An alarm definition file is structured as an 'alarms' array which
    // contains alarm objects, which in turn contain the defails for an
    // alarm.

    if (!(doc.HasMember("alarms") && doc["alarms"].IsArray()))
    {
      snmp_log(LOG_ERR, "%s: 'alarms' array missing", _path.c_str());
      return false;
    }

    const rapidjson::Value& alarm_objs = doc["alarms"];

    for (rapidjson::SizeType idx = 0; idx < alarm_objs.Size(); idx++)
    {
      AlarmDef def;

      if (alarm_objs[idx].IsObject())
      {
        const rapidjson::Value& alarm_obj = alarm_objs[idx];

        if (!parse_def(idx, alarm_obj, def))
        {
          return false;
        }
      }
      else
      {
        snmp_log(LOG_ERR, "%s: alarm definition %d: not an object", _path.c_str(), idx);
        return false;
      }

      defs.push_back(def);
    }
  }
  catch (std::domain_error)
  {
    snmp_log(LOG_ERR, "%s: unexpected rapidjson assertion", _path.c_str());
    return false;
  }

  return true;
}

time_t AlarmDefFile::last_modified()
{
  time_t mod_time = 0;
  struct stat status;

  if (stat(_path.c_str(), &status) == 0)
  {
    mod_time = status.st_mtime;
  }

  return mod_time;
}

std::vector<AlarmDefFile>& AlarmDefFile::alarm_def_files()
{
  static const char def_file_root[]  = "/etc/clearwater/alarm-defs/";
  static const char def_file_suffix[] = "-alarms.json";

  if (_def_files.size() == 0)
  {
    DIR* dir_p = opendir(def_file_root);

    if (dir_p != NULL)
    {
      struct dirent  entry;
      struct dirent* entry_p;

      while ((readdir_r(dir_p, &entry, &entry_p) == 0) && entry_p)
      {
        if (entry.d_type == DT_REG)
        {
          if (boost::algorithm::ends_with(entry.d_name, def_file_suffix))
          {
            AlarmDefFile def_file(std::string(def_file_root) + entry.d_name);

            _def_files.push_back(def_file);
          } 
        }
      }
    }
    else
    {
      snmp_log(LOG_ERR, "opendir failed: %s: %s", def_file_root, strerror(errno));
    }

    closedir(dir_p);
  }

  return _def_files;
}

// Open alarm definition file and perform a stream parse to rapidjson
// document representation.
bool AlarmDefFile::parse_doc(rapidjson::Document& doc)
{
  bool doc_ok = true;

  FILE* fp = fopen(_path.c_str(), "rb");
  if (fp == NULL)
  {
    snmp_log(LOG_ERR, "fopen failed: %s: %s", _path.c_str(), strerror(errno));
    return false;
  }

  char buf[4096];
  rapidjson::FileReadStream rs(fp, buf, sizeof(buf));

  doc.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(rs);

  if (doc.HasParseError())
  {
    snmp_log(LOG_ERR, "%s: parse error at %d", _path.c_str(), (int) doc.GetErrorOffset());
    doc_ok = false;
  }

  fclose(fp);

  return doc_ok;
}

// Extract alarm details from JSON object to internal representation while
// verifying all expected fields are present and valid.
bool AlarmDefFile::parse_def(unsigned int idx, const rapidjson::Value& obj, AlarmDef& def)
{
  bool def_ok = true;

  // Adjust the alarm definition index from zero relative to 1 relative for error logging
  // (i.e. the first definition will be 1 (vs. 0) when reporting an error).
  idx++;

  if (obj.HasMember("identifier") && obj["identifier"].IsString())
  {
    def.identifier(obj["identifier"].GetString());
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'identifier' missing or not String", _path.c_str(), idx);
    def_ok = false;
  }

  if (obj.HasMember("index") && obj["index"].IsUint())
  {
    def.index(obj["index"].GetUint());
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'index' missing or not Uint", _path.c_str(), idx);
    def_ok = false;
  }

  if (obj.HasMember("severity") && obj["severity"].IsString())
  {
    static std::map<std::string, AlarmDef::Severity> xlate = {{"CLEARED",       AlarmDef::SEVERITY_CLEARED},
                                                              {"INDETERMINATE", AlarmDef::SEVERITY_INDETERMINATE},
                                                              {"CRITICAL",      AlarmDef::SEVERITY_CRITICAL}, 
                                                              {"MAJOR",         AlarmDef::SEVERITY_MAJOR},
                                                              {"MINOR",         AlarmDef::SEVERITY_MINOR},
                                                              {"WARNING",       AlarmDef::SEVERITY_WARNING}};
    if (xlate.count(obj["severity"].GetString()))
    {
      def.severity(xlate[obj["severity"].GetString()]);
    }
    else 
    {
      snmp_log(LOG_ERR, "%s: alarm object %d: 'severity' unrecognized value", _path.c_str(), idx);
      def_ok = false;
    }
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'severity' missing or not String", _path.c_str(), idx);
    def_ok = false;
  }

  if (obj.HasMember("cause") && (obj["cause"].IsString() || obj["cause"].IsUint()))
  {
    if (obj["cause"].IsString())
    {
      static std::map<std::string, int> xlate = {{"SOFTWARE_ERROR",                   163},
                                                 {"UNDERLAYING_RESOURCE_UNAVAILABLE", 165}};

      if (xlate.count(obj["cause"].GetString()))
      {
        def.cause(xlate[obj["cause"].GetString()]);
      }
      else 
      {
        snmp_log(LOG_ERR, "%s: alarm object %d: 'cause' unrecognized value", _path.c_str(), idx);
        def_ok = false;
      }
    }
    else
    {
      def.cause(obj["cause"].GetUint());
    }
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'cause' missing or not String/Uint", _path.c_str(), idx);
    def_ok = false;
  }

  if (obj.HasMember("description") && obj["description"].IsString())
  {
    if (strlen(obj["description"].GetString()) <= 255) 
    {
      def.description(obj["description"].GetString());
    }
    else
    {
      snmp_log(LOG_ERR, "%s: alarm object %d: 'description' exceeds 255 char limit", _path.c_str(), idx);
      def_ok = false;
    }
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'description' missing or not String", _path.c_str(), idx);
    def_ok = false;
  }

  if (obj.HasMember("details") && obj["details"].IsString())
  {
    if (strlen(obj["details"].GetString()) <= 255) 
    {
      def.details(obj["details"].GetString());
    }
    else
    {
      snmp_log(LOG_ERR, "%s: alarm object %d: 'details' exceeds 255 char limit", _path.c_str(), idx);
      def_ok = false;
    }
  }
  else
  {
    snmp_log(LOG_ERR, "%s: alarm object %d: 'details' missing or not String", _path.c_str(), idx);
    def_ok = false;
  }

  return def_ok;
}

bool AlarmDefs::load()
{
  bool load_ok = true;

  std::vector<AlarmDefFile>& files = AlarmDefFile::alarm_def_files();

  for (std::vector<AlarmDefFile>::iterator f_it = files.begin(); f_it != files.end(); f_it++)
  {
    std::list<AlarmDef> defs;

    if (f_it->parse(defs))
    {
      _defs.splice(_defs.end(), defs);
    }
  }

  if (_defs.size() > 0)
  {
    std::map<unsigned int, AlarmDef*> dup_check;

    for (std::list<AlarmDef>::iterator d_it = _defs.begin(); d_it != _defs.end(); d_it++)
    {
      _id_to_def[d_it->identifier()] = &(*d_it);

      if (d_it->severity() == AlarmDef::SEVERITY_CLEARED)
      {
        _idx_to_clear_def[d_it->index()] = &(*d_it);
      }

      unsigned int index_severity = (d_it->index() << 3) | d_it->severity();

      if (dup_check.count(index_severity))
      {
        snmp_log(LOG_ERR, "duplicate index/severity: %d/%d; conflicting identifiers: %s, %s", d_it->index(), d_it->severity(),
                                                  d_it->identifier().c_str(), dup_check[index_severity]->identifier().c_str());
        load_ok = false;
      }
      else
      {
        dup_check[index_severity] = &(*d_it);
      }
    } 
  }
  else
  {
    snmp_log(LOG_ERR, "no valid alarm definitions");
    load_ok = false;
  }

  return load_ok;
}

AlarmDef* AlarmDefs::get_definition(const std::string& id)
{
  std::map<std::string, AlarmDef*>::iterator it = _id_to_def.find(id);

  return (it != _id_to_def.end()) ? it->second : &_invalid_def;
}

AlarmDef* AlarmDefs::get_clear_definition(unsigned int idx)
{
  std::map<unsigned int, AlarmDef*>::iterator it = _idx_to_clear_def.find(idx);

  return (it != _idx_to_clear_def.end()) ? it->second : &_invalid_def;
}
