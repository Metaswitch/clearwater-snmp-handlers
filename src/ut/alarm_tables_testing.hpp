#include <algorithm>
#include <string>
#include "utils.h"
#include "stdlib.h"

class CustomDefs: public SNMPTest
{
public:
  CustomDefs() : SNMPTest("16162") { }
  AlarmTableDef* def_cleared;
  AlarmTableDef* def_raised;
  AlarmTableDef* def1_cleared;
  AlarmTableDef* def1_raised;
  AlarmTableDef* def2_cleared;
  AlarmTableDef* def2_raised;
  AlarmTableDef* def3_cleared;
  AlarmTableDef* def3_raised;
  AlarmTableDef* def4_cleared;
  AlarmTableDef* def4_raised;
  AlarmTableDef* def5_cleared;
  AlarmTableDef* def5_raised;
  AlarmTableDef* def6_cleared;
  AlarmTableDef* def_raised_critical;
  AlarmTableDef* def_raised_major;

  static void SetUpTestCase();

  void SetUp();
  void TearDown();
};

void CustomDefs::SetUpTestCase()
{
  SNMPTest::SetUpTestCase(); 
  initialize_table_alarmModelTable();
  initialize_table_ituAlarmTable();
  init_alarmActiveTable("10.154.153.133");
}

void CustomDefs::SetUp()
{
  AlarmDef::SeverityDetails cleared(AlarmDef::CLEARED,
                                    "Test alarm cleared description",
                                    "Test alarm cleared details",
                                    "Test alarm cleared cause",
                                    "Test alarm cleared effect",
                                    "Test alarm cleared action");
  AlarmDef::SeverityDetails raised(AlarmDef::CRITICAL,
                                   "Test alarm raised description",
                                   "Test alarm raised details",
                                   "Test alarm raised cause",
                                   "Test alarm raised effect",
                                   "Test alarm raised action");
  AlarmDef::AlarmDefinition example(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised});
  def_cleared = new AlarmTableDef(example, cleared);
  def_raised = new AlarmTableDef(example, raised);

  AlarmDef::SeverityDetails cleared1(AlarmDef::CLEARED,
                                     "First alarm cleared description",
                                     "First alarm cleared details",
                                     "First alarm cleared cause",
                                     "First alarm cleared effect",
                                     "First alarm cleared action");
  AlarmDef::SeverityDetails raised1(AlarmDef::CRITICAL,
                                    "First alarm raised description",
                                    "First alarm raised details",
                                    "First alarm raised cause",
                                    "First alarm raised effect",
                                    "First alarm raised action");
  AlarmDef::AlarmDefinition example1(6666, AlarmDef::SOFTWARE_ERROR, {cleared1, raised1});
  def1_cleared = new AlarmTableDef(example1, cleared1);
  def1_raised = new AlarmTableDef(example1, raised1);

  AlarmDef::SeverityDetails cleared2(AlarmDef::CLEARED,
                                     "Second alarm cleared description",
                                     "Second alarm cleared details",
                                     "Second alarm cleared cause",
                                     "Second alarm cleared effect",
                                     "Second alarm cleared action");
  AlarmDef::SeverityDetails raised2(AlarmDef::CRITICAL,
                                    "Second alarm raised description",
                                    "Second alarm raised details",
                                    "Second alarm raised cause",
                                    "Second alarm raised effect",
                                    "Second alarm raised action");
  AlarmDef::AlarmDefinition example2(6667, AlarmDef::SOFTWARE_ERROR, {cleared2, raised2});
  def2_cleared = new AlarmTableDef(example2, cleared2);
  def2_raised = new AlarmTableDef(example2, raised2);

  AlarmDef::SeverityDetails cleared3(AlarmDef::CLEARED,
                                     "Third alarm cleared description",
                                     "Third alarm cleared details",
                                     "Third alarm cleared cause",
                                     "Third alarm cleared effect",
                                     "Third alarm cleared action");
  AlarmDef::SeverityDetails raised3(AlarmDef::CRITICAL,
                                    "Third alarm raised description",
                                    "Third alarm raised details",
                                    "Third alarm raised cause",
                                    "Third alarm raised effect",
                                    "Third alarm raised action");
  AlarmDef::AlarmDefinition example3(6668, AlarmDef::SOFTWARE_ERROR, {cleared3, raised3});
  def3_cleared = new AlarmTableDef(example3, cleared3);
  def3_raised = new AlarmTableDef(example3, raised3);

  AlarmDef::SeverityDetails cleared4(AlarmDef::CLEARED,
                                     "Fourth alarm cleared description",
                                     "Fourth alarm cleared details",
                                     "Fourth alarm cleared cause",
                                     "Fourth alarm cleared effect",
                                     "Fourth alarm cleared action");
  AlarmDef::SeverityDetails raised4(AlarmDef::CRITICAL,
                                    "Fourth alarm raised description",
                                    "Fourth alarm raised details",
                                    "Fourth alarm raised cause",
                                    "Fourth alarm raised effect",
                                    "Fourth alarm raised action");
  AlarmDef::AlarmDefinition example4(6669, AlarmDef::SOFTWARE_ERROR, {cleared4, raised4});
  def4_cleared = new AlarmTableDef(example4, cleared4);
  def4_raised = new AlarmTableDef(example4, raised4);

  AlarmDef::SeverityDetails cleared5(AlarmDef::CLEARED,
                                     "Fifth alarm cleared description",
                                     "Fifth alarm cleared details",
                                     "Fifth alarm cleared cause",
                                     "Fifth alarm cleared effect",
                                     "Fifth alarm cleared action");
  AlarmDef::SeverityDetails raised5(AlarmDef::CRITICAL,
                                    "Fifth alarm raised description",
                                    "Fifth alarm raised details",
                                    "Fifth alarm raised cause",
                                    "Fifth alarm raised effect",
                                    "Fifth alarm raised action");
  AlarmDef::AlarmDefinition example5(6670, AlarmDef::SOFTWARE_ERROR, {cleared5, raised5});
  def5_cleared = new AlarmTableDef(example5, cleared5);
  def5_raised = new AlarmTableDef(example5, raised5);

  AlarmDef::SeverityDetails cleared6(AlarmDef::CLEARED,
                                     "Test alarm cleared description",
                                     "Test alarm cleared details",
                                     "Test alarm cleared cause",
                                     "Test alarm cleared effect",
                                     "Test alarm cleared action");
  AlarmDef::SeverityDetails raised_critical(AlarmDef::CRITICAL,
                                            "Test alarm critical raised description",
                                            "Test alarm critical raised details",
                                            "Test alarm critical raised cause",
                                            "Test alarm critical raised effect",
                                            "Test alarm critical raised action");
  AlarmDef::SeverityDetails raised_major(AlarmDef::MAJOR,
                                         "Test alarm major raised description",
                                         "Test alarm major raised details",
                                         "Test alarm major raised cause",
                                         "Test alarm major raised effect",
                                         "Test alarm major raised action");
  AlarmDef::AlarmDefinition example6(6666, AlarmDef::SOFTWARE_ERROR, {cleared, raised_critical, raised_major});
  def6_cleared = new AlarmTableDef(example6, cleared6);
  def_raised_critical = new AlarmTableDef(example, raised_critical);
  def_raised_major = new AlarmTableDef(example, raised_major);
}

