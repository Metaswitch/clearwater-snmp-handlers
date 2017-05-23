/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include "oidtree.hpp"
#include <iostream>

static void dump_oidmap(OIDMap m);

bool OIDTree::get(OID requested_oid, int& output_result)
{
  _map_lock.lock();
  bool retval = false;
  OIDMap::iterator oid_location = _oidmap.find(requested_oid);
  if (oid_location == _oidmap.end())
  {
    retval = false;
  }
  else
  {
    output_result = oid_location->second;
    retval = true;
  }
  _map_lock.unlock();
  return retval;
}

bool OIDTree::get_next(OID requested_oid, OID& output_oid, int& output_result)
{
  _map_lock.lock();
  bool retval = false;
  OIDMap::iterator oid_location = _oidmap.upper_bound(requested_oid);

  if (oid_location == _oidmap.end())
  {
    retval = false;
  }
  else
  {
    output_oid = oid_location->first;
    output_result = oid_location->second;
    retval = true;
  }
  _map_lock.unlock();
  return retval;
}

void OIDTree::remove(OID key)
{
  _map_lock.lock();
  _oidmap.erase(key);
  _map_lock.unlock();
}

void OIDTree::remove_subtree(OID root_oid)
{
  _map_lock.lock();

  // Create a temporary copy, to avoid iterating over the map we're
  // deleting items from.
  OIDMap tmp_oidmap = _oidmap;
  for(OIDMap::iterator it = tmp_oidmap.begin();
      it != tmp_oidmap.end();
      ++it)
  {
    OID this_oid = it->first;
    if (root_oid.subtree_contains(this_oid))
    {
      _oidmap.erase(this_oid);
    }
  }
  _map_lock.unlock();
}

void OIDTree::replace_subtree(OID root_oid, OIDMap update)
{
  _map_lock.lock();
  remove_subtree(root_oid);
  _oidmap.insert(update.begin(), update.end());

  _map_lock.unlock();
}


void OIDTree::set(OID key, int value)
{
  _map_lock.lock();
  _oidmap[key] = value;
  _map_lock.unlock();
}

void OIDTree::dump()
{
  dump_oidmap(_oidmap);
}

static void dump_oidmap(OIDMap m) {
  for(OIDMap::iterator it = m.begin();
      it != m.end();
      ++it)
  {
    std::cerr << it->first.to_string() << " " << it->second << "\n";
  }
}
