#
# Project Clearwater - IMS in the Cloud
# Copyright (C) 2013 Metaswitch Networks Ltd
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version, along with the "Special Exception" for use of
# the program along with SSL, set forth below. This program is distributed
# in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more
# details. You should have received a copy of the GNU General Public
# License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
#
# The author can be reached by email at clearwater@metaswitch.com or by
# post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
#
# Special Exception
# Metaswitch Networks Ltd grants you permission to copy, modify,
# propagate, and distribute a work formed by combining OpenSSL with The
# Software, or a work derivative of such a combination, even if such
# copying, modification, propagation, or distribution would otherwise
# violate the terms of the GPL. You must comply with the GPL in all
# respects for all of the code used other than OpenSSL.
# "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
# Project and licensed under the OpenSSL Licenses, or a work based on such
# software and licensed under the OpenSSL Licenses.
# "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
# under which the OpenSSL Project distributes the OpenSSL toolkit software,
# as those licenses appear in the file LICENSE-OPENSSL.


# this should come first so make does the right thing by default
all: deb

sprout_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o sprout_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp sproutdata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

bono_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o bono_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp bonodata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

homestead_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o homestead_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp homesteaddata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

# Makefile for Clearwater infrastructure packages

DEB_COMPONENT := clearwater-snmp-handlers
DEB_MAJOR_VERSION := 1.0${DEB_VERSION_QUALIFIER}
DEB_NAMES := clearwater-snmp-handler-bono clearwater-snmp-handler-sprout clearwater-snmp-handler-homestead

include build-infra/cw-deb.mk

.PHONY: deb
deb: sprout_handler.so bono_handler.so homestead_handler.so deb-only

.PHONY: all deb-only deb

