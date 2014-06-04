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
    OIDInetAddr oid_addr(*it_ip);

    if (oid_addr.isValid())
    {
      OID this_oid = _root_oid;
      this_oid.append(oid_addr);

      int connections_to_this_ip = atoi(it_val->c_str());
      new_subtree[this_oid] = connections_to_this_ip;
    }
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

// This handler is provided for the case where a stat only provides a single
// value, but it is only refreshed at set periods of time, not every time it
// changes.
void SingleNumberWithScopeStatHandler::handle(std::vector<std::string> msgs)
{
  OIDMap new_subtree;

  OID count_oid = _root_oid;
  count_oid.append("1.2");

  // First two entries are the statistic name and the string "OK", so
  // skip them
  new_subtree[count_oid] = atoi(msgs[2].c_str());
  _tree->replace_subtree(_root_oid, new_subtree);
}

// This handler is provided for the case where average statistics are required
// as well as a total count
void AccumulatedWithCountStatHandler::handle(std::vector<std::string> msgs)
{
  OID average_oid = _root_oid;
  average_oid.append("1.2");

  OID variance_oid = _root_oid;
  variance_oid.append("1.3");

  OID hwm_oid = _root_oid;
  hwm_oid.append("1.4");

  OID lwm_oid = _root_oid;
  lwm_oid.append("1.5");

  OID count_oid = _root_oid;
  count_oid.append("1.6");

  // First two entries are the statistic name and the string "OK", so
  // skip them
  OIDMap new_subtree = {{average_oid, atoi(msgs[2].c_str())},
                        {variance_oid, atoi(msgs[3].c_str())},
// Note that HWM and LWM are in a different order in SNMP and 0MQ
                        {hwm_oid, atoi(msgs[5].c_str())},
                        {lwm_oid, atoi(msgs[4].c_str())},
                        {count_oid, atoi(msgs[6].c_str())}
  };

  _tree->replace_subtree(_root_oid, new_subtree);
}
