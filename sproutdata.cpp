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

OID latency_oid = OID("1.2.826.0.1.1578918.9.3.1");
OID hss_latency_oid = OID("1.2.826.0.1.1578918.9.3.3.2");
OID hss_digest_latency_oid = OID("1.2.826.0.1.1578918.9.3.3.3");
OID hss_subscription_latency_oid = OID("1.2.826.0.1.1578918.9.3.3.4");
OID hss_user_auth_latency_oid = OID("1.2.826.0.1.1578918.9.3.3.5");
OID hss_location_latency_oid = OID("1.2.826.0.1.1578918.9.3.3.6");
OID xdm_latency_oid = OID("1.2.826.0.1.1578918.9.3.2.2");
OID homers_oid = OID("1.2.826.0.1.1578918.9.3.2.1");
OID homesteads_oid = OID("1.2.826.0.1.1578918.9.3.3.1");
OID incoming_requests_oid = OID("1.2.826.0.1.1578918.9.3.6");
OID rejected_overload_oid = OID("1.2.826.0.1.1578918.9.3.7");
OID queue_size_oid = OID("1.2.826.0.1.1578918.9.3.8");

AccumulatedWithCountStatHandler latency_handler(latency_oid, &tree);
AccumulatedWithCountStatHandler hss_latency_handler(hss_latency_oid, &tree);
AccumulatedWithCountStatHandler hss_digest_latency_handler(hss_digest_latency_oid, &tree);
AccumulatedWithCountStatHandler hss_subscription_latency_handler(hss_subscription_latency_oid, &tree);
AccumulatedWithCountStatHandler hss_user_auth_latency_handler(hss_user_auth_latency_oid, &tree);
AccumulatedWithCountStatHandler hss_location_latency_handler(hss_location_latency_oid, &tree);
AccumulatedWithCountStatHandler xdm_latency_handler(xdm_latency_oid, &tree);
IPCountStatHandler connected_homers_handler(homers_oid, &tree);
IPCountStatHandler connected_homesteads_handler(homesteads_oid, &tree);
SingleNumberWithScopeStatHandler incoming_requests_handler(incoming_requests_oid, &tree);
SingleNumberWithScopeStatHandler rejected_overload_handler(rejected_overload_oid, &tree);
AccumulatedWithCountStatHandler queue_size_handler(queue_size_oid, &tree);

NodeData::NodeData()
{
  name = "sprout_handler";
  port = "6666";
  root_oid = OID("1.2.826.0.1.1578918.9.3");
  stats = {"latency_us",
           "hss_latency_us",
           "hss_digest_latency_us",
           "hss_subscription_latency_us",
           "hss_user_auth_latency_us",
           "hss_location_latency_us",
           "xdm_latency_us",
           "connected_homers",
           "connected_homesteads",
           "incoming_requests",
           "rejected_overload",
           "queue_size"
          };
  stat_to_handler = {{"latency_us", &latency_handler},
    {"hss_latency_us", &hss_latency_handler},
    {"hss_digest_latency_us", &hss_digest_latency_handler},
    {"hss_subscription_latency_us", &hss_subscription_latency_handler},
    {"hss_user_auth_latency_us", &hss_user_auth_latency_handler},
    {"hss_location_latency_us", &hss_location_latency_handler},
    {"xdm_latency_us", &xdm_latency_handler},
    {"connected_homers", &connected_homers_handler},
    {"connected_homesteads", &connected_homesteads_handler},
    {"incoming_requests", &incoming_requests_handler},
    {"rejected_overload", &rejected_overload_handler},
    {"queue_size", &queue_size_handler}
  };
};

NodeData node_data;

extern "C" {
  // SNMPd looks for an init_<module_name> function in this library
  void init_sprout_handler()
  {
    initialize_handler();
  }
}