void CustomDefs::TearDown()
{
  alarmActiveTable_trap_handler(*def_cleared);
  alarmActiveTable_trap_handler(*def1_cleared);
  alarmActiveTable_trap_handler(*def2_cleared);
  alarmActiveTable_trap_handler(*def3_cleared);
  alarmActiveTable_trap_handler(*def4_cleared);
  alarmActiveTable_trap_handler(*def5_cleared);
  alarmActiveTable_trap_handler(*def6_cleared);
  delete def_cleared;
  delete def_raised;
  delete def1_cleared;
  delete def1_raised;
  delete def2_cleared;
  delete def2_raised;
  delete def3_cleared;
  delete def3_raised;
  delete def4_cleared;
  delete def4_raised;
  delete def5_cleared;
  delete def5_raised;
  delete def6_cleared;
  delete def_raised_critical;
  delete def_raised_major;
}

std::string get_index_from_row(std::string snmpwalk_row)
{
  std::vector<std::string> index_fields;
  // This splits up each part of the index field for an alarm. The alarm's
  // index is the last element of the index field.
  // For example snmpwalk_row =
  // .1.3.6.1.2.1.118.1.2.2.1.8.0.11.7.224.1.11.11.22.19.0.45.0.0.14 = 0
  // and from this we want the alarm's index (14). First we extract the data
  // after the last decimal point '14 = 0'
  Utils::split_string(snmpwalk_row, '.', index_fields);
  std::vector<std::string> alarm_index;
  // As the alarm's index is immediately followed by a space, we can use this
  // symbol as a delimiter to extract just the alarm's index.
  // So in the above example the 25th chunk of the index field
  // '14 = 0' becomes just '14' the alarm's index.
  Utils::split_string(index_fields[25], ' ', alarm_index);
  return alarm_index[0];
}
