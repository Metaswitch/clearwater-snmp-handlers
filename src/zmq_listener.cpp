/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include "zmq_listener.hpp"
#include "zmq_message_handler.hpp"
#include "nodedata.hpp"
#include "globals.hpp"
#include <string>
#include <vector>
#include <ctime>

bool ZMQListener::connect_and_subscribe()
{
  // Create the context.
  _ctx = zmq_ctx_new();
  if (_ctx == NULL)
  {
    perror("zmq_ctx_new");
    return false;
  }

  // Create the socket and connect it to the host.
  _sck = zmq_socket(_ctx, ZMQ_SUB);
  if (_sck == NULL)
  {
    perror("zmq_socket");
    return false;
  }
  std::string ep = std::string("ipc:///var/run/clearwater/stats/") + _node_data->name;
  if (zmq_connect(_sck, ep.c_str()) != 0)
  {
    perror("zmq_connect");
    return false;
  }

  for (std::vector<std::string>::iterator it = _node_data->stats.begin();
       it != _node_data->stats.end();
       it++)
  {
    // Subscribe to the specified statistic.
    if (zmq_setsockopt(_sck, ZMQ_SUBSCRIBE, it->c_str(), strlen(it->c_str())) != 0)
    {
      perror("zmq_setsockopt");
      return false;
    }
  }

  return true;
}

// Listen for ZMQ publishes for a given host/port/statistic name, then
// update the statistics structs with that information.  This loops forever,
// so should be run in its own thread.
void ZMQListener::handle_requests_forever()
{
  if (!connect_and_subscribe())
  {
    return;
  };
  // Main loop of the thread - listen for ZMQ publish messages. Once a
  // whole block of data has been read, call the appropriate handler
  // function to populate data->struct_ptr with the stats received.
  while (1)
  {
    // Spin round until we've got all the messages in this block.
    int64_t more = 0;
    size_t more_sz = sizeof(more);
    std::vector<std::string> msgs;
    int rc;

    do
    {
      zmq_msg_t msg;
      if (zmq_msg_init(&msg) != 0)
      {
        perror("zmq_msg_init");
        return;
      }
      while (((rc = zmq_msg_recv(&msg, _sck, 0)) == -1) && (errno == EINTR))
      {
        // Ignore possible errors caused by a syscall being interrupted by a signal. This can 
        // occur at start-up due to SIGRT_1 (for which snmpd does not apparently set SA_RESTART). 
      }
      if (rc == -1)
      {
        perror("zmq_msg_recv");
        return;
      }
      msgs.push_back(std::string((char*)zmq_msg_data(&msg), zmq_msg_size(&msg)));
      while (((rc = zmq_getsockopt(_sck, ZMQ_RCVMORE, &more, &more_sz)) == -1) && (errno == EINTR))
      {
        // Ignore possible errors caused by a syscall being interrupted by a signal. This can 
        // occur at start-up due to SIGRT_1 (for which snmpd does not apparently set SA_RESTART). 
      }
      if (rc == -1)
      {
        perror("zmq_getsockopt");
        return;
      }
      zmq_msg_close(&msg);
    }
    while (more);
    last_seen_time.store(time(NULL));
    if ((msgs.size() >= 2) && (msgs[1].compare("OK") == 0))
    {
      std::string message_type = msgs[0];
      ZMQMessageHandler* handler = _node_data->stat_to_handler.at(message_type);
      handler->handle(msgs);
    }
  }
};

ZMQListener::~ZMQListener()
{
  // Close the socket.
  if (zmq_close(_sck) != 0)
  {
    perror("zmq_close");
  }
  _sck = NULL;

  // Destroy the context.
  if (zmq_ctx_destroy(_ctx) != 0)
  {
    perror("zmq_ctx_destroy");
  }
  _ctx = NULL;
}
