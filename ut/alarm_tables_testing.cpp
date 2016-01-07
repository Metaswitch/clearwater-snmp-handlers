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
#include "utils.h"
#include "stdlib.h"
#include <algorithm>
#include <string>

std::string alarmModelTable_entry = "1.3.6.1.2.1.118.1.1.2.1";
std::string ituAlarmTable_entry = "1.3.6.1.2.1.121.1.1.1.1";
std::string alarmActiveTable_table = "1.3.6.1.2.1.118.1.2.2";

char buf[1024];

TEST_F(SNMPTest, ModelTable)
{ 
  // Create a custom alarm definition
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL, "Test alarm raised description", "Test alarm raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised(example, raised);
  AlarmTableDefs::get_instance().insert_def(def_cleared);
  AlarmTableDefs::get_instance().insert_def(def_raised);

  init_alarmModelTable();
  
  // Verifies the above alarm data is exactly what is stored in
  // AlarmModelTable by querying the sixth column in alarmModelTable which
  // should contain the description of the alarm)
  ASSERT_STREQ("\"Test alarm cleared description\"\n", snmp_get_raw(alarmModelTable_entry + ".6.0.6666.1", buf, sizeof(buf)));

  //Here we use AlarmModelState (6) to query AlarmModelTable, for details of the
  //mapping to ituAlarmPerceivedSeverity see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("\"Test alarm raised description\"\n", snmp_get_raw(alarmModelTable_entry + ".6.0.6666.6", buf, sizeof(buf)));

  // Verifies the third column (which contains the notification type) is the
  // alarmClearState oid and the alarmActiveState oid for the cleared and raised
  // alarm respectively.
  ASSERT_STREQ("iso.3.6.1.2.1.118.0.3\n", snmp_get_raw(alarmModelTable_entry + ".3.0.6666.1", buf, sizeof(buf)));
  ASSERT_STREQ("iso.3.6.1.2.1.118.0.2\n", snmp_get_raw(alarmModelTable_entry + ".3.0.6666.6", buf, sizeof(buf)));
}

TEST_F(SNMPTest, ituAlarmTable)
{
  // Creates a custom alarm definition
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL, "Test alarm raised description", "Test alarm raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised(example, raised);
  AlarmTableDefs::get_instance().insert_def(def_cleared);
  AlarmTableDefs::get_instance().insert_def(def_raised);
  
  init_ituAlarmTable();

  ASSERT_STREQ("\"Test alarm cleared details\"\n", snmp_get_raw(ituAlarmTable_entry + ".4.0.6666.1", buf, sizeof(buf)));
  //Here we use ituAlarmPerceivedSeverity (3) to query ituAlarmTable, for
  //details of the mapping to AlarmModelState see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("\"Test alarm raised details\"\n", snmp_get_raw(ituAlarmTable_entry + ".4.0.6666.3", buf, sizeof(buf)));
  ASSERT_EQ(163, snmp_get(ituAlarmTable_entry + ".3.0.6666.1"));
  ASSERT_EQ(163, snmp_get(ituAlarmTable_entry + ".3.0.6666.3"));
}

