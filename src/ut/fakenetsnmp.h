/**
 * @file fakenetsnmp.h
 *
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
 */

#pragma once

#include <pthread.h>

#include "gmock/gmock.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

class NetSnmpInterface
{
public:
  NetSnmpInterface();

  virtual void send_v2trap(netsnmp_variable_list *, snmp_callback, void*) = 0;

  bool trap_complete(int count, int timeout);
  void trap_signal();

  void log_insert(const char* data);
  bool log_contains(const char* fragment);

private:
  pthread_mutex_t _mutex;
  pthread_cond_t  _cond;

  int _trap_count;

  std::string _logged;
};

class MockNetSnmpInterface : public NetSnmpInterface
{
public:
  MOCK_METHOD3(send_v2trap, void(netsnmp_variable_list *, snmp_callback, void*));
};

void cwtest_intercept_netsnmp(NetSnmpInterface* intf);
void cwtest_restore_netsnmp();

