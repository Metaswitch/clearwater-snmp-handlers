/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef NODEDATA_HPP
#define NODEDATA_HPP

#include <vector>
#include <map>
#include <string>
#include "oid.hpp"
#include "zmq_message_handler.hpp"

class NodeData
{
public:
  NodeData(std::string _name,
           OID _root_oid,
           std::vector<std::string> _stats,
           std::map<std::string, ZMQMessageHandler*> _stat_to_handler) :
    name(_name),
    root_oid(_root_oid),
    stats(_stats),
    stat_to_handler(_stat_to_handler)
  {}

  std::string name;
  OID root_oid;
  std::vector<std::string> stats;
  std::map<std::string, ZMQMessageHandler*> stat_to_handler;
};

#endif
