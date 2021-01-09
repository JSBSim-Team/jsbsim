/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGCondition.cpp
 Author:       Jon S. Berndt
 Date started: 1/2/2003

 -------------- Copyright (C) 2003 Jon S. Berndt (jon@jsbsim.org) --------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "FGCondition.h"
#include "FGPropertyValue.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"
#include "FGParameterValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// This constructor is called when tests are inside an element
FGCondition::FGCondition(Element* element, FGPropertyManager* PropertyManager)
  : Logic(elUndef), TestParam1(nullptr), TestParam2(nullptr),
    Comparison(ecUndef)
{
  InitializeConditionals();

  string logic = element->GetAttributeValue("logic");
  if (!logic.empty()) {
    if (logic == "OR") Logic = eOR;
    else if (logic == "AND") Logic = eAND;
    else { // error
      cerr << element->ReadFrom()
           << "Unrecognized LOGIC token " << logic << endl;
      throw std::invalid_argument("FGCondition: unrecognized logic value:'" + logic + "'");
    }
  } else {
    Logic = eAND; // default
  }

  for (unsigned int i=0; i<element->GetNumDataLines(); i++) {
    string data = element->GetDataLine(i);
    conditions.push_back(new FGCondition(data, PropertyManager, element));
  }

  Element* condition_element = element->GetElement();
  const string& elName = element->GetName();

  while (condition_element) {
    string tagName = condition_element->GetName();

    if (tagName != elName) {
      cerr << condition_element->ReadFrom()
           << "Unrecognized tag <" << tagName << "> in the condition statement."
           << endl;
      throw std::invalid_argument("FGCondition: unrecognized tag:'" + tagName + "'");
    }

    conditions.push_back(new FGCondition(condition_element, PropertyManager));
    condition_element = element->GetNextElement();
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This constructor is called when there are no nested test groups inside the
// condition

FGCondition::FGCondition(const string& test, FGPropertyManager* PropertyManager,
                         Element* el)
  : Logic(elUndef), TestParam1(nullptr), TestParam2(nullptr),
    Comparison(ecUndef)
{
  InitializeConditionals();

  vector<string> test_strings = split(test, ' ');

  if (test_strings.size() == 3) {
    TestParam1 = new FGPropertyValue(test_strings[0], PropertyManager);
    conditional = test_strings[1];
    TestParam2 = new FGParameterValue(test_strings[2], PropertyManager);
  } else {
    cerr << el->ReadFrom()
         << "  Conditional test is invalid: \"" << test
         << "\" has " << test_strings.size() << " elements in the "
         << "test condition." << endl;
    throw std::invalid_argument("FGCondition: incorrect number of test elements:" + std::to_string(test_strings.size()));
  }

  Comparison = mComparison[conditional];
  if (Comparison == ecUndef) {
    throw std::invalid_argument("FGCondition: Comparison operator: \""+conditional
          +"\" does not exist.  Please check the conditional.");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCondition::InitializeConditionals(void)
{
  mComparison["EQ"] = eEQ;
  mComparison["NE"] = eNE;
  mComparison["GT"] = eGT;
  mComparison["GE"] = eGE;
  mComparison["LT"] = eLT;
  mComparison["LE"] = eLE;
  mComparison["eq"] = eEQ;
  mComparison["ne"] = eNE;
  mComparison["gt"] = eGT;
  mComparison["ge"] = eGE;
  mComparison["lt"] = eLT;
  mComparison["le"] = eLE;
  mComparison["=="] = eEQ;
  mComparison["!="] = eNE;
  mComparison[">"]  = eGT;
  mComparison[">="] = eGE;
  mComparison["<"]  = eLT;
  mComparison["<="] = eLE;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGCondition::~FGCondition(void)
{
  for (auto cond: conditions) delete cond;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGCondition::Evaluate(void )
{
  bool pass = false;

  if (!TestParam1) {

    if (Logic == eAND) {

      pass = true;
      for (auto cond: conditions) {
        if (!cond->Evaluate()) pass = false;
      }

    } else { // Logic must be eOR

      pass = false;
      for (auto cond: conditions) {
        if (cond->Evaluate()) pass = true;
      }

    }

  } else {

    double compareValue = TestParam2->GetValue();

    switch (Comparison) {
    case ecUndef:
      cerr << "Undefined comparison operator." << endl;
      break;
    case eEQ:
      pass = TestParam1->getDoubleValue() == compareValue;
      break;
    case eNE:
      pass = TestParam1->getDoubleValue() != compareValue;
      break;
    case eGT:
      pass = TestParam1->getDoubleValue() > compareValue;
      break;
    case eGE:
      pass = TestParam1->getDoubleValue() >= compareValue;
      break;
    case eLT:
      pass = TestParam1->getDoubleValue() < compareValue;
      break;
    case eLE:
      pass = TestParam1->getDoubleValue() <= compareValue;
      break;
    default:
     cerr << "Unknown comparison operator." << endl;
    }
  }

  return pass;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCondition::PrintCondition(string indent)
{
  string scratch;

  if (!conditions.empty()) {

    switch(Logic) {
    case (elUndef):
      scratch = " UNSET";
      cerr << "unset logic for test condition" << endl;
      break;
    case (eAND):
      scratch = indent + "if all of the following are true: {";
      break;
    case (eOR):
      scratch = indent + "if any of the following are true: {";
      break;
    default:
      scratch = " UNKNOWN";
      cerr << "Unknown logic for test condition" << endl;
    }
    cout << scratch << endl;

    for (auto cond: conditions) {
      cond->PrintCondition(indent + "  ");
      cout << endl;
    }

    cout << indent << "}";

  } else {
    cout << indent << TestParam1->GetName() << " " << conditional
         << " " << TestParam2->GetName();
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

void FGCondition::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGCondition" << endl;
    if (from == 1) cout << "Destroyed:    FGCondition" << endl;
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
