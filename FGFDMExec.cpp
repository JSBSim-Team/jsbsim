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
#  if defined(sgi) && !defined(__GNUC__)
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
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"
#include "FGInitialCondition.h"
#include "FGPropertyManager.h"

static const char *IdSrc = "$Id: FGFDMExec.cpp,v 1.81 2002/03/09 11:55:33 apeden Exp $";
static const char *IdHdr = ID_FDMEXEC;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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

  IdFDM = FDMctr;
  FDMctr++;

  try {
    char* num = getenv("JSBSIM_DEBUG");
    if (!num) debug_lvl = 1;
    else debug_lvl = atoi(num); // set debug level
  } catch (...) {               // if error set to 1
    debug_lvl = 1;
  }

  if(master == NULL)
    master = new FGPropertyManager;

  instance = master->GetNode("/fdm/jsbsim",IdFDM,true);  
  
  Debug(0);
  Allocate();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFDMExec::~FGFDMExec()
{
  DeAllocate();

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

  Debug(2);

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
  bool result = true;
  string token;
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

  if (!ReadPrologue(&AC_cfg)) return false;

  while ((AC_cfg.GetNextConfigLine() != string("EOF")) &&
         (token = AC_cfg.GetValue()) != string("/FDM_CONFIG")) {
    if (token == "METRICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Metrics" << fgdef << endl;
      if (!ReadMetrics(&AC_cfg)) result = false;
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

  if (debug_lvl & 1) { // Standard console startup message output
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

