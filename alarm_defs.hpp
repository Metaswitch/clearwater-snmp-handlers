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

#ifndef ALARM_DEFS_HPP
#define ALARM_DEFS_HPP

#include <time.h>

#include <string>
#include <vector>
#include <list>
#include <map>

#include <rapidjson/document.h>

// Container for data needed to generate an entry of the Alarm Model Table
// and ITU Alarm Table; and for sending alarmActiveState and alarmClearState
// inform notifications.

class AlarmDef
{
public:
  enum Severity {
    SEVERITY_UNDEF,
    SEVERITY_CLEARED,
    SEVERITY_INDETERMINATE,
    SEVERITY_CRITICAL,
    SEVERITY_MAJOR,
    SEVERITY_MINOR,
    SEVERITY_WARNING
  };

  AlarmDef() : _index(0) {}

  unsigned int index() {return _index;} 
  void index(unsigned int index) {_index = index;}

  unsigned int state();

  unsigned int cause() {return _cause;}
  void cause(unsigned int cause) {_cause = cause;}

  Severity severity() {return _severity;}
  void severity(Severity severity) {_severity = severity;}

  const std::string& identifier() {return _identifier;}
  void identifier(const char* identifier) {_identifier = identifier;}

  const std::string& description() {return _description;}
  void description(const char* description) {_description = description;}

  const std::string& details() {return _details;}
  void details(const char* details) {_details = details;}

  bool is_valid() {return _index != 0;}
  bool is_not_clear() {return _severity != SEVERITY_CLEARED;}

private:
  unsigned int _index;
  unsigned int _cause;

  Severity _severity;
  
  std::string _identifier;
  std::string _description;
  std::string _details;
};

// Abstraction for an alarm definition file. Each of these files, located in
// /etc/clearwater/alarm-defs, contains JSON representations of the alarms that
// may be issued by a single clearwater component (e.g. sprout, chronos, etc).

class AlarmDefFile
{
public:
  // Parse file and return a list of extracted alarm definitions if 
  // successful.
  bool parse(std::list<AlarmDef>& defs);

  // Obtain last modification time of file. Zero return indicates failure.
  time_t last_modified();

  // Return a list of alarm definition files from alarm-defs directory. 
  static std::vector<AlarmDefFile>& alarm_def_files();

private:
  AlarmDefFile() : _path(NULL) {}
  AlarmDefFile(const std::string& path) : _path(path) {}

  bool parse_doc(rapidjson::Document& doc);
  bool parse_def(unsigned int obj_idx, const rapidjson::Value& obj, AlarmDef& def);

  std::string _path;

  static std::vector<AlarmDefFile> _def_files;
};

// Iterators for enumerating all alarm definitions, and only those with a severity
// of CLEARED. Subclassed from map iterator to hide pair template. Only operations
// defined are supported.

class AlarmDefsIterator : public std::map<std::string, AlarmDef*>::iterator
{
public:
  AlarmDefsIterator(std::map<std::string, AlarmDef*>::iterator iter) : std::map<std::string, AlarmDef*>::iterator(iter) {}

  AlarmDef& operator*()  {return *(std::map<std::string, AlarmDef*>::iterator::operator*().second);}
  AlarmDef* operator->() {return   std::map<std::string, AlarmDef*>::iterator::operator*().second ;}
};


class AlarmClearIterator : public std::map<unsigned int, AlarmDef*>::iterator
{
public:
  AlarmClearIterator(std::map<unsigned int, AlarmDef*>::iterator iter) : std::map<unsigned int, AlarmDef*>::iterator(iter) {}

  AlarmDef& operator*()  {return *(std::map<unsigned int, AlarmDef*>::iterator::operator*().second);}
  AlarmDef* operator->() {return   std::map<unsigned int, AlarmDef*>::iterator::operator*().second ;}
};

// Singleton class providing access to the alarm definitions for this node.

class AlarmDefs
{
public:
  // Read alarm definitions from JSON files, and build access maps. Should only
  // be called once at sub-agent initialization.
  bool load();

  // Retrieve alarm definition by alarm identifier string.
  AlarmDef* get_definition(const std::string& id);

  // Retrieve alarm definition with CLEARED severity for alarm model index.
  AlarmDef* get_clear_definition(unsigned int idx);

  // Iterator helpers for enumerating all alarm definitions.
  AlarmDefsIterator begin() {return _id_to_def.begin();}
  AlarmDefsIterator end() {return _id_to_def.end();}

  // Iterator helpers for enumerating alarm definitions with CLEARED severity.
  AlarmClearIterator begin_clear() {return _idx_to_clear_def.begin();}
  AlarmClearIterator end_clear() {return _idx_to_clear_def.end();}

  static AlarmDefs& get_instance() {return _instance;}

private:
  AlarmDefs() {};

  AlarmDef _invalid_def;

  std::list<AlarmDef> _defs;

  std::map<std::string, AlarmDef*>  _id_to_def;
  std::map<unsigned int, AlarmDef*> _idx_to_clear_def;

  static AlarmDefs _instance;
};

#endif

