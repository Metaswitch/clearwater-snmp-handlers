/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2014 Metaswitch Networks Ltd
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version, along with the "Special Exception" for use of
* the program along with SSL, set forth below. This program is distributed
* in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more
* details. You should have received a copy of the GNU General Public
* License along with this program. If not, see
* <http://www.gnu.org/licenses/>.
*
* The author can be reached by email at clearwater@metaswitch.com or by
* post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
*
* Special Exception
* Metaswitch Networks Ltd grants you permission to copy, modify,
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