TEST_F(SNMPTest, ActiveTableMultipleAlarms)
{
  init_alarmActiveTable("10.154.153.133");
  // Creates three custom alarm definitions and raises the alarms.
  AlarmDef::SeverityDetails cleared1(AlarmDef::CLEARED, "First alarm cleared description", "First alarm cleared details");
  AlarmDef::SeverityDetails raised1(AlarmDef::CRITICAL, "First alarm raised description", "First alarm raised details");
  AlarmDef::AlarmDefinition example1(6666, AlarmDef::SOFTWARE_ERROR, {cleared1, raised1});
  AlarmTableDef def1_cleared(example1, cleared1);
  AlarmTableDef def1_raised(example1, raised1);
  alarmActiveTable_trap_handler(def1_raised);

  AlarmDef::SeverityDetails cleared2(AlarmDef::CLEARED, "Second alarm cleared description", "Second alarm cleared details");
  AlarmDef::SeverityDetails raised2(AlarmDef::CRITICAL, "Second alarm raised description", "Second alarm raised details");
  AlarmDef::AlarmDefinition example2(6667, AlarmDef::SOFTWARE_ERROR, {cleared2, raised2});
  AlarmTableDef def2_cleared(example2, cleared2);
  AlarmTableDef def2_raised(example2, raised2);
  alarmActiveTable_trap_handler(def2_raised);

  AlarmDef::SeverityDetails cleared3(AlarmDef::CLEARED, "Third alarm cleared description", "Third alarm cleared details");
  AlarmDef::SeverityDetails raised3(AlarmDef::CRITICAL, "Third alarm raised description", "Third alarm raised details");
  AlarmDef::AlarmDefinition example3(6668, AlarmDef::SOFTWARE_ERROR, {cleared3, raised3});
  AlarmTableDef def3_cleared(example3, cleared3);
  AlarmTableDef def3_raised(example3, raised3);
  alarmActiveTable_trap_handler(def3_raised);

  // Clears the second alarm.
  alarmActiveTable_trap_handler(def2_cleared);

  // Creates two more custom alarms and raises them.
  AlarmDef::SeverityDetails cleared4(AlarmDef::CLEARED, "Fourth alarm cleared description", "Fourth alarm cleared details");
  AlarmDef::SeverityDetails raised4(AlarmDef::CRITICAL, "Fourth alarm raised description", "Fourth alarm raised details");
  AlarmDef::AlarmDefinition example4(6669, AlarmDef::SOFTWARE_ERROR, {cleared4, raised4});
  AlarmTableDef def4_cleared(example4, cleared4);
  AlarmTableDef def4_raised(example4, raised4);
  alarmActiveTable_trap_handler(def4_raised);

  AlarmDef::SeverityDetails cleared5(AlarmDef::CLEARED, "Fifth alarm cleared description", "Fifth alarm cleared details");
  AlarmDef::SeverityDetails raised5(AlarmDef::CRITICAL, "Fifth alarm raised description", "Fifth alarm raised details");
  AlarmDef::AlarmDefinition example5(6670, AlarmDef::SOFTWARE_ERROR, {cleared5, raised5});
  AlarmTableDef def5_cleared(example5, cleared5);
  AlarmTableDef def5_raised(example5, raised5);
  alarmActiveTable_trap_handler(def5_raised);

  // Clears the fifth and the first alarm (so the only alarms that should be
  // raised still are the third and the fourth)
  alarmActiveTable_trap_handler(def5_cleared);
  alarmActiveTable_trap_handler(def1_cleared);
  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. Each alarm has 11
  // columns and there should be 2 alarms raised.
  ASSERT_EQ(22, entries.size());
  
  // Clears remaining alarms.
  alarmActiveTable_trap_handler(def3_cleared);
  alarmActiveTable_trap_handler(def4_cleared);
  // Checks there are now no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0, entries_update.size());
}

TEST_F(SNMPTest, ActiveTableRepeatAlarm)
{
  // Creates a custom alarm definition and raises it.
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL, "Test alarm raised description", "Test alarm raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised(example, raised);
  alarmActiveTable_trap_handler(def_raised);

  // Raises the same alarm again.
  alarmActiveTable_trap_handler(def_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. The second alarm
  // raised was identical to the first and hence there should only be one entry
  // which has 11 columns.
  ASSERT_EQ(11, entries.size());

  alarmActiveTable_trap_handler(def_cleared);
  // Checks there are now no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0, entries_update.size());
}

TEST_F(SNMPTest, ActiveTableClearingUnraisedAlarm)
{
  // Creates a custom alarm definition (but doesn't raise it).
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL, "Test alarm raised description", "Test alarm raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised(example, raised);

  // Clears the alarm (despite not raising it beforehand).
  alarmActiveTable_trap_handler(def_cleared);

  // Checks there are now no entries in the table
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0, entries_update.size());
}

