/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSwitch.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

The switch component is defined as follows (see the API documentation for more
information):

@code
<switch name="switch1">
  <default value="{property|value}"/>
  <test logic="{AND|OR}" value="{property|value}">
    {property} {conditional} {property|value}
    <test logic="{AND|OR}">
      {property} {conditional} {property|value}
      ...
    </test>
    ...
  </test>
  <test logic="{AND|OR}" value="{property|value}">
    {property} {conditional} {property|value}
    ...
  </test>
  ...
</switch>
@endcode

Also, see the header file (FGSwitch.h) for further details.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGSwitch.h"
#include "models/FGFCS.h"
#include "math/FGCondition.h"
#include "input_output/FGLog.h"
#include "math/FGRealValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGSwitch::FGSwitch(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  string value;
  unique_ptr<Test> current_test;
  auto PropertyManager = fcs->GetPropertyManager();

  bind(element, PropertyManager.get()); // Bind() this component here in case it is used in its own
                                        // definition for a sample-and-hold
  Element* test_element = element->FindElement("default");
  if (test_element) {
    try {
      current_test = make_unique<Test>();
      value = test_element->GetAttributeValue("value");
      current_test->setTestValue(value, Name, PropertyManager, test_element);
      current_test->Default = true;
      auto output_value = current_test->OutputValue.ptr();
      if (delay > 0 && dynamic_cast<FGRealValue*>(output_value)) { // If there is a delay
        double v = output_value->GetValue();
        for (unsigned int i=0; i<delay-1; i++) {  // Initialize the delay buffer to the default value
          output_array[i] = v;                    // for the switch if that value is a number.
        }
      }
      tests.push_back(current_test.release());
    } catch (const BaseException& e) {
      FGXMLLogging log(fcs->GetExec()->GetLogger(), test_element, LogLevel::ERROR);
      log << e.what() << "\n"
          << "    Default value IGNORED.\n";
    }
  }

  test_element = element->FindElement("test");
  while (test_element) {
    try {
      current_test = make_unique<Test>();
      current_test->condition = make_unique<FGCondition>(test_element, PropertyManager);
      value = test_element->GetAttributeValue("value");
      current_test->setTestValue(value, Name, PropertyManager, test_element);
      tests.push_back(current_test.release());
    } catch (const BaseException& e) {
      FGXMLLogging log(fcs->GetExec()->GetLogger(), test_element, LogLevel::ERROR);
      log << e.what() << "\n"
          << "    Test IGNORED.\n";
    }

    test_element = element->FindNextElement("test");
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSwitch::~FGSwitch()
{
  for (auto test: tests) delete test;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSwitch::Run(void )
{
  bool pass = false;
  double default_output=0.0;

  // To detect errors early, make sure all conditions and values can be
  // evaluated in the first time step.
  if (!initialized) {
    initialized = true;
    VerifyProperties();
  }

  for (auto test: tests) {
    if (test->Default) {
      default_output = test->OutputValue->GetValue();
    } else {
      pass = test->condition->Evaluate();
    }

    if (pass) {
      Output = test->OutputValue->GetValue();
      break;
    }
  }

  if (!pass) Output = default_output;

  if (delay != 0) Delay();
  Clip();
  SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSwitch::VerifyProperties(void)
{
  for (auto test: tests) {
    if (!test->Default) {
      test->condition->Evaluate();
    }
    test->OutputValue->GetValue();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicitly requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGSwitch::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
      unsigned int i = 0;
      for (auto test: tests) {
        if (test->Default) {
          log << "      Switch default value is: " << test->GetOutputName();
        } else {
          log << "      Switch takes test " << i << " value (" << test->GetOutputName() << ")\n";

          test->condition->PrintCondition("      ");
        }
        log << "\n";
        ++i;
      }
      for (auto node: OutputNodes)
        log << "      OUTPUT: " << node->getNameString() << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGSwitch\n";
    if (from == 1) log << "Destroyed:    FGSwitch\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

} //namespace JSBSim
