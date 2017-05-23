/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef ZMQ_LISTENER_HPP
#define ZMQ_LISTENER_HPP

#include <zmq.h>
#include "zmq_message_handler.hpp"
#include "nodedata.hpp"

class ZMQListener
{
public:
  ZMQListener(NodeData* node_data) : _node_data(node_data) {}
  ~ZMQListener();
  bool connect_and_subscribe();
  void handle_requests_forever();

private:
  NodeData* _node_data;
  void* _ctx;
  void* _sck;
};

#endif