TEST_F(SNMPTest, ActiveTableSameAlarmDifferentSeverities)
{
  // Creates a custom alarm definition.
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised_critical(AlarmDef::CRITICAL, "Test alarm critical raised description", "Test alarm critical raised details");
  AlarmDef::SeverityDetails raised_major(AlarmDef::MAJOR, "Test alarm major raised description", "Test alarm major raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised_critical, raised_major});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised_critical(example, raised_critical);
  AlarmTableDef def_raised_major(example, raised_major);

  // Raises the critical alarm followed by the major alarm.
  alarmActiveTable_trap_handler(def_raised_critical);
  alarmActiveTable_trap_handler(def_raised_major);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  // Checks that the table has the right number of entries. The second alarm
  // should have overwritten the first and hence there should only be one entry
  // with 11 columns.
  ASSERT_EQ(11, entries.size()); 

  // Calculates which column should contain the alarm description based off the
  // column number definitions within alarm_active_table.hpp  
  int colnum_description = COLUMN_ALARMACTIVEDESCRIPTION - alarmActiveTable_COL_MIN;
  // Checks that the alarm description column contains latest severity data.
  std::size_t found = entries[colnum_description].find("Test alarm major raised description");
  ASSERT_NE(std::string::npos, found);

  // Clears the alarm and checks there are now no entries in the table.
  alarmActiveTable_trap_handler(def_cleared);
  std::vector<std::string> entries_update = snmp_walk(alarmActiveTable_table);
  ASSERT_EQ(0, entries_update.size());
}

TEST_F(SNMPTest, ActiveTableIndex)
{
  // Creates three custom alarm definitions and raises the alarms.
  AlarmDef::SeverityDetails cleared1(AlarmDef::CLEARED, "First alarm cleared description", "First alarm cleared details");
  AlarmDef::SeverityDetails raised1(AlarmDef::CRITICAL, "First alarm raised description", "First alarm raised details");
  AlarmDef::AlarmDefinition example1(6666, AlarmDef::SOFTWARE_ERROR, {cleared1, raised1});
  AlarmTableDef def1_cleared(example1, cleared1);
  AlarmTableDef def1_raised(example1, raised1);
  alarmActiveTable_trap_handler(def1_raised);

  AlarmDef::SeverityDetails cleared2(AlarmDef::CLEARED, "Second alarm cleared description", "Second alarm cleared details");
  AlarmDef::SeverityDetails raised2(AlarmDef::CRITICAL, "Second alarm raised description", "Second alarm raised details");
  AlarmDef::AlarmDefinition example2(6667, AlarmDef::SOFTWARE_ERROR, {cleared2, raised2});
  AlarmTableDef def2_cleared(example2, cleared2);
  AlarmTableDef def2_raised(example2, raised2);
  alarmActiveTable_trap_handler(def2_raised);

  AlarmDef::SeverityDetails cleared3(AlarmDef::CLEARED, "Third alarm cleared description", "Third alarm cleared details");
  AlarmDef::SeverityDetails raised3(AlarmDef::CRITICAL, "Third alarm raised description", "Third alarm raised details");
  AlarmDef::AlarmDefinition example3(6668, AlarmDef::SOFTWARE_ERROR, {cleared3, raised3});
  AlarmTableDef def3_cleared(example3, cleared3);
  AlarmTableDef def3_raised(example3, raised3);
  alarmActiveTable_trap_handler(def3_raised);

  // Shells out to snmpwalk to find all entries in Alarm Active Table.
  std::vector<std::string> entries = snmp_walk(alarmActiveTable_table);

  
  // Retrieves the current date and time.
  struct timespec ts;
  struct tm timing;
  time_t rawtime;
  clock_gettime(CLOCK_REALTIME, &ts);
  rawtime = ts.tv_sec;
  timing = *localtime(&rawtime);
  int day = timing.tm_mday;
  int month = 1 + timing.tm_mon;

  // Splits each part of the index field (separated by ".") for the first alarm into a vector.
  std::vector<std::string> index_fields1;
  Utils::split_string(entries[0], '.', index_fields1);

  // As defined in RFC3877 the 17th and 16th elements of the index field are the day and
  // month that the alarm was raised respectively. This checks those values are
  // the current day and month as retreived above. 
  ASSERT_EQ(std::to_string(day), index_fields1[17]);
  ASSERT_EQ(std::to_string(month), index_fields1[16]);
 
  // Checks the index of the first alarm contains 26 elements as defined in RFC
  // 3877.
  ASSERT_EQ(26, index_fields1.size());
  
  // Retreives each alarm's index (which should be a strictly monotonically
  // increasing integer which indexes the entries within the table and is the
  // 26th and last element of the index field) from the index field of the table.
  // As the alarm's index is immediately proceeded by a space and equality
  // symbol in an snmpwalk command we retreive just the alarm's index by using
  // the space symbol as a delimiter in split_string.
  std::vector<std::string> alarm_index1;
  Utils::split_string(index_fields1[25], ' ', alarm_index1);

  std::vector<std::string> index_fields2;
  Utils::split_string(entries[13], '.', index_fields2);
  std::vector<std::string> alarm_index2;
  Utils::split_string(index_fields2[25], ' ', alarm_index2);

  std::vector<std::string> index_fields3;
  Utils::split_string(entries[26], '.', index_fields3);
  std::vector<std::string> alarm_index3;
  Utils::split_string(index_fields3[25], ' ', alarm_index3);

  // Checks the indexes for the three alarms are strictly monotonically
  // increasing in the order they were raised.
  ASSERT_LT(atoi(alarm_index1[0].c_str()), atoi(alarm_index2[0].c_str()));
  ASSERT_LT(atoi(alarm_index2[0].c_str()), atoi(alarm_index3[0].c_str()));

}

