/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGScript.cpp
 Author:       Jon S. Berndt
 Date started: 12/21/01
 Purpose:      Loads and runs JSBSim scripts.

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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

This class wraps up the simulation scripting routines.

HISTORY
--------------------------------------------------------------------------------
12/21/01   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_IOSTREAM
#  include STL_ITERATOR
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <iostream.h>
#  else
#    include <iostream>
#  endif
#  include <iterator>
#endif

#include "FGScript.h"
#include "FGConfigFile.h"

static const char *IdSrc = "$Id: FGScript.cpp,v 1.10 2002/09/07 21:54:46 apeden Exp $";
static const char *IdHdr = ID_FGSCRIPT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Constructor

FGScript::FGScript(FGFDMExec* fgex) : FDMExec(fgex)
{
  State = FDMExec->GetState();
  PropertyManager=FDMExec->GetPropertyManager();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGScript::~FGScript()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::LoadScript( string script )
{
  FGConfigFile Script(script);
  string token="";
  string aircraft="";
  string initialize="";
  string prop_name;
  bool result = false;
  double dt = 0.0;
  struct condition *newCondition;

  if (!Script.IsOpen()) return false;

  Script.GetNextConfigLine();
  if (Script.GetValue("runscript").length() <= 0) {
    cerr << "File: " << script << " is not a script file" << endl;
    delete FDMExec;
    return false; 
  }
  ScriptName = Script.GetValue("name");
  Scripted = true;

  if (debug_lvl > 0) cout << "Reading and running from script file " << ScriptName << endl << endl;

  while (Script.GetNextConfigLine() != string("EOF") && Script.GetValue() != string("/runscript")) {
    token = Script.GetValue();
    if (token == "use") {
      if ((token = Script.GetValue("aircraft")) != string("")) {
        aircraft = token;
        result = FDMExec->LoadModel(aircraft);
        if (!result) {
          cerr << "Aircraft file " << aircraft << " was not found" << endl;
          exit(-1);
        }
        if (debug_lvl > 0) cout << "  Use aircraft: " << token << endl;
      } else if ((token = Script.GetValue("initialize")) != string("")) {
        initialize = token;
        if (debug_lvl > 0) cout << "  Use reset file: " << token << endl;
      } else {
        cerr << "Unknown 'use' keyword: \"" << token << "\"" << endl;
      }
    } else if (token == "run") {
      StartTime = strtod(Script.GetValue("start").c_str(), NULL);
      State->Setsim_time(StartTime);
      EndTime   = strtod(Script.GetValue("end").c_str(), NULL);
      dt        = strtod(Script.GetValue("dt").c_str(), NULL);
      State->Setdt(dt);
      Script.GetNextConfigLine();
      token = Script.GetValue();
      while (token != string("/run")) {

        if (token == "when") {
          Script.GetNextConfigLine();
          token = Script.GetValue();
          newCondition = new struct condition();
          while (token != string("/when")) {
            if (token == "parameter") {
              prop_name = State->GetPropertyName( Script.GetValue("name") );
              newCondition->TestParam.push_back( PropertyManager->GetNode(prop_name) );
              newCondition->TestValue.push_back(strtod(Script.GetValue("value").c_str(), NULL));
              newCondition->Comparison.push_back(Script.GetValue("comparison"));
            } else if (token == "set") {
              prop_name = State->GetPropertyName( Script.GetValue("name") );
              newCondition->SetParam.push_back( PropertyManager->GetNode(prop_name) );
              newCondition->SetValue.push_back(strtod(Script.GetValue("value").c_str(), NULL));
              newCondition->Triggered.push_back(false);
              newCondition->OriginalValue.push_back(0.0);
              newCondition->newValue.push_back(0.0);
              newCondition->StartTime.push_back(0.0);
              newCondition->EndTime.push_back(0.0);
              string tempCompare = Script.GetValue("type");
              if      (tempCompare == "FG_DELTA") newCondition->Type.push_back(FG_DELTA);
              else if (tempCompare == "FG_BOOL")  newCondition->Type.push_back(FG_BOOL);
              else if (tempCompare == "FG_VALUE") newCondition->Type.push_back(FG_VALUE);
              else                                newCondition->Type.push_back((eType)0);
              tempCompare = Script.GetValue("action");
              if      (tempCompare == "FG_RAMP") newCondition->Action.push_back(FG_RAMP);
              else if (tempCompare == "FG_STEP") newCondition->Action.push_back(FG_STEP);
              else if (tempCompare == "FG_EXP")  newCondition->Action.push_back(FG_EXP);
              else                               newCondition->Action.push_back((eAction)0);
              
              if (Script.GetValue("persistent") == "true")
                newCondition->Persistent.push_back(true);
              else
                newCondition->Persistent.push_back(false);
		
              newCondition->TC.push_back(strtod(Script.GetValue("tc").c_str(), NULL));
	      
            } else {
              cerr << "Unrecognized keyword in script file: \" [when] " << token << "\"" << endl;
            }
            Script.GetNextConfigLine();
            token = Script.GetValue();
          }
          Conditions.push_back(*newCondition);
          Script.GetNextConfigLine();
          token = Script.GetValue();

        } else {
          cerr << "Error reading script file: expected \"when\", got \"" << token << "\"" << endl;
        }

      }
    } else if (token.empty()) {
     // do nothing
    } else {
      cerr << "Unrecognized keyword in script file: \"" << token << "\" [runscript] " << endl;
    }
  }

  if (aircraft == "") {
    cerr << "Aircraft file not loaded in script" << endl;
    exit(-1);
  }

  Debug(4);


  FGInitialCondition *IC=FDMExec->GetIC();
  if ( ! IC->Load( initialize )) {
    cerr << "Initialization unsuccessful" << endl;
    exit(-1);
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::RunScript(void)
{
  vector <struct condition>::iterator iC = Conditions.begin();
  bool truth = false;
  bool WholeTruth = false;
  unsigned i;

  double currentTime = State->Getsim_time();
  double newSetValue = 0;

  if (currentTime > EndTime) return false;

  while (iC < Conditions.end()) {
    // determine whether the set of conditional tests for this condition equate
    // to true
    for (i=0; i<iC->TestValue.size(); i++) {
           if (iC->Comparison[i] == "lt")
              truth = iC->TestParam[i]->getDoubleValue() <  iC->TestValue[i];
      else if (iC->Comparison[i] == "le")
              truth = iC->TestParam[i]->getDoubleValue() <= iC->TestValue[i];
      else if (iC->Comparison[i] == "eq")
              truth = iC->TestParam[i]->getDoubleValue() == iC->TestValue[i];
      else if (iC->Comparison[i] == "ge")
              truth = iC->TestParam[i]->getDoubleValue() >= iC->TestValue[i];
      else if (iC->Comparison[i] == "gt")
              truth = iC->TestParam[i]->getDoubleValue() >  iC->TestValue[i];
      else if (iC->Comparison[i] == "ne")
              truth = iC->TestParam[i]->getDoubleValue() != iC->TestValue[i];
      else
              cerr << "Bad comparison" << endl;

      if (i == 0) WholeTruth = truth;
      else        WholeTruth = WholeTruth && truth;

      if (!truth && iC->Persistent[i] && iC->Triggered[i]) iC->Triggered[i] = false;
    }

    // if the conditions are true, do the setting of the desired parameters

    if (WholeTruth) {
      for (i=0; i<iC->SetValue.size(); i++) {
        if ( ! iC->Triggered[i]) {
          iC->OriginalValue[i] = iC->SetParam[i]->getDoubleValue();
          switch (iC->Type[i]) {
          case FG_VALUE:
            iC->newValue[i] = iC->SetValue[i];
            break;
          case FG_DELTA:
            iC->newValue[i] = iC->OriginalValue[i] + iC->SetValue[i];
            break;
          case FG_BOOL:
            iC->newValue[i] = iC->SetValue[i];
            break;
          default:
            cerr << "Invalid Type specified" << endl;
            break;
          }
          iC->Triggered[i] = true;
          iC->StartTime[i] = currentTime;
        }

        switch (iC->Action[i]) {
        case FG_RAMP:
        newSetValue = (currentTime - iC->StartTime[i])/(iC->TC[i])
                      * (iC->newValue[i] - iC->OriginalValue[i]) + iC->OriginalValue[i];
          if (newSetValue > iC->newValue[i]) newSetValue = iC->newValue[i];
          break;
        case FG_STEP:
          newSetValue = iC->newValue[i];
          break;
        case FG_EXP:
          newSetValue = (1 - exp(-(currentTime - iC->StartTime[i])/(iC->TC[i])))
              * (iC->newValue[i] - iC->OriginalValue[i]) + iC->OriginalValue[i];
          break;
        default:
          cerr << "Invalid Action specified" << endl;
          break;
        }
        iC->SetParam[i]->setDoubleValue(newSetValue);
      }
    }
    iC++;
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

void FGScript::Debug(int from)
{
  unsigned int i;

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    } else if (from == 3) {
    } else if (from == 4)  { // print out script data
      vector <struct condition>::iterator iterConditions = Conditions.begin();
      int count=0;

      cout << "\n  Script goes from " << StartTime << " to " << EndTime
           << " with dt = " << State->Getdt() << endl << endl;

      while (iterConditions < Conditions.end()) {
        cout << "  Condition: " << count++ << endl;
        cout << "    if (";

        for (i=0; i<iterConditions->TestValue.size(); i++) {
          if (i>0) cout << " and" << endl << "        ";
          cout << "(" << iterConditions->TestParam[i]->GetName()
                      << " " << iterConditions->Comparison[i] << " "
                      << iterConditions->TestValue[i] << ")";
        }
        cout << ") then {";

        for (i=0; i<iterConditions->SetValue.size(); i++) {
          cout << endl << "      set " << iterConditions->SetParam[i]->GetName()
               << " to " << iterConditions->SetValue[i];

          switch (iterConditions->Type[i]) {
          case FG_VALUE:
            cout << " (constant";
            break;
          case FG_DELTA:
            cout << " (delta";
            break;
          case FG_BOOL:
            cout << " (boolean";
            break;
          default:
            cout << " (unspecified type";
          }

          switch (iterConditions->Action[i]) {
          case FG_RAMP:
            cout << " via ramp";
            break;
          case FG_STEP:
            cout << " via step";
            break;
          case FG_EXP:
            cout << " via exponential approach";
            break;
          default:
            cout << " via unspecified action";
          }

          if (!iterConditions->Persistent[i]) cout << endl
                             << "                              once";
          else cout << endl
                             << "                              repeatedly";

          if (iterConditions->Action[i] == FG_RAMP ||
              iterConditions->Action[i] == FG_EXP) cout << endl
                             << "                              with time constant "
                             << iterConditions->TC[i];
        }
        cout << ")" << endl << "    }" << endl << endl;

        iterConditions++;
      }

      cout << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGScript" << endl;
    if (from == 1) cout << "Destroyed:    FGScript" << endl;
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

