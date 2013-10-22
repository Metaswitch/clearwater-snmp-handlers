# this should come first so make does the right thing by default
all: deb

sprout_handler.so:
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o sprout_handler.so custom_handler.cpp oid.cpp oidtree.cpp sproutdata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread

bono_handler.so:
	g++ `net-snmp-config --cflags` -Wall -std=c++0x -g -O0 -fPIC -shared -o bono_handler.so custom_handler.cpp oid.cpp oidtree.cpp bonodata.cpp zmq_listener.cpp zmq_message_handler.cpp `net-snmp-config --libs` -lzmq -lpthread
# Makefile for Clearwater infrastructure packages

DEB_COMPONENT := clearwater-snmp-handlers
DEB_MAJOR_VERSION := 1.0
DEB_NAMES := clearwater-snmp-handler-bono clearwater-snmp-handler-sprout
DEB_ARCH := amd64

include build-infra/cw-deb.mk

.PHONY: deb
deb: sprout_handler.so bono_handler.so deb-only

.PHONY: all deb-only deb

