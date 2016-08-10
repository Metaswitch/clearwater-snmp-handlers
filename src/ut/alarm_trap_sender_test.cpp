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
    *os << "trap is being sent";
  }
  
  virtual void DescribeNegationTo(::std::ostream* os) const {
     *os << "trap is being sent";
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

class AlarmTrapSenderTest : public ::testing::Test
{
public:
  AlarmTrapSenderTest()
  {
    cwtest_completely_control_time();
    cwtest_intercept_netsnmp(&_ms);
  }

  virtual ~AlarmTrapSenderTest()
  {
    delete _alarm_scheduler; _alarm_scheduler = NULL;
    delete _alarm_table_defs; _alarm_table_defs = NULL;
    _collector.call_all_callbacks();

    cwtest_restore_netsnmp();
    cwtest_reset_time();
  }

private:
  std::set<NotificationType> _snmp_notifications;
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

TEST_F(AlarmTrapSenderTest, SetEnterpriseAlarm)
{   
  _snmp_notifications.insert(NotificationType::ENTERPRISE);
  _alarm_table_defs = new AlarmTableDefs();
  _alarm_table_defs->initialize(std::string(UT_DIR).append("/valid_alarms/"));
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, _snmp_notifications);

  // Here we are checking for severity reported as an AlarmModelState value
  oid trap_type[] = {1,2,826,0,1,1578918,19444,9,2,1,1};
  oid alarm_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2,1000};
  oid zero_dot_zero[] = {0,0};
  COLLECT_CALL(send_v2trap(EnterpriseTrapVars(*(trap_type),
                                              "201608081100",
                                              "Process fail",
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

TEST_F(AlarmTrapSenderTest, SetRFCAlarm)
{
  _snmp_notifications.insert(NotificationType::RFC3877);
  _alarm_table_defs = new AlarmTableDefs();
  _alarm_table_defs->initialize(std::string(UT_DIR).append("/valid_alarms/"));
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, _snmp_notifications);

  COLLECT_CALL(send_v2trap(RFCTrapVars(RFCTrapVarsMatcher::ACTIVE, 1000), _, _));
  _alarm_scheduler->issue_alarm("test", "1000.3");
  _ms.trap_complete(1, 5);
}

TEST_F(AlarmTrapSenderTest, SetBothAlarms)
{
  _snmp_notifications.insert(NotificationType::RFC3877);
  _snmp_notifications.insert(NotificationType::ENTERPRISE);
  _alarm_table_defs = new AlarmTableDefs();
  _alarm_table_defs->initialize(std::string(UT_DIR).append("/valid_alarms/"));
  _alarm_scheduler = new AlarmScheduler(_alarm_table_defs, _snmp_notifications);

  oid trap_type[] = {1,2,826,0,1,1578918,19444,9,2,1,1};
  oid alarm_oid[] = {1,3,6,1,2,1,118,1,1,2,1,3,0,1,2,1000};
  oid zero_dot_zero[] = {0,0};
  COLLECT_CALL(send_v2trap(EnterpriseTrapVars(*(trap_type),
                                              "201608081100",
                                              "Process fail",
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
