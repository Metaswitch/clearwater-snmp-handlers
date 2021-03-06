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

#ifndef ALARM_MODEL_TABLE_HPP
#define ALARM_MODEL_TABLE_HPP

#include <alarm_table_defs.hpp>
#include "oid_definitions.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alarmModelTable_context_s {
    netsnmp_index _index; /** THIS MUST BE FIRST!!! */

    AlarmTableDef* _alarm_table_def;

} alarmModelTable_context;

void init_alarmModelTable(AlarmTableDefs& defs);
int initialize_table_alarmModelTable(void);
int alarmModelTable_get_value(netsnmp_request_info*, netsnmp_index*, netsnmp_table_request_info*);

alarmModelTable_context* alarmModelTable_create_row_context(char*, unsigned long, unsigned long);
int alarmModelTable_index_to_oid(char*, unsigned long, unsigned long, netsnmp_index*);
void alarmModelTable_insert_defs(AlarmTableDefs& defs);

/*************************************************************
 *
 *  column number definitions for table alarmModelTable
 */
#define COLUMN_ALARMMODELINDEX 1
#define COLUMN_ALARMMODELSTATE 2
#define COLUMN_ALARMMODELNOTIFICATIONID 3
#define COLUMN_ALARMMODELVARBINDINDEX 4
#define COLUMN_ALARMMODELVARBINDVALUE 5
#define COLUMN_ALARMMODELDESCRIPTION 6
#define COLUMN_ALARMMODELSPECIFICPOINTER 7
#define COLUMN_ALARMMODELVARBINDSUBTREE 8
#define COLUMN_ALARMMODELRESOURCEPREFIX 9
#define COLUMN_ALARMMODELROWSTATUS 10
#define alarmModelTable_COL_MIN 3
#define alarmModelTable_COL_MAX 10

#define ROWSTATUS_ACTIVE 1

#define ITUALARMTABLEROW_INDEX 13
#define ITUALARMTABLEROW_SEVERITY 14

#ifdef __cplusplus
}
#endif

#endif /** ALARMMODELTABLE_HPP */
