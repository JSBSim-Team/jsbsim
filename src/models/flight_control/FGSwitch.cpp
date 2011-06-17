/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSwitch.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

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
#include <iostream>
#include <cstdlib>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGSwitch.cpp,v 1.22 2011/06/17 12:12:19 jberndt Exp $";
static const char *IdHdr = ID_SWITCH;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGSwitch::FGSwitch(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  string value, logic;
  struct test *current_test;
  Element *test_element, *condition_element;

  FGFCSComponent::bind(); // Bind() this component here in case it is used
                          // in its own definition for a sample-and-hold

  test_element = element->GetElement();
  while (test_element) {
    if (test_element->GetName() == "default") {
      current_test = new struct test;
      current_test->Logic = eDefault;
      tests.push_back(current_test);
    } else if (test_element->GetName() == "test") { // here's a test
      current_test = new struct test;
      logic = test_element->GetAttributeValue("logic");
      if (logic == "OR") current_test->Logic = eOR;
      else if (logic == "AND") current_test->Logic = eAND;
      else if (logic.size() == 0) current_test->Logic = eAND; // default
      else { // error
        cerr << "Unrecognized LOGIC token " << logic << " in switch component: " << Name << endl;
      }
      for (unsigned int i=0; i<test_element->GetNumDataLines(); i++) {
        string input_data = test_element->GetDataLine(i);
        if (input_data.size() <= 1) {
          // Make sure there are no bad data lines that consist solely of whitespace
          cerr << fgred << "  Bad data line in switch component: " << Name << reset << endl;
          continue;
        }
        current_test->conditions.push_back(new FGCondition(input_data, PropertyManager));
      }

      condition_element = test_element->GetElement(); // retrieve condition groups
      while (condition_element) {
        current_test->conditions.push_back(new FGCondition(condition_element, PropertyManager));
        condition_element = test_element->GetNextElement();
      }

      tests.push_back(current_test);
    }

    string el_name = test_element->GetName();
    if (   el_name != "output"
        && el_name != "description"
        && el_name != "delay" )
    {
      value = test_element->GetAttributeValue("value");
      if (value.empty()) {
        cerr << "No VALUE supplied for switch component: " << Name << endl;
      } else {
        if (is_number(value)) {
          current_test->OutputVal = atof(value.c_str());
        } else {
          // "value" must be a property if execution passes to here.
          if (value[0] == '-') {
            current_test->sign = -1.0;
            value.erase(0,1);
          } else {
            current_test->sign = 1.0;
          }
          FGPropertyManager *node = PropertyManager->GetNode(value, false);
          if (node) {
            current_test->OutputProp = new FGPropertyValue(node);
          } else {
            current_test->OutputProp = new FGPropertyValue(value,
                                                           PropertyManager);
          }
        }
      }
    }
    test_element = element->GetNextElement();
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSwitch::~FGSwitch()
{
  for (unsigned int i=0; i<tests.size(); i++) {
    for (unsigned int j=0; j<tests[i]->conditions.size(); j++) delete tests[i]->conditions[j];
    delete tests[i]->OutputProp;
    delete tests[i];
  }

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSwitch::Run(void )
{
  bool pass = false;
  double default_output=0.0;

  for (unsigned int i=0; i<tests.size(); i++) {
    if (tests[i]->Logic == eDefault) {
      default_output = tests[i]->GetValue();
    } else if (tests[i]->Logic == eAND) {
      pass = true;
      for (unsigned int j=0; j<tests[i]->conditions.size(); j++) {
        if (!tests[i]->conditions[j]->Evaluate()) pass = false;
      }
    } else if (tests[i]->Logic == eOR) {
      pass = false;
      for (unsigned int j=0; j<tests[i]->conditions.size(); j++) {
        if (tests[i]->conditions[j]->Evaluate()) pass = true;
      }
    } else {
      cerr << "Invalid logic test" << endl;
    }

    if (pass) {
      Output = tests[i]->GetValue();
      break;
    }
  }
  
  if (!pass) Output = default_output;

  if (delay != 0) Delay();
  Clip();
  if (IsOutput) SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
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
  string comp, scratch;
  string indent = "        ";
  bool first = false;

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      for (unsigned int i=0; i<tests.size(); i++) {

        scratch = " if ";

        switch(tests[i]->Logic) {
        case (elUndef):
          comp = " UNSET ";
          cerr << "Unset logic for test condition" << endl;
          break;
        case (eAND):
          comp = " AND ";
          break;
        case (eOR):
          comp=" OR ";
          break;
        case (eDefault):
          scratch = " by default.";
          break;
        default:
          comp = " UNKNOWN ";
          cerr << "Unknown logic for test condition" << endl;
        }

        if (tests[i]->OutputProp != 0L)
          if (tests[i]->sign < 0)
            cout << indent << "Switch VALUE is - " << tests[i]->OutputProp->GetName() << scratch << endl;
          else
            cout << indent << "Switch VALUE is " << tests[i]->OutputProp->GetName() << scratch << endl;
        else
          cout << indent << "Switch VALUE is " << tests[i]->OutputVal << scratch << endl;

        first = true;
        for (unsigned int j=0; j<tests[i]->conditions.size(); j++) {
          if (!first) cout << indent << comp << " ";
          else cout << indent << " ";
          first = false;
          tests[i]->conditions[j]->PrintCondition();
          cout << endl;
        }
        cout << endl;
      }
      if (IsOutput) {
        for (unsigned int i=0; i<OutputNodes.size(); i++)
          cout << "      OUTPUT: " << OutputNodes[i]->getName() << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGSwitch" << endl;
    if (from == 1) cout << "Destroyed:    FGSwitch" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} //namespace JSBSim

