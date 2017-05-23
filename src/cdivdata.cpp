/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
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
