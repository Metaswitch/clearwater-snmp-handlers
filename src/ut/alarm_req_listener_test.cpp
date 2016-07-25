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

#include "mock_alarm_scheduler.h"
#include "fakezmq.h"
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

static const char issuer1[] = "sprout";
static const char issuer2[] = "homestead";

class AlarmReqListenerTest : public ::testing::Test
{
public:
  AlarmReqListenerTest()
  {
    cwtest_completely_control_time();

    _alarm_table_defs = new AlarmTableDefs();
    _alarm_scheduler = new MockAlarmScheduler(_alarm_table_defs);
    _alarm_req_listener = new AlarmReqListener(_alarm_scheduler);
    _alarm_req_listener->start(NULL);
    _alarm_manager = new AlarmManager();
    _alarm_1 = new Alarm(_alarm_manager, issuer1, 1000, AlarmDef::CRITICAL);
    _alarm_2 = new Alarm(_alarm_manager, issuer2, 1001, AlarmDef::CRITICAL);

    // Wait until the AlarmReRaiser is waiting before we start (so it doesn't
    // try and reraise any alarms unexpectedly).
    _alarm_manager->alarm_re_raiser()->_condition->block_till_waiting();
  }

  virtual ~AlarmReqListenerTest()
  {
    delete _alarm_2; _alarm_2 = NULL;
    delete _alarm_1; _alarm_1 = NULL;
    delete _alarm_manager; _alarm_manager = NULL;
    _alarm_req_listener->stop();
    delete _alarm_req_listener; _alarm_req_listener = NULL;
    delete _alarm_scheduler; _alarm_scheduler = NULL;
    delete _alarm_table_defs; _alarm_table_defs = NULL;

    cwtest_reset_time();
  }

private:
  CapturingTestLogger _log;
  AlarmTableDefs* _alarm_table_defs;
  MockAlarmScheduler* _alarm_scheduler;
  AlarmReqListener* _alarm_req_listener;
  AlarmManager* _alarm_manager;
  Alarm* _alarm_1;
  Alarm* _alarm_2;
};

class AlarmReqListenerZmqErrorTest : public ::testing::Test
{
public:
  AlarmReqListenerZmqErrorTest() :
    _c(1),
    _s(2)
  {
    cwtest_intercept_zmq(&_mz);

    _alarm_table_defs = new AlarmTableDefs();
    _alarm_scheduler = new MockAlarmScheduler(_alarm_table_defs);
    _alarm_req_listener = new AlarmReqListener(_alarm_scheduler);
  }

  virtual ~AlarmReqListenerZmqErrorTest()
  {
    delete _alarm_req_listener; _alarm_req_listener = NULL;
    delete _alarm_scheduler; _alarm_scheduler = NULL;
    delete _alarm_table_defs; _alarm_table_defs = NULL;

    cwtest_restore_zmq();
  }

private:
  CapturingTestLogger _log;
  MockZmqInterface _mz;
  int _c;
  int _s;
  AlarmTableDefs* _alarm_table_defs;
  MockAlarmScheduler* _alarm_scheduler;
  AlarmReqListener* _alarm_req_listener;
};

// Check that setting/clearing alarms are translated into the correct
// issuers/index/severity
TEST_F(AlarmReqListenerTest, IssueAlarms)
{
  EXPECT_CALL(*_alarm_scheduler, issue_alarm("sprout", "1000.3"));
  EXPECT_CALL(*_alarm_scheduler, issue_alarm("homestead", "1001.1"));

  _alarm_1->set();
  _alarm_2->clear();

  sleep(1);
}

// Check that the ZMQ request to sync alarms is translated into an internal
// request to sync alarms
TEST_F(AlarmReqListenerTest, SyncAlarms)
{
  EXPECT_CALL(*_alarm_scheduler, sync_alarms());
  std::vector<std::string> req;
  req.push_back("sync-alarms");
  _alarm_manager->alarm_req_agent()->alarm_request(req);
  sleep(1);
}

TEST_F(AlarmReqListenerTest, InvalidZmqRequest)
{
  std::vector<std::string> req;
  req.push_back("invalid-request");
  _alarm_manager->alarm_req_agent()->alarm_request(req);
  sleep(1);
  EXPECT_TRUE(_log.contains("unexpected alarm request"));
}

TEST_F(AlarmReqListenerZmqErrorTest, CreateContext)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(ReturnNull());

  EXPECT_FALSE(_alarm_req_listener->start(NULL));
  EXPECT_TRUE(_log.contains("zmq_ctx_new failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, CreateSocket)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)) .WillOnce(ReturnNull());

  EXPECT_FALSE(_alarm_req_listener->start(NULL));

  EXPECT_TRUE(_log.contains("zmq_socket failed:"));
}

TEST_F(AlarmReqListenerZmqErrorTest, BindSocket)
{
  EXPECT_CALL(_mz, zmq_ctx_new()).WillOnce(Return(&_c));
  EXPECT_CALL(_mz, zmq_socket(_,_)).WillOnce(Return(&_s));
  EXPECT_CALL(_mz, zmq_setsockopt(_,_,_,_)).WillOnce(Return(0));
  EXPECT_CALL(_mz, zmq_bind(_,_)).WillOnce(Return(-1));

  EXPECT_FALSE(_alarm_req_listener->start(NULL));

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

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

  EXPECT_TRUE(_alarm_req_listener->start(NULL));

  _mz.call_complete(ZmqInterface::ZMQ_CLOSE, 5);

  _alarm_req_listener->stop();

  EXPECT_TRUE(_log.contains("zmq_ctx_destroy failed:"));
}
