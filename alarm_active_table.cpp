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

/*
 *  Note: this file originally auto-generated by mib2c using
 *        : mib2c.array-user.conf 15997 2007-03-25 22:28:35Z dts12 $
 *
 *  $Id:$
 */

#include "alarm_active_table.hpp"
#include "log.h"

#include <string>

static netsnmp_handler_registration* my_handler = NULL;
static netsnmp_table_array_callbacks cb;

std::map<unsigned long, alarmActiveTable_context*> alarm_indexes_to_rows;
unsigned long int alarm_counter = 0;
static std::string local_ip;
const std::string empty_string = "";
/************************************************************
 *
 *  Initialize the alarmActiveTable table
 */
int init_alarmActiveTable(std::string ip)
{
  // This sets the local ip address of the box which issued the alarm.
  // This is read from the local config file and passed from 
  // alarms_agent
  local_ip = ip;
  netsnmp_table_registration_info *table_info;

  if (my_handler)
  {
    // LCOV_EXCL_START
    TRC_ERROR("initialize_table_alarmActiveTable called again");
    return SNMP_ERR_NOERROR;
    // LCOV_EXCL_STOP
  }

  memset(&cb, 0x00, sizeof(cb));

  /** create the table structure itself */
  table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);

  my_handler = netsnmp_create_handler_registration("alarmActiveTable",
                                                   netsnmp_table_array_helper_handler,
                                                   alarmActiveTable_oid,
                                                   alarmActiveTable_oid_len,
                                                   HANDLER_CAN_RONLY);
            
  if (!my_handler || !table_info)
  {
    // LCOV_EXCL_START 
    TRC_ERROR("malloc failed: initialize_table_alarmActiveTable");
    return SNMP_ERR_GENERR;
    // LCOV_EXCL_STOP
  }

  /*
   * Setting up the table's definition
   */

  /** index: alarmListName */
  netsnmp_table_helper_add_index(table_info, ASN_OCTET_STR);
  /** index: alarmActiveDateAndTime */
  netsnmp_table_helper_add_index(table_info, ASN_OCTET_STR);
  /** index: alarmActiveIndex */
  netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);

  table_info->min_column = alarmActiveTable_COL_MIN;
  table_info->max_column = alarmActiveTable_COL_MAX;

  /*
   * registering the table with the master agent
   */
  cb.get_value = alarmActiveTable_get_value;
  cb.container = netsnmp_container_find("alarmActiveTable_primary:"
                                        "alarmActiveTable:"
                                        "table_container");
  cb.can_set = 0;

  TRC_DEBUG("initialize_table_alarmActiveTable", "Registering as table array\n");

  netsnmp_table_container_register(my_handler, table_info, &cb, cb.container, 1);

  return SNMP_ERR_NOERROR;
}

/************************************************************
 *
 *  This routine is called for get requests to copy the data
 *  from the context to the varbind for the request.
 */
