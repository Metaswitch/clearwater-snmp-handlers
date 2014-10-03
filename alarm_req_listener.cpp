/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2014 Metaswitch Networks Ltd
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

#include <string.h>
#include <zmq.h>

#include <sstream>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "alarm_req_listener.hpp"
#include "alarm_trap_sender.hpp"

AlarmReqListener AlarmReqListener::_instance;

bool AlarmReqListener::start()
{
  if (!zmq_init_ctx())
  {
    return false;
  }

  int rc = pthread_create(&_thread, NULL, &listener_thread, (void*) &_instance);
  if (rc != 0)
  {
    snmp_log(LOG_ERR, "error creating listener thread: %s", strerror(rc));

    zmq_clean_ctx();
    return false;
  }

  return true;
}

void AlarmReqListener::stop()
{
  zmq_clean_ctx();

  pthread_join(_thread, NULL);
}

void* AlarmReqListener::listener_thread(void* alarm_req_listener)
{
  ((AlarmReqListener*) alarm_req_listener)->listener();

  return NULL;
}

AlarmReqListener::AlarmReqListener() : _ctx(NULL), _sck(NULL)
{
}

bool AlarmReqListener::zmq_init_ctx()
{
  _ctx = zmq_ctx_new();
  if (_ctx == NULL)
  {
    snmp_log(LOG_ERR, "zmq_ctx_new failed: %s", zmq_strerror(errno));
    return false;
  }

  return true;
}

bool AlarmReqListener::zmq_init_sck()
{
  _sck = zmq_socket(_ctx, ZMQ_REP);
  if (_sck == NULL)
  {
    snmp_log(LOG_ERR, "zmq_socket failed: %s", zmq_strerror(errno));
    return false;
  }

  std::stringstream ss;
  ss << "tcp://127.0.0.1:" << ZMQ_PORT; 

  int rc;
  while (((rc = zmq_bind(_sck, ss.str().c_str())) == -1) && (errno == EINTR))
  {
    // Ignore possible errors due to a syscall being interrupted by a signal.
  }

  if (rc == -1)
  {
    snmp_log(LOG_ERR, "zmq_bind failed: %s", zmq_strerror(errno));
    return false;
  }

  return true;
}

void AlarmReqListener::zmq_clean_ctx()
{
  if (_ctx)
  {
    if (zmq_ctx_destroy(_ctx) == -1)
    {
      snmp_log(LOG_ERR, "zmq_ctx_destroy failed: %s", zmq_strerror(errno));
    }

    _ctx = NULL;
  }
}

void AlarmReqListener::zmq_clean_sck()
{
  if (_sck)
  {
    if (zmq_close(_sck) == -1)
    {
      snmp_log(LOG_ERR, "zmq_close failed: %s", zmq_strerror(errno));
    }

    _sck = NULL;
  }
}

void AlarmReqListener::listener()
{
  if (!zmq_init_sck())
  {
    return;
  }

  while (1)
  {
    std::vector<std::string> msg;

    if (!next_msg(msg))
    {
      zmq_clean_sck();
      return;
    }

    if ((msg[0].compare("issue-alarm") == 0) && (msg.size() == 3))
    {
      AlarmTrapSender::get_instance().issue_alarm(msg[1], msg[2]);
    }
    else if ((msg[0].compare("clear-alarms") == 0) && (msg.size() == 2))
    {
      AlarmTrapSender::get_instance().clear_alarms(msg[1]);
    }
    else if ((msg[0].compare("sync-alarms") == 0) && (msg.size() == 1))
    {
      AlarmTrapSender::get_instance().sync_alarms();
    }
    else
    {
      snmp_log(LOG_ERR, "unexpected alarm request: %s", msg[0].c_str());
    }

    reply("ok");
  }
}

bool AlarmReqListener::next_msg(std::vector<std::string>& msg)
{
  int rc;

  int more;
  size_t more_sz = sizeof(more);

  do
  {
    zmq_msg_t msg_part;

    if (zmq_msg_init(&msg_part) != 0)
    {
      snmp_log(LOG_ERR, "zmq_msg_init failed: %s", zmq_strerror(errno));
      return false;
    }

    while (((rc = zmq_msg_recv(&msg_part, _sck, 0)) == -1) && (errno == EINTR))
    {
      // Ignore possible errors due to a syscall being interrupted by a signal.
    }

    if (rc == -1)
    {
      if (errno != ETERM)
      {
        snmp_log(LOG_ERR, "zmq_msg_recv failed: %s", zmq_strerror(errno));
      }

      return false;
    }

    msg.push_back(std::string((char*)zmq_msg_data(&msg_part), zmq_msg_size(&msg_part)));

    while (((rc = zmq_getsockopt(_sck, ZMQ_RCVMORE, &more, &more_sz)) == -1) && (errno == EINTR))
    {
      // Ignore possible errors due to a syscall being interrupted by a signal.
    }

    if (rc == -1)
    {
      if (errno != ETERM)
      {
        snmp_log(LOG_ERR, "zmq_getsockopt failed: %s", zmq_strerror(errno));
      }

      return false;
    }

    if (zmq_msg_close(&msg_part) == -1)
    {
      snmp_log(LOG_ERR, "zmq_msg_close failed: %s", zmq_strerror(errno));
      return false;
    }

  } while(more);

  return true;
}

void AlarmReqListener::reply(const char* response)
{
  if (zmq_send(_sck, response, strlen(response), 0) == -1)
  {
    snmp_log(LOG_ERR, "zmq_send failed: %s", zmq_strerror(errno));
  }
}

