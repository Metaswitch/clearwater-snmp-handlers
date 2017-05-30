/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include <string.h>
#include <zmq.h>

#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "log.h"
#include "alarm_req_listener.hpp"

bool AlarmReqListener::start(sem_t* term_sem)
{
  _term_sem = term_sem;
  if (!zmq_init_ctx())
  {
    return false;
  }

  if (!zmq_init_sck())
  {
    return false;
  }

  pthread_mutex_init(&_start_mutex, NULL);
  pthread_cond_init(&_start_cond, NULL);
  pthread_mutex_lock(&_start_mutex);

  int rc = pthread_create(&_thread, NULL, &listener_thread, (void*)this);
  if (rc == 0)
  {
    pthread_cond_wait(&_start_cond, &_start_mutex);
  }

  pthread_mutex_unlock(&_start_mutex);

  pthread_cond_destroy(&_start_cond);
  pthread_mutex_destroy(&_start_mutex);

  if (rc != 0)
  {
    // LCOV_EXCL_START - No mock for pthread_create
    TRC_ERROR("error creating listener thread: %s", strerror(rc));
    zmq_clean_ctx();
    return false;
    // LCOV_EXCL_STOP
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
  AlarmReqListener* l = (AlarmReqListener*)alarm_req_listener;
  l->listener();

  // If this thread exits, fire the termination semaphore to ensure the main thread shuts down.
  if (l->_term_sem)
  {
    sem_post(l->_term_sem); // LCOV_EXCL_LINE
  }

  return NULL;
}

bool AlarmReqListener::zmq_init_ctx()
{
  _ctx = zmq_ctx_new();
  if (_ctx == NULL)
  {
    TRC_ERROR("zmq_ctx_new failed: %s", zmq_strerror(errno));
    return false;
  }

  return true;
}

bool AlarmReqListener::zmq_init_sck()
{
  _sck = zmq_socket(_ctx, ZMQ_REP);

  if (_sck == NULL)
  {
    TRC_ERROR("zmq_socket failed: %s", zmq_strerror(errno));
    return false;
  }

  // Set a LINGER period of 0 so that we exit immediately on shutdown
  int linger = 0;
  zmq_setsockopt(_sck, ZMQ_LINGER, &linger, sizeof(linger));

#ifdef UNIT_TEST
  std::string sck_file = std::string("/tmp/ut-alarms-socket-" + std::to_string(getpid()));
#else
  std::string sck_file = std::string("/var/run/clearwater/alarms");
#endif
  std::string sck_url = std::string("ipc://" + sck_file);
  TRC_INFO("AlarmReqListener: ss='%s'", sck_url.c_str());

  int rc = remove(sck_file.c_str());

  if (rc == -1)
  {
    TRC_ERROR("remove(%s) failed: %s", sck_file.c_str(), strerror(errno));

    if (errno != ENOENT)
    {
      // LCOV_EXCL_START
      return false;
      // LCOV_EXCL_STOP
    }
  }

  while (((rc = zmq_bind(_sck, sck_url.c_str())) == -1) && (errno == EINTR))
  {
    // Ignore possible errors due to a syscall being interrupted by a signal.
  }

  if (rc == -1)
  {
    TRC_ERROR("zmq_bind failed: %s", zmq_strerror(errno));
    return false;
  }

  rc=chmod(sck_file.c_str(), 0777);
  if (rc == -1)
  {
    TRC_ERROR("chmod(%s, 0777) failed: %s", sck_file.c_str(), strerror(errno));
    // We don't return false in UT as the chmod always fails (because the
    // previous zmq calls are mocked out the socket file isn't created so can't
    // be chmodded).
#ifndef UNIT_TEST
    return false;
#endif
  }

  return true;
}

void AlarmReqListener::zmq_clean_ctx()
{
  if (_ctx)
  {
    if (zmq_ctx_destroy(_ctx) == -1)
    {
      TRC_ERROR("zmq_ctx_destroy failed: %s", zmq_strerror(errno));
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
      TRC_ERROR("zmq_close failed: %s", zmq_strerror(errno));
    }

    _sck = NULL;
  }
}

void AlarmReqListener::listener()
{
  pthread_mutex_lock(&_start_mutex);
  pthread_cond_signal(&_start_cond);
  pthread_mutex_unlock(&_start_mutex);
  while (1)
  {
    std::vector<std::string> msg;

    if (!next_msg(msg))
    {
      zmq_clean_sck();
      return;
    }
    // The ZMQ message read here contains the name of the alarm
    // issuer e.g. "monit" and the alarm identifier e.g. "1000.3"
    // Note the alarm identifier uses ituAlarmPerceivedSeverity. We can
    // translate this to alarmModelState using the function 
    // AlarmTableDef::state - this mapping is described in RFC 3877
    // section 5.4: https://tools.ietf.org/html/rfc3877#section-5.4
    if ((msg[0].compare("issue-alarm") == 0) && (msg.size() == 3))
    {
      _alarm_scheduler->issue_alarm(msg[1], msg[2]);
    }
    else if ((msg[0].compare("sync-alarms") == 0) && (msg.size() == 1))
    {
      _alarm_scheduler->sync_alarms();
    }
    else if ((msg[0].compare("poll") == 0) && (msg.size() == 1))
    {
      // do nothing, just reply "ok"
    }
    else
    {
      TRC_ERROR("unexpected alarm request: %s, %lu", msg[0].c_str(), msg.size());
    }

    reply("ok");
  }
}

bool AlarmReqListener::next_msg(std::vector<std::string>& msg)
{
  int rc;

  int more = 0;
  size_t more_sz = sizeof(more);

  do
  {
    zmq_msg_t msg_part;

    if (zmq_msg_init(&msg_part) != 0)
    {
      TRC_ERROR("zmq_msg_init failed: %s", zmq_strerror(errno));
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
        TRC_ERROR("zmq_msg_recv failed: %s", zmq_strerror(errno));
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
        TRC_ERROR("zmq_getsockopt failed: %s", zmq_strerror(errno));
      }

      return false;
    }

    if (zmq_msg_close(&msg_part) == -1)
    {
      TRC_ERROR("zmq_msg_close failed: %s", zmq_strerror(errno));
      return false;
    }

  } while(more);

  return true;
}

void AlarmReqListener::reply(const char* response)
{
  if (zmq_send(_sck, response, strlen(response), 0) == -1)
  {
    TRC_ERROR("zmq_send failed: %s", zmq_strerror(errno));
  }
}
