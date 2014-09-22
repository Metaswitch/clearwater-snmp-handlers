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

/*
 *  Note: this file originally auto-generated by mib2c using
 *         : mib2c.array-user.conf 15997 2007-03-25 22:28:35Z dts12 $
 *
 *  $Id:$
 */

#ifndef ITU_ALARM_TABLE_HPP
#define ITU_ALARM_TABLE_HPP

#include <alarm_defs.hpp>

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


typedef struct ituAlarmTable_context_s {
    netsnmp_index _index; /** THIS MUST BE FIRST!!! */

    AlarmDef* _alarm_def;

} ituAlarmTable_context;


/*************************************************************
 *
 *  function declarations
 */
void init_ituAlarmTable(void);
int initialize_table_ituAlarmTable(void);
int ituAlarmTable_get_value(netsnmp_request_info*, netsnmp_index*, netsnmp_table_request_info*);

ituAlarmTable_context* ituAlarmTable_create_row_context(char*, unsigned long, long);
int ituAlarmTable_index_to_oid(char*, unsigned long, long, netsnmp_index*);


/*************************************************************
 *
 *  oid declarations
 */
extern oid ituAlarmTable_oid[];
extern size_t ituAlarmTable_oid_len;

#define ituAlarmTable_TABLE_OID 1,3,6,1,2,1,121,1,1,1

#define ALARM_MODEL_TABLE_ROW_OID 1,3,6,1,2,1,118,1,1,2,1,3,0,1,2


/*************************************************************
 *
 *  column number definitions for table ituAlarmTable
 */
#define COLUMN_ITUALARMPERCEIVEDSEVERITY 1
#define COLUMN_ITUALARMEVENTTYPE 2
#define COLUMN_ITUALARMPROBABLECAUSE 3
#define COLUMN_ITUALARMADDITIONALTEXT 4
#define COLUMN_ITUALARMGENERICMODEL 5
#define ituAlarmTable_COL_MIN 2
#define ituAlarmTable_COL_MAX 5


#define IANAITUEVENTTYPE_OPERATIONALVIOLATION 8

#define ALARMMODELTABLEROW_INDEX 13
#define ALARMMODELTABLEROW_STATE 14


#ifdef __cplusplus
}
#endif

#endif /** ALARMMODELTABLE_HPP */