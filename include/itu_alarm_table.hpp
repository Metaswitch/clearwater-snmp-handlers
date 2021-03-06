/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

/*
 *  Note: this file originally auto-generated by mib2c using
 *         : mib2c.array-user.conf 15997 2007-03-25 22:28:35Z dts12 $
 *
 *  $Id:$
 */

#ifndef ITU_ALARM_TABLE_HPP
#define ITU_ALARM_TABLE_HPP

#include <alarm_table_defs.hpp>
#include "oid_definitions.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ituAlarmTable_context_s {
    netsnmp_index _index; /** THIS MUST BE FIRST!!! */

    AlarmTableDef* _alarm_table_def;

} ituAlarmTable_context;

void init_ituAlarmTable(AlarmTableDefs& defs);
int initialize_table_ituAlarmTable(void);
int ituAlarmTable_get_value(netsnmp_request_info*, netsnmp_index*, netsnmp_table_request_info*);

ituAlarmTable_context* ituAlarmTable_create_row_context(char*, unsigned long, long);
int ituAlarmTable_index_to_oid(char*, unsigned long, long, netsnmp_index*);
void ituAlarmTable_insert_defs(AlarmTableDefs& defs);

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
