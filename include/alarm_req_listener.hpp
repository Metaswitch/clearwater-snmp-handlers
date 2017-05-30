/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef ALARM_REQ_LISTENER_HPP
#define ALARM_REQ_LISTENER_HPP

#include <vector>
#include <string>
#include <semaphore.h>

#include "alarm_scheduler.hpp"
 
// Singleton which provides a listener thead to accept alarm requests from
// clients via ZMQ, then generates alarmActiveState/alarmClearState inform
// notifications as appropriate based upon these requests.

class AlarmReqListener
{
public:
  AlarmReqListener(AlarmScheduler* alarm_scheduler) :
    _ctx(NULL),
    _sck(NULL),
    _alarm_scheduler(alarm_scheduler)
  {}

  // Initialize ZMQ context and start listener thread.
  bool start(sem_t* term_sem);

  // Gracefully stop the listener thread and remove ZMQ context.
  void stop();

private:
  enum {ZMQ_PORT = 6664};

  static void* listener_thread(void* alarm_req_listener);

  bool zmq_init_ctx();
  bool zmq_init_sck();

  void zmq_clean_ctx();
  void zmq_clean_sck();

  void listener();

  bool next_msg(std::vector<std::string>& msg);

  void reply(const char* response);

  pthread_t _thread;

  pthread_mutex_t _start_mutex;
  pthread_cond_t  _start_cond;

  void* _ctx;
  void* _sck;
  AlarmScheduler* _alarm_scheduler;

  sem_t* _term_sem;
};

#endif

