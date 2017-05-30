/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef CUSTOM_HANDLER_HPP
#define CUSTOM_HANDLER_HPP

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "nodedata.hpp"

/* function declarations */
void initialize_handler(NodeData* node_data);
Netsnmp_Node_Handler clearwater_handler;

#endif /** BONOLATENCYTABLE_H */
