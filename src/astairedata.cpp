/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include "globals.hpp"
#include "nodedata.hpp"
#include "custom_handler.hpp"

class AstaireGlobalStatHandler: public ZMQMessageHandler
{
public:
  AstaireGlobalStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string> msgs)
  {
    OID buckets_needing_resync_oid(_root_oid, "1.0");
    OID buckets_resynchronized_oid(_root_oid, "2.0");
    OID entries_resynchronized_oid(_root_oid, "3.0");
    OID data_resynchronized_oid(_root_oid, "4.0");
    OID bandwidth_oid(_root_oid, "5.1.2.1");

    if (msgs.size() >= 7 )
    {
      _tree->set(buckets_needing_resync_oid, atoi(msgs[2].c_str()));
      _tree->set(buckets_resynchronized_oid, atoi(msgs[3].c_str()));
      _tree->set(entries_resynchronized_oid, atoi(msgs[4].c_str()));
      _tree->set(data_resynchronized_oid, atoi(msgs[5].c_str()));
      _tree->set(bandwidth_oid, atoi(msgs[6].c_str()));
    }
    else
    {
      snmp_log(LOG_INFO, "AstaireGlobalStatHandler received too short globals - %d < 7", (int)msgs.size());
      _tree->remove(buckets_needing_resync_oid);
      _tree->remove(buckets_resynchronized_oid);
      _tree->remove(entries_resynchronized_oid);
      _tree->remove(data_resynchronized_oid);
      _tree->remove(bandwidth_oid);
    }
  }
};

class AstaireConnectionStatHandler: public ZMQMessageHandler
{
public:
  AstaireConnectionStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string> msgs)
  {
    OID connection_base_oid(_root_oid, "6.1");
    OIDMap connection_tree;
    OID bucket_base_oid(_root_oid, "7.1");
    OIDMap bucket_tree;
    OID bucket_bandwidth_base_oid(_root_oid, "8.1");
    OIDMap bucket_bandwidth_tree;

    int connection = 0;
    for (int ii = 2; ii < (int)msgs.size(); )
    {
      // Check that we have enough fields for at least one connection.
      if ((int)msgs.size() >= ii + 5)
      {
        // Connections are indexed by internet address and port (first two fields).
        OIDInetAddr oid_addr(msgs[ii].c_str());
        int port = atoi(msgs[ii + 1].c_str());
        if (oid_addr.isValid())
        {
          // Calculate the field OIDs.
          OID buckets_needing_resync_oid(connection_base_oid, 4);
          buckets_needing_resync_oid.append(oid_addr);
          buckets_needing_resync_oid.append(port);
          OID buckets_resynchronized_oid(connection_base_oid, 5);
          buckets_resynchronized_oid.append(oid_addr);
          buckets_resynchronized_oid.append(port);

          // Set the fields in the subtree map.
          connection_tree[buckets_needing_resync_oid] = atoi(msgs[ii + 2].c_str());
          connection_tree[buckets_resynchronized_oid] = atoi(msgs[ii + 3].c_str());
      
          // Read the number of buckets, calculate the number of fields we expect and
          // keep parsing if we've got enough.
          int num_buckets = atoi(msgs[ii + 4].c_str());
          ii += 5;
          int end_buckets = ii + num_buckets * 4;
          if ((int)msgs.size() >= end_buckets)
          {
            int bucket = 0;
            for (; ii < end_buckets; ii += 4)
            {
              // Buckets are indexed by their identity (first field).
              int bucket_id = atoi(msgs[ii].c_str());

              // Calculate the field OIDs.  Note that indices always go at the end.
              OID bucket_entries_resynchronized_oid(bucket_base_oid, 5);
              bucket_entries_resynchronized_oid.append(oid_addr);
              bucket_entries_resynchronized_oid.append(port);
              bucket_entries_resynchronized_oid.append(bucket_id);
              OID bucket_data_resynchronized_oid(bucket_base_oid, 6);
              bucket_data_resynchronized_oid.append(oid_addr);
              bucket_data_resynchronized_oid.append(port);
              bucket_data_resynchronized_oid.append(bucket_id);
              OID bucket_bandwidth_oid(bucket_bandwidth_base_oid, 6);
              bucket_bandwidth_oid.append(oid_addr);
              bucket_bandwidth_oid.append(port);
              bucket_bandwidth_oid.append(bucket_id);
              bucket_bandwidth_oid.append(1);

              // Set the fields in the subtree map.
              bucket_tree[bucket_entries_resynchronized_oid] = atoi(msgs[ii + 1].c_str());
              bucket_tree[bucket_data_resynchronized_oid] = atoi(msgs[ii + 2].c_str());
              bucket_bandwidth_tree[bucket_bandwidth_oid] = atoi(msgs[ii + 3].c_str());

              bucket++;
            }
          }
          else
          {
            snmp_log(LOG_INFO, "AstaireConnectionStatHandler received too short bucket list - %d < %d", (int)msgs.size(), end_buckets);
            break;
          }
        }
        else
        {
          snmp_log(LOG_INFO, "AstaireConnectionStatHandler received invalid IP address - %s", msgs[ii].c_str());
          break;
        }
      }
      else
      {
        snmp_log(LOG_INFO, "AstaireConnectionStatHandler received too short connection - %d < %d", (int)msgs.size(), ii + 4);
        break;
      }
      connection++;
    }

    _tree->replace_subtree(connection_base_oid, connection_tree);
    _tree->replace_subtree(bucket_base_oid, bucket_tree);
    _tree->replace_subtree(bucket_bandwidth_base_oid, bucket_bandwidth_tree);
  }
};

OID astaire_oid = OID("1.2.826.0.1.1578918.9.9");

AstaireGlobalStatHandler global_handler(astaire_oid, &tree);
AstaireConnectionStatHandler connection_handler(astaire_oid, &tree);

NodeData astaire_node_data("astaire",
                           astaire_oid,
                           {"astaire_global",
                            "astaire_connections"
                           },
                           {{"astaire_global", &global_handler},
                            {"astaire_connections", &connection_handler}
                           });

extern "C" {
  // SNMPd looks for an init_<module_name> function in this library
  void init_astaire_handler()
  {
    initialize_handler(&astaire_node_data);
  }
}
