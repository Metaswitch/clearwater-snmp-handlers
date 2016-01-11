Testing
=======

This document details how to test different components of the SNMP Alarm Agent. 

Unit Testing
------------

To run the SNMP Alarm Agent unit test suite, change to the top-level
clearwater-snmp-handlers directory and issue make test (this will also run the
integration tests outlined below).

SNMP Alarm Agent unit tests use the Google Test framework and are responsible
for testing the following components of the SNMP Alarm Agent:

- The JSON Parser
  - Tests that any errors in the JSON alarm definitions files (for example the
    alarm description being too long) will cause the tables to not initialise
    with an appropriate log error message.
- The alarm req listener
  - Tests that appropriate traps are being sent whenever alarms are cleared or
    raised.

Note that to ensure that we do not call any SNMP commands in unit tests we
include fake SNMP functions with no useful output. These functions are used in
place of real SNMP commands  whenever make test is issued.

The output from the unit test run will look like this:
```
[==========] Running 31 tests from 3 test cases.
[----------] Global test environment set-up.
[----------] 12 tests from AlarmTableDefsTest
[ RUN      ] AlarmTableDefsTest.InitializationNoFiles
[       OK ] AlarmTableDefsTest.InitializationNoFiles (0 ms)
[ RUN      ] AlarmTableDefsTest.InitializationMultiDef
[       OK ] AlarmTableDefsTest.InitializationMultiDef (0 ms)
.
.
.
.
[ RUN      ] AlarmReqListenerZmqErrorTest.CloseSocket
[       OK ] AlarmReqListenerZmqErrorTest.CloseSocket (0 ms)
[ RUN      ] AlarmReqListenerZmqErrorTest.DestroyContext
[       OK ] AlarmReqListenerZmqErrorTest.DestroyContext (0 ms)
[----------] 9 tests from AlarmReqListenerZmqErrorTest (2 ms total)

[----------] Global test environment tear-down
[==========] 31 tests from 3 test cases ran. (316 ms total)
[  PASSED  ] 31 tests.
```

Integration Testing
-------------------

To run the SNMP Alarm Agent integration test suite, change to the top-level
clearwater-snmp-handlers directory and issue make fvtest.

The integration tests also use the Google Test framework and are responsible for
testing the following components of the SNMP Alarm Agent.

- The Alarm Model Table
- The itu Alarm Table
  - Tests that when queried using snmp get commands these tables will output the
    data exactly as it appears in an alarm definition. 
- The Alarm Active Table
  - Tests the logic behind when this table adds a row and deletes a row. For
    example this tests that when two alarms are raised (with the second having a
    different severity) the table deletes the entry for the first alarm and
    creates an entry for the second.

These tests (as opposed to the unit tests) do rely on using SNMP commands. As
such we have included a minimal SNMP Alarm Agent, with snmpget and walk
functions, which are available to the integration testing suite.

The output from the integration test run will look like this:
```
[==========] Running 7 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 7 tests from SNMPTest
[ RUN      ] SNMPTest.ModelTable
[       OK ] SNMPTest.ModelTable (21 ms)
[ RUN      ] SNMPTest.ituAlarmTable
[       OK ] SNMPTest.ituAlarmTable (17 ms)
[ RUN      ] SNMPTest.ActiveTableMultipleAlarms
[       OK ] SNMPTest.ActiveTableMultipleAlarms (12 ms)
[ RUN      ] SNMPTest.ActiveTableRepeatAlarm
[       OK ] SNMPTest.ActiveTableRepeatAlarm (10 ms)
[ RUN      ] SNMPTest.ActiveTableClearingUnraisedAlarm
[       OK ] SNMPTest.ActiveTableClearingUnraisedAlarm (5 ms)
[ RUN      ] SNMPTest.ActiveTableSameAlarmDifferentSeverities
[       OK ] SNMPTest.ActiveTableSameAlarmDifferentSeverities (10 ms)
[ RUN      ] SNMPTest.ActiveTableIndex
[       OK ] SNMPTest.ActiveTableIndex (9 ms)
[----------] 7 tests from SNMPTest (84 ms total)

[----------] Global test environment tear-down
[==========] 7 tests from 1 test case ran. (98 ms total)
[  PASSED  ] 7 tests.
```
