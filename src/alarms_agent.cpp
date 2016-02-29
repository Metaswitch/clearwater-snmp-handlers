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
#include <semaphore.h>

#include "utils.h"
#include "snmp_agent.h"
#include "logger.h"
#include "log.h"
#include "alarm_table_defs.hpp"
#include "alarm_req_listener.hpp"
#include "alarm_model_table.hpp"
#include "itu_alarm_table.hpp"
#include "alarm_active_table.hpp"

static sem_t term_sem;
// Signal handler that triggers termination.
void agent_terminate_handler(int sig)
{
  sem_post(&term_sem);
}

enum OptionTypes
{
  OPT_COMMUNITY=256+1,
  OPT_LOCAL_IP,
  OPT_SNMP_IPS,
  OPT_LOG_LEVEL,
  OPT_LOG_DIR
};

const static struct option long_opt[] =
{
  { "community",                       required_argument, 0, OPT_COMMUNITY},
  { "snmp-ips",                        required_argument, 0, OPT_SNMP_IPS},
  { "local_ip",                        required_argument, 0, OPT_LOCAL_IP},
  { "log-level",                       required_argument, 0, OPT_LOG_LEVEL},
  { "log-dir",                         required_argument, 0, OPT_LOG_DIR},
};

static void usage(void)
{
    puts("Options:\n"
         "\n"
         " --snmp-ips <ip>,<ip>       Send SNMP notifications to the specified IPs\n"
         " --community <name>         Include the given community string on notifications\n"
         " --log-dir <directory>\n"
         "                            Log to file in specified directory\n"
         " --log-level N              Set log level to N (default: 4)\n"
        );
}

int main (int argc, char **argv)
{
  std::vector<std::string> trap_ips;
  char* community = NULL;
  std::string local_ip = "0.0.0.0";
  std::string logdir = "";
  int loglevel = 4;
  int c;
  int optind;

  opterr = 0;
  while ((c = getopt_long(argc, argv, "", long_opt, &optind)) != -1)
  {
    switch (c)
      {
      case OPT_COMMUNITY:
        community = optarg;
        break;
      case OPT_SNMP_IPS:
        Utils::split_string(optarg, ',', trap_ips);
        break;
      case OPT_LOCAL_IP:
        local_ip = optarg;       
        break;
      case OPT_LOG_LEVEL:
        loglevel = atoi(optarg);
        break;
      case OPT_LOG_DIR:
        logdir = optarg;
        break;
      default:
        usage();
        abort();
      }
  }
  
  Log::setLoggingLevel(loglevel);
  Log::setLogger(new Logger(logdir, "clearwater-alarms"));
  snmp_setup("clearwater-alarms");
  sem_init(&term_sem, 0, 0);
  // Connect to the informsinks
  for (std::vector<std::string>::iterator ii = trap_ips.begin();
       ii != trap_ips.end();
       ii++)
  {
    create_trap_session(const_cast<char*>(ii->c_str()), 162, community,
                        SNMP_VERSION_2c, SNMP_MSG_INFORM);  
  }

  // Initialise the ZMQ listeners and alarm tables
  // Pull in any local alarm definitions off the node.
  std::string alarms_path = "/usr/share/clearwater/infrastructure/alarms/";
  AlarmTableDefs::get_instance().initialize(alarms_path);

  // Exit if the ReqListener wasn't able to fully start
  if (!AlarmReqListener::get_instance().start(&term_sem))
  {
    TRC_ERROR("Hit error starting the listener - shutting down");
    return 0;
  }

  init_alarmModelTable();
  init_ituAlarmTable();
  init_alarmActiveTable(local_ip);
 
  init_snmp_handler_threads("clearwater-alarms");

  TRC_STATUS("Alarm agent has started");
 
  signal(SIGTERM, agent_terminate_handler);
  
  sem_wait(&term_sem);
  snmp_terminate("clearwater-alarms");

  return 0;
}
