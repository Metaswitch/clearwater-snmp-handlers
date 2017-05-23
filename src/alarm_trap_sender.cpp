/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
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

#define CREATE_NETSNMP_LIST(LIST_NAME) \
  netsnmp_variable_list LIST_NAME; \
  memset(&LIST_NAME, 0x00, sizeof(LIST_NAME)); 

// This is a date in the format YYYYMMDDHHMM that specfies when the current MIB
// version was implemented. This should be changed whenever new MIBs are
// introduced.
std::string MIB_VERSION = "201608081100";

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
  for (std::set<NotificationType>::iterator it = _snmp_notifications.begin();
       it != _snmp_notifications.end();
       ++it)
  {
    switch (*it)
    {
      case NotificationType::RFC3877:
        send_rfc3877_trap(alarm_table_def);
        break;
      case NotificationType::ENTERPRISE:
        send_enterprise_trap(alarm_table_def);
        break;
      default:
        // LCOV_EXCL_START
        TRC_ERROR("Unknown notification type passed to the trap sender");
        break;
        // LCOV_EXCL_STOP
    }
  }
}

void AlarmTrapSender::send_rfc3877_trap(const AlarmTableDef& alarm_table_def)
{
  TRC_INFO("RFC3877 compliant trap with alarm ID %d.%d being sent", alarm_table_def.alarm_index(),
                                                                    alarm_table_def.state());

  static const oid snmp_trap_oid[] = {1,3,6,1,6,3,1,1,4,1,0};
  static const oid clear_oid[] = {1,3,6,1,2,1,118,0,3};
  static const oid active_oid[] = {1,3,6,1,2,1,118,0,2};

  static const oid model_ptr_oid[] = {1,3,6,1,2,1,118,1,2,2,1,13,0};
  static const oid model_row_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2};

  static const oid resource_id_oid[] = {1,3,6,1,2,1,118,1,2,2,1,10,0};
  static const oid zero_dot_zero[] = {0,0};

  CREATE_NETSNMP_LIST(var_trap);
  CREATE_NETSNMP_LIST(var_model_row);
  CREATE_NETSNMP_LIST(var_resource_id);

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

