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

#ifndef ALARM_REQ_LISTENER_HPP
#define ALARM_REQ_LISTENER_HPP

#include <vector>
#include <string>
#include <semaphore.h>
 
// Singleton which provides a listener thead to accept alarm requests from
// clients via ZMQ, then generates alarmActiveState/alarmClearState inform
// notifications as appropriate based upon these requests.

class AlarmReqListener
{
public:
  static AlarmReqListener& get_instance() {return _instance;}

  // Initialize ZMQ context and start listener thread.
  bool start(sem_t* term_sem);

  // Gracefully stop the listener thread and remove ZMQ context.
  void stop();

private:
  enum {ZMQ_PORT = 6664};

  static void* listener_thread(void* alarm_req_listener);

  AlarmReqListener();

  bool zmq_init_ctx();
  bool zmq_init_sck();

  void zmq_clean_ctx();
  void zmq_clean_sck();

  void listener();

  bool next_msg(std::vector<std::string>& msg);

  void reply(const char* response);

  static AlarmReqListener _instance;

  pthread_t _thread;

  pthread_mutex_t _start_mutex;
  pthread_cond_t  _start_cond;

  void* _ctx;
  void* _sck;

  sem_t* _term_sem;
};

#endif

