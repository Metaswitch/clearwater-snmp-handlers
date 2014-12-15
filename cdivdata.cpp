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

OID cdiv_total_oid = OID("1.2.826.0.1.1578918.9.7.1.1.2");
OID cdiv_unconditional_oid = OID("1.2.826.0.1.1578918.9.7.1.1.3");
OID cdiv_busy_oid = OID("1.2.826.0.1.1578918.9.7.1.1.4");
OID cdiv_not_registered_oid = OID("1.2.826.0.1.1578918.9.7.1.1.5");
OID cdiv_no_answer_oid = OID("1.2.826.0.1.1578918.9.7.1.1.6");
OID cdiv_not_reachable_oid = OID("1.2.826.0.1.1578918.9.7.1.1.7");

BareStatHandler cdiv_total_handler(cdiv_total_oid, &tree);
BareStatHandler cdiv_unconditional_handler(cdiv_unconditional_oid, &tree);
BareStatHandler cdiv_busy_handler(cdiv_busy_oid, &tree);
BareStatHandler cdiv_not_registered_handler(cdiv_not_registered_oid, &tree);
BareStatHandler cdiv_no_answer_handler(cdiv_no_answer_oid, &tree);
BareStatHandler cdiv_not_reachable_handler(cdiv_not_reachable_oid, &tree);

NodeData cdiv_node_data("sprout",
                        OID("1.2.826.0.1.1578918.9.7"),
                        {"cdiv_total",
                         "cdiv_unconditional",
                         "cdiv_busy", 
                         "cdiv_not_registered",
                         "cdiv_no_answer", 
                         "cdiv_not_reachable"},
                        {{"cdiv_total", &cdiv_total_handler},
                         {"cdiv_unconditional", &cdiv_unconditional_handler},
                         {"cdiv_busy", &cdiv_busy_handler},
                         {"cdiv_not_registered", &cdiv_not_registered_handler},
                         {"cdiv_no_answer", &cdiv_no_answer_handler},
                         {"cdiv_not_reachable", &cdiv_not_reachable_handler}
                        });

extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_cdiv_handler()
  {
    initialize_handler(&cdiv_node_data);
  }
}
