/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFDMExec.cpp
 Author:       Jon S. Berndt
 Date started: 11/17/98
 Purpose:      Schedules and runs the model routines.
 Called by:    The GUI.

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

This class wraps up the simulation scheduling routines.

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <time.h>
#  include <simgear/compiler.h>
#  include STL_IOSTREAM
#  include STL_ITERATOR
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <iostream.h>
#    include <time.h>
#  else
#    include <iostream>
#    include <ctime>
#  endif
#  include <iterator>
#endif

#include "FGFDMExec.h"
#include "FGState.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGPropulsion.h"
#include "FGMassBalance.h"
#include "FGGroundReactions.h"
#include "FGAerodynamics.h"
#include "FGInertial.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"

static const char *IdSrc = "$Id: FGFDMExec.cpp,v 1.70 2001/12/07 00:45:56 jberndt Exp $";
static const char *IdHdr = ID_FDMEXEC;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

short debug_lvl;  // This describes to any interested entity the debug level
                  // requested by setting the JSBSIM_DEBUG environment variable.
                  // The bitmasked value choices are as follows:
                  // a) unset: In this case (the default) JSBSim would only print
                  //    out the normally expected messages, essentially echoing
                  //    the config files as they are read. If the environment
                  //    variable is not set, debug_lvl is set to 1 internally
                  // b) 0: This requests JSBSim not to output any messages
                  //    whatsoever.
                  // c) 1: This value explicity requests the normal JSBSim
                  //    startup messages
                  // d) 2: This value asks for a message to be printed out when
                  //    a class is instantiated
                  // e) 4: When this value is set, a message is displayed when a
                  //    FGModel object executes its Run() method
                  // f) 8: When this value is set, various runtime state variables
                  //    are printed out periodically
                  // g) 16: When set various parameters are sanity checked and
                  //    a message is printed out when they go out of bounds.

unsigned int FGFDMExec::FDMctr = 0;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Constructor

