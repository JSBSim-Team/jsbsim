/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGCondition.cpp
 Author:       Jon S. Berndt
 Date started: 1/2/2003

 -------------- Copyright (C) 2003 Jon S. Berndt (jsb@hal-pc.org) --------------

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

static const char *IdSrc = "$Id: FGCondition.cpp,v 1.13 2005/01/20 12:45:00 jberndt Exp $";
static const char *IdHdr = ID_CONDITION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

string FGCondition::indent = "        ";


FGCondition::FGCondition(FGConfigFile* AC_cfg, FGPropertyManager* PropertyManager) :
  PropertyManager(PropertyManager)
{
  mComparison["EQ"] = eEQ;
  mComparison["NE"] = eNE;
  mComparison["GT"] = eGT;
  mComparison["GE"] = eGE;
  mComparison["LT"] = eLT;
  mComparison["LE"] = eLE;
  mComparison["=="] = eEQ;
  mComparison["!="] = eNE;
  mComparison[">"]  = eGT;
  mComparison[">="] = eGE;
  mComparison["<"]  = eLT;
  mComparison["<="] = eLE;

  TestParam1  = TestParam2 = 0L;
  TestValue   = 0.0;
  Comparison  = ecUndef;
  Logic       = elUndef;
  conditions.clear();

  if (AC_cfg->GetValue("CONDITION_GROUP").empty()) {  // define a condition

    *AC_cfg >> property1 >> conditional >> property2;
    TestParam1 = PropertyManager->GetNode(property1, true);
    Comparison = mComparison[conditional];

    if (property2.find_first_not_of("-.0123456789eE") == string::npos) {
      TestValue = atof(property2.c_str());
    } else {
      TestParam2 = PropertyManager->GetNode(property2, true);
    }

    isGroup = false;

  } else { // define a condition group

    if (AC_cfg->GetValue("LOGIC") == "OR")       Logic = eOR;
    else if (AC_cfg->GetValue("LOGIC") == "AND") Logic = eAND;

    AC_cfg->GetNextConfigLine();
    while (AC_cfg->GetValue() != string("/CONDITION_GROUP")) {
      conditions.push_back(FGCondition(AC_cfg, PropertyManager));
    }
    isGroup = true;
    AC_cfg->GetNextConfigLine();
  }

  Debug(0);
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

  if (Logic == eAND) {

    iConditions = conditions.begin();
    pass = true;
    while (iConditions < conditions.end()) {
      if (!iConditions->Evaluate()) pass = false;
      *iConditions++;
    }

  } else if (Logic == eOR) {

    pass = false;
    while (iConditions < conditions.end()) {
      if (iConditions->Evaluate()) pass = true;
      *iConditions++;
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
      scratch = " if all of the following are true";
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
      cout << TestParam1->GetName() << " " << conditional << " " << TestParam2->GetName();
    else
      cout << TestParam1->GetName() << " " << conditional << " " << TestValue;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCondition::convert(void)
{
  if (conditions.empty())
    cout << "                " << property1 << " " << conditional << " " << property2 << endl;
  else
    for (int i; i<conditions.size(); i++) conditions[i].convert();
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

