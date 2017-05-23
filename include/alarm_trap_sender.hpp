/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef ALARM_TRAPS_HPP
#define ALARM_TRAPS_HPP

#include <time.h>

#include <string>
#include <map>
#include <set>
#include <cstdint>

#include "alarm_table_defs.hpp"

enum NotificationType
{
  RFC3877,
  ENTERPRISE
};

class AlarmScheduler;

// Class providing methods for generating alarmActiveState and
// alarmClearState inform notifications.
class AlarmTrapSender
{
public:
  void initialise(AlarmScheduler* alarm_scheduler,
                  std::set<NotificationType> snmp_notifications,
                  std::string hostname)
    { _alarm_scheduler    = alarm_scheduler;
      _snmp_notifications = snmp_notifications;
      _hostname = hostname; }

  void send_trap(const AlarmTableDef& alarm_table_def);

  // Callback triggered when an alarm send completes (either successfully
  // or not).
  //
  // @param op - NETSNMP operation code
  // @param alarm_table_def - The alarm entry that was being raised
  void alarm_trap_send_callback(int op,
                                const AlarmTableDef& alarm_table_def);

  static AlarmTrapSender& get_instance() {return _instance;}

private:
  AlarmTrapSender() : _alarm_scheduler(NULL) {}
  AlarmScheduler* _alarm_scheduler;
  std::set<NotificationType> _snmp_notifications;
  std::string _hostname;
  static AlarmTrapSender _instance;
  // Sends an RFC3877 compliant trap based upon the specified alarm definition.
  // net-snmp will handle the retries if needed. 
  void send_rfc3877_trap(const AlarmTableDef& alarm_table_def);
  // Sends an Enterprise MIB style trap based upon the specified alarm
  // definition. net-snmp will handle the retries if needed.
  void send_enterprise_trap(const AlarmTableDef& alarm_table_def);
};

#endif
