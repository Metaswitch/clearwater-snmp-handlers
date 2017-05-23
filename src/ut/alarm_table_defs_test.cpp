/**
 * @file alarm_table_defs_test.cpp
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
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
