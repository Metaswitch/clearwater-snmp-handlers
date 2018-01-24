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

OID os_file_descriptors_max_oid = OID("1.2.826.0.1.1578918.17.1");
OID os_file_descriptors_cur_oid = OID("1.2.826.0.1.1578918.17.2");
OID os_file_descriptors_perc_oid = OID("1.2.826.0.1.1578918.17.3");


BareStatHandler os_fd_max_handler(os_file_descriptors_max_oid, &tree);
BareStatHandler os_fd_cur_handler(os_file_descriptors_cur_oid, &tree);
BareStatHandler os_fd_perc_handler(os_file_descriptors_perc_oid, &tree);

NodeData os_fd_data("os",
                    OID("1.2.826.0.1.1578918.17"),
                    {"file_descriptors_max",
                     "file_descriptors_cur",
                     "file_descriptors_perc"},
                    {{"file_descriptors_max", &os_fd_max_handler},
                     {"file_descriptors_cur", &os_fd_cur_handler},
                     {"file_descriptors_perc", &os_fd_perc_handler}
                    });


extern "C"
{
  // SNMPd looks for an init_<module_name> function in this library
  void init_os_handler()
  {
    initialize_handler(&os_fd_data);
  }
}
