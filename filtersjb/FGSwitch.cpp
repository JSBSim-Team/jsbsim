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

static const char *IdSrc = "$Id: FGSwitch.cpp,v 1.26 2003/06/11 13:39:48 jberndt Exp $";
static const char *IdHdr = ID_SWITCH;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGSwitch::FGSwitch(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                       AC_cfg(AC_cfg)
{
  string token, value;
  struct test *current_test;
  struct FGCondition *current_condition;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");

  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {

    // See the above documentation, or the API docs, for information on what
    // the SWITCH component is supposed to look like in the configuration file.
    // Below, the switch component is read in.

    if (token == "TEST") {
      tests.push_back(*(new test));
      current_test = &tests.back();

      if (AC_cfg->GetValue("LOGIC") == "OR") {
        current_test->Logic = eOR;
      } else if (AC_cfg->GetValue("LOGIC") == "AND") {
        current_test->Logic = eAND;
      } else if (AC_cfg->GetValue("LOGIC") == "DEFAULT") {
        current_test->Logic = eDefault;
      } else { // error
        cerr << "Unrecognized LOGIC token  in switch component: " << Name << endl;
      }
      
      value = AC_cfg->GetValue("VALUE");
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

      AC_cfg->GetNextConfigLine();
      while (AC_cfg->GetValue() != string("/TEST")) {
        current_test->conditions.push_back(*(new FGCondition(AC_cfg, PropertyManager)));
      }
    }
    AC_cfg->GetNextConfigLine();
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

  FGFCSComponent::Run(); // call the base class for initialization of Input

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

