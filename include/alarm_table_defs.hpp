/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef ALARM_TABLE_DEFS_HPP
#define ALARM_TABLE_DEFS_HPP

#include <map>

#include "alarmdefinition.h"

// Container for data needed to generate entries of the Alarm Model Table
// and ITU Alarm Table.
class AlarmTableDef
{
public:
  enum {
    MIB_STRING_LEN = 255
  };

  AlarmTableDef() :
    _valid(false),
    _alarm_definition(),
    _severity_details() {}

  AlarmTableDef(const AlarmDef::AlarmDefinition alarm_definition,
                const AlarmDef::SeverityDetails severity_details) :
    _valid(true),
    _alarm_definition(alarm_definition),
    _severity_details(severity_details) {}

  unsigned int state() const;

  const std::string& name() const     {return _alarm_definition._name;}
  unsigned int alarm_index() const    {return _alarm_definition._index;} 
  AlarmDef::Cause alarm_cause() const {return _alarm_definition._cause;}

  AlarmDef::Severity severity() const             {return _severity_details._severity;}
  const std::string& description() const          {return _severity_details._description;}
  const std::string& details() const              {return _severity_details._details;}
  const std::string& cause() const                {return _severity_details._cause;}
  const std::string& effect() const               {return _severity_details._effect;}
  const std::string& action() const               {return _severity_details._action;}
  const std::string& extended_details() const     {return _severity_details._extended_details;}
  const std::string& extended_description() const {return _severity_details._extended_description;}

  // LCOV_EXCL_START
  bool is_valid() const     {return _valid;}
  bool is_not_clear() const {return severity() != AlarmDef::CLEARED;}
  // LCOV_EXCL_STOP

private:
  bool _valid;

  AlarmDef::AlarmDefinition _alarm_definition;
  AlarmDef::SeverityDetails _severity_details;
};

// Unique key for alarm table definitions is comprised of alarm index and
// alarm severity.
class AlarmTableDefKey
{
public:
  AlarmTableDefKey(unsigned int index, unsigned int severity) :
    _index(index), _severity(severity) {}

  bool operator<(const AlarmTableDefKey& rhs) const;

private:
  unsigned int _index;
  unsigned int _severity;
};

// Iterator for enumerating all alarm table definitions. Subclassed from 
// map's iterator to hide pair template. Only operations defined are
// supported.
class AlarmTableDefsIterator : public std::map<AlarmTableDefKey, AlarmTableDef>::iterator
{
public:
  AlarmTableDefsIterator(std::map<AlarmTableDefKey, AlarmTableDef>::iterator iter) : std::map<AlarmTableDefKey, AlarmTableDef>::iterator(iter) {}

  AlarmTableDef& operator*()  {return   std::map<AlarmTableDefKey, AlarmTableDef>::iterator::operator*().second ;}
  AlarmTableDef* operator->() {return &(std::map<AlarmTableDefKey, AlarmTableDef>::iterator::operator*().second);}
};

/// Class providing access to alarm table definitions.
class AlarmTableDefs
{
public:
  // Generate alarm table definitions based on JSON files on the node
  bool initialize(std::string& path);

  // Retrieve alarm definition for specified index/severity
  AlarmTableDef& get_definition(unsigned int index,
                                unsigned int severity);

  // Iterator helpers for enumerating alarm table definitions.
  AlarmTableDefsIterator begin() {return _key_to_def.begin();}
  AlarmTableDefsIterator end()   {return _key_to_def.end();}

private:
  // Insert an AlarmTableDef into the _key_to_def list, so that it can
  // later be retrieved with get_definition.
  void insert_def(AlarmTableDef);

  // Populate the map of alarm definitions
  bool populate_map(std::string path,
                    std::map<unsigned int, unsigned int>& dup_check);

  std::map<AlarmTableDefKey, AlarmTableDef> _key_to_def;

  AlarmTableDef _invalid_def;
};

#endif