FGFDMExec::FGFDMExec(void)
{
  Frame           = 0;
  FirstModel      = 0;
  Error           = 0;
  State           = 0;
  Atmosphere      = 0;
  FCS             = 0;
  Propulsion      = 0;
  MassBalance     = 0;
  Aerodynamics    = 0;
  Inertial        = 0;
  GroundReactions = 0;
  Aircraft        = 0;
  Translation     = 0;
  Rotation        = 0;
  Position        = 0;
  Auxiliary       = 0;
  Output          = 0;

  terminate = false;
  frozen = false;
  modelLoaded = false;
  Scripted = false;

  IdFDM = FDMctr;
  FDMctr++;

  try {
    char* num = getenv("JSBSIM_DEBUG");
    if (!num) debug_lvl = 1;
    else debug_lvl = atoi(num); // set debug level
  } catch (...) {               // if error set to 1
    debug_lvl = 1;
  }

  if (debug_lvl > 0) {
    cout << "\n\n     " << highint << underon << "JSBSim Flight Dynamics Model v"
                                   << JSBSim_version << underoff << normint << endl;
    cout << halfint << "            [cfg file spec v" << needed_cfg_version << "]\n\n";
    cout << normint << "JSBSim startup beginning ...\n\n";
  }

  if (debug_lvl & 2) cout << "Instantiated: FGFDMExec" << endl;

  Allocate();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFDMExec::~FGFDMExec() {
  DeAllocate();
  if (debug_lvl & 2) cout << "Destroyed:    FGFDMExec" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::Allocate(void) {

  bool result=true;

  Atmosphere      = new FGAtmosphere(this);
  FCS             = new FGFCS(this);
  Propulsion      = new FGPropulsion(this);
  MassBalance     = new FGMassBalance(this);
  Aerodynamics    = new FGAerodynamics (this);
  Inertial        = new FGInertial(this);
  GroundReactions = new FGGroundReactions(this);
  Aircraft        = new FGAircraft(this);
  Translation     = new FGTranslation(this);
  Rotation        = new FGRotation(this);
  Position        = new FGPosition(this);
  Auxiliary       = new FGAuxiliary(this);
  Output          = new FGOutput(this);

  State        = new FGState(this); // This must be done here, as the FGState
                                    // class needs valid pointers to the above
                                    // model classes
  
  // Initialize models so they can communicate with each other

  if (!Atmosphere->InitModel()) {
    cerr << fgred << "Atmosphere model init failed" << fgdef << endl;
    Error+=1;}
  if (!FCS->InitModel())        {
    cerr << fgred << "FCS model init failed" << fgdef << endl;
    Error+=2;}
  if (!Propulsion->InitModel()) {
    cerr << fgred << "FGPropulsion model init failed" << fgdef << endl;
    Error+=4;}
  if (!MassBalance->InitModel()) {
    cerr << fgred << "FGMassBalance model init failed" << fgdef << endl;
    Error+=8;}
  if (!Aerodynamics->InitModel()) {
    cerr << fgred << "FGAerodynamics model init failed" << fgdef << endl;
    Error+=16;}
  if (!Inertial->InitModel()) {
    cerr << fgred << "FGInertial model init failed" << fgdef << endl;
    Error+=32;}
  if (!GroundReactions->InitModel())   {
    cerr << fgred << "Ground Reactions model init failed" << fgdef << endl;
    Error+=64;}
  if (!Aircraft->InitModel())   {
    cerr << fgred << "Aircraft model init failed" << fgdef << endl;
    Error+=128;}
  if (!Translation->InitModel()){
    cerr << fgred << "Translation model init failed" << fgdef << endl;
    Error+=256;}
  if (!Rotation->InitModel())   {
    cerr << fgred << "Rotation model init failed" << fgdef << endl;
    Error+=512;}
  if (!Position->InitModel())   {
    cerr << fgred << "Position model init failed" << fgdef << endl;
    Error+=1024;}
  if (!Auxiliary->InitModel())  {
    cerr << fgred << "Auxiliary model init failed" << fgdef << endl;
    Error+=2058;}
  if (!Output->InitModel())     {
    cerr << fgred << "Output model init failed" << fgdef << endl;
    Error+=4096;}

  if (Error > 0) result = false;

  // Schedule a model. The second arg (the integer) is the pass number. For
  // instance, the atmosphere model gets executed every fifth pass it is called
  // by the executive. Everything else here gets executed each pass.

  Schedule(Atmosphere,      1);
  Schedule(FCS,             1);
  Schedule(Propulsion,      1);
  Schedule(MassBalance,     1);
  Schedule(Aerodynamics,    1);
  Schedule(Inertial,        1);
  Schedule(GroundReactions, 1);
  Schedule(Aircraft,        1);
  Schedule(Rotation,        1);
  Schedule(Translation,     1);
  Schedule(Position,        1);
  Schedule(Auxiliary,       1);
  Schedule(Output,          1);

  modelLoaded = false;

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::DeAllocate(void) {

  if ( Atmosphere != 0 )     delete Atmosphere;
  if ( FCS != 0 )            delete FCS;
  if ( Propulsion != 0)      delete Propulsion;
  if ( MassBalance != 0)     delete MassBalance;
  if ( Aerodynamics != 0)    delete Aerodynamics;
  if ( Inertial != 0)        delete Inertial;
  if ( GroundReactions != 0) delete GroundReactions;
  if ( Aircraft != 0 )       delete Aircraft;
  if ( Translation != 0 )    delete Translation;
  if ( Rotation != 0 )       delete Rotation;
  if ( Position != 0 )       delete Position;
  if ( Auxiliary != 0 )      delete Auxiliary;
  if ( Output != 0 )         delete Output;
  if ( State != 0 )          delete State;

  FirstModel  = 0L;
  Error       = 0;

  State           = 0;
  Atmosphere      = 0;
  FCS             = 0;
  Propulsion      = 0;
  MassBalance     = 0;
  Aerodynamics    = 0;
  Inertial        = 0;
  GroundReactions = 0;
  Aircraft        = 0;
  Translation     = 0;
  Rotation        = 0;
  Position        = 0;
  Auxiliary       = 0;
  Output          = 0;

  modelLoaded = false;
  return modelLoaded;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGFDMExec::Schedule(FGModel* model, int rate)
{
  FGModel* model_iterator;

  model_iterator = FirstModel;

  if (model_iterator == 0L) {                  // this is the first model

    FirstModel = model;
    FirstModel->NextModel = 0L;
    FirstModel->SetRate(rate);

  } else {                                     // subsequent model

    while (model_iterator->NextModel != 0L) {
      model_iterator = model_iterator->NextModel;
    }
    model_iterator->NextModel = model;
    model_iterator->NextModel->SetRate(rate);

  }
  
  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::Run(void)
{
  FGModel* model_iterator;

  if (frozen) return true;

  model_iterator = FirstModel;
  if (model_iterator == 0L) return false;

  if (Scripted) {
    RunScript();
    if (State->Getsim_time() >= EndTime) return false;
  }

  if (debug_lvl & 4)
    cout << "================== Frame: " << Frame << "  Time: "
         << State->Getsim_time() << endl;

  while (!model_iterator->Run()) {
    model_iterator = model_iterator->NextModel;
    if (model_iterator == 0L) break;
  }

  frame = Frame++;
  State->IncrTime();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::RunIC(FGInitialCondition *fgic)
{
  State->Suspend();
  State->Initialize(fgic);
  Run();
  State->Resume();
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadModel(string APath, string EPath, string model)
{
  bool result = false;

  string aircraftCfgFileName;

  AircraftPath = APath;
  EnginePath   = EPath;

# ifndef macintosh
  aircraftCfgFileName = AircraftPath + "/" + model + "/" + model + ".xml";
# else
  aircraftCfgFileName = AircraftPath + ";" + model + ";" + model + ".xml";
# endif

  FGConfigFile AC_cfg(aircraftCfgFileName);
  if (!AC_cfg.IsOpen()) return false;

  if (modelLoaded) {
    DeAllocate();
    Allocate();
  }

  result = Aircraft->Load(&AC_cfg);

  if (result) {
    modelLoaded = true;
    if (debug_lvl > 0) cout << "\n\nJSBSim startup complete\n\n";
  } else {
    cerr << fgred
         << "  FGFDMExec: Failed to load aircraft and/or engine model"
         << fgdef << endl;
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadScript(string script)
{
  FGConfigFile Script(script);
  string token="";
  string aircraft="";
  string initialize="";
  bool result=false;
  double dt=0.0;
  unsigned i;
  struct condition *newCondition;

  if (!Script.IsOpen()) return false;

  Script.GetNextConfigLine();
  ScriptName = Script.GetValue("name");
  Scripted = true;
  if (debug_lvl > 0) cout << "Reading Script File " << ScriptName << endl;

  while (Script.GetNextConfigLine() != string("EOF") && Script.GetValue() != string("/runscript")) {
    token = Script.GetValue();
    if (token == "use") {
      if ((token = Script.GetValue("aircraft")) != string("")) {
        aircraft = token;
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
              newCondition->TestParam.push_back(State->GetParameterIndex(Script.GetValue("name")));
              newCondition->TestValue.push_back(strtod(Script.GetValue("value").c_str(), NULL));
              newCondition->Comparison.push_back(Script.GetValue("comparison"));
            } else if (token == "set") {
              newCondition->SetParam.push_back(State->GetParameterIndex(Script.GetValue("name")));
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
    } else {
      cerr << "Unrecognized keyword in script file: \"" << token << "\" [runscript] " << endl;
    }
  }

  if (aircraft == "") {
    cerr << "Aircraft file not loaded in script" << endl;
    exit(-1);
  }

  // print out conditions for double-checking if requested

  if (debug_lvl > 0) {
    vector <struct condition>::iterator iterConditions = Conditions.begin();
    int count=0;

    cout << "\n  Script goes from " << StartTime << " to " << EndTime
         << " with dt = " << dt << endl << endl;

    while (iterConditions < Conditions.end()) {
      cout << "  Condition: " << count++ << endl;
      cout << "    if (";

      for (i=0; i<iterConditions->TestValue.size(); i++) {
        if (i>0) cout << " and" << endl << "        ";
        cout << "(" << State->paramdef[iterConditions->TestParam[i]]
                    << iterConditions->Comparison[i] << " "
                    << iterConditions->TestValue[i] << ")";
      }
      cout << ") then {";

      for (i=0; i<iterConditions->SetValue.size(); i++) {
        cout << endl << "      set" << State->paramdef[iterConditions->SetParam[i]]
             << "to " << iterConditions->SetValue[i];

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

  result = LoadModel("aircraft", "engine", aircraft);
  if (!result) {
    cerr << "Aircraft file " << aircraft << " was not found" << endl;
	  exit(-1);
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::RunScript(void)
{
  vector <struct condition>::iterator iC = Conditions.begin();
  bool truth = false;
  bool WholeTruth = false;
  unsigned i;

  double currentTime = State->Getsim_time();
  double newSetValue = 0;

  while (iC < Conditions.end()) {
    // determine whether the set of conditional tests for this condition equate
    // to true
    for (i=0; i<iC->TestValue.size(); i++) {
           if (iC->Comparison[i] == "lt")
              truth = State->GetParameter(iC->TestParam[i]) <  iC->TestValue[i];
      else if (iC->Comparison[i] == "le")
              truth = State->GetParameter(iC->TestParam[i]) <= iC->TestValue[i];
      else if (iC->Comparison[i] == "eq")
              truth = State->GetParameter(iC->TestParam[i]) == iC->TestValue[i];
      else if (iC->Comparison[i] == "ge")
              truth = State->GetParameter(iC->TestParam[i]) >= iC->TestValue[i];
      else if (iC->Comparison[i] == "gt")
              truth = State->GetParameter(iC->TestParam[i]) >  iC->TestValue[i];
      else if (iC->Comparison[i] == "ne")
              truth = State->GetParameter(iC->TestParam[i]) != iC->TestValue[i];
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
          iC->OriginalValue[i] = State->GetParameter(iC->SetParam[i]);
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
        State->SetParameter(iC->SetParam[i], newSetValue);
      }
    }
    iC++;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFDMExec::Debug(void)
{
    //TODO: Add your source code here
}

