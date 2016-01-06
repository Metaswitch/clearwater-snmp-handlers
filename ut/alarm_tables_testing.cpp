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
  // sleep(100);
  // Verifies the above alarm data is exactly what is stored in
  // AlarmModelTable.
  
  ASSERT_STREQ("Test alarm cleared description", snmp_get_raw("1.3.6.1.2.1.118.1.1.2.1.6.0.6666.1"));
  //Here we use AlarmModelState (6) to query AlarmModelTable, for details of the
  //mapping to ituAlarmPerceivedSeverity see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("Test alarm raised description", snmp_get_raw("1.3.6.1.2.1.118.1.1.2.1.6.0.6666.6"));
  ASSERT_STREQ(".3.6.1.2.1.118.0.3", snmp_get_raw("1.3.6.1.2.1.118.1.1.2.1.3.0.6666.1"));
  ASSERT_STREQ(".3.6.1.2.1.118.0.2", snmp_get_raw("1.3.6.1.2.1.118.1.1.2.1.3.0.6666.6"));
}

TEST_F(SNMPTest, ituAlarmTable)
{
  // Create a custom alarm definition
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED, "Test alarm cleared description", "Test alarm cleared details");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL, "Test alarm raised description", "Test alarm raised details");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  AlarmTableDef def_cleared(example, cleared);
  AlarmTableDef def_raised(example, raised);
  AlarmTableDefs::get_instance().insert_def(def_cleared);
  AlarmTableDefs::get_instance().insert_def(def_raised);
  
  init_ituAlarmTable();
  //sleep(100);
  ASSERT_STREQ("Test alarm cleared details", snmp_get_raw("1.3.6.1.2.1.121.1.1.1.1.4.0.6666.1"));
  //Here we use ituAlarmPerceivedSeverity (3) to query ituAlarmTable, for
  //details of the mapping to AlarmModelState see
  //https://tools.ietf.org/html/rfc3877#section-5.4
  ASSERT_STREQ("Test alarm raised details", snmp_get_raw("1.3.6.1.2.1.121.1.1.1.1.4.0.6666.3"));
  ASSERT_EQ(163, snmp_get("1.3.6.1.2.1.121.1.1.1.1.3.0.6666.1"));
  ASSERT_EQ(163, snmp_get("1.3.6.1.2.1.121.1.1.1.1.3.0.6666.3"));
} 
