/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/


#ifndef OIDTREE_HPP
#define OIDTREE_HPP

#include "oid.hpp"
#include <map>
#include <mutex>

class OIDCompare
{
public:
  bool operator()(const OID& a, const OID& b)
  {
    return (snmp_oid_compare(a.get_ptr(), a.get_len(),
                             b.get_ptr(), b.get_len()) == -1);
  }
};

typedef std::map<OID, int, OIDCompare> OIDMap;


class OIDTree
{
public:
  bool get(OID, int&);
  bool get_next(OID, OID&, int&);
  void set(OID, int);
  void remove(OID);
  void remove_subtree(OID);
  void replace_subtree(OID, OIDMap);
  void dump();

private:
  OIDMap _oidmap;
  std::recursive_mutex _map_lock;
};

#endif
