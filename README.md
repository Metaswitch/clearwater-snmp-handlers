Project Clearwater is backed by Metaswitch Networks.  We have discontinued active support for this project as of 1st December 2019.  The mailing list archive is available in GitHub.  All of the documentation and source code remains available for the community in GitHub.  Metaswitch’s Clearwater Core product, built on Project Clearwater, remains an active and successful commercial offering.  Please contact clearwater@metaswitch.com for more information.

clearwater-snmp-handlers
========================

Overview
--------

This repository holds the code for SNMP extension modules to provide stats and alarms for Project Clearwater nodes.

Packages
--------

It contains the following packages:

* clearwater-snmp-alarm-agent: alarms for Clearwater nodes
* clearwater-snmp-handler-cdiv: stats for Call Diversion AS nodes
* clearwater-snmp-handler-memento: stats for Memento (HTTP) nodes
* clearwater-snmp-handler-memento-as: stats for Memento (SIP) AS nodes

Building Project Clearwater MIB
-------------------------------

The MIB for Project Clearwater is auto-generated from MIB fragments in `mib-generator`. To build the MIB, run `mib-generator/cw_mib_generator.py` using Python.
