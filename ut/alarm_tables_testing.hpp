#include <algorithm>
#include <string>
#include "utils.h"
#include "stdlib.h"

class CustomDefs: public SNMPTest
{
public:
  AlarmDef::SeverityDetails cleared;
  AlarmDef::SeverityDetails raised;
  AlarmDef::AlarmDefinition example;
  AlarmTableDef* def_cleared;
  AlarmTableDef* def_raised;
  
  AlarmDef::SeverityDetails cleared1;
  AlarmDef::SeverityDetails raised1;
  AlarmDef::AlarmDefinition example1;
  AlarmTableDef* def1_cleared;
  AlarmTableDef* def1_raised;
  
  AlarmDef::SeverityDetails cleared2;
  AlarmDef::SeverityDetails raised2;
  AlarmDef::AlarmDefinition example2;
  AlarmTableDef* def2_cleared;
  AlarmTableDef* def2_raised;
 
  AlarmDef::SeverityDetails cleared3;
  AlarmDef::SeverityDetails raised3;
  AlarmDef::AlarmDefinition example3;
  AlarmTableDef* def3_cleared;
  AlarmTableDef* def3_raised;
 
  AlarmDef::SeverityDetails cleared4;
  AlarmDef::SeverityDetails raised4;
  AlarmDef::AlarmDefinition example4;
  AlarmTableDef* def4_cleared;
  AlarmTableDef* def4_raised;

  AlarmDef::SeverityDetails cleared5;
  AlarmDef::SeverityDetails raised5;
  AlarmDef::AlarmDefinition example5;
  AlarmTableDef* def5_cleared;
  AlarmTableDef* def5_raised;

  AlarmDef::SeverityDetails cleared6;
  AlarmDef::SeverityDetails raised_critical;
  AlarmDef::SeverityDetails raised_major;
  AlarmDef::AlarmDefinition example6;
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
  cleared._severity = AlarmDef::CLEARED;
  cleared._description = "Test alarm cleared description";
  cleared._details = "Test alarm cleared details";
  raised._severity = AlarmDef::CRITICAL;
  raised._description = "Test alarm raised description";
  raised._details = "Test alarm raised details";
  example._index = 6666;
  example._cause = AlarmDef::SOFTWARE_ERROR;
  example._severity_details = {cleared, raised};
  def_cleared = new AlarmTableDef(example, cleared);
  def_raised = new AlarmTableDef(example, raised);

  cleared1._severity = AlarmDef::CLEARED;
  cleared1._description = "First alarm cleared description";
  cleared1._details = "First alarm cleared details";
  raised1._severity = AlarmDef::CRITICAL;
  raised1._description = "First alarm raised description";
  raised1._details = "First alarm raised details";
  example1._index = 6666;
  example1._cause = AlarmDef::SOFTWARE_ERROR;
  example1._severity_details = {cleared1, raised1};
  def1_cleared = new AlarmTableDef(example1, cleared1);
  def1_raised = new AlarmTableDef(example1, raised1);

  cleared2._severity = AlarmDef::CLEARED;
  cleared2._description = "Second alarm cleared description";
  cleared2._details = "Second alarm cleared details";
  raised2._severity = AlarmDef::CRITICAL;
  raised2._description = "Second alarm raised description";
  raised2._details = "Second alarm raised details";
  example2._index = 6667;
  example2._cause = AlarmDef::SOFTWARE_ERROR;
  example2._severity_details = {cleared2, raised2};
  def2_cleared = new AlarmTableDef(example2, cleared2);
  def2_raised = new AlarmTableDef(example2, raised2);

  cleared3._severity = AlarmDef::CLEARED;
  cleared3._description = "Third alarm cleared description";
  cleared3._details = "Third alarm cleared details";
  raised3._severity = AlarmDef::CRITICAL;
  raised3._description = "Third alarm raised description";
  raised3._details = "Third alarm raised details";
  example3._index = 6668;
  example3._cause = AlarmDef::SOFTWARE_ERROR;
  example3._severity_details = {cleared3, raised3};
  def3_cleared = new AlarmTableDef(example3, cleared3);
  def3_raised = new AlarmTableDef(example3, raised3);

  cleared4._severity = AlarmDef::CLEARED;
  cleared4._description = "Fourth alarm cleared description";
  cleared4._details = "Fourth alarm cleared details";
  raised4._severity = AlarmDef::CRITICAL;
  raised4._description = "Fourth alarm raised description";
  raised4._details = "Fourth alarm raised details";
  example4._index = 6669;
  example4._cause = AlarmDef::SOFTWARE_ERROR;
  example4._severity_details = {cleared4, raised4};
  def4_cleared = new AlarmTableDef(example4, cleared4);
  def4_raised = new AlarmTableDef(example4, raised4);

  cleared5._severity = AlarmDef::CLEARED;
  cleared5._description = "Fifth alarm cleared description";
  cleared5._details = "Fifth alarm cleared details";
  raised5._severity = AlarmDef::CRITICAL;
  raised5._description = "Fifth alarm raised description";
  raised5._details = "Fifth alarm raised details";
  example5._index = 6670;
  example5._cause = AlarmDef::SOFTWARE_ERROR;
  example5._severity_details = {cleared5, raised5};
  def5_cleared = new AlarmTableDef(example5, cleared5);
  def5_raised = new AlarmTableDef(example5, raised5);
  
  cleared6._severity = AlarmDef::CLEARED;
  cleared6._description = "Test alarm cleared description";
  cleared6._details = "Test alarm cleared details";
  raised_critical._severity = AlarmDef::CRITICAL;
  raised_critical._description = "Test alarm critical raised description";
  raised_critical._details = "Test alarm critical raised details";
  raised_major._severity = AlarmDef::MAJOR;
  raised_major._description = "Test alarm major raised description";
  raised_major._details = "Test alarm major raised details";
  example6._index = 666;
  example6._cause = AlarmDef::SOFTWARE_ERROR;
  example6._severity_details = {cleared6, raised_critical, raised_major};
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
  Utils::split_string(snmpwalk_row, '.', index_fields);
  std::vector<std::string> alarm_index;
  // As the alarm's index is immediately followed by a space, we can use this
  // symbol as a delimiter to extra just the alarm's index.
  Utils::split_string(index_fields[25], ' ', alarm_index);
  return alarm_index[0];
}
