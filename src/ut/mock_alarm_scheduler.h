/**
 * @file mock_alarm_scheduler.h
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
 */

#ifndef MOCK_ALARM_SCHEDULER_H__
#define MOCK_ALARM_SCHEDULER_H__

#include "gmock/gmock.h"
#include "alarm_scheduler.hpp"

class MockAlarmScheduler : public AlarmScheduler
{
public:
  MockAlarmScheduler(AlarmTableDefs* alarm_table_defs,
                     std::set<NotificationType> snmp_notifications) :
    AlarmScheduler(alarm_table_defs, snmp_notifications, "hostname1")
  {}

  MOCK_METHOD2(issue_alarm, void(const std::string& issuer,
                                 const std::string& identifier));
  MOCK_METHOD0(sync_alarms, void());
};

#endif
