/**
 * @file alarm_req_listener_test.cpp
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

#include <unistd.h>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "alarm.h"
#include "alarmdefinition.h"

#include "alarm_model_table.hpp"
#include "alarm_req_listener.hpp"
#include "alarm_trap_sender.hpp"
#include "test_utils.hpp"

#include "fakezmq.h"
#include "fakenetsnmp.h"
#include "fakelogger.h"
#include "test_interposer.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::InSequence;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

static const char issuer1[] = "sprout";
static const char issuer2[] = "homestead";

class AlarmReqListenerTest : public ::testing::Test
{
public:
  AlarmReqListenerTest() :
    _alarm_1(issuer1, 1000, AlarmDef::CRITICAL),
    _alarm_2(issuer1, 1001, AlarmDef::CRITICAL),
    _alarm_3(issuer2, 1002, AlarmDef::CRITICAL)
  {
    cwtest_completely_control_time();
    cwtest_advance_time_ms(_delta_ms);

    cwtest_intercept_netsnmp(&_ms);

    AlarmReqListener::get_instance().start(NULL);
    AlarmReqAgent::get_instance().start();
  }

  virtual ~AlarmReqListenerTest()
  {
    AlarmFilter::get_instance().forget_issue_times();
    AlarmTrapSender::get_instance().forget_observed_alarms();
    AlarmManager::get_instance().forget_alarm_list();
    AlarmReqAgent::get_instance().stop();
    AlarmReqListener::get_instance().stop();

    cwtest_restore_netsnmp();
    cwtest_reset_time();
  }

  static void SetUpTestCase()
  {
    AlarmTableDefs::get_instance().initialize(std::string(UT_DIR).append("/valid_alarms/"));
  }

  void sync_alarms()
  {
    std::vector<std::string> req;

    req.push_back("sync-alarms");

    AlarmReqAgent::get_instance().alarm_request(req);
  }

  void sync_alarms_no_clear()
  {
    std::vector<std::string> req;

    req.push_back("sync-alarms-no-clear");

    AlarmReqAgent::get_instance().alarm_request(req);
  }

  void issue_malformed_alarm()
  {
    std::vector<std::string> req;

    req.push_back("issue-alarm");
    req.push_back("sprout");
    req.push_back("one.two");

    AlarmReqAgent::get_instance().alarm_request(req);
  }

  void issue_unknown_alarm()
  {
    std::vector<std::string> req;

    req.push_back("issue-alarm");
    req.push_back("sprout");
    req.push_back("0000.0");

    AlarmReqAgent::get_instance().alarm_request(req);
  }

  void invalid_zmq_request()
  {
    std::vector<std::string> req;

    req.push_back("invalid-request");

    AlarmReqAgent::get_instance().alarm_request(req);
  }

  void advance_time_ms(long delta_ms)
  {
    _delta_ms += delta_ms;

    cwtest_advance_time_ms(delta_ms);
  }

private:
  MockNetSnmpInterface _ms;
  CapturingTestLogger _log;
  Alarm _alarm_1;
  Alarm _alarm_2;
  Alarm _alarm_3;
  static long _delta_ms;
};

long AlarmReqListenerTest::_delta_ms = 0;

class AlarmReqListenerZmqErrorTest : public ::testing::Test
{
public:
  AlarmReqListenerZmqErrorTest() :
    _c(1),
    _s(2)
  {
    cwtest_intercept_netsnmp(&_ms);
    cwtest_intercept_zmq(&_mz);
  }

  virtual ~AlarmReqListenerZmqErrorTest()
  {
    cwtest_restore_zmq();
    cwtest_restore_netsnmp();
  }

private:
  MockNetSnmpInterface _ms;
  CapturingTestLogger _log;
  MockZmqInterface _mz;
  int _c;
  int _s;
};

class TrapVarsMatcher : public MatcherInterface<netsnmp_variable_list*> {
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

inline Matcher<netsnmp_variable_list*> TrapVars(TrapVarsMatcher::TrapType trap_type,
                                                unsigned int alarm_index) {
  return MakeMatcher(new TrapVarsMatcher(trap_type, alarm_index));
}

TEST_F(AlarmReqListenerTest, ClearAlarmNoSet)
{
  EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                        1000)));

  _alarm_1._clear_state.issue();
  _ms.trap_complete(1, 5);
}

TEST_F(AlarmReqListenerTest, SetAlarm)
{
  EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                        1000)));

  _alarm_1.set();
  _ms.trap_complete(1, 5);
}

TEST_F(AlarmReqListenerTest, ClearAlarm)
{
  {
    InSequence s;
    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));
  }
  _alarm_1.set();

  _alarm_1.clear();
  _ms.trap_complete(2, 5);
}

// Raises an alarm, waits thirty seconds, raises the same alarm in the
// same state and then clears the alarm. We should only expect two traps, for
// the initial raising and the clearing, raising the alarm in a repeated state
// should not cause a trap to be sent.
TEST_F(AlarmReqListenerTest, SetAlarmRepeatedState)
{
  {
    InSequence s;
    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));
    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));
  }
  _alarm_1.set();
  advance_time_ms(30);
  _alarm_1.set();
  _alarm_1.clear();
  _ms.trap_complete(2, 5);
}

TEST_F(AlarmReqListenerTest, ClearAlarms)
{
  {
    InSequence s;

    EXPECT_CALL(_ms, send_v2trap(_)).
      Times(3);

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1001)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1002)));
  }

  _alarm_1.set();
  _alarm_2.set();
  _alarm_3.set();

  AlarmState::clear_all(issuer1);
  AlarmState::clear_all(issuer2);

  _ms.trap_complete(6, 5);
}

TEST_F(AlarmReqListenerTest, SyncAlarms)
{
  int alarm_count = 3;

  {
    InSequence s;

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(_)).
      Times(alarm_count);

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));
  }

  _alarm_1.set();

  sync_alarms();

  _ms.trap_complete(alarm_count + 2, 5);
}

TEST_F(AlarmReqListenerTest, SyncAlarmsNoClear)
{
  {
    InSequence s;

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));
  }

  sync_alarms_no_clear();

  _alarm_1.set();
  _alarm_1.clear();

  _ms.trap_complete(2, 5);
}

TEST_F(AlarmReqListenerTest, AlarmFilter)
{
  {
    InSequence s;

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1001)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1001)));
  }

  for (int idx = 0; idx < 10; idx++)
  {
    _alarm_1.set();
    _alarm_1.clear();
  }

  _alarm_2.set();
  _alarm_2.clear();

  _ms.trap_complete(4, 5);
}

TEST_F(AlarmReqListenerTest, AlarmFilterClean)
{
  {
    InSequence s;

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::ACTIVE,
                                          1001)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1000)));

    EXPECT_CALL(_ms, send_v2trap(TrapVars(TrapVarsMatcher::CLEAR,
                                          1001)));
  }

  _alarm_1.set();
  _ms.trap_complete(1, 5);

  advance_time_ms(AlarmFilter::CLEAN_FILTER_TIME - 2000);

  _alarm_2.set();
  _ms.trap_complete(1, 5);

  advance_time_ms(3000);

  _alarm_1.clear();
  _alarm_2.clear();
  _ms.trap_complete(2, 5);
}

TEST_F(AlarmReqListenerTest, InvalidZmqRequest)
{
  AlarmManager::get_instance().forget_alarm_list();
  EXPECT_CALL(_ms, send_v2trap(_)).
    Times(0);

  invalid_zmq_request();
  usleep(100000);

  EXPECT_TRUE(_log.contains("unexpected alarm request"));
}

TEST_F(AlarmReqListenerTest, InvalidAlarmIdentifier)
{
  AlarmManager::get_instance().forget_alarm_list();
  EXPECT_CALL(_ms, send_v2trap(_)).
    Times(0);

  issue_malformed_alarm();
  usleep(100000);

  EXPECT_TRUE(_log.contains("malformed alarm identifier"));
}

TEST_F(AlarmReqListenerTest, UnknownAlarmIdentifier)
{
  AlarmManager::get_instance().forget_alarm_list();
  EXPECT_CALL(_ms, send_v2trap(_)).
    Times(0);

  issue_unknown_alarm();
  usleep(100000);

  EXPECT_TRUE(_log.contains("unknown alarm definition"));
}

TEST_F(AlarmReqListenerZmqErrorTest, CreateContext)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(ReturnNull());

  EXPECT_FALSE(AlarmReqListener::get_instance().start(NULL));
  EXPECT_TRUE(_log.contains("zmq_ctx_new failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, CreateSocket)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)) .WillOnce(ReturnNull());

  EXPECT_FALSE(AlarmReqListener::get_instance().start(NULL));

  EXPECT_TRUE(_log.contains("zmq_socket failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, BindSocket)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(-1));

  EXPECT_FALSE(AlarmReqListener::get_instance().start(NULL));

  EXPECT_TRUE(_log.contains("zmq_bind failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, MsgReceive)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_recv(_,_,_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(0));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_msg_recv failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, GetSockOpt)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_recv(_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_getsockopt(_,_,_,_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(0));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_getsockopt failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, MsgClose)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_recv(_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_getsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_close(_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(0));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_msg_close failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, Send)
{
  {
    InSequence s;
    EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
    EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
    EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_msg_recv(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_getsockopt(_,_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_msg_close(_)).WillOnce(Return(0));
    EXPECT_CALL(_mz, zmq_send(_,_,_,_)).WillOnce(Return(-1));
    EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(-1));
  }

  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(0));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_send failed:"));
  EXPECT_TRUE(_log.contains("zmq_msg_init failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, CloseSocket)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(0));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_close failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, DestroyContext)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_msg_init(_)).WillOnce(Return(-1));
  EXPECT_CALL(_mz, zmq_close(_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_ctx_destroy(_)).WillOnce(Return(-1));

  EXPECT_TRUE(AlarmReqListener::get_instance().start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  AlarmReqListener::get_instance().stop();

  EXPECT_TRUE(_log.contains("zmq_ctx_destroy failed:"));
}
