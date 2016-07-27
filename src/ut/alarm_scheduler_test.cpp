/**
 * @file alarm_scheduler_test.cpp
 *
 * Project Clearwater - IMS in the Cloud
 * Copyright (C) 2016  Metaswitch Networks Ltd
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

#include <unistd.h>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "alarm_scheduler.hpp"
#include "test_utils.hpp"

#include "fakenetsnmp.h"
#include "fakelogger.h"
#include "test_interposer.hpp"

using ::testing::_;
using ::testing::A;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::InSequence;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::SaveArg;
using ::testing::Invoke;

class TrapVarsMatcher : public MatcherInterface<netsnmp_variable_list*>
{
public:
  enum TrapType
  {
    UNDEFINED,
    CLEAR,
    ACTIVE
  };

  enum OctetIndices
  {
    LAST_TRAP_OID_OCTET = 8,
    ALARM_IDX_ROW_OID_OCTET = 13
  };

  explicit TrapVarsMatcher(TrapType trap_type,
                           unsigned int alarm_index) :
      _trap_type(trap_type),
      _alarm_index(alarm_index) {}

  virtual bool MatchAndExplain(netsnmp_variable_list* vl,
                               MatchResultListener* listener) const {
    TrapType type = UNDEFINED;
    unsigned int index;

    if (vl->val.objid[LAST_TRAP_OID_OCTET] == 3)
    {
      type = CLEAR;
    }
    else if (vl->val.objid[LAST_TRAP_OID_OCTET] == 2)
    {
      type = ACTIVE;
    }

    vl = vl->next_variable;

    if (vl != NULL)
    {
      index = vl->val.objid[ALARM_IDX_ROW_OID_OCTET];
    }

    return (_trap_type == type) && (_alarm_index == index);
  }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "trap is " << _alarm_index << ((_trap_type == CLEAR) ? " CLEAR" : " ACTIVE");
  }

  virtual void DescribeNegationTo(::std::ostream* os) const {
    *os << "trap is not " << _alarm_index << ((_trap_type == CLEAR) ? " CLEAR" : " ACTIVE");
  }
 private:
  TrapType _trap_type;
  unsigned int _alarm_index;
};

class SNMPCallbackCollector
{
public:
  void call_all_callbacks()
  {
    snmp_session session;
    session.peername = strdup("peer");

    for (auto ii = _callbacks.begin();
         ii != _callbacks.end();
         ++ii)
    {
      ii->first(NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE, &session, 0, NULL, ii->second);
    }

    _callbacks.clear();
    free(session.peername);
  }

  void collect_callback(netsnmp_variable_list* ignored, snmp_callback callback, void* correlator)
  {
    _callbacks.emplace_back(callback, correlator);
  }

private:
  std::vector<std::pair<snmp_callback, void*>> _callbacks;
};

class AlarmSchedulerTest : public ::testing::Test
{
public:
  AlarmSchedulerTest()
  {
    cwtest_completely_control_time();
    cwtest_intercept_netsnmp(&_ms);

    _alarm_table_defs = new AlarmTableDefs();
    _alarm_table_defs->initialize(std::string(UT_DIR).append("/valid_alarms/"));
    _alarm_scheduler = new AlarmScheduler(_alarm_table_defs);
  }

  virtual ~AlarmSchedulerTest()
  {
    delete _alarm_scheduler; _alarm_scheduler = NULL;
    delete _alarm_table_defs; _alarm_table_defs = NULL;
    _collector.call_all_callbacks();

    cwtest_restore_netsnmp();
    cwtest_reset_time();
  }

private:
  MockNetSnmpInterface _ms;
  CapturingTestLogger _log;
  SNMPCallbackCollector _collector;
  AlarmTableDefs* _alarm_table_defs;
  AlarmScheduler* _alarm_scheduler;
};

// Safely set up an expect call for an SNMP trap send that will succeed.
#define COLLECT_CALL(CALL) EXPECT_CALL(_ms, CALL).                                \
  WillOnce(Invoke(&_collector, &SNMPCallbackCollector::collect_callback))
#define COLLECT_CALLS(N, CALL) EXPECT_CALL(_ms, CALL).                            \
  Times(N).                                                                       \
  WillRepeatedly(Invoke(&_collector, &SNMPCallbackCollector::collect_callback))

inline Matcher<netsnmp_variable_list*> TrapVars(TrapVarsMatcher::TrapType trap_type,
                                                unsigned int alarm_index) {
  return MakeMatcher(new TrapVarsMatcher(trap_type, alarm_index));
}

// Simple test that raising an alarm triggers an INFORM to be sent immediately
TEST_F(AlarmSchedulerTest, SetAlarm)
{
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Simple test that clearing an alarm triggers an INFORM to be sent immediately
// (from a clean start).
TEST_F(AlarmSchedulerTest, ClearAlarm)
{
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.1");
  _ms.trap_complete(1, 5);
}

// Test that clearing a set alarm only clears the alarm after a delay
TEST_F(AlarmSchedulerTest, SetAndClearAlarm)
{
  // Set the alarm - this should raise the alarm straight away
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);

  // Now clear the alarm. Block until we're waiting for the heap (note - this
  // doesn't prove we're waiting on the alarm to be due, but the next check
  // will fail if we aren't).
  _alarm_scheduler->issue_alarm("test", "1000.1");
  _alarm_scheduler->_cond->block_till_waiting();

  // Advance time so that the alarm is due to be sent, and check that the
  // cleared alarm is sent.
  cwtest_advance_time_ms(AlarmScheduler::ALARM_REDUCED_DELAY);
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR, 1000), _, _));
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test that repeated alarms only generate one INFORM
TEST_F(AlarmSchedulerTest, SetAlarmRepeatedState)
{
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Test that increasing alarm severity generates INFORMs immediately
TEST_F(AlarmSchedulerTest, SetMultiAlarmIncreasingState)
{
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.1");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.5");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.4");
  _ms.trap_complete(1, 5);
}


// Test that syncing alarms generates INFORMs for all known alarms immediately
TEST_F(AlarmSchedulerTest, SyncAlarms)
{
  // Put our three alarms into a state we expect
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                        1000), _, _));

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                        1001), _, _));

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                        1002), _, _));

  _alarm_scheduler->issue_alarm("test", "1000.3");
  _alarm_scheduler->issue_alarm("test", "1001.1");
  _alarm_scheduler->issue_alarm("test", "1002.1");

  _ms.trap_complete(3, 5);

  // Now call sync_alarms and expect those states to be retransmitted
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                        1000), _, _));

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                        1001), _, _));

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                        1002), _, _));

  _alarm_scheduler->sync_alarms();

  _ms.trap_complete(3, 5);
}

// Test that an alarm flicker situation doesn't cause flickering INFORMs
// (by sending multiple set/clears, and checking that this doesn't
// generate informs).
TEST_F(AlarmSchedulerTest, AlarmFlicker)
{
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                    1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                    1000), _, _));
  for (int idx = 0; idx < 10; idx++)
  {
    _alarm_scheduler->issue_alarm("test", "1000.3");
    _alarm_scheduler->issue_alarm("test", "1000.1");
  }

  cwtest_advance_time_ms(AlarmScheduler::ALARM_REDUCED_DELAY);
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test that a failed alarm is retried after a delay
TEST_F(AlarmSchedulerTest, AlarmFailedToSend)
{
  snmp_callback callback;
  void* correlator;
  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    WillOnce(DoAll(SaveArg<1>(&callback),
                   SaveArg<2>(&correlator)));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);

  // Now report the send as failed.
  snmp_session session;
  session.peername = strdup("peer");
  callback(NETSNMP_CALLBACK_OP_TIMED_OUT, &session, 2, NULL, correlator);
  free(session.peername);

  // Now advance time by the retry delay amount. This triggers the retry to be
  // sent
  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                    1000), _, _));
  cwtest_advance_time_ms(AlarmScheduler::ALARM_RETRY_DELAY);
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test that when the raise alarm fails, but a clear alarm has been sent in later,
// the clear alarm only is sent after a delay
TEST_F(AlarmSchedulerTest, AlarmFailedToSendClearedInInterval)
{
  snmp_callback callback;
  void* correlator;
  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    WillOnce(DoAll(SaveArg<1>(&callback),
                   SaveArg<2>(&correlator)));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);

  _alarm_scheduler->issue_alarm("test", "1000.1");

  // Now report the send as failed. which will not attempt to resend the
  // alarm.
  snmp_session session;
  session.peername = strdup("peer");
  callback(NETSNMP_CALLBACK_OP_TIMED_OUT, &session, 2, NULL, correlator);
  free(session.peername);

  COLLECT_CALL(send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                    1000), _, _));
  cwtest_advance_time_ms(AlarmScheduler::ALARM_REDUCED_DELAY);
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test handling of an invalid alarm
TEST_F(AlarmSchedulerTest, InvalidAlarmIdentifier)
{
  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    Times(0);

  _alarm_scheduler->issue_alarm("test", "not_digits.3");
  EXPECT_TRUE(_log.contains("malformed alarm identifier"));
}

// Test handling of an unknown alarm
TEST_F(AlarmSchedulerTest, UnknownAlarmIdentifier)
{
  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    Times(0);

  _alarm_scheduler->issue_alarm("test", "6000.3");
  EXPECT_TRUE(_log.contains("Unknown alarm definition"));
}
