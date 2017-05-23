/**
 * Copyright (C) Metaswitch Networks 2017
 * If license terms are provided to you in a COPYING file in the root directory
 * of the source code repository by which you are accessing this code, then
 * the license outlined in that COPYING file applies to your use.
 * Otherwise no rights are granted except for those provided to you by
 * Metaswitch Networks in a separate written agreement.
*/

#ifndef ZMQ_MESSAGE_HANDLER_HPP
#define ZMQ_MESSAGE_HANDLER_HPP

#include "oid.hpp"
#include <string>
#include <vector>
#include "oidtree.hpp"

class ZMQMessageHandler
{
public:
  ZMQMessageHandler(OID oid, OIDTree* tree) : _root_oid(oid), _tree(tree) {};
  virtual void handle(std::vector<std::string>) = 0;
protected:
  OID _root_oid;
  OIDTree* _tree;
};

class IPCountStatHandler: public ZMQMessageHandler
{
public:
  IPCountStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string>);
};

class BareStatHandler: public ZMQMessageHandler
{
public:
  BareStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string>);
};

class SingleNumberStatHandler: public ZMQMessageHandler
{
public:
  SingleNumberStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string>);
};

class SingleNumberWithScopeStatHandler: public ZMQMessageHandler
{
public:
  SingleNumberWithScopeStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string>);
};

class AccumulatedWithCountStatHandler: public ZMQMessageHandler
{
public:
  AccumulatedWithCountStatHandler(OID oid, OIDTree* tree) : ZMQMessageHandler(oid, tree) {};
  void handle(std::vector<std::string>);
};

#endif
