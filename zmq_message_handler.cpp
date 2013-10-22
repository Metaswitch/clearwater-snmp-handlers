/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2013 Metaswitch Networks Ltd
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

#include "zmq_message_handler.hpp"
#include "nodedata.hpp"
#include "globals.hpp"
#include <cstdlib>

ZMQMessageHandler::ZMQMessageHandler(OID root_oid, OIDTree* tree)
{
  _root_oid = root_oid;
  _tree = tree;
}

void IPCountStatHandler::handle(std::vector<std::string> msgs)
{
  // Messages are in [ip_address, count, ip_address, count] pairs
  OIDMap new_subtree;
  // First two entries are the statistic name and the string "OK", so
  // skip them
  for (std::vector<std::string>::iterator it_ip = (msgs.begin() + 2), it_val = (msgs.begin() + 3);
       (it_ip != msgs.end()) && (it_val != msgs.end());
       it_ip++++, it_val++++)
  {
    OID this_oid = _root_oid;
    std::string ip_address = *it_ip;
    int connections_to_this_ip = atoi(it_val->c_str());
    this_oid.append(ip_address);

    new_subtree[this_oid] = connections_to_this_ip;
  }
  _tree->replace_subtree(_root_oid, new_subtree);
}

void SingleNumberStatHandler::handle(std::vector<std::string> msgs)
{
  OIDMap new_subtree;

  OID this_oid = _root_oid;
  this_oid.append("0"); // Indicates a scalar value in SNMP

  // First two entries are the statistic name and the string "OK", so
  // skip them

  new_subtree[this_oid] = atoi(msgs[2].c_str());
  _tree->replace_subtree(_root_oid, new_subtree);
}

/* OIDMap LatencyStatHandler::handle(std::vector<std::string> msgs)
{
  OIDMap returnmap;
  OID root_oid = node_data.stat_to_root_oid[msgs[0]];

  OID average_oid = root_oid;
  average_oid.append("1");

  OID variance_oid = root_oid;
  variance_oid.append("2");

  OID lwm_oid = root_oid;
  lwm_oid.append("3");

  OID hwm_oid = root_oid;
  hwm_oid.append("4");

  // First two entries are the statistic name and the string "OK", so
  // skip them
  returnmap[average_oid] = atoi(msgs[2].c_str());
  returnmap[variance_oid] = atoi(msgs[3].c_str());
  returnmap[lwm_oid] = atoi(msgs[4].c_str());
  returnmap[hwm_oid] = atoi(msgs[5].c_str());
  return returnmap;
  } */

void MultipleNumberStatHandler::handle(std::vector<std::string> msgs)
{
  OIDMap new_subtree;
  // First two entries are the statistic name and the string "OK", so
  // skip them
  int oid_index = 1;
  for (std::vector<std::string>::iterator it = msgs.begin() + 2;
       it != msgs.end();
       it++, oid_index++)
  {
    OID this_oid = _root_oid;
    char oid_index_str[4];
    snprintf(oid_index_str, 3, "%d", oid_index);
    this_oid.append(oid_index_str);

    new_subtree[this_oid] = atoi(it->c_str());
  }
  _tree->replace_subtree(_root_oid, new_subtree);
}