void AlarmTrapSender::send_enterprise_trap(const AlarmTableDef& alarm_table_def)
{
  TRC_INFO("Enterprise MIB trap with alarm ID %d.%d being sent", alarm_table_def.alarm_index(),
                                                                 alarm_table_def.state());

  // Here we define OIDs according to the CLEARWATER-ENTERPRISE-MIB
  static const std::string severity_to_string[] = {"UNDEFINED_SEVERITY", 
                                                   "CLEARED",
                                                   "INDETERMINATE", 
                                                   "CRITICAL", 
                                                   "MAJOR", 
                                                   "MINOR", 
                                                   "WARNING"};
  static const oid snmp_trap_oid[] = {1,3,6,1,6,3,1,1,4,1,0};
  static const oid trap_type_oid[] = {1,3,6,1,4,1,19444,12,2,0,1};
  static const oid MIB_version_oid[] = {1,2,826,0,1,1578918,12,1,1};
  static const oid alarm_name_oid[] = {1,3,6,1,4,1,19444,12,2,0,2};
  static const oid alarm_oid_oid[] = {1,3,6,1,4,1,19444,12,2,0,3};
  static const oid resource_id_oid[] = {1,3,6,1,4,1,19444,12,2,0,4};
  static const oid zero_dot_zero[] = {0,0};
  static const oid model_row_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2};
  static const oid alarm_severity_oid[] = {1,3,6,1,4,1,19444,12,2,0,5};
  static const oid alarm_description_oid[] = {1,3,6,1,4,1,19444,12,2,0,6};
  static const oid alarm_details_oid[] = {1,3,6,1,4,1,19444,12,2,0,7};
  static const oid alarm_cause_oid[] = {1,3,6,1,4,1,19444,12,2,0,8};
  static const oid alarm_effect_oid[] = {1,3,6,1,4,1,19444,12,2,0,9};
  static const oid alarm_action_oid[] = {1,3,6,1,4,1,19444,12,2,0,10};
  static const oid alarm_hostname_oid[] = {1,3,6,1,4,1,19444,12,2,0,12};
  
  // We create variable lists, wipe them clean and then link them to each other
  // so only the first needs to be passed to sent_v2trap
  CREATE_NETSNMP_LIST(var_trap);
  CREATE_NETSNMP_LIST(var_MIB_version);
  CREATE_NETSNMP_LIST(var_alarm_name);
  CREATE_NETSNMP_LIST(var_alarm_oid);
  CREATE_NETSNMP_LIST(var_resource_id);
  CREATE_NETSNMP_LIST(var_alarm_severity);
  CREATE_NETSNMP_LIST(var_alarm_description);
  CREATE_NETSNMP_LIST(var_alarm_details);
  CREATE_NETSNMP_LIST(var_alarm_cause);
  CREATE_NETSNMP_LIST(var_alarm_effect);
  CREATE_NETSNMP_LIST(var_alarm_action);
  CREATE_NETSNMP_LIST(var_alarm_hostname);
  
  var_trap.next_variable = &var_MIB_version;
  var_MIB_version.next_variable = &var_alarm_name;
  var_alarm_name.next_variable = &var_alarm_oid;
  var_alarm_oid.next_variable = &var_resource_id;
  var_resource_id.next_variable = &var_alarm_severity;
  var_alarm_severity.next_variable = &var_alarm_description;
  var_alarm_description.next_variable = &var_alarm_details;
  var_alarm_details.next_variable = &var_alarm_cause;
  var_alarm_cause.next_variable = &var_alarm_effect;
  var_alarm_effect.next_variable = &var_alarm_action;
  var_alarm_action.next_variable = &var_alarm_hostname;
  var_alarm_hostname.next_variable = NULL;

  // Sets the object id of each of the variable lists to the OIDs defined above,
  // these can be thought of as the keys for each of the variable bindings.
  snmp_set_var_objid(&var_trap, snmp_trap_oid, OID_LENGTH(snmp_trap_oid));
  snmp_set_var_objid(&var_MIB_version, MIB_version_oid, OID_LENGTH(MIB_version_oid));
  snmp_set_var_objid(&var_alarm_name, alarm_name_oid, OID_LENGTH(alarm_name_oid));
  snmp_set_var_objid(&var_alarm_oid, alarm_oid_oid, OID_LENGTH(alarm_oid_oid));
  snmp_set_var_objid(&var_resource_id, resource_id_oid, OID_LENGTH(resource_id_oid));
  snmp_set_var_objid(&var_alarm_severity, alarm_severity_oid, OID_LENGTH(alarm_severity_oid));
  snmp_set_var_objid(&var_alarm_description, alarm_description_oid, OID_LENGTH(alarm_description_oid));
  snmp_set_var_objid(&var_alarm_details, alarm_details_oid, OID_LENGTH(alarm_details_oid));
  snmp_set_var_objid(&var_alarm_cause, alarm_cause_oid, OID_LENGTH(alarm_cause_oid));
  snmp_set_var_objid(&var_alarm_effect, alarm_effect_oid, OID_LENGTH(alarm_effect_oid));
  snmp_set_var_objid(&var_alarm_action, alarm_action_oid, OID_LENGTH(alarm_action_oid));
  snmp_set_var_objid(&var_alarm_hostname, alarm_hostname_oid, OID_LENGTH(alarm_hostname_oid));

  // Sets the value of each of the variable lists, these can be thought of as
  // the values which correspond to the above keys in the variable bindings.
  snmp_set_var_typed_value(&var_trap, ASN_OBJECT_ID, 
                           (u_char*) trap_type_oid, 
                           sizeof(trap_type_oid));
  snmp_set_var_typed_value(&var_MIB_version, ASN_OCTET_STR,
                           MIB_VERSION.c_str(),
                           MIB_VERSION.length());
  snmp_set_var_typed_value(&var_alarm_name, ASN_OCTET_STR,
                           alarm_table_def.name().c_str(),
                           alarm_table_def.name().length());
  snmp_set_var_typed_value(&var_alarm_oid, ASN_OBJECT_ID,
                           (u_char*) model_row_oid,
                           sizeof(model_row_oid));

  var_alarm_oid.val.objid[ALARMMODELTABLEROW_INDEX] = alarm_table_def.alarm_index();
  
  snmp_set_var_typed_value(&var_resource_id, ASN_OBJECT_ID, 
                           (u_char*) zero_dot_zero, 
                           sizeof(zero_dot_zero));
  snmp_set_var_typed_value(&var_alarm_severity, ASN_OCTET_STR,
                           severity_to_string[alarm_table_def.severity()].c_str(),
                           severity_to_string[alarm_table_def.severity()].length());
  snmp_set_var_typed_value(&var_alarm_description, ASN_OCTET_STR, 
                           alarm_table_def.extended_description().c_str(), 
                           alarm_table_def.extended_description().length());
  snmp_set_var_typed_value(&var_alarm_details, ASN_OCTET_STR, 
                           alarm_table_def.extended_details().c_str(), 
                           alarm_table_def.extended_details().length());
  snmp_set_var_typed_value(&var_alarm_cause, ASN_OCTET_STR, 
                           alarm_table_def.cause().c_str(), 
                           alarm_table_def.cause().length());
  snmp_set_var_typed_value(&var_alarm_effect, ASN_OCTET_STR, 
                           alarm_table_def.effect().c_str(), 
                           alarm_table_def.effect().length());
  snmp_set_var_typed_value(&var_alarm_action, ASN_OCTET_STR, 
                           alarm_table_def.action().c_str(), 
                           alarm_table_def.action().length());
  snmp_set_var_typed_value(&var_alarm_hostname, ASN_OCTET_STR,
                           _hostname.c_str(),
                           _hostname.length());

  // Sends the trap passing the function the first of the linked lists.
  send_v2trap(&var_trap, ::alarm_trap_send_callback, (void*)&alarm_table_def);

  snmp_reset_var_buffers(&var_trap);
}
