/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/


#ifndef OID_HPP
#define OID_HPP

extern "C"
{
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
}

#include <vector>
#include <string>

#include "oid_inet_addr.hpp"

class OID
{
public:
  OID() {};
  OID(oid);
  OID(OID, oid);
  OID(oid*, int);
  OID(OID, oid*, int);
  OID(std::string);
  OID(OID, std::string);
  OID(OIDInetAddr);
  OID(OID, OIDInetAddr);
  void print_state() const;
  bool equals(OID);
  bool subtree_contains(OID);

  const oid* get_ptr() const;
  int get_len() const;
  void append(oid);
  void append(oid*, int);
  void append(std::string);
  void append(OIDInetAddr);
  std::string to_string() const;
  void dump() const;
private:
  std::vector<oid> _oids;
};

#endif
