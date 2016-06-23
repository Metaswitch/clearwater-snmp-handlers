/**
 * Project Clearwater - IMS in the cloud.
 * Copyright (C) 2015  Metaswitch Networks Ltd
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version, along with the "Special Exception" for use of
 * the program along with SSL, set forth below. This program is distributed
 * in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * The author can be reached by email at clearwater@metaswitch.com or by
 * post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
 *
 * Special Exception
 * Metaswitch Networks Ltd  grants you permission to copy, modify,
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

#include "test_snmp.h"
#include "alarm_model_table.hpp"
#include "itu_alarm_table.hpp"
#include "alarm_active_table.hpp"
#include "alarm_tables_testing.hpp"

std::string alarmModelTable_entry = "1.3.6.1.2.1.118.1.1.2.1";
std::string ituAlarmTable_entry = "1.3.6.1.2.1.121.1.1.1.1";
std::string alarmActiveTable_table = "1.3.6.1.2.1.118.1.2.2";


//Alarms are stored in ITU Alarm Table using ituAlarmPerceivedSeverity.
//Alarm Model Table stores alarms according to alarmModelState. The
//mapping between severity and state is described in RFC 3877
//section 5.4: https://tools.ietf.org/html/rfc3877#section-5.4
//and can be done using the below array.
static const unsigned int severity_to_state[] = {2, 1, 2, 6, 5, 4, 3};

std::string model_description = std::to_string(COLUMN_ALARMMODELDESCRIPTION);
std::string model_notification = std::to_string(COLUMN_ALARMMODELNOTIFICATIONID);
std::string str_cleared = std::to_string(AlarmDef::CLEARED);
std::string str_state_raised = std::to_string(severity_to_state[AlarmDef::CRITICAL]);

std::string itu_details = std::to_string(COLUMN_ITUALARMADDITIONALTEXT);
std::string itu_cause = std::to_string(COLUMN_ITUALARMPROBABLECAUSE);
std::string str_severity_raised = std::to_string(AlarmDef::CRITICAL);

char buf[1024];

TEST_F(CustomDefs, ModelTable)
{
  AlarmTableDefs::get_instance().insert_def(*def_cleared);
  AlarmTableDefs::get_instance().insert_def(*def_raised);

  alarmModelTable_insert_defs();

  // Verifies the custom alarm data (contained in the header file) is exactly what
  // is stored in the table. We use the index of the alarm "0.6666" in the
  // snmp_get_raw command.
  ASSERT_STREQ("\"Test alarm cleared description\"\n", snmp_get_raw(alarmModelTable_entry + "." + model_description + ".0.6666." + str_cleared, buf, sizeof(buf)));

  //Here we use AlarmModelState to query AlarmModelTable, for details of the
  //mapping to ituAlarmPerceivedSeverity see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("\"Test alarm raised description\"\n", snmp_get_raw(alarmModelTable_entry + "." + model_description + ".0.6666." + str_state_raised, buf, sizeof(buf)));

  // Verifies the third column (which contains the notification type) is the
  // alarmClearState oid and the alarmActiveState oid for the cleared and raised
  // alarm respectively.
  ASSERT_STREQ(".1.3.6.1.2.1.118.0.3\n", snmp_get_raw(alarmModelTable_entry + "." + model_notification + ".0.6666." + str_cleared, buf, sizeof(buf)));
  ASSERT_STREQ(".1.3.6.1.2.1.118.0.2\n", snmp_get_raw(alarmModelTable_entry + "." + model_notification + ".0.6666." + str_state_raised, buf, sizeof(buf)));
}

TEST_F(CustomDefs, ituAlarmTable)
{
  AlarmTableDefs::get_instance().insert_def(*def_cleared);
  AlarmTableDefs::get_instance().insert_def(*def_raised);

  ituAlarmTable_insert_defs();

  ASSERT_STREQ("\"Test alarm cleared details\"\n", snmp_get_raw(ituAlarmTable_entry + "." + itu_details + ".0.6666." +str_cleared, buf, sizeof(buf)));
  //Here we use ituAlarmPerceivedSeverity to query ituAlarmTable, for
  //details of the mapping to AlarmModelState see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("\"Test alarm raised details\"\n", snmp_get_raw(ituAlarmTable_entry + "." + itu_details + ".0.6666." + str_severity_raised, buf, sizeof(buf)));
  ASSERT_EQ(163u, snmp_get(ituAlarmTable_entry + "." + itu_cause + ".0.6666." + str_cleared));
  ASSERT_EQ(163u, snmp_get(ituAlarmTable_entry + "." + itu_cause + ".0.6666." + str_severity_raised));
}

TEST_F(CustomDefs, ActiveTableMultipleAlarms)
{
  // The alarmActiveTable is initialised in the SetUp function defined in the
  // header file.

  // Raises three alarms.
  alarmActiveTable_trap_handler(*def1_raised);
  alarmActiveTable_trap_handler(*def2_raised);
  alarmActiveTable_trap_handler(*def3_raised);

  // Clears the second alarm.
  alarmActiveTable_trap_handler(*def2_cleared);

  // Raises two more alarms.
  alarmActiveTable_trap_handler(*def4_raised);
  alarmActiveTable_trap_handler(*def5_raised);

  // Clears the fifth and the first alarm (so the only alarms that should be
  // raised still are the third and the fourth)
  alarmActiveTable_trap_handler(*def5_cleared);
  alarmActiveTable_trap_handler(*def1_cleared);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. Each alarm has 11
  // columns and there should be 2 alarms raised.
  ASSERT_EQ(22u, entries.size());

  // Clears remaining alarms.
  alarmActiveTable_trap_handler(*def3_cleared);
  alarmActiveTable_trap_handler(*def4_cleared);

  // Checks there are now no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0u, entries_update.size());
}

TEST_F(CustomDefs, ActiveTableRepeatAlarm)
{
  // Raises an alarm.
  alarmActiveTable_trap_handler(*def_raised);

  // Raises the same alarm again.
  alarmActiveTable_trap_handler(*def_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. The second alarm
  // raised was identical to the first and hence there should only be one entry
  // which has 11 columns.
  ASSERT_EQ(11u, entries.size());

  // Clears the alarm.
  alarmActiveTable_trap_handler(*def_cleared);

  // Checks there are now no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0u, entries_update.size());
}

TEST_F(CustomDefs, ActiveTableClearingUnraisedAlarm)
{
  // Clears an alarm (despite not raising it beforehand).
  alarmActiveTable_trap_handler(*def_cleared);

  // Checks there are no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0u, entries_update.size());
}

TEST_F(CustomDefs, ActiveTableSameAlarmDifferentSeverities)
{

  // Raises the critical alarm followed by the major alarm.
  alarmActiveTable_trap_handler(*def_raised_critical);
  alarmActiveTable_trap_handler(*def_raised_major);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. The second alarm
  // should have overwritten the first and hence there should only be one entry
  // with 11 columns.
  ASSERT_EQ(11u, entries.size());

  // Calculates which column should contain the alarm description based off the
  // column number definitions within alarm_active_table.hpp
  int colnum_description = COLUMN_ALARMACTIVEDESCRIPTION - alarmActiveTable_COL_MIN;
  // Checks that the alarm description column contains latest severity data.
  std::size_t found = entries[colnum_description].find("Test alarm major raised description");
  ASSERT_NE(std::string::npos, found);

  // Clears the alarm and checks there are now no entries in the table.
  alarmActiveTable_trap_handler(*def_cleared);
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0u, entries_update.size());
}

TEST_F(CustomDefs, ActiveTableIndexTimeStamp)
{
  // Raises three alarms.
  alarmActiveTable_trap_handler(*def1_raised);
  alarmActiveTable_trap_handler(*def2_raised);
  alarmActiveTable_trap_handler(*def3_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Retrieves the current date.
  time_t ts = time(NULL);
  struct tm timing = *localtime(&ts);
  int day = timing.tm_mday;
  int month = 1 + timing.tm_mon;

  // Splits each part of the index field (separated by ".") for the
  // first alarm into a vector.
  std::vector<std::string> index_fields1;
  Utils::split_string(entries[0], '.', index_fields1);

  // As defined in RFC3877 the 17th and 16th elements of the index
  // field are the day and month that the alarm was raised respectively.
  // This checks those values are the current day and month as retreived
  // above.
  ASSERT_EQ(std::to_string(day), index_fields1[17]);
  ASSERT_EQ(std::to_string(month), index_fields1[16]);
}

TEST_F(CustomDefs, AlarmTableIndexSize)
{
  //Raises an alarm.
  alarmActiveTable_trap_handler(*def1_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Splits each part of the index field (separated by ".") of the
  // alarm into a vector.
  std::vector<std::string> index_fields;
  Utils::split_string(entries[0], '.', index_fields);

  // Checks the index of the first alarm contains 26 elements as defined
  // in RFC 3877.
  ASSERT_EQ(26u, index_fields.size());
}

TEST_F(CustomDefs, AlarmTableAlarmsIndex)
{
  // Raises three alarms.
  alarmActiveTable_trap_handler(*def1_raised);
  alarmActiveTable_trap_handler(*def2_raised);
  alarmActiveTable_trap_handler(*def3_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks the indexes for the three alarms are strictly monotonically
  // increasing in the order they were raised.
  ASSERT_LT(atoi(get_index_from_row(entries[0]).c_str()), atoi(get_index_from_row(entries[13]).c_str()));
  ASSERT_LT(atoi(get_index_from_row(entries[13]).c_str()), atoi(get_index_from_row(entries[26]).c_str()));
}
