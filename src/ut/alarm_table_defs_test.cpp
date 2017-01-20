/**
 * @file alarm_table_defs_test.cpp
 *
 * Project Clearwater - IMS in the Cloud
 * Copyright (C) 2014  Metaswitch Networks Ltd
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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "alarm_model_table.hpp"
#include "test_utils.hpp"

#include "fakenetsnmp.h"
#include "fakelogger.h"

using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StartsWith;

class AlarmTableDefsTest : public ::testing::Test
{
public:
  AlarmTableDefsTest() :
    _defs(AlarmTableDefs())
  {
    cwtest_intercept_netsnmp(&_ms);
  }

  virtual ~AlarmTableDefsTest()
  {
    cwtest_restore_netsnmp();
  }

private:
  AlarmTableDefs _defs;
  MockNetSnmpInterface _ms;
  CapturingTestLogger _log;
};

TEST_F(AlarmTableDefsTest, InitializationNoFiles)
{
  EXPECT_FALSE(_defs.initialize(std::string(UT_DIR).append("NOT_A_REAL_PATH")));
}

TEST_F(AlarmTableDefsTest, InitializationMultiDef)
{
  EXPECT_FALSE(_defs.initialize(std::string(UT_DIR).append("/multi_definition/")));
  EXPECT_TRUE(_log.contains("multiply defined"));
}

TEST_F(AlarmTableDefsTest, InitializationInvalidJson)
{
  EXPECT_FALSE(_defs.initialize(std::string(UT_DIR).append("/invalid_json/")));
  EXPECT_TRUE(_log.contains("Invalid JSON file"));
}

TEST_F(AlarmTableDefsTest, ValidTableDefLookup)
{
  EXPECT_TRUE(_defs.initialize(std::string(UT_DIR).append("/valid_alarms/")));

  AlarmTableDef& _def = _defs.get_definition(1000,
                                             AlarmDef::CRITICAL);

  EXPECT_TRUE(_def.is_valid());
  EXPECT_THAT((int)_def.state(), Eq(6));
  EXPECT_THAT((int)_def.alarm_cause(), Eq(AlarmDef::SOFTWARE_ERROR));
  EXPECT_THAT(_def.name(), StrEq("PROCESS_FAIL"));
  EXPECT_THAT(_def.description(), StrEq("Process failure"));
  EXPECT_THAT(_def.details(), StartsWith("Monit has detected that the process has failed"));
  EXPECT_THAT(_def.cause(), StrEq("Cause"));
  EXPECT_THAT(_def.effect(), StrEq("Effect"));
  EXPECT_THAT(_def.action(), StrEq("Action"));
  // The JSON file contains no extended details or description so we test that
  // we have used the regular details and description in place.
  EXPECT_THAT(_def.extended_details(), 
              StartsWith("Monit has detected that the process has failed"));
  EXPECT_THAT(_def.extended_description(), StrEq("Process failure"));
}

TEST_F(AlarmTableDefsTest, InvalidTableDefLookup) 
{
  EXPECT_TRUE(_defs.initialize(std::string(UT_DIR).append("/valid_alarms/")));

  AlarmTableDef& _def = _defs.get_definition(0, 0);

  EXPECT_FALSE(_def.is_valid());
}
