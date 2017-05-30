/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#include "oid_inet_addr.hpp"


void OIDInetAddr::setAddr(const std::string& addrStr)
{
  if (inet_pton(AF_INET, addrStr.c_str(), &_addr) == 1) {
    _type = ipv4; 
  } else if (inet_pton(AF_INET6, addrStr.c_str(), &_addr) == 1) {
    _type = ipv6;
  } else {
    _type = unknown;
  }
}

std::vector<unsigned char> OIDInetAddr::toOIDBytes()
{
  std::vector<unsigned char> oidBytes;

  if (isValid()) {
    unsigned char addrLen = (_type == ipv4) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

    oidBytes.push_back(_type);
    oidBytes.push_back(addrLen);
    oidBytes.insert(oidBytes.end(), (unsigned char *) &_addr, (unsigned char *) &_addr + addrLen);
  }

  return oidBytes;
}

