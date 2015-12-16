/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2015 Metaswitch Networks Ltd
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
