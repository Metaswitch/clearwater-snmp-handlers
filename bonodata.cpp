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

#include "globals.hpp"
#include "nodedata.hpp"
#include "custom_handler.hpp"
#include "zmq_message_handler.hpp"

OID latency_oid = OID("1.2.826.0.1.1578918.9.2.2");
OID client_count_oid = OID("1.2.826.0.1.1578918.9.2.1");
OID sprouts_oid = OID("1.2.826.0.1.1578918.9.2.3.1");

LatencyStatHandler latency_handler(latency_oid, &tree);
SingleNumberStatHandler client_count_handler(client_count_oid, &tree);
IPCountStatHandler connected_sprouts_handler(sprouts_oid, &tree);

NodeData::NodeData()
{
  name = "bono_handler";
  port = "6666";
  root_oid = OID("1.2.826.0.1.1578918.9.2");
  stats = {"latency_us", "client_count", "connected_sprouts"};
  stat_to_handler = {{"latency_us", &latency_handler},
                     {"client_count", &client_count_handler},
                     {"connected_sprouts", &connected_sprouts_handler}};
};

NodeData node_data;

extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_bono_handler()
  {
    initialize_handler();
  }
}
