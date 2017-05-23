/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "oid.hpp"

// Constructors - can either create from an oid[] or a string

OID::OID(oid x)
{
  append(x);
}

OID::OID(OID parent_oid, oid x) :
  _oids(parent_oid._oids)
{
  append(x);
}

OID::OID(oid* oids_ptr, int len)
{
  append(oids_ptr, len);
}

OID::OID(OID parent_oid, oid* oids_ptr, int len) :
  _oids(parent_oid._oids)
{
  append(oids_ptr, len);
}

OID::OID(std::string oidstr)
{
  append(oidstr);
}

OID::OID(OID parent_oid, std::string oidstr) :
  _oids(parent_oid._oids)
{
  append(oidstr);
}

OID::OID(OIDInetAddr oid_addr)
{
  append(oid_addr);
}

OID::OID(OID parent_oid, OIDInetAddr oid_addr) :
  _oids(parent_oid._oids)
{
  append(oid_addr);
}

// Functions to expose the underlying oid* for compatability with the
// C API

const oid* OID::get_ptr() const
{
  return &_oids[0];
}

int OID::get_len() const
{
  return _oids.size();
}

bool OID::equals(OID other_oid)
{
  return (netsnmp_oid_equals(get_ptr(), get_len(),
                             other_oid.get_ptr(), other_oid.get_len()) == 0);
}

bool OID::subtree_contains(OID other_oid)
{
  return (snmp_oidtree_compare(get_ptr(), get_len(),
                               other_oid.get_ptr(), other_oid.get_len()) == 0);
}

void OID::append(oid x)
{
  _oids.push_back(x);
}

void OID::append(oid* oids_ptr, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    _oids.push_back(*(oids_ptr+i));
  }
}

// Appends the given OID string to this OID
// e.g. OID("1.2.3.4").append("5.6") is OID("1.2.3.4.5.6")
void OID::append(std::string oidstr)
{
  std::vector<std::string> result;
  boost::split(result, oidstr, boost::is_any_of("."));
  for (std::vector<std::string>::iterator it = result.begin() ; it != result.end(); ++it)
  {
    if (!it->empty())   // Ignore an initial dot
    {
      _oids.push_back(atoi(it->c_str()));
    }
  }
}

void OID::append(OIDInetAddr oid_addr)
{
  std::vector<unsigned char> oid_bytes = oid_addr.toOIDBytes();
  _oids.insert(_oids.end(), oid_bytes.begin(), oid_bytes.end());
}

std::string OID::to_string() const
{
  std::stringstream ss;
  for (std::vector<oid>::const_iterator it = _oids.begin() ; it != _oids.end(); ++it)
  {
    ss << "." << *it;
  }
  return ss.str();
}

// Debugging tool - prints this OID to stderr
void OID::dump() const
{
  std::cerr << to_string();
}

