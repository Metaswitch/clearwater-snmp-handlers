/**
 * @file fakenetsnmp.cpp
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
 */

#include <string>

#include "fakenetsnmp.h"

static NetSnmpInterface* netsnmp_intf_p = NULL;

NetSnmpInterface::NetSnmpInterface() :
  _trap_count(0)
{
  pthread_mutex_init(&_mutex, NULL);

  pthread_condattr_t cond_attr;
  pthread_condattr_init(&cond_attr);
  pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);

  pthread_cond_init(&_cond, &cond_attr);

  pthread_condattr_destroy(&cond_attr);
}

bool NetSnmpInterface::trap_complete(int count, int timeout)
{
  struct timespec wake_time;
  int rc = 0;

  clock_gettime(CLOCK_MONOTONIC, &wake_time);
  wake_time.tv_sec += timeout;

  pthread_mutex_lock(&_mutex);
  while ((rc == 0) && (_trap_count != count))
  {
    rc = pthread_cond_timedwait(&_cond, &_mutex, &wake_time);
  }
  _trap_count = 0;
  pthread_mutex_unlock(&_mutex);

  return (rc == 0);
}

void NetSnmpInterface::trap_signal()
{
  pthread_mutex_lock(&_mutex);
  _trap_count++;
  pthread_cond_signal(&_cond);
  pthread_mutex_unlock(&_mutex);
}

void NetSnmpInterface::log_insert(const char* data)
{
  std::string line(data);
  line.append("\n");

  pthread_mutex_lock(&_mutex);
  _logged.append(line);
  pthread_mutex_unlock(&_mutex);
}

bool NetSnmpInterface::log_contains(const char* fragment)
{
  bool result;
  pthread_mutex_lock(&_mutex);
  result = _logged.find(fragment) != std::string::npos;
  pthread_mutex_unlock(&_mutex);
  return result;
}

void cwtest_intercept_netsnmp(NetSnmpInterface* intf)
{
  netsnmp_intf_p = intf;
}

void cwtest_restore_netsnmp()
{
  netsnmp_intf_p = NULL;
}


#ifdef __cplusplus
extern          "C" {
#endif

int netsnmp_table_array_helper_handler(netsnmp_mib_handler *handler,
                                       netsnmp_handler_registration *reginfo,
                                       netsnmp_agent_request_info *reqinfo,
                                       netsnmp_request_info *requests)
{
  return 0;
}

netsnmp_handler_registration *
netsnmp_create_handler_registration(const char *name, Netsnmp_Node_Handler*
                                    handler_access_method, const oid *reg_oid,
                                    size_t reg_oid_len, int modes)
{
  static netsnmp_handler_registration hr;
  return &hr;
}

int netsnmp_table_container_register(netsnmp_handler_registration *reginfo,
                                     netsnmp_table_registration_info *tabreq,
                                     netsnmp_table_array_callbacks *cb,
                                     netsnmp_container *container,
                                     int group_rows)
{
  return 0;
}

void send_v2trap(netsnmp_variable_list *variable_list, snmp_callback callback, void* correlator)
{
  if (netsnmp_intf_p)
  {
    netsnmp_intf_p->send_v2trap(variable_list, callback, correlator);

    netsnmp_intf_p->trap_signal();
  }
}

int snmp_log(int priority, const char *format, ...)
{
  if (netsnmp_intf_p)
  {
    va_list args;
    va_start(args, format);

    char buf[1024];
    const int bufSize = sizeof(buf)/sizeof(buf[0]);

    vsnprintf(buf, bufSize, format, args);

    va_end(args);

    netsnmp_intf_p->log_insert(buf);
  }

  return 0;
}

#ifdef __cplusplus
}
#endif

