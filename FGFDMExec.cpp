/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFDMExec.cpp
 Author:       Jon S. Berndt
 Date started: 11/17/98
 Purpose:      Schedules and runs the model routines.

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
#  include <simgear/compiler.h>
#  include STL_IOSTREAM
#  include STL_ITERATOR
#else
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <iostream.h>
#  else
#    include <iostream>
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
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"
#include "FGInitialCondition.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGFDMExec.cpp,v 1.107 2004/06/19 09:38:46 ehofman Exp $";
static const char *IdHdr = ID_FDMEXEC;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

unsigned int FGFDMExec::FDMctr = 0;
FGPropertyManager* FGFDMExec::master=0;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void checkTied ( FGPropertyManager *node )
{
  int N = node->nChildren();
  string name;

  for (int i=0; i<N; i++) {
    if (node->getChild(i)->nChildren() ) {
      checkTied( (FGPropertyManager*)node->getChild(i) );
    } else if ( node->getChild(i)->isTied() ) {
      name = ((FGPropertyManager*)node->getChild(i))->GetFullyQualifiedName();
      cerr << name << " is tied" << endl;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Constructor

FGFDMExec::FGFDMExec(FGPropertyManager* root)
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
  Propagate       = 0;
  Auxiliary       = 0;
  Output          = 0;
  IC              = 0;
  Trim            = 0;

  terminate = false;
  frozen = false;
  modelLoaded = false;
  IsSlave = false;

  IdFDM = FDMctr;
  FDMctr++;

  try {
    char* num = getenv("JSBSIM_DEBUG");
    if (num) debug_lvl = atoi(num); // set debug level
  } catch (...) {               // if error set to 1
    debug_lvl = 1;
  }

  if (root == 0)  master= new FGPropertyManager;
  else            master = root;

  instance = master->GetNode("/fdm/jsbsim",IdFDM,true);


  Debug(0);

  // this is here to catch errors in binding member functions
  // to the property tree.
  try {
    Allocate();
  } catch ( string msg ) {
    cout << "Caught error: " << msg << endl;
    exit(1);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFDMExec::~FGFDMExec()
{
  try {
    DeAllocate();
    checkTied( instance );
  } catch ( string msg ) {
    cout << "Caught error: " << msg << endl;
  }

  for (unsigned int i=1; i<SlaveFDMList.size(); i++) delete SlaveFDMList[i]->exec;
  SlaveFDMList.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::Allocate(void)
{
  bool result=true;

  Atmosphere      = new FGAtmosphere(this);
  FCS             = new FGFCS(this);
  Propulsion      = new FGPropulsion(this);
  MassBalance     = new FGMassBalance(this);
  Aerodynamics    = new FGAerodynamics (this);
  Inertial        = new FGInertial(this);
  GroundReactions = new FGGroundReactions(this);
  Aircraft        = new FGAircraft(this);
  Propagate       = new FGPropagate(this);
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
  if (!Propagate->InitModel())   {
    cerr << fgred << "Propagate model init failed" << fgdef << endl;
    Error+=512;}
  if (!Auxiliary->InitModel())  {
    cerr << fgred << "Auxiliary model init failed" << fgdef << endl;
    Error+=2058;}
  if (!Output->InitModel())     {
    cerr << fgred << "Output model init failed" << fgdef << endl;
    Error+=4096;}

  if (Error > 0) result = false;

  IC = new FGInitialCondition(this);

  // Schedule a model. The second arg (the integer) is the pass number. For
  // instance, the atmosphere model gets executed every fifth pass it is called
  // by the executive. Everything else here gets executed each pass.
  // IC and Trim objects are NOT scheduled.

  Schedule(Atmosphere,      1);
  Schedule(FCS,             1);
  Schedule(Propulsion,      1);
  Schedule(MassBalance,     1);
  Schedule(Aerodynamics,    1);
  Schedule(Inertial,        1);
  Schedule(GroundReactions, 1);
  Schedule(Aircraft,        1);
  Schedule(Propagate,       1);
  Schedule(Auxiliary,       1);
  Schedule(Output,          1);

  modelLoaded = false;

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::DeAllocate(void)
{
  delete Atmosphere;
  delete FCS;
  delete Propulsion;
  delete MassBalance;
  delete Aerodynamics;
  delete Inertial;
  delete GroundReactions;
  delete Aircraft;
  delete Propagate;
  delete Auxiliary;
  delete Output;
  delete State;

  delete IC;
  delete Trim;

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
  Propagate       = 0;
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

  Debug(2);

  for (unsigned int i=1; i<SlaveFDMList.size(); i++) {
//    SlaveFDMList[i]->exec->State->Initialize(); // Transfer state to the slave FDM
//    SlaveFDMList[i]->exec->Run();
  }

  while (model_iterator != 0L) {
    model_iterator->Run();
    model_iterator = model_iterator->NextModel;
  }

  frame = Frame++;
  State->IncrTime();
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::RunIC(void)
{
  State->Suspend();
  State->Initialize(IC);
  Run();
  State->Resume();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

vector <string> FGFDMExec::EnumerateFDMs(void)
{
  vector <string> FDMList;

  FDMList.push_back(Aircraft->GetAircraftName());

  for (unsigned int i=1; i<SlaveFDMList.size(); i++) {
    FDMList.push_back(SlaveFDMList[i]->exec->GetAircraft()->GetAircraftName());
  }

  return FDMList;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::LoadModel(string AircraftPath, string EnginePath, string model)
{

  FGFDMExec::AircraftPath = AircraftPath;
  FGFDMExec::EnginePath = EnginePath;

  return LoadModel(model, false);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


bool FGFDMExec::LoadModel(string model, bool addModelToPath)
{
  
  bool result = true;
  string token;
  string aircraftCfgFileName;

  if( AircraftPath.empty() || EnginePath.empty() ) {
    cerr << "Error: attempted to load aircraft with undefined ";
    cerr << "aircraft and engine paths" << endl;
    return false;
  }

  aircraftCfgFileName = AircraftPath;
# ifndef macintosh
  if (addModelToPath) aircraftCfgFileName += "/" + model;
  aircraftCfgFileName += "/" + model + ".xml";
# else
  if (addModelToPath) aircraftCfgFileName += ";"  + model;
  aircraftCfgFileName += ";"  + model + ".xml";
# endif

  FGConfigFile AC_cfg(aircraftCfgFileName);
  if (!AC_cfg.IsOpen()) return false;

  modelName = model;

  if (modelLoaded) {
    DeAllocate();
    Allocate();
  }

  if (!ReadPrologue(&AC_cfg)) return false;

  while ((AC_cfg.GetNextConfigLine() != string("EOF")) &&
         (token = AC_cfg.GetValue()) != string("/FDM_CONFIG")) {
    if (token == "METRICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Metrics" << fgdef << endl;
      if (!ReadMetrics(&AC_cfg)) result = false;
    } else if (token == "SLAVE") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Slave flight vehicle: " << fgdef
                                        << AC_cfg.GetValue("NAME") << endl;
      if (!ReadSlave(&AC_cfg)) result = false;
    } else if (token == "AERODYNAMICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Aerodynamics" << fgdef << endl;
      if (!ReadAerodynamics(&AC_cfg)) result = false;
    } else if (token == "UNDERCARRIAGE") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Landing Gear" << fgdef << endl;
      if (!ReadUndercarriage(&AC_cfg)) result = false;
    } else if (token == "PROPULSION") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Propulsion" << fgdef << endl;
      if (!ReadPropulsion(&AC_cfg)) result = false;
    } else if (token == "FLIGHT_CONTROL") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Flight Control" << fgdef << endl;
      if (!ReadFlightControls(&AC_cfg)) result = false;
    } else if (token == "AUTOPILOT") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Autopilot" << fgdef << endl;
      if (!ReadFlightControls(&AC_cfg)) result = false;
    } else if (token == "OUTPUT") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Output directives" << fgdef << endl;
      if (!ReadOutput(&AC_cfg)) result = false;
    }
  }

  if (result) {
    modelLoaded = true;
    Debug(3);
  } else {
    cerr << fgred
         << "  FGFDMExec: Failed to load aircraft and/or engine model"
         << fgdef << endl;
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadPrologue(FGConfigFile* AC_cfg)
{
  string token = AC_cfg->GetValue();
  string scratch;
  string AircraftName;

  AircraftName = AC_cfg->GetValue("NAME");
  Aircraft->SetAircraftName(AircraftName);

  if (debug_lvl > 0) cout << underon << "Reading Aircraft Configuration File"
            << underoff << ": " << highint << AircraftName << normint << endl;
  scratch = AC_cfg->GetValue("VERSION").c_str();

  CFGVersion = AC_cfg->GetValue("VERSION");
  Release    = AC_cfg->GetValue("RELEASE");

  if (debug_lvl > 0)
    cout << "                            Version: " << highint << CFGVersion
                                                    << normint << endl;
  if (CFGVersion != needed_cfg_version) {
    cerr << endl << fgred << "YOU HAVE AN INCOMPATIBLE CFG FILE FOR THIS AIRCRAFT."
            " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cerr << "Current version needed is: " << needed_cfg_version << endl;
    cerr << "         You have version: " << CFGVersion << endl << fgdef << endl;
    return false;
  }

  if (Release == "ALPHA") {
    system("banner ALPHA");
    cout << endl << endl
         << highint << "This aircraft model is an " << fgred << Release
         << reset << highint << " release!!!" << endl << endl << reset
         << "This aircraft model may not even properly load, and probably"
         << " will not fly as expected." << endl << endl
         << fgred << highint << "Use this model for development purposes ONLY!!!"
         << normint << reset << endl << endl;
  } else if (Release == "BETA") {
    system("banner BETA");
    cout << endl << endl
         << highint << "This aircraft model is a " << fgred << Release
         << reset << highint << " release!!!" << endl << endl << reset
         << "This aircraft model probably will not fly as expected." << endl << endl
         << fgblue << highint << "Use this model for development purposes ONLY!!!"
         << normint << reset << endl << endl;
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadSlave(FGConfigFile* AC_cfg)
{
  // Add a new slaveData object to the slave FDM list
  // Populate that slaveData element with a new FDMExec object
  // Set the IsSlave flag for that FDMExec object
  // Get the aircraft name
  // set debug level to print out no additional data for slave objects
  // Load the model given the aircraft name
  // reset debug level to prior setting

  int saved_debug_lvl = debug_lvl;
  string token;

  SlaveFDMList.push_back(new slaveData);
  SlaveFDMList.back()->exec = new FGFDMExec();
  SlaveFDMList.back()->exec->SetSlave();

  string AircraftName = AC_cfg->GetValue("FILE");

  debug_lvl = 0;                 // turn off debug output for slave vehicle

  SlaveFDMList.back()->exec->SetAircraftPath( AircraftPath );
  SlaveFDMList.back()->exec->SetEnginePath( EnginePath );
  SlaveFDMList.back()->exec->LoadModel(AircraftName);
  debug_lvl = saved_debug_lvl;   // turn debug output back on for master vehicle

  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != string("/SLAVE")) {
    *AC_cfg >> token;
    if      (token == "XLOC")  { *AC_cfg >> SlaveFDMList.back()->x;    }
    else if (token == "YLOC")  { *AC_cfg >> SlaveFDMList.back()->y;    }
    else if (token == "ZLOC")  { *AC_cfg >> SlaveFDMList.back()->z;    }
    else if (token == "PITCH") { *AC_cfg >> SlaveFDMList.back()->pitch;}
    else if (token == "YAW")   { *AC_cfg >> SlaveFDMList.back()->yaw;  }
    else if (token == "ROLL")  { *AC_cfg >> SlaveFDMList.back()->roll;  }
    else cerr << "Unknown identifier: " << token << " in slave vehicle definition" << endl;
  }

  if (debug_lvl > 0)  {
    cout << "      X = " << SlaveFDMList.back()->x << endl;
    cout << "      Y = " << SlaveFDMList.back()->y << endl;
    cout << "      Z = " << SlaveFDMList.back()->z << endl;
    cout << "      Pitch = " << SlaveFDMList.back()->pitch << endl;
    cout << "      Yaw = " << SlaveFDMList.back()->yaw << endl;
    cout << "      Roll = " << SlaveFDMList.back()->roll << endl;
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadPropulsion(FGConfigFile* AC_cfg)
{
  if (!Propulsion->Load(AC_cfg)) {
    cerr << "  Propulsion not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadFlightControls(FGConfigFile* AC_cfg)
{
  if (!FCS->Load(AC_cfg)) {
    cerr << "  Flight Controls not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadAerodynamics(FGConfigFile* AC_cfg)
{
  if (!Aerodynamics->Load(AC_cfg)) {
    cerr << "  Aerodynamics not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadUndercarriage(FGConfigFile* AC_cfg)
{
  if (!GroundReactions->Load(AC_cfg)) {
    cerr << "  Ground Reactions not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadMetrics(FGConfigFile* AC_cfg)
{
  if (!Aircraft->Load(AC_cfg)) {
    cerr << "  Aircraft metrics not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFDMExec::ReadOutput(FGConfigFile* AC_cfg)
{
  if (!Output->Load(AC_cfg)) {
    cerr << "  Output not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyManager* FGFDMExec::GetPropertyManager(void) {
  return instance;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrim* FGFDMExec::GetTrim(void) {
  delete Trim;
  Trim = new FGTrim(this,tNone);
  return Trim;
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

void FGFDMExec::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1 && IdFDM == 0) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n\n     " << highint << underon << "JSBSim Flight Dynamics Model v"
                                     << JSBSim_version << underoff << normint << endl;
      cout << halfint << "            [cfg file spec v" << needed_cfg_version << "]\n\n";
      cout << normint << "JSBSim startup beginning ...\n\n";
    } else if (from == 3) {
      cout << "\n\nJSBSim startup complete\n\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFDMExec" << endl;
    if (from == 1) cout << "Destroyed:    FGFDMExec" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
    if (from == 2) {
      cout << "================== Frame: " << Frame << "  Time: "
           << State->Getsim_time() << endl;
    }
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
}

