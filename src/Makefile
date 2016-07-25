TARGETS := cw_alarm_agent cdiv_handler.so memento_as_handler.so memento_handler.so astaire_handler.so
TEST_TARGETS := cw_alarm_test cw_alarm_fvtest

CPPFLAGS_TEST += -Imodules/cpp-common/test_utils

AGENT_COMMON_SOURCES := alarmdefinition.cpp \
                        alarm_table_defs.cpp \
                        alarm_active_table.cpp \
                        alarm_model_table.cpp \
                        alarm_req_listener.cpp \
                        alarm_trap_sender.cpp \
                        itu_alarm_table.cpp \
                        alarm_scheduler.cpp \
                        json_alarms.cpp \
                        log.cpp \
                        logger.cpp \
                        timer_heap.cpp \
                        utils.cpp
cw_alarm_agent_SOURCES := alarms_agent.cpp \
                          snmp_agent.cpp \
                          ${AGENT_COMMON_SOURCES}
cw_alarm_test_SOURCES := test_main.cpp \
                         alarm.cpp \
                         alarm_table_defs_test.cpp \
                         alarm_req_listener_test.cpp \
                         alarm_scheduler_test.cpp \
                         test_interposer.cpp \
                         fakenetsnmp.cpp \
                         fakelogger.cpp \
                         fakezmq.cpp \
                         pthread_cond_var_helper.cpp \
                         ${AGENT_COMMON_SOURCES}
cw_alarm_fvtest_SOURCES := test_main.cpp \
                           alarm.cpp \
                           alarm_tables_testing.cpp \
                           pthread_cond_var_helper.cpp \
                           ${AGENT_COMMON_SOURCES}
fvtest: run_cw_alarm_fvtest
AGENT_COMMON_CPPFLAGS := -I../include \
                         -I../modules/cpp-common/include \
                         -I../modules/rapidjson/include
cw_alarm_agent_CPPFLAGS := ${AGENT_COMMON_CPPFLAGS}
cw_alarm_test_CPPFLAGS := ${AGENT_COMMON_CPPFLAGS}
cw_alarm_fvtest_CPPFLAGS := ${AGENT_COMMON_CPPFLAGS}

cw_alarm_test_COVERAGE_EXCLUSIONS := ^modules/rapidjson|^modules/cpp-common/test_utils|^modules/cpp-common/include|^modules/cpp-common/src|alarm_active_table.cpp|alarm_model_table.cpp|itu_alarm_table.cpp
cw_alarm_fvtest_COVERAGE_EXCLUSIONS := ^modules/rapidjson|^modules/cpp-common/test_utils|^modules/cpp-common/include|^modules/cpp-common/src|alarm_req_listener.cpp|alarm_trap_sender.cpp|alarm_trap_sender.hpp|alarm_table_defs.cpp|alarm_scheduler.cpp

AGENT_COMMON_LDFLAGS := -lzmq \
                        -lpthread \
                        -ldl \
                        -lboost_system \
                        -lboost_filesystem \
                        `net-snmp-config --agent-libs`
cw_alarm_agent_LDFLAGS := ${AGENT_COMMON_LDFLAGS}
cw_alarm_test_LDFLAGS := ${AGENT_COMMON_LDFLAGS}
cw_alarm_fvtest_LDFLAGS := ${AGENT_COMMON_LDFLAGS}

PLUGINS_COMMON_SOURCES := custom_handler.cpp oid.cpp oidtree.cpp oid_inet_addr.cpp zmq_listener.cpp zmq_message_handler.cpp
cdiv_handler.so_SOURCES := cdivdata.cpp ${PLUGINS_COMMON_SOURCES}
memento_as_handler.so_SOURCES := mementoasdata.cpp ${PLUGINS_COMMON_SOURCES}
memento_handler.so_SOURCES := mementodata.cpp ${PLUGINS_COMMON_SOURCES}
astaire_handler.so_SOURCES := astairedata.cpp ${PLUGINS_COMMON_SOURCES}

PLUGINS_COMMON_CPPFLAGS := -fPIC -I../include
cdiv_handler.so_CPPFLAGS := ${PLUGINS_COMMON_CPPFLAGS}
memento_as_handler.so_CPPFLAGS := ${PLUGINS_COMMON_CPPFLAGS}
memento_handler.so_CPPFLAGS := ${PLUGINS_COMMON_CPPFLAGS}
astaire_handler.so_CPPFLAGS := ${PLUGINS_COMMON_CPPFLAGS}

PLUGINS_COMMON_LDFLAGS := -lzmq -lpthread `net-snmp-config --agent-libs` -fPIC -shared
cdiv_handler.so_LDFLAGS := ${PLUGINS_COMMON_LDFLAGS}
memento_as_handler.so_LDFLAGS := ${PLUGINS_COMMON_LDFLAGS}
memento_handler.so_LDFLAGS := ${PLUGINS_COMMON_LDFLAGS}
astaire_handler.so_LDFLAGS := ${PLUGINS_COMMON_LDFLAGS}

VPATH := ../modules/cpp-common/src ../modules/cpp-common/test_utils ut

include ../build-infra/cpp.mk

