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

#include <stdexcept>

#include "alarm_trap_sender.hpp"
#include "itu_alarm_table.hpp"

AlarmFilter AlarmFilter::_instance;

AlarmTrapSender AlarmTrapSender::_instance;

bool ActiveAlarmList::update(AlarmDef* alarm_def, const std::string& issuer)
{
  bool updated = false;

  std::map<unsigned int, AlarmListEntry>::iterator it = _idx_to_entry.find(alarm_def->index());

  if (alarm_def->is_not_clear())
  {
    if ((it == _idx_to_entry.end()) || (alarm_def->severity() != it->second.alarm_def()->severity()))
    {
      _idx_to_entry[alarm_def->index()] = AlarmListEntry(alarm_def, issuer);
      updated = true;
    }
  }
  else
  {
    if (it != _idx_to_entry.end())
    {
      _idx_to_entry.erase(it);
      updated = true;
    }
  }

  return updated;
}

void ActiveAlarmList::remove(ActiveAlarmIterator& it)
{
  _idx_to_entry.erase(it);
}

bool AlarmFilter::alarm_filtered(unsigned int index, AlarmDef::Severity severity)
{
  unsigned long now = current_time_ms();

  if (now > _clean_time)
  {
    _clean_time = now + CLEAN_FILTER_TIME;

    std::map<unsigned int, unsigned long>::iterator it = _issue_times.begin();
    while (it != _issue_times.end())
    {
      if (now > (it->second + ALARM_FILTER_TIME))
      {
        std::map<unsigned int, unsigned long>::iterator it_tmp = it;
        _issue_times.erase(it_tmp);
      }

      it++;
    }
  }

  unsigned int index_severity = (index << 3) | severity;

  std::map<unsigned int, unsigned long>::iterator it = _issue_times.find(index_severity);
 
  bool filtered = false;

  if (it != _issue_times.end())
  {
    if (now > (it->second + ALARM_FILTER_TIME))
    {
      _issue_times[index_severity] = now;
    }
    else
    {
      filtered = true;
    }
  }
  else
  {
    _issue_times[index_severity] = now;
  } 

  return filtered;
}


unsigned long AlarmFilter::current_time_ms()
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

void AlarmTrapSender::issue_alarm(const std::string& issuer, const std::string& identifier)
{
  AlarmDef* alarm_def = AlarmDefs::get_instance().get_definition(identifier);

  if (alarm_def->is_valid()) 
  {
    if (_active_alarms.update(alarm_def, issuer))
    {
      if (!AlarmFilter::get_instance().alarm_filtered(alarm_def->index(), alarm_def->severity()))
      {
        send_trap(alarm_def);
      }
    }
  }
}

void AlarmTrapSender::clear_alarms(const std::string& issuer)
{
  AlarmDefs& defs = AlarmDefs::get_instance();

  ActiveAlarmIterator it = _active_alarms.begin();
  while (it != _active_alarms.end())
  {
    if (it->issuer() == issuer)
    {
      AlarmDef* clear_def = defs.get_clear_definition(it->alarm_def()->index());
 
      if (!AlarmFilter::get_instance().alarm_filtered(clear_def->index(), clear_def->severity()))
      {
        send_trap(clear_def);
      }

      ActiveAlarmIterator it_tmp = it;
      _active_alarms.remove(it_tmp);
    }

    it++;
  }
}

void AlarmTrapSender::sync_alarms()
{
  AlarmDefs& defs = AlarmDefs::get_instance();

  for (AlarmClearIterator it = defs.begin_clear(); it != defs.end_clear(); it++)
  {
    send_trap(&(*it));
  }

  for (ActiveAlarmIterator it = _active_alarms.begin(); it != _active_alarms.end(); it++)
  {
    send_trap(it->alarm_def());
  }
}

// Sends an alarmActiveState or alarmClearState inform notification based
// upon the specified alarm definition. net-snmp will handle the required
// retires if needed.

void AlarmTrapSender::send_trap(AlarmDef* alarm)
{
  static const oid snmp_trap_oid[] = {1,3,6,1,6,3,1,1,4,1,0};
  static const oid clear_oid[] = {1,3,6,1,2,1,118,0,3};
  static const oid active_oid[] = {1,3,6,1,2,1,118,0,2};

  static const oid model_ptr_oid[] = {1,3,6,1,2,1,118,1,2,2,1,13,0};
  static const oid model_row_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2};

  static const oid resource_id_oid[] = {1,3,6,1,2,1,118,1,2,2,1,10,0};
  static const oid zero_dot_zero[] = {0,0};

  netsnmp_variable_list var_trap;
  netsnmp_variable_list var_model_row;
  netsnmp_variable_list var_resource_id;

  memset(&var_trap, 0x00, sizeof(var_trap));
  memset(&var_model_row, 0x00, sizeof(var_model_row));
  memset(&var_resource_id, 0x00, sizeof(var_resource_id));

  var_trap.next_variable = &var_model_row;
  var_model_row.next_variable = &var_resource_id;
  var_resource_id.next_variable = NULL;

  snmp_set_var_objid(&var_trap, snmp_trap_oid, OID_LENGTH(snmp_trap_oid));
  if (alarm->severity() == AlarmDef::SEVERITY_CLEARED)
  {
    snmp_set_var_typed_value(&var_trap, ASN_OBJECT_ID, (u_char*) clear_oid, sizeof(clear_oid));
  }
  else
  {
    snmp_set_var_typed_value(&var_trap, ASN_OBJECT_ID, (u_char*) active_oid, sizeof(active_oid));
  }

  snmp_set_var_objid(&var_model_row, model_ptr_oid, OID_LENGTH(model_ptr_oid));
  snmp_set_var_typed_value(&var_model_row, ASN_OBJECT_ID, (u_char*) model_row_oid, sizeof(model_row_oid));

  var_model_row.val.objid[ALARMMODELTABLEROW_INDEX] = alarm->index();
  var_model_row.val.objid[ALARMMODELTABLEROW_STATE] = alarm->state();

  snmp_set_var_objid(&var_resource_id, resource_id_oid, OID_LENGTH(resource_id_oid));
  snmp_set_var_typed_value(&var_resource_id, ASN_OBJECT_ID, (u_char*) zero_dot_zero, sizeof(zero_dot_zero));

  send_v2trap(&var_trap);

  snmp_reset_var_buffers(&var_trap);
}

