/**
 * Copyright (C) Metaswitch Networks 2016
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef OID_INET_ADDR_HPP
#define OID_INET_ADDR_HPP

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <vector>


// Helper class used to convert IPv4 and IPv6 address strings to protocol
// independent internet address table index OIDs as described in RFC 4001.

class OIDInetAddr
{
public:
  OIDInetAddr() {_type = unknown;}
  OIDInetAddr(const std::string& addrStr) {setAddr(addrStr);}

  void setAddr(const std::string&);
  bool isValid() {return _type != unknown;}
  std::vector<unsigned char> toOIDBytes();

private:
  enum { unknown, ipv4, ipv6 } _type;

  union {
    struct in_addr  v4;
    struct in6_addr v6;
  } _addr;
};

#endif

