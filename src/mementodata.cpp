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

OID memento_incoming_request_oid = OID("1.2.826.0.1.1578918.9.8.2.1.1.2");
OID memento_rejected_overload_oid = OID("1.2.826.0.1.1578918.9.8.2.1.1.3");
OID memento_http_latency_oid = OID("1.2.826.0.1.1578918.9.8.2.2");
OID memento_cassandra_read_latency_oid = OID("1.2.826.0.1.1578918.9.8.2.3");
OID memento_record_size_oid = OID("1.2.826.0.1.1578918.9.8.2.4");
OID memento_record_length_oid = OID("1.2.826.0.1.1578918.9.8.2.5");

BareStatHandler memento_incoming_request_handler(memento_incoming_request_oid, &tree);
BareStatHandler memento_rejected_overload_handler(memento_rejected_overload_oid, &tree);
AccumulatedWithCountStatHandler memento_http_latency_handler(memento_http_latency_oid, &tree);
AccumulatedWithCountStatHandler memento_cassandra_read_latency_handler(memento_cassandra_read_latency_oid, &tree);
AccumulatedWithCountStatHandler memento_record_size_handler(memento_record_size_oid, &tree);
AccumulatedWithCountStatHandler memento_record_length_handler(memento_record_length_oid, &tree);

NodeData memento_http_node_data("memento",
                                OID("1.2.826.0.1.1578918.9.8.2"),
                                {"http_incoming_requests",
                                 "http_rejected_overload",
                                 "http_latency_us",
                                 "cassandra_read_latency",
                                 "record_size",
                                 "record_length"},
                                {{"http_incoming_requests", &memento_incoming_request_handler},
                                 {"http_rejected_overload", &memento_rejected_overload_handler},
                                 {"http_latency_us", &memento_http_latency_handler},
                                 {"cassandra_read_latency", &memento_cassandra_read_latency_handler},
                                 {"record_size", &memento_record_size_handler},
                                 {"record_length", &memento_record_length_handler}
                                });

OID memento_auth_challenges_oid = OID("1.2.826.0.1.1578918.9.8.3.2.1.2");
OID memento_auth_attempts_oid = OID("1.2.826.0.1.1578918.9.8.3.2.1.3");
OID memento_auth_successes_oid = OID("1.2.826.0.1.1578918.9.8.3.2.1.4");
OID memento_auth_failures_oid = OID("1.2.826.0.1.1578918.9.8.3.2.1.5");
OID memento_auth_stales_oid = OID("1.2.826.0.1.1578918.9.8.3.2.1.6");

BareStatHandler memento_auth_challenges_handler(memento_auth_challenges_oid, &tree);
BareStatHandler memento_auth_attempts_handler(memento_auth_attempts_oid, &tree);
BareStatHandler memento_auth_successes_handler(memento_auth_successes_oid, &tree);
BareStatHandler memento_auth_failures_handler(memento_auth_failures_oid, &tree);
BareStatHandler memento_auth_stales_handler(memento_auth_stales_oid, &tree);

NodeData memento_auth_node_data("memento",
                                OID("1.2.826.0.1.1578918.9.8.3"),
                                {"auth_challenges",
                                 "auth_attempts",
                                 "auth_successes",
                                 "auth_failures",
                                 "auth_stales"},
                                {{"auth_challenges", &memento_auth_challenges_handler},
                                 {"auth_attempts", &memento_auth_attempts_handler},
                                 {"auth_successes", &memento_auth_successes_handler},
                                 {"auth_failures", &memento_auth_failures_handler},
                                 {"auth_stales", &memento_auth_stales_handler}
                                });

extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_memento_handler()
  {
    initialize_handler(&memento_http_node_data);
    initialize_handler(&memento_auth_node_data);
  }
}
