/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSwitch.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

The SWITCH component is defined as follows (see the API documentation for more
information):

<COMPONENT NAME="switch1" TYPE="SWITCH">
  <TEST LOGIC="{AND|OR|DEFAULT}" OUTPUT="{property|value}">
    {property} {conditional} {property|value}
    <CONDITION_GROUP LOGIC="{AND|OR}">
      {property} {conditional} {property|value}
      ...
    </CONDITION_GROUP>
    ...
  </TEST>
  <TEST LOGIC="{AND|OR}" OUTPUT="{property|value}">
    {property} {conditional} {property|value}
    ...
  </TEST>
  ...
</COMPONENT>

Also, see the header file (FGSwitch.h) for further details.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGSwitch.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGSwitch.cpp,v 1.4 2006/01/13 05:22:19 jberndt Exp $";
static const char *IdHdr = ID_SWITCH;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGSwitch::FGSwitch(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  string value, logic;
  struct test *current_test;
  Element *test_element, *condition_element;

  test_element = element->GetElement();
  while (test_element) {
    if (test_element->GetName() == "default") {
      tests.push_back(test());
      current_test = &tests.back();
      current_test->Logic = eDefault;
    } else if (test_element->GetName() == "test") { // here's a test
      tests.push_back(test());
      current_test = &tests.back();
      logic = test_element->GetAttributeValue("logic");
      if (logic == "OR") current_test->Logic = eOR;
      else if (logic == "AND") current_test->Logic = eAND;
      else if (logic.size() == 0) current_test->Logic = eAND; // default
      else { // error
        cerr << "Unrecognized LOGIC token " << logic << " in switch component: " << Name << endl;
      }
      for (int i=0; i<test_element->GetNumDataLines(); i++)
        current_test->conditions.push_back(FGCondition(test_element->GetDataLine(i), PropertyManager));

      condition_element = test_element->GetElement(); // retrieve condition groups
      while (condition_element) {
        current_test->conditions.push_back(FGCondition(condition_element, PropertyManager));
        condition_element = test_element->GetNextElement();
      }

    }

    if (test_element->GetName() != "output") { // this is not an output element
      value = test_element->GetAttributeValue("value");
      if (value.empty()) {
        cerr << "No VALUE supplied for switch component: " << Name << endl;
      } else {
        if (value.find_first_not_of("-.0123456789eE") == string::npos) {
          // if true (and execution falls into this block), "value" is a number.
          current_test->OutputVal = atof(value.c_str());
        } else {
          // "value" must be a property if execution passes to here.
          if (value[0] == '-') {
            current_test->sign = -1.0;
            value.erase(0,1);
          } else {
            current_test->sign = 1.0;
          }
          current_test->OutputProp = PropertyManager->GetNode(value);
        }
      }
    }

    test_element = element->GetNextElement();
  }

  FGFCSComponent::bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSwitch::~FGSwitch()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSwitch::Run(void )
{
  vector <test>::iterator iTests = tests.begin();
  vector <FGCondition>::iterator iConditions;
  bool pass = false;

  while (iTests < tests.end()) {
    iConditions = iTests->conditions.begin();

    if (iTests->Logic == eDefault) {
      Output = iTests->GetValue();
    } else if (iTests->Logic == eAND) {
      pass = true;
      while (iConditions < iTests->conditions.end()) {
        if (!iConditions->Evaluate()) pass = false;
        *iConditions++;
      }
    } else if (iTests->Logic == eOR) {
      pass = false;
      while (iConditions < iTests->conditions.end()) {
        if (iConditions->Evaluate()) pass = true;
        *iConditions++;
      }
    } else {
      cerr << "Invalid logic test" << endl;
    }

    if (pass) {
      Output = iTests->GetValue();
      break;
    }
    *iTests++;
  }

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
  vector <test>::iterator iTests = tests.begin();
  vector <FGCondition>::iterator iConditions;
  string comp, scratch;
  string indent = "        ";
  bool first = false;

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      while (iTests < tests.end()) {

        scratch = " if ";

        switch(iTests->Logic) {
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

        if (iTests->OutputProp != 0L)
          if (iTests->sign < 0)
            cout << indent << "Switch VALUE is - " << iTests->OutputProp->GetName() << scratch << endl;
          else
            cout << indent << "Switch VALUE is " << iTests->OutputProp->GetName() << scratch << endl;
        else
          cout << indent << "Switch VALUE is " << iTests->OutputVal << scratch << endl;

        iConditions = iTests->conditions.begin();
        first = true;
        while (iConditions < iTests->conditions.end()) {
          if (!first) cout << indent << comp << " ";
          else cout << indent << " ";
          first = false;
          iConditions->PrintCondition();
          cout << endl;
          *iConditions++;
        }
        cout << endl;
        *iTests++;
      }
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
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