int alarmActiveTable_get_value(netsnmp_request_info* request,
                               netsnmp_index* item,
                               netsnmp_table_request_info* table_info)
{
  netsnmp_variable_list* var = request->requestvb;
  alarmActiveTable_context* context = (alarmActiveTable_context*) item;
  switch (table_info->colnum)
  {
    case COLUMN_ALARMACTIVEENGINEID:
    {
      // Cannot obtain with SNMPv2 so use zero-length-string //
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) empty_string.c_str(),
                               strlen(empty_string.c_str()));
    }
    break;

    case COLUMN_ALARMACTIVEENGINEADDRESSTYPE:
    {
      // This is of type InetAddressType. RFC 3291 asserts that this
      // should be an integer with 1 and 2 mapping to ipv4 and ipv6
      // respectively.
      int ip_type = (local_ip.find(":") == std::string::npos) ? 1 : 2;
      snmp_set_var_typed_value(var, ASN_INTEGER,
                               (u_char*) &ip_type,
                               sizeof(ip_type));
  
    }
    break;

    case COLUMN_ALARMACTIVEENGINEADDRESS:
    {
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) local_ip.c_str(),
                               strlen(local_ip.c_str()));
    }
    break;

    case COLUMN_ALARMACTIVECONTEXTNAME:
    {
      // We do not support multiple contexts so this object
      // will be set to the empty string
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) empty_string.c_str(),
                               strlen(empty_string.c_str()));
    }
    break;
    
    case COLUMN_ALARMACTIVEVARIABLES:
    {
      // The number of variables in alarmActiveVariableTable for
      // this alarm. See as we have no such table this will be 0.
      uint32_t variables = 0;
      snmp_set_var_typed_value(var, ASN_UNSIGNED,
                               (u_char*) &variables,
                               sizeof(variables));
    }
    break;

    case COLUMN_ALARMACTIVENOTIFICATIONID:
    {
      // We do not store cleared alarms in the Active Alarm Table.
      // Hence this value will always be the alarm active state oid.
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) alarm_active_state_oid,
                               sizeof(alarm_active_state_oid));
    }
    break;
    
    case COLUMN_ALARMACTIVERESOURCEID:
    {
      // None of our alarms have resources so this will always
      // be the 0.0 OID
      
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) zero_dot_zero_oid,
                               sizeof(zero_dot_zero_oid));
    }
    break;
    
    case COLUMN_ALARMACTIVEDESCRIPTION:
    {
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) context->_alarm_table_def->description().c_str(),
                               context->_alarm_table_def->description().length());
    }
    break;

    case COLUMN_ALARMACTIVELOGPOINTER:
    {
      // We don't keep log entries so this will always be
      // the 0.0 OID
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) zero_dot_zero_oid,
                               sizeof(zero_dot_zero_oid));
    }
    break;

    case COLUMN_ALARMACTIVEMODELPOINTER:
    {
      oid* model_pointer;
      // We add 3 here to accomodate for the leading "0" (to be inserted before
      // the alarm's index), the alarm's index and the alarm's severity.
      int model_pointer_len = alarmModelTable_oid_len + entry_column_oid_len + 3;
      // Append the index array on to the Alarm Model Table OID, inserting
      // the OID ".1.3." for the entry in the first non index column and
      // inserting a leading "0" before the alarm's index.
      model_pointer = new oid[model_pointer_len];
      std::copy(alarmModelTable_oid, alarmModelTable_oid + alarmModelTable_oid_len, model_pointer);
      std::copy(entry_column_oid, entry_column_oid + entry_column_oid_len, model_pointer + alarmModelTable_oid_len);
      model_pointer[alarmModelTable_oid_len + entry_column_oid_len] = 0;
      model_pointer[alarmModelTable_oid_len + entry_column_oid_len + 1] = context->_alarm_table_def->alarm_index(); 
      model_pointer[alarmModelTable_oid_len + entry_column_oid_len + 2] = context->_alarm_table_def->severity();      
      
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) model_pointer,
                               model_pointer_len * sizeof(oid));
      delete[](model_pointer);

    }
    break;
    
    case COLUMN_ALARMACTIVESPECIFICPOINTER:
    {
      // This would point to the model-specific active alarm list
      // for this alarm. We do not support model-specific Alarm-MIB
      // and hence this will always be the 0.0 OID
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) zero_dot_zero_oid,
                               sizeof(zero_dot_zero_oid));
    }
    break;
    
    
    default: /** We shouldn't get here */
    {
      // LCOV_EXCL_START
      TRC_ERROR("unknown column: alarmActiveTable_get_value");
      return SNMP_ERR_GENERR;
      // LCOV_EXCL_STOP
    }
  }
  return SNMP_ERR_NOERROR;
}

/*************************************************************
 *
 * Find the current time and use it to fill the d_time struct
 * which is formatted to be compliant with RFC 3877
 */
alarmActiveTable_SNMPDateTime alarm_time_issued(void)
{
  alarmActiveTable_SNMPDateTime d_time;
  struct timespec ts;
  struct tm timing;
  time_t rawtime;
  int UTC_offset;

  clock_gettime(CLOCK_REALTIME, &ts);
  rawtime = ts.tv_sec;
  // This updates the variable timezone declared within time.h which
  // tells us how offset (in seconds) from UTC we are and stores a 
  // broken down time representation in the structure timing
  timing = *localtime(&rawtime);
  UTC_offset = std::abs(timezone) / 3600;
  // htons converts the integer from host byte order to network
  // byte order as specified in RFC 2579.
  d_time.year = htons(1900 + timing.tm_year);
  d_time.month = 1 + timing.tm_mon;
  d_time.day = timing.tm_mday;
  d_time.hour = timing.tm_hour;
  d_time.minute = timing.tm_min;
  d_time.second = timing.tm_sec;
  d_time.decisecond = 0;
  d_time.direction = (timezone > 0) ? '+' : '-';
  d_time.timegeozone = UTC_offset;
  d_time.mintimezone = (std::abs(timezone) % 3600) / 60;
  return d_time;
}

/***********************************************************
 *
 * Logic tests to determine how to handle each alarm that
 * is raised. This ensures each alarm has at most one entry
 * in Alarm Active Table.
 */
void alarmActiveTable_trap_handler(AlarmTableDef& def)
{
  // This stops UTs from failing when the alarm table is uninitialized.
  if (my_handler)
  {
    alarmActiveTable_context* existing_row = alarm_indexes_to_rows[def.alarm_index()];
    // 1) New alarm
    if (existing_row == NULL && def.severity() != AlarmDef::CLEARED)
    {
      alarmActiveTable_create_row(def); 
    }
    // 2) Existing alarm with different severity
    else if (existing_row != NULL && def.severity() != AlarmDef::CLEARED && def.severity() != existing_row->_alarm_table_def->severity())
    {
      alarmActiveTable_delete_row(def);
      alarmActiveTable_create_row(def);
    }
    // 3) Existing alarm with same severity
    else if (existing_row != NULL && def.severity() != AlarmDef::CLEARED && def.severity() == existing_row->_alarm_table_def->severity())
    {
      // No-op
    }
    // 4) Clearing a raised alarm
    else if (existing_row != NULL && def.severity() == AlarmDef::CLEARED)
    {
      alarmActiveTable_delete_row(def);
    }
    // 5) Clearing an unraised alarm 
    else if (existing_row == NULL && def.severity() == AlarmDef::CLEARED)
    {
      // No-op
    }
  }
  return;
}

