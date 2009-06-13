/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGCondition.cpp
 Author:       Jon S. Berndt
 Date started: 1/2/2003

 -------------- Copyright (C) 2003 Jon S. Berndt (jsb@hal-pc.org) --------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGCondition.h"
#include <vector>

namespace JSBSim {

static const char *IdSrc = "$Id: FGCondition.cpp,v 1.7 2009/06/13 02:41:58 jberndt Exp $";
static const char *IdHdr = ID_CONDITION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

string FGCondition::indent = "        ";

// This constructor is called when tests are inside an element
FGCondition::FGCondition(Element* element, FGPropertyManager* PropertyManager) :
  PropertyManager(PropertyManager), isGroup(true)
{
  string property1, property2, logic;
  Element* condition_element;

  InitializeConditionals();

  TestParam1  = TestParam2 = 0L;
  TestValue   = 0.0;
  Comparison  = ecUndef;
  Logic       = elUndef;
  conditions.clear();

  logic = element->GetAttributeValue("logic");
  if (!logic.empty()) {
    if (logic == "OR") Logic = eOR;
    else if (logic == "AND") Logic = eAND;
    else { // error
      cerr << "Unrecognized LOGIC token " << logic << endl;
    }
  } else {
    Logic = eAND; // default
  }

  condition_element = element->GetElement();
  while (condition_element) {
    conditions.push_back(new FGCondition(condition_element, PropertyManager));
    condition_element = element->GetNextElement();
  }
  for (unsigned int i=0; i<element->GetNumDataLines(); i++) {
    string data = element->GetDataLine(i);
    conditions.push_back(new FGCondition(data, PropertyManager));
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This constructor is called when there are no nested test groups inside the
// condition

FGCondition::FGCondition(string test, FGPropertyManager* PropertyManager) :
  PropertyManager(PropertyManager), isGroup(false)
{
  string property1, property2, compare_string;
  vector <string> test_strings;

  InitializeConditionals();

  TestParam1  = TestParam2 = 0L;
  TestValue   = 0.0;
  Comparison  = ecUndef;
  Logic       = elUndef;
  conditions.clear();

  test_strings = split(test, ' ');
  if (test_strings.size() == 3) {
    property1 = test_strings[0];
    conditional = test_strings[1];
    property2 = test_strings[2];
  } else {
    cerr << endl << "  Conditional test is invalid: \"" << test
         << "\" has " << test_strings.size() << " elements in the "
         << "test condition." << endl;
    exit(-1);
  }

  TestParam1 = PropertyManager->GetNode(property1, true);
  Comparison = mComparison[conditional];
  if (is_number(property2)) {
    TestValue = atof(property2.c_str());
  } else {
    TestParam2 = PropertyManager->GetNode(property2, true);
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
  for (unsigned int i=0; i<conditions.size(); i++) delete conditions[i];

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGCondition::Evaluate(void )
{
  bool pass = false;
  double compareValue;

  if (TestParam1 == 0L) {

    if (Logic == eAND) {

      pass = true;
      for (unsigned int i=0; i<conditions.size(); i++) {
        if (!conditions[i]->Evaluate()) pass = false;
      }

    } else { // Logic must be eOR

      pass = false;
      for (unsigned int i=0; i<conditions.size(); i++) {
        if (conditions[i]->Evaluate()) pass = true;
      }

    }

  } else {

    if (TestParam2 != 0L) compareValue = TestParam2->getDoubleValue();
    else compareValue = TestValue;

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

void FGCondition::PrintCondition(void )
{
  string scratch;

  if (isGroup) {
    switch(Logic) {
    case (elUndef):
      scratch = " UNSET";
      cerr << "unset logic for test condition" << endl;
      break;
    case (eAND):
      scratch = " if all of the following are true:";
      break;
    case (eOR):
      scratch = " if any of the following are true:";
      break;
    default:
      scratch = " UNKNOWN";
      cerr << "Unknown logic for test condition" << endl;
    }

    cout << scratch << endl;
    for (unsigned int i=0; i<conditions.size(); i++) conditions[i]->PrintCondition();

  } else {
    if (TestParam2 != 0L)
      cout << "    " << TestParam1->GetName() << " " << conditional << " " << TestParam2->GetName();
    else
      cout << "    " << TestParam1->GetName() << " " << conditional << " " << TestValue;
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} //namespace JSBSim

