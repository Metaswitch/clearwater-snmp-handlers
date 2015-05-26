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
                       fakenetsnmp.cpp \
                       fakezmq.cpp

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
COVERAGE_XML = $(TEST_OUT_DIR)/coverage_$(TARGET_TEST).xml
COVERAGE_LIST_TMP = $(TEST_OUT_DIR)/coverage_list_tmp
COVERAGE_LIST = $(TEST_OUT_DIR)/coverage_list
COVERAGE_MASTER_LIST = ut/coverage-not-yet
VG_XML = $(TEST_OUT_DIR)/vg_$(TARGET_TEST).memcheck
VG_OUT = $(TEST_OUT_DIR)/vg_$(TARGET_TEST).txt
VG_LIST = $(TEST_OUT_DIR)/vg_$(TARGET_TEST)_list
VG_SUPPRESS = $(TARGET_TEST).supp

COVERAGEFLAGS = $(OBJ_DIR_TEST) --object-directory=$(shell pwd) --root=${ROOT} \
                --exclude='(^modules/gmock/|^modules/cpp-common/include/|^modules/cpp-common/test_utils/|^ut/)' \
                --sort-percentage

EXTRA_CLEANS += *.o *.so *.d \
                $(TEST_XML) \
                $(COVERAGE_XML) \
                $(VG_XML) $(VG_OUT) \
                $(OBJ_DIR_TEST)/*.gcno \
                $(OBJ_DIR_TEST)/*.gcda \
                *.gcov

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

-include *.d

%.o : %.cpp
	g++ `net-snmp-config --cflags` -Wall ${CPPFLAGS} -fPIC -MMD -c -o $@ $<

COMMON_OBJECTS := custom_handler.o oid.o oidtree.o oid_inet_addr.o zmq_listener.o zmq_message_handler.o

sprout_handler.so: sproutdata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

bono_handler.so: bonodata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

homestead_handler.so: homesteaddata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

cdiv_handler.so: cdivdata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

alarm_handler.so: alarm_handler.o alarmdefinition.o alarm_table_defs.o alarm_model_table.o alarm_req_listener.o alarm_trap_sender.o itu_alarm_table.o
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

memento_as_handler.so: mementoasdata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

memento_handler.so: mementodata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

astaire_handler.so: astairedata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

chronos_handler.so: chronosdata.o ${COMMON_OBJECTS}
	g++ -o $@ $^ ${LDFLAGS} -fPIC -shared

# Makefile for Clearwater infrastructure packages

DEB_COMPONENT := clearwater-snmp-handlers
DEB_MAJOR_VERSION := 1.0${DEB_VERSION_QUALIFIER}
DEB_NAMES := clearwater-snmp-handler-bono clearwater-snmp-handler-sprout clearwater-snmp-handler-homestead clearwater-snmp-handler-cdiv clearwater-snmp-handler-alarm clearwater-snmp-handler-memento-as clearwater-snmp-handler-memento clearwater-snmp-handler-astaire clearwater-snmp-handler-chronos

# Add dependencies to deb-only (target will be added by build-infra)
deb-only: sprout_handler.so bono_handler.so homestead_handler.so cdiv_handler.so alarm_handler.so memento_handler.so memento_as_handler.so astaire_handler.so chronos_handler.so

include build-infra/cw-deb.mk

.PHONY: deb
deb: deb-only

.PHONY: test
test: run_test coverage vg coverage-check vg-check

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
	(mkdir -p /var/run/clearwater; if [ $$? -ne 0 ]; then ( printf "\nFor tests to run, as root do the following:\n  $$ mkdir -p /var/run/clearwater\n  $$ chmod -R o+wr /var/run/clearwater\n" ); exit 1; fi)
	rm -f $(TEST_XML)
	rm -f $(OBJ_DIR_TEST)/*.gcda
	$(TARGET_BIN_TEST) $(EXTRA_TEST_ARGS) --gtest_output=xml:$(TEST_XML)

.PHONY: coverage
coverage: | run_test
	$(GCOVR) $(COVERAGEFLAGS) --xml > $(COVERAGE_XML)

# Check that we have 100% coverage of all files except those that we
# have declared we're being relaxed on.  In particular, all new files
# must have 100% coverage or be added to $(COVERAGE_MASTER_LIST).
# The string "Marking build unstable" is recognised by the CI scripts
# and if it is found the build is marked unstable.
.PHONY: coverage-check
coverage-check: | coverage
	@xmllint --xpath '//class[@line-rate!="1.0"]/@filename' $(COVERAGE_XML) \
                | tr ' ' '\n' \
                | grep filename= \
                | cut -d\" -f2 \
                | sort > $(COVERAGE_LIST_TMP)
	@sort $(COVERAGE_MASTER_LIST) | comm -23 $(COVERAGE_LIST_TMP) - > $(COVERAGE_LIST)
	@if grep -q ^ $(COVERAGE_LIST) ; then \
                echo "Error: some files unexpectedly have less than 100% code coverage:" ; \
                cat $(COVERAGE_LIST) ; \
                /bin/false ; \
                echo "Marking build unstable." ; \
        fi

# Get quick coverage data at the command line. Add --branches to get branch info
# instead of line info in report.  *.gcov files generated in current directory
# if you need to see full detail.
.PHONY: coverage_raw
coverage_raw: | run_test
	$(GCOVR) $(COVERAGEFLAGS) --keep

.PHONY: debug
debug: | build_test
	gdb --args $(TARGET_BIN_TEST) $(EXTRA_TEST_ARGS)

# Don't run VG against death tests; they don't play nicely.
# Be aware that running this will count towards coverage.
# Don't send output to console, or it might be confused with the full
# unit-test run earlier.
# Test failure should not lead to build failure - instead we observe
# test failure from Jenkins.
.PHONY: vg
vg: | build_test
	-valgrind --xml=yes --xml-file=$(VG_XML) $(VGFLAGS) \
          $(TARGET_BIN_TEST) --gtest_filter='-*DeathTest*' $(EXTRA_TEST_ARGS) > $(VG_OUT) 2>&1

# Check whether there were any errors from valgrind. Output to screen any errors found,
# and details of where to find the full logs.
# The output file will contain <error><kind>ERROR</kind></error>, or 'XPath set is empty'
# if there are no errors.
.PHONY: vg-check
vg-check: | vg
	@xmllint --xpath '//error/kind' $(VG_XML) 2>&1 | \
                sed -e 's#<kind>##g' | \
                sed -e 's#</kind>#\n#g' | \
                sort > $(VG_LIST)
	@if grep -q -v "XPath set is empty" $(VG_LIST) ; then \
                echo "Error: some memory errors have been detected" ; \
                cat $(VG_LIST) ; \
                echo "See $(VG_XML) for further details." ; \
        fi

.PHONY: vg_raw
vg_raw: | build_test
	-valgrind --gen-suppressions=all --show-reachable=yes $(VGFLAGS) \
          $(TARGET_BIN_TEST) --gtest_filter='-*DeathTest*' $(EXTRA_TEST_ARGS)


# Build rules for GMock/GTest library.
$(OBJ_DIR_TEST)/gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GTEST_DIR)/include -I$(GMOCK_DIR) -I$(GMOCK_DIR)/include \
            -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(OBJ_DIR_TEST)/gmock-all.o : $(GMOCK_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GTEST_DIR)/include -I$(GMOCK_DIR) -I$(GMOCK_DIR)/include \
            -c $(GMOCK_DIR)/src/gmock-all.cc -o $@

