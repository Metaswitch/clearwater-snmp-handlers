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

ROOT ?= ${PWD}
MK_DIR := ${ROOT}/mk

ALARM_INCLUDES := -I${ROOT}/modules/cpp-common/include

GTEST_DIR := $(ROOT)/modules/gmock/gtest
GMOCK_DIR := $(ROOT)/modules/gmock

TARGET := handler

TARGET_TEST := handler_test

TARGET_SOURCES := alarmdefinition.cpp \
                  alarm_handler.cpp \
                  alarm_table_defs.cpp \
                  alarm_req_listener.cpp \
                  alarm_trap_sender.cpp \
                  alarm_model_table.cpp \
                  itu_alarm_table.cpp 

TARGET_SOURCES_TEST := test_main.cpp \
                       alarm.cpp \
                       log.cpp \
                       logger.cpp \
                       alarm_table_defs_test.cpp \
                       alarm_req_listener_test.cpp \
                       test_interposer.cpp \
                       fakenetsnmp.cpp

TARGET_EXTRA_OBJS_TEST := gmock-all.o \
                          gtest-all.o

CPPFLAGS += -std=c++0x -ggdb3 
CPPFLAGS += -I$(ROOT) \
            -I$(ROOT)/modules/cpp-common/include

CPPFLAGS_TEST += -DUNIT_TEST \
                 -fprofile-arcs -ftest-coverage \
                 -O0 \
                 -fno-access-control \
                 -I$(GTEST_DIR)/include -I$(GMOCK_DIR)/include
CPPFLAGS_TEST += -I$(ROOT)/modules/cpp-common/test_utils

LDFLAGS += -lzmq \
           -lpthread \
           `net-snmp-config --libs` 

#LDFLAGS_TEST += -Wl,-rpath=$(ROOT)/usr/lib

# Add modules/cpp-common/src as a VPATH to pull in required common modules
VPATH := ${ROOT}/modules/cpp-common/src:${ROOT}/modules/cpp-common/test_utils

TEST_XML = $(TEST_OUT_DIR)/test_detail_$(TARGET_TEST).xml

# Now the GMock / GTest boilerplate.
GTEST_HEADERS := $(GTEST_DIR)/include/gtest/*.h \
                 $(GTEST_DIR)/include/gtest/internal/*.h
GMOCK_HEADERS := $(GMOCK_DIR)/include/gmock/*.h \
                 $(GMOCK_DIR)/include/gmock/internal/*.h \
                 $(GTEST_HEADERS)

GTEST_SRCS_ := $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)
GMOCK_SRCS_ := $(GMOCK_DIR)/src/*.cc $(GMOCK_HEADERS)
# End of boilerplate


include ${MK_DIR}/platform.mk

sprout_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o sprout_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp sproutdata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

bono_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o bono_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp bonodata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

homestead_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o homestead_handler.so custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp homesteaddata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

alarm_handler.so: *.cpp *.hpp
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared ${ALARM_INCLUDES} -o alarm_handler.so alarm_handler.cpp modules/cpp-common/src/alarmdefinition.cpp alarm_table_defs.cpp alarm_model_table.cpp alarm_req_listener.cpp alarm_trap_sender.cpp itu_alarm_table.cpp `net-snmp-config --libs` -lzmq -lpthread


# Makefile for Clearwater infrastructure packages

DEB_COMPONENT := clearwater-snmp-handlers
DEB_MAJOR_VERSION := 1.0${DEB_VERSION_QUALIFIER}
DEB_NAMES := clearwater-snmp-handler-bono clearwater-snmp-handler-sprout clearwater-snmp-handler-homestead clearwater-snmp-handler-alarm

include build-infra/cw-deb.mk

.PHONY: deb
deb: sprout_handler.so bono_handler.so homestead_handler.so alarm_handler.so deb-only

.PHONY: all deb-only deb


# Run the test.  You can set EXTRA_TEST_ARGS to pass extra arguments
# to the test, e.g.,
#
#   make EXTRA_TEST_ARGS=--gtest_filter=StatefulProxyTest* run_test
#
# runs just the StatefulProxyTest tests.
#
# Ignore failure here; it will be detected by Jenkins.
.PHONY: run_test
run_test: build_test | $(TEST_OUT_DIR)
	rm -f $(TEST_XML)
	rm -f $(OBJ_DIR_TEST)/*.gcda
	$(TARGET_BIN_TEST) $(EXTRA_TEST_ARGS) --gtest_output=xml:$(TEST_XML)


# Build rules for GMock/GTest library.
$(OBJ_DIR_TEST)/gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GTEST_DIR)/include -I$(GMOCK_DIR) -I$(GMOCK_DIR)/include \
            -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(OBJ_DIR_TEST)/gmock-all.o : $(GMOCK_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GTEST_DIR)/include -I$(GMOCK_DIR) -I$(GMOCK_DIR)/include \
            -c $(GMOCK_DIR)/src/gmock-all.cc -o $@

