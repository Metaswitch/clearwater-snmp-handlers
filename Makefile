# Top level Makefile for building cw_alarm_agent

# this should come first so make does the right thing by default
all: build

ROOT ?= ${PWD}
MK_DIR := ${ROOT}/mk
PREFIX ?= ${ROOT}/usr
INSTALL_DIR ?= ${PREFIX}
MODULE_DIR := ${ROOT}/modules

DEB_COMPONENT := clearwater-snmp-handlers
DEB_MAJOR_VERSION := 1.0${DEB_VERSION_QUALIFIER}
DEB_NAMES := clearwater-snmp-handler-cdiv clearwater-snmp-alarm-agent clearwater-snmp-handler-memento-as clearwater-snmp-handler-memento clearwater-snmp-handler-astaire

# Add dependencies to deb-only (target will be added by build-infra)
deb-only: cw_alarm_agent cdiv_handler.so memento_handler.so memento_as_handler.so astaire_handler.so

INCLUDE_DIR := ${INSTALL_DIR}/include
LIB_DIR := ${INSTALL_DIR}/lib

SUBMODULES := 

CW_ALARM_AGENT_DIR := ${ROOT}/src
CW_ALARM_AGENT_TEST_DIR := ${ROOT}/tests

cw_alarm_agent:
	${MAKE} -C ${CW_ALARM_AGENT_DIR}

cw_alarm_agent_test:
	${MAKE} -C ${CW_ALARM_AGENT_DIR} test

cw_alarm_agent_full_test:
	${MAKE} -C ${CW_ALARM_AGENT_DIR} full_test

cw_alarm_agent_clean:
	${MAKE} -C ${CW_ALARM_AGENT_DIR} clean

cw_alarm_agent_distclean: CW_ALARM_AGENT_clean

.PHONY: cw_alarm_agent CW_ALARM_AGENT_test cw_alarm_agent_clean cw_alarm_agent_distclean
build: ${SUBMODULES} cw_alarm_agent

test: ${SUBMODULES} cw_alarm_agent_test

full_test: ${SUBMODULES} cw_alarm_agent_full_test

testall: $(patsubst %, %_test, ${SUBMODULES}) full_test

clean: $(patsubst %, %_clean, ${SUBMODULES}) cw_alarm_agent_clean
	rm -rf ${ROOT}/usr
	rm -rf ${ROOT}/build

distclean: $(patsubst %, %_distclean, ${SUBMODULES}) cw_alarm_agent_distclean
	rm -rf ${ROOT}/usr
	rm -rf ${ROOT}/build

include build-infra/cw-deb.mk

.PHONY: deb
deb: build deb-only

.PHONY: all build test clean distclean

