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

namespace JSBSim {

static const char *IdSrc = "$Id: FGCondition.cpp,v 1.3 2006/08/30 12:04:34 jberndt Exp $";
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
    conditions.push_back(FGCondition(condition_element, PropertyManager));
    condition_element = element->GetNextElement();
  }
  for (int i=0; i<element->GetNumDataLines(); i++) {
    string data = element->GetDataLine(i);
    conditions.push_back(FGCondition(data, PropertyManager));
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//This constructor is called when there are no nested test groups inside the
// condition

FGCondition::FGCondition(string test, FGPropertyManager* PropertyManager) :
  PropertyManager(PropertyManager), isGroup(false)
{
  string property1, property2, compare_string;
  Element* condition_element;

  InitializeConditionals();

  TestParam1  = TestParam2 = 0L;
  TestValue   = 0.0;
  Comparison  = ecUndef;
  Logic       = elUndef;
  conditions.clear();

  int start = 0, end = 0;
  start = test.find_first_not_of(" ");
  end = test.find_first_of(" ", start+1);
  property1 = test.substr(start,end-start);
  start = test.find_first_not_of(" ",end);
  end = test.find_first_of(" ",start+1);
  conditional = test.substr(start,end-start);
  start = test.find_first_not_of(" ",end);
  end = test.find_first_of(" ",start+1);
  property2 = test.substr(start,end-start);

  TestParam1 = PropertyManager->GetNode(property1, true);
  Comparison = mComparison[conditional];
  if (property2.find_first_not_of("-.0123456789eE") == string::npos) {
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
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGCondition::Evaluate(void )
{
  vector <FGCondition>::iterator iConditions;
  bool pass = false;
  double compareValue;

  if (TestParam1 == 0L) {

    if (Logic == eAND) {

      iConditions = conditions.begin();
      pass = true;
      while (iConditions < conditions.end()) {
        if (!iConditions->Evaluate()) pass = false;
        *iConditions++;
      }

    } else { // Logic must be eOR

      pass = false;
      while (iConditions < conditions.end()) {
        if (iConditions->Evaluate()) pass = true;
        *iConditions++;
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
  vector <FGCondition>::iterator iConditions;
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

    iConditions = conditions.begin();
    cout << scratch << endl;
    while (iConditions < conditions.end()) {
      iConditions->PrintCondition();
      *iConditions++;
    }
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

