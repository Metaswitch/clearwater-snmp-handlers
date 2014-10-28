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

#include "fakenetsnmp.h"

using ::testing::Eq;
using ::testing::StrEq;
using ::testing::StartsWith;

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_multi_def =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      },
      {AlarmDef::CRITICAL,
        "Description",
        "Details."
      }
    }
  },

  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      },
      {AlarmDef::CRITICAL,
        "Description",
        "Details."
      }
    }
  }
};

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_clear_missing =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CRITICAL,
        "Description",
        "Details."
      }
    }
  }
};

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_non_clear_missing =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      }
    }
  }
};

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_desc_too_long =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      },
      {AlarmDef::CRITICAL,
        "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
        "123456789012345678901234567890123456789012345678901234567890",
        "Details."
      }
    }
  }
};

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_details_too_long =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      },
      {AlarmDef::CRITICAL,
        "Description",
        "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
        "123456789012345678901234567890123456789012345678901234567890"
      }
    }
  }
};

const std::vector<AlarmDef::AlarmDefinition> alarm_definitions_undef_severity =
{
  {AlarmDef::SPROUT_PROCESS_FAIL, AlarmDef::SOFTWARE_ERROR,
    {
      {AlarmDef::CLEARED,
        "Description",
        "Details."
      },
      {AlarmDef::UNDEFINED_SEVERITY,
        "Description",
        "Details."
      }
    }
  }
};

class AlarmTableDefsTest : public ::testing::Test
{
public:
  AlarmTableDefsTest() :
    _defs(AlarmTableDefs::get_instance())
  {
    cwtest_intercept_netsnmp(&_ms);
  }

  virtual ~AlarmTableDefsTest()
  {
    cwtest_restore_netsnmp();
  }

private:
  AlarmTableDefs& _defs;
  MockNetSnmpInterface _ms;
};

TEST_F(AlarmTableDefsTest, InitializationOk)
{
  EXPECT_TRUE(_defs.initialize());  
}

TEST_F(AlarmTableDefsTest, InitializationMultiDef)
{
  EXPECT_FALSE(_defs.initialize(alarm_definitions_multi_def));  
  EXPECT_TRUE(_ms.log_contains("multiply defined"));
}

TEST_F(AlarmTableDefsTest, InitializationClearMissing)
{
  EXPECT_FALSE(_defs.initialize(alarm_definitions_clear_missing));  
  EXPECT_TRUE(_ms.log_contains("define a CLEARED"));
}

TEST_F(AlarmTableDefsTest, InitializationNonClearMissing)
{
  EXPECT_FALSE(_defs.initialize(alarm_definitions_non_clear_missing));  
  EXPECT_TRUE(_ms.log_contains("define at least one non-CLEARED"));
}

TEST_F(AlarmTableDefsTest, InitializationDescTooLong)
{
  EXPECT_FALSE(_defs.initialize(alarm_definitions_desc_too_long));  
  EXPECT_TRUE(_ms.log_contains("'description' exceeds"));
}

TEST_F(AlarmTableDefsTest, InitializationDetailsTooLong)
{
  EXPECT_FALSE(_defs.initialize(alarm_definitions_details_too_long));  
  EXPECT_TRUE(_ms.log_contains("'details' exceeds"));
}

TEST_F(AlarmTableDefsTest, ValidTableDefLookup)
{
  _defs.initialize();

  AlarmTableDef& _def = _defs.get_definition(AlarmDef::SPROUT_PROCESS_FAIL,
                                             AlarmDef::CRITICAL);

  EXPECT_TRUE(_def.is_valid());
  EXPECT_THAT((int)_def.state(), Eq(6));
  EXPECT_THAT((int)_def.cause(), Eq(AlarmDef::SOFTWARE_ERROR));
  EXPECT_THAT(_def.description(), StrEq("Sprout: Process failure"));
  EXPECT_THAT(_def.details(), StartsWith("Monit has detected that the Sprout process has failed."));
}

TEST_F(AlarmTableDefsTest, InvalidTableDefLookup)
{
  _defs.initialize();

  AlarmTableDef& _def = _defs.get_definition(0, 0);

  EXPECT_FALSE(_def.is_valid());
}

TEST_F(AlarmTableDefsTest, InvalidStateMapping)
{
  _defs.initialize(alarm_definitions_undef_severity);  

  AlarmTableDef& _def = _defs.get_definition(AlarmDef::SPROUT_PROCESS_FAIL,
                                             AlarmDef::UNDEFINED_SEVERITY);

  EXPECT_TRUE(_def.is_valid());
  EXPECT_THAT((int)_def.state(), Eq(2));
}

