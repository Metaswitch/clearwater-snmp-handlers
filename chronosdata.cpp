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

#include "globals.hpp"
#include "nodedata.hpp"
#include "custom_handler.hpp"
#include "zmq_message_handler.hpp"

OID nodes_query_oid = OID("1.2.826.0.1.1578918.9.10.1");
OID timers_processed_oid = OID("1.2.826.0.1.1578918.9.10.2");

SingleNumberStatHandler nodes_query_handler(nodes_query_oid, &tree);
SingleNumberWithScopeStatHandler timers_processed_handler(timers_processed_oid, &tree);

NodeData chronos_node_data("chronos",
                           OID("1.2.826.0.1.1578918.9.10"),
                           {"chronos_scale_nodes_to_query",
                            "chronos_scale_timers_processed"
                           },
                           {{"chronos_scale_nodes_to_query", &nodes_query_handler},
                            {"chronos_scale_timers_processed", &timers_processed_handler}
                           });

extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_chronos_handler()
  {
    initialize_handler(&chronos_node_data);
  }
}
