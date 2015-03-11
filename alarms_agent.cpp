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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_trap.h>

#include "alarm_table_defs.hpp"
#include "alarm_req_listener.hpp"
#include "alarm_model_table.hpp"
#include "itu_alarm_table.hpp"

int main (int argc, char **argv)
{
  char* trap_ip = NULL;
  char* community = NULL;
  int c;

  opterr = 0;
  while ((c = getopt (argc, argv, "i:c:")) != -1)
  {
    switch (c)
      {
      case 'c':
        community = optarg;
        break;
      case 'i':
        trap_ip = optarg;
        break;
      default:
        abort ();
      }
  }

  // Log SNMP library output to syslog
  snmp_enable_calllog();

  // Set ourselves up as a subagent
  netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
  init_agent("clearwater-alarms");

  // Connect to the informsink
  create_trap_session(trap_ip, 161, community,
                      SNMP_VERSION_2c, SNMP_MSG_INFORM);  

  // Initialise the ZMQ listeners and alarm tables
  AlarmTableDefs::get_instance().initialize();
  AlarmReqListener::get_instance().start();
  init_alarmModelTable();
  init_ituAlarmTable();

  // Run forever
  init_snmp("clearwater-alarms");

  while (1)
  {
    agent_check_and_process(1);
  }

  /* at shutdown time */
  snmp_shutdown("clearwater_alarms");

  return 0;
}