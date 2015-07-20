/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2015 Metaswitch Networks Ltd
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
#include <signal.h>
#include <string>
#include <vector>

#include "utils.h"
#include "alarm_table_defs.hpp"
#include "alarm_req_listener.hpp"
#include "alarm_model_table.hpp"
#include "itu_alarm_table.hpp"

bool done = false;

// Signal handler that triggers termination.
void terminate_handler(int sig)
{
  done = true;
}

int main (int argc, char **argv)
{
  std::vector<std::string> trap_ips;
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
        Utils::split_string(optarg, ',', trap_ips);
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

  // Connect to the informsinks
  for (std::vector<std::string>::iterator ii = trap_ips.begin();
       ii != trap_ips.end();
       ii++)
  {
    create_trap_session(const_cast<char*>(ii->c_str()), 161, community,
                        SNMP_VERSION_2c, SNMP_MSG_INFORM);  
  }

  // Initialise the ZMQ listeners and alarm tables
  // Pull in any local alarm definitions off the node. This is currently a 
  // hard coded path to a single file - this should be a (configurable?) path
  // to a folder. 
  std::string local_alarms_path = "/usr/share/clearwater/infrastructure/local_alarms.json";
  AlarmTableDefs::get_instance().initialize(local_alarms_path);
  AlarmReqListener::get_instance().start();
  init_alarmModelTable();
  init_ituAlarmTable();

  // Run forever
  init_snmp("clearwater-alarms");

  signal(SIGTERM, terminate_handler);
  
  while (!done)
  {
    agent_check_and_process(1);
  }

  snmp_shutdown("clearwater-alarms");

  return 0;
}
