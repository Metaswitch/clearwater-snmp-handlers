/**
 * @file alarm_scheduler_test.cpp
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
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
    
class EnterpriseTrapVarsMatcher : public MatcherInterface<netsnmp_variable_list*>
{
public:
  explicit EnterpriseTrapVarsMatcher(oid trap_type,
                                     std::string MIB_version,
                                     std::string name,
                                     oid alarm_oid,
                                     oid resource_id,
                                     std::string severity,
                                     std::string description,
                                     std::string details,
                                     std::string cause,
                                     std::string effect,
                                     std::string action) :
      _trap_type(trap_type),
      _MIB_version(MIB_version),
      _name(name),
      _alarm_oid(alarm_oid),
      _resource_id(resource_id),
      _severity(severity),
      _description(description),
      _details(details),
      _cause(cause),
      _effect(effect),
      _action(action) {}

  virtual bool MatchAndExplain(netsnmp_variable_list* vl,
                               MatchResultListener* listener) const {
    
    oid trap_type = *(vl->val.objid);
    vl = vl->next_variable;
    std::string MIB_version(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable;
    std::string name(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable;
    oid alarm_oid = *(vl->val.objid);
    vl = vl->next_variable;
    oid resource_id = *(vl->val.objid);
    vl = vl->next_variable;
    std::string severity(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable;
    std::string description(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable; 
    std::string details(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable; 
    std::string cause(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable; 
    std::string effect(reinterpret_cast<char const*>(vl->val.string));
    vl = vl->next_variable; 
    std::string action(reinterpret_cast<char const*>(vl->val.string));

    return (_description == description) && (_details == details) && (_cause == cause)
           && (_effect == effect) && (_action == action) && (_resource_id == resource_id)
           && (_alarm_oid == alarm_oid) && (_name == name) && (_MIB_version == MIB_version)
           && (_trap_type == trap_type);
  }

  virtual void DescribeTo(::std::ostream* os) const {
    *os << "trap is: " << _name <<  ", with description: " << _description 
      << ",  details: " << _details << ", cause: " << _cause << ", effect: " << _effect 
      << " and action: " << _action;
  }
  
  virtual void DescribeNegationTo(::std::ostream* os) const {
     *os << "trap is not: " << _name <<  ", and doesn't have description: " << _description 
      << ",  details: " << _details << ", cause: " << _cause << ", effect: " << _effect 
      << " and action: " << _action;
  }
 
private:
  oid _trap_type;
  std::string _MIB_version;
  std::string _name;
  oid _alarm_oid;
  oid _resource_id;
  std::string _severity;
  std::string _description;
  std::string _details;
  std::string _cause;
  std::string _effect;
  std::string _action;
};

class RFCTrapVarsMatcher : public MatcherInterface<netsnmp_variable_list*>
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

  explicit RFCTrapVarsMatcher(TrapType trap_type,
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

inline Matcher<netsnmp_variable_list*> EnterpriseTrapVars(oid trap_type,
                                                          std::string MIB_version,
                                                          std::string name,
                                                          oid alarm_oid,
                                                          oid resource_id,
                                                          std::string severity,
                                                          std::string description,
                                                          std::string details,
                                                          std::string cause,
                                                          std::string effect,
                                                          std::string action) {
  return MakeMatcher(new EnterpriseTrapVarsMatcher(trap_type,
                                                   MIB_version,
                                                   name,
                                                   alarm_oid,
                                                   resource_id,
                                                   severity,
                                                   description,
                                                   details,
                                                   cause,
                                                   effect,
                                                   action));
}

inline Matcher<netsnmp_variable_list*> RFCTrapVars(RFCTrapVarsMatcher::TrapType trap_type,
                                                   unsigned int alarm_index) {
  return MakeMatcher(new RFCTrapVarsMatcher(trap_type, alarm_index));
}

// Simple test that raising an RFC3877 compliant alarm triggers an INFORM to be 
// sent immediately
TEST_F(AlarmSchedulerTest, SetRFCAlarm)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Simple test that raising an Enterprise MIB style alarm triggers an INFORM to
// be sent immediately
TEST_F(AlarmSchedulerTest, SetEnterpriseAlarm)
{  
  std::set<NotificationType> snmp_notifications; 
  snmp_notifications.insert(NotificationType::ENTERPRISE);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  // Here we are checking for severity reported as an AlarmModelState value
  oid trap_type[] = {1,2,826,0,1,1578918,19444,9,2,1,1};
  oid alarm_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2,1000};
  oid zero_dot_zero[] = {0,0};
  COLLECT_CALL(send_v2trap(EnterpriseTrapVars(*(trap_type),
                                              "201608081100",
                                              "PROCESS_FAIL",
                                              *(alarm_oid),
                                              *(zero_dot_zero),
                                              "6",
                                              "Process failure",
                                              "Monit has detected that the process has failed.",
                                              "Cause",
                                              "Effect",
                                              "Action"), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Simple test that raising both an RFC3877 compliant alarm and an Enterprise
// MIB style alarm triggers two INFORMs to be sent immediately
TEST_F(AlarmSchedulerTest, SetBothAlarms)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  snmp_notifications.insert(NotificationType::ENTERPRISE);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  oid trap_type[] = {1,2,826,0,1,1578918,19444,9,2,1,1};
  oid alarm_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2,1000};
  oid zero_dot_zero[] = {0,0};
  COLLECT_CALL(send_v2trap(EnterpriseTrapVars(*(trap_type),
                                              "201608081100",
                                              "PROCESS_FAIL",
                                              *(alarm_oid),
                                              *(zero_dot_zero),
                                              "6",
                                              "Process failure",
                                              "Monit has detected that the process has failed.",
                                              "Cause",
                                              "Effect",
                                              "Action"), _, _));

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Simple test that clearing an alarm triggers an INFORM to be sent immediately
// (from a clean start).
TEST_F(AlarmSchedulerTest, ClearAlarm)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.1");
  _ms.trap_complete(1, 5);
}

// Test that clearing a set alarm only clears the alarm after a delay
TEST_F(AlarmSchedulerTest, SetAndClearAlarm)
{ 
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  // Set the alarm - this should raise the alarm straight away
  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 1000), _, _));
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
  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR, 1000), _, _));
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test that repeated alarms only generate one INFORM
TEST_F(AlarmSchedulerTest, SetAlarmRepeatedState)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

// Test that increasing alarm severity generates INFORMs immediately
TEST_F(AlarmSchedulerTest, SetMultiAlarmIncreasingState)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.1");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.5");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 2000), _, _));
  _alarm_scheduler->issue_alarm("test", "2000.4");
  _ms.trap_complete(1, 5);
}


// Test that syncing alarms generates INFORMs for all known alarms immediately
TEST_F(AlarmSchedulerTest, SyncAlarms)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  // Put our three alarms into a state we expect
  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE,
                                        1000), _, _));

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
                                        1001), _, _));

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
                                        1002), _, _));

  _alarm_scheduler->issue_alarm("test", "1000.3");
  _alarm_scheduler->issue_alarm("test", "1001.1");
  _alarm_scheduler->issue_alarm("test", "1002.1");

  _ms.trap_complete(3, 5);

  // Now call sync_alarms and expect those states to be retransmitted
  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE,
                                        1000), _, _));

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
                                        1001), _, _));

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
                                        1002), _, _));

  _alarm_scheduler->sync_alarms();

  _ms.trap_complete(3, 5);
}

// Test that an alarm flicker situation doesn't cause flickering INFORMs
// (by sending multiple set/clears, and checking that this doesn't
// generate informs).
TEST_F(AlarmSchedulerTest, AlarmFlicker)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE,
                                    1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
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
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

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
  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE,
                                       1000), _, _));
  cwtest_advance_time_ms(AlarmScheduler::ALARM_RETRY_DELAY);
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test that when the raise alarm fails, but a clear alarm has been sent in later,
// the clear alarm only is sent after a delay
TEST_F(AlarmSchedulerTest, AlarmFailedToSendClearedInInterval)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

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

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::CLEAR,
                                       1000), _, _));
  cwtest_advance_time_ms(AlarmScheduler::ALARM_REDUCED_DELAY);
  _alarm_scheduler->_cond->signal();
  _ms.trap_complete(1, 5);
}

// Test handling of an invalid alarm
TEST_F(AlarmSchedulerTest, InvalidAlarmIdentifier)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    Times(0);

  _alarm_scheduler->issue_alarm("test", "not_digits.3");
  EXPECT_TRUE(_log.contains("malformed alarm identifier"));
}

// Test handling of an unknown alarm
TEST_F(AlarmSchedulerTest, UnknownAlarmIdentifier)
{
  std::set<NotificationType> snmp_notifications;
  snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, snmp_notifications, "hostname1");

  EXPECT_CALL(_ms, send_v2trap(_, _, _)).
    Times(0);

  _alarm_scheduler->issue_alarm("test", "6000.3");
  EXPECT_TRUE(_log.contains("Unknown alarm definition"));
}
