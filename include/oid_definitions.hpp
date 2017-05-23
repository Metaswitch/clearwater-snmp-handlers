/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef OID_DEFINITIONS_HPP
#define OID_DEFINITIONS_HPP

#include <alarm_table_defs.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <net-snmp/library/snmp_assert.h>
    
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/library/container.h>
#include <net-snmp/agent/table_array.h>

#define alarmModelTable_TABLE_OID 1,3,6,1,2,1,118,1,1,2
#define alarmActiveTable_TABLE_OID 1,3,6,1,2,1,118,1,2,2
#define ituAlarmTable_TABLE_OID 1,3,6,1,2,1,121,1,1,1
#define ZERO_DOT_ZERO_OID 0,0
#define ALARM_ACTIVE_STATE_OID 1,3,6,1,2,1,118,0,2
#define ALARM_CLEAR_STATE_OID 1,3,6,1,2,1,118,0,3
#define ITU_ALARM_TABLE_ROW_OID 1,3,6,1,2,1,121,1,1,1,1,2,0,1,1
#define ALARM_MODEL_TABLE_ROW_OID 1,3,6,1,2,1,118,1,1,2,1,3,0,1,2
#define ENTRY_COLUMN_OID 1,3

const oid alarmModelTable_oid[] = { alarmModelTable_TABLE_OID };
const size_t alarmModelTable_oid_len = OID_LENGTH(alarmModelTable_oid);
const oid alarmActiveTable_oid[] = { alarmActiveTable_TABLE_OID };
const size_t alarmActiveTable_oid_len = OID_LENGTH(alarmActiveTable_oid);
const oid ituAlarmTable_oid[] = { ituAlarmTable_TABLE_OID };
const size_t ituAlarmTable_oid_len = OID_LENGTH(ituAlarmTable_oid);
const oid zero_dot_zero_oid[] = { ZERO_DOT_ZERO_OID };
const oid alarm_active_state_oid[] = { ALARM_ACTIVE_STATE_OID };
const oid alarm_clear_state_oid[] = { ALARM_CLEAR_STATE_OID };
const oid itu_alarm_table_row_oid[] = { ITU_ALARM_TABLE_ROW_OID };
const oid alarm_model_table_row_oid[] = { ALARM_MODEL_TABLE_ROW_OID };
const oid entry_column_oid[] = { ENTRY_COLUMN_OID };
const size_t entry_column_oid_len = OID_LENGTH(entry_column_oid);

#ifdef __cplusplus
}
#endif

#endif /** OID_DEFINITIONS_HPP */
