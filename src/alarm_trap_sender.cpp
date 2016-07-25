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
#include <time.h>
#include <cmath>
#include <arpa/inet.h>

#include "log.h"
#include "alarm_trap_sender.hpp"
#include "alarm_scheduler.hpp"
#include "itu_alarm_table.hpp"
#include "alarm_active_table.hpp"

AlarmTrapSender AlarmTrapSender::_instance;

static int alarm_trap_send_callback(int op,
                                    snmp_session* session,
                                    int req_id,
                                    snmp_pdu* pdu,
                                    void* correlator)
{
  AlarmTableDef* alarm_table_def = (AlarmTableDef*)correlator; correlator = NULL;
  AlarmTrapSender::get_instance().alarm_trap_send_callback(op, *alarm_table_def);
  return 1;
}

void AlarmTrapSender::alarm_trap_send_callback(
                                           int op,
                                           const AlarmTableDef& alarm_table_def)
{
  switch (op)
  {
  case NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE:
    TRC_DEBUG("Alarm successfully delivered");
    break;
  case NETSNMP_CALLBACK_OP_SEND_FAILED:
    // There is no path through NETSNMP that uses this OP.  If there were, we'd
    // want to delay before re-trying, but this is non-trivial to accomplish
    // with the current architecture.  For now, if this OP occurs, we'll treat
    // it as a successful send.
    // LCOV_EXCL_START - Unhittable
    TRC_WARNING("Ignoring failed alarm send");
    break;
    // LCOV_EXCL_STOP
  case NETSNMP_CALLBACK_OP_TIMED_OUT:
    TRC_DEBUG("Failed to deliver alarm");
    _alarm_scheduler->handle_failed_alarm((AlarmTableDef&)alarm_table_def);
    break;
  default:
    // LCOV_EXCL_START - logic error
    TRC_DEBUG("Ignoring unexpected alarm delivery callback (%d)", op);
    break;
    // LCOV_EXCL_STOP
  }
}


// Sends an alarmActiveState or alarmClearState inform notification based
// upon the specified alarm definition. net-snmp will handle the required
// retires if needed.
void AlarmTrapSender::send_trap(const AlarmTableDef& alarm_table_def)
{
  TRC_INFO("Trap with alarm ID %d.%d being sent", alarm_table_def.alarm_index(), alarm_table_def.state());

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
  if (alarm_table_def.severity() == AlarmDef::CLEARED)
  {
    snmp_set_var_typed_value(&var_trap, ASN_OBJECT_ID, (u_char*) clear_oid, sizeof(clear_oid));
  }
  else
  {
    snmp_set_var_typed_value(&var_trap, ASN_OBJECT_ID, (u_char*) active_oid, sizeof(active_oid));
  }

  snmp_set_var_objid(&var_model_row, model_ptr_oid, OID_LENGTH(model_ptr_oid));
  snmp_set_var_typed_value(&var_model_row, ASN_OBJECT_ID, (u_char*) model_row_oid, sizeof(model_row_oid));

  var_model_row.val.objid[ALARMMODELTABLEROW_INDEX] = alarm_table_def.alarm_index();
  var_model_row.val.objid[ALARMMODELTABLEROW_STATE] = alarm_table_def.state();

  snmp_set_var_objid(&var_resource_id, resource_id_oid, OID_LENGTH(resource_id_oid));
  snmp_set_var_typed_value(&var_resource_id, ASN_OBJECT_ID, (u_char*) zero_dot_zero, sizeof(zero_dot_zero));

  send_v2trap(&var_trap, ::alarm_trap_send_callback, (void*)&alarm_table_def); 

  snmp_reset_var_buffers(&var_trap);
}
