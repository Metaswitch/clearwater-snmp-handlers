/**
 * Copyright (C) Metaswitch Networks 2016
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

OID memento_completed_calls_oid = OID("1.2.826.0.1.1578918.9.8.1.1.1.2");
OID memento_failed_calls_oid = OID("1.2.826.0.1.1578918.9.8.1.1.1.3");
OID memento_not_recorded_overload_oid = OID("1.2.826.0.1.1578918.9.8.1.1.1.4");
OID memento_cassandra_read_latency_oid = OID("1.2.826.0.1.1578918.9.8.1.2");
OID memento_cassandra_write_latency_oid = OID("1.2.826.0.1.1578918.9.8.1.3");

BareStatHandler memento_completed_calls_handler(memento_completed_calls_oid, &tree);
BareStatHandler memento_failed_calls_handler(memento_failed_calls_oid, &tree);
BareStatHandler memento_not_recorded_overload_handler(memento_not_recorded_overload_oid, &tree);
AccumulatedWithCountStatHandler memento_cassandra_read_latency_handler(memento_cassandra_read_latency_oid, &tree);
AccumulatedWithCountStatHandler memento_cassandra_write_latency_handler(memento_cassandra_write_latency_oid, &tree);

NodeData memento_as_node_data("sprout",
                              OID("1.2.826.0.1.1578918.9.8.1"),
                              {"memento_completed_calls",
                               "memento_failed_calls",
                               "memento_not_recorded_overload",
                               "memento_cassandra_read_latency",
                               "memento_cassandra_write_latency"},
                              {{"memento_completed_calls", &memento_completed_calls_handler},
                               {"memento_failed_calls", &memento_failed_calls_handler},
                               {"memento_not_recorded_overload", &memento_not_recorded_overload_handler},
                               {"memento_cassandra_read_latency", &memento_cassandra_read_latency_handler},
                               {"memento_cassandra_write_latency", &memento_cassandra_write_latency_handler}
                              });

extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_memento_as_handler()
  {
    initialize_handler(&memento_as_node_data);
  }
}
