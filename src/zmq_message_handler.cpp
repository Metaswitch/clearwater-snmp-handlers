/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include "zmq_message_handler.hpp"
#include "nodedata.hpp"
#include "globals.hpp"
#include <cstdlib>

void IPCountStatHandler::handle(std::vector<std::string> msgs)
{
  // Messages are in [ip_address, count, ip_address, count] pairs
  OIDMap new_subtree;
  // First two entries are the statistic name and the string "OK", so
  // skip them
  for (std::vector<std::string>::iterator it_ip = (msgs.begin() + 2), it_val = (msgs.begin() + 3);
       (it_ip != msgs.end()) && (it_val != msgs.end());
       it_ip++++, it_val++++)
  {
    OIDInetAddr oid_addr(*it_ip);

    if (oid_addr.isValid())
    {
      OID this_oid = _root_oid;

      // The root OIDs for the ConnectionCount tables (bonoConnectedSproutsTable,
      // sproutConnectedHomersTable, sproutConnectedHomesteadsTable and
      // mementoConnectedHomesteadsTable) don't contain the element for the
      // table Entry (always "1") or the element for the ConnectionCount
      // (always "3"), so insert these before adding the IP address index.
      this_oid.append("1.3");
      this_oid.append(oid_addr);

      int connections_to_this_ip = atoi(it_val->c_str());
      new_subtree[this_oid] = connections_to_this_ip;
    }
  }
  _tree->replace_subtree(_root_oid, new_subtree);
}

void BareStatHandler::handle(std::vector<std::string> msgs)
{
  // First two entries are the statistic name and the string "OK", so
  // skip them
  if (msgs.size() >= 3)
  {
    _tree->set(_root_oid, atoi(msgs[2].c_str()));
  }
}

void SingleNumberStatHandler::handle(std::vector<std::string> msgs)
{
  if (msgs.size() >= 3)
  {
    OIDMap new_subtree;
  
    OID this_oid = _root_oid;
    this_oid.append("0"); // Indicates a scalar value in SNMP
  
    // First two entries are the statistic name and the string "OK", so
    // skip them
  
    new_subtree[this_oid] = atoi(msgs[2].c_str());
    _tree->replace_subtree(_root_oid, new_subtree);
  }
  else
  {
    _tree->remove_subtree(_root_oid);
  }
}

// This handler is provided for the case where a stat only provides a single
// value, but it is only refreshed at set periods of time, not every time it
// changes.
void SingleNumberWithScopeStatHandler::handle(std::vector<std::string> msgs)
{
  if (msgs.size() >= 3)
  {
    OIDMap new_subtree;
  
    OID count_oid = _root_oid;
    count_oid.append("1.2");
  
    // First two entries are the statistic name and the string "OK", so
    // skip them
    new_subtree[count_oid] = atoi(msgs[2].c_str());
    _tree->replace_subtree(_root_oid, new_subtree);
  }
  else
  {
    _tree->remove_subtree(_root_oid);
  }
}

// This handler is provided for the case where average statistics are required
// as well as a total count
void AccumulatedWithCountStatHandler::handle(std::vector<std::string> msgs)
{
  if (msgs.size() >= 7)
  {
    OID average_oid = _root_oid;
    average_oid.append("1.2");
   
    OID variance_oid = _root_oid;
    variance_oid.append("1.3");
   
    OID hwm_oid = _root_oid;
    hwm_oid.append("1.4");
   
    OID lwm_oid = _root_oid;
    lwm_oid.append("1.5");
   
    OID count_oid = _root_oid;
    count_oid.append("1.6");
   
    // First two entries are the statistic name and the string "OK", so
    // skip them
    OIDMap new_subtree = {{average_oid, atoi(msgs[2].c_str())},
                          {variance_oid, atoi(msgs[3].c_str())},
    // Note that HWM and LWM are in a different order in SNMP and 0MQ
                          {hwm_oid, atoi(msgs[5].c_str())},
                          {lwm_oid, atoi(msgs[4].c_str())},
                          {count_oid, atoi(msgs[6].c_str())}
    };
   
    _tree->replace_subtree(_root_oid, new_subtree);
  }
  else
  {
    _tree->remove_subtree(_root_oid);
  }
}