/************************************************************
 * 
 *  Create a new row context and initialize its oid index.
 */
void alarmActiveTable_create_row(AlarmTableDef& def)
{
  alarmActiveTable_context* ctx = SNMP_MALLOC_TYPEDEF(alarmActiveTable_context);
  if (!ctx)
  {
    // LCOV_EXCL_START
    TRC_ERROR("malloc failed: alarmActiveTable_create_row");
    return;
    // LCOV_EXCL_STOP
  }
  ctx->_alarm_table_def = &def;
  alarmActiveTable_SNMPDateTime datetime = alarm_time_issued(); 
  alarm_counter++;      
  if (alarmActiveTable_index_to_oid((char*) "", datetime, alarm_counter, &ctx->_index) != SNMP_ERR_NOERROR)
  {
    // LCOV_EXCL_START
    if (ctx->_index.oids != NULL)
    {
      free(ctx->_index.oids);
    }

    free(ctx);
    return;
    // LCOV_EXCL_STOP
  }
  if (ctx)
  {
    CONTAINER_INSERT(cb.container, ctx);
    // This creates a mapping from the index of the current alarm raised
    // to a pointer which points to the row in Active Alarm Table corresponding
    // to this alarm. This helps us determine whether an alarm has been 
    // previously raised.
    alarm_indexes_to_rows[def.alarm_index()] = ctx;
  }
  // Reset the alarm counter if it reaches its max limit.
  if (alarm_counter == UINT_MAX)
  {
    alarm_counter = 0;  // LCOV_EXCL_LINE
  }
  return;
}

/************************************************************
 *
 *  Remove an old row and free up its row context
 */
void alarmActiveTable_delete_row(AlarmTableDef& def)
{
  alarmActiveTable_context* existing_row = alarm_indexes_to_rows[def.alarm_index()];
  CONTAINER_REMOVE(cb.container, existing_row);
  // After we remove the old row we also free up the pointer that was
  // pointing to it.
  free(existing_row->_index.oids);
  free(existing_row);
  alarm_indexes_to_rows[def.alarm_index()] = NULL;
  return;
}

/************************************************************
 *
 *  Convert table index components to an oid.
 */
int alarmActiveTable_index_to_oid(char* name,
                                 alarmActiveTable_SNMPDateTime datetime,
                                 unsigned long counter,
                                 netsnmp_index *oid_idx)
{
  int err = SNMP_ERR_NOERROR;
  netsnmp_variable_list var_alarmListName;
  netsnmp_variable_list var_alarmActiveDateAndTime;
  netsnmp_variable_list var_alarmActiveIndex;

  /*
   * set up varbinds
   */
  memset(&var_alarmListName, 0x00, sizeof(var_alarmListName));
  var_alarmListName.type = ASN_OCTET_STR;
  memset(&var_alarmActiveDateAndTime, 0x00, sizeof(var_alarmActiveDateAndTime));
  var_alarmActiveDateAndTime.type = ASN_OCTET_STR;
  memset(&var_alarmActiveIndex, 0x00, sizeof(var_alarmActiveIndex));
  var_alarmActiveIndex.type = ASN_UNSIGNED;

  /*
   * chain index varbinds together
   */
  var_alarmListName.next_variable = &var_alarmActiveDateAndTime;
  var_alarmActiveDateAndTime.next_variable = &var_alarmActiveIndex;
  var_alarmActiveIndex.next_variable =  NULL;
  
  snmp_set_var_value(&var_alarmListName, (u_char*) name, strlen(name));
  //Here we use the value 11 as that is how many octets the datetime structure
  //contains. We were not able to find the size of the structure using sizeof
  //due to structure padding.
  snmp_set_var_value(&var_alarmActiveDateAndTime, (u_char*) &datetime, 11);
  snmp_set_var_value(&var_alarmActiveIndex, (u_char*) &counter, sizeof(counter));

  err = build_oid(&oid_idx->oids, &oid_idx->len, NULL, 0, &var_alarmListName);
  if (err)
  {
    TRC_ERROR("error %d converting index to oid: alarmActiveTable_index_to_oid", err); // LCOV_EXCL_LINE
  }

  /*
   * parsing may have allocated memory. free it.
   */
  snmp_reset_var_buffers(&var_alarmListName);

  return err;
}
