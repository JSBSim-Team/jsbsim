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
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <iostream>
#    include <ctime>
#  else
#    include <iostream.h>
#    include <time.h>
#  endif
#else
#  include <iostream>
#  include <ctime>
#endif

#include "FGFDMExec.h"
#include "FGState.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGPropulsion.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"

static const char *IdSrc = "$Id: FGFDMExec.cpp,v 1.26 2001/02/22 17:25:20 jberndt Exp $";
static const char *IdHdr = "ID_FDMEXEC";

char highint[5]  = {27, '[', '1', 'm', '\0'      };
char halfint[5]  = {27, '[', '2', 'm', '\0'      };
char normint[6]  = {27, '[', '2', '2', 'm', '\0' };
char reset[5]    = {27, '[', '0', 'm', '\0'      };
char underon[5]  = {27, '[', '4', 'm', '\0'      };
char underoff[6] = {27, '[', '2', '4', 'm', '\0' };
char fgblue[6]   = {27, '[', '3', '4', 'm', '\0' };
char fgcyan[6]   = {27, '[', '3', '6', 'm', '\0' };
char fgred[6]    = {27, '[', '3', '1', 'm', '\0' };
char fggreen[6]  = {27, '[', '3', '2', 'm', '\0' };
char fgdef[6]    = {27, '[', '3', '9', 'm', '\0' };

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


// Constructor

FGFDMExec::FGFDMExec(void)
{
  FirstModel  = 0;
  Error       = 0;
  State       = 0;
  Atmosphere  = 0;
  FCS         = 0;
  Propulsion  = 0;
  Aircraft    = 0;
  Translation = 0;
  Rotation    = 0;
  Position    = 0;
  Auxiliary   = 0;
  Output      = 0;

  terminate = false;
  frozen = false;
  modelLoaded = false;

  cout << "\n\n     " << highint << underon << "JSBSim Flight Dynamics Model v"
                                 << JSBSIM_VERSION << underoff << normint << endl;
  cout << halfint << "            [cfg file spec v" << NEEDED_CFG_VERSION << "]\n\n";
  cout << normint << "JSBSim startup beginning ...\n\n";

  Allocate();
}

FGFDMExec::~FGFDMExec(void) {
  DeAllocate();
}

bool FGFDMExec::Allocate(void) {

  bool result=true;

  Atmosphere  = new FGAtmosphere(this);
  FCS         = new FGFCS(this);
  Propulsion  = new FGPropulsion(this);
  Aircraft    = new FGAircraft(this);
  Translation = new FGTranslation(this);
  Rotation    = new FGRotation(this);
  Position    = new FGPosition(this);
  Auxiliary   = new FGAuxiliary(this);
  Output      = new FGOutput(this);

  State       = new FGState(this);

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
  if (!Aircraft->InitModel())   {
    cerr << fgred << "Aircraft model init failed" << fgdef << endl;
    Error+=8;}
  if (!Translation->InitModel()){
    cerr << fgred << "Translation model init failed" << fgdef << endl;
    Error+=16;}
  if (!Rotation->InitModel())   {
    cerr << fgred << "Rotation model init failed" << fgdef << endl;
    Error+=32;}
  if (!Position->InitModel())   {
    cerr << fgred << "Position model init failed" << fgdef << endl;
    Error+=64;}
  if (!Auxiliary->InitModel())  {
    cerr << fgred << "Auxiliary model init failed" << fgdef << endl;
    Error+=128;}
  if (!Output->InitModel())     {
    cerr << fgred << "Output model init failed" << fgdef << endl;
    Error+=256;}

  if (Error > 0) result = false;

  // Schedule a model. The second arg (the integer) is the pass number. For
  // instance, the atmosphere model gets executed every fifth pass it is called
  // by the executive. Everything else here gets executed each pass.

  Schedule(Atmosphere,  1);
  Schedule(FCS,         1);
  Schedule(Propulsion,  1);
  Schedule(Aircraft,    1);
  Schedule(Rotation,    1);
  Schedule(Translation, 1);
  Schedule(Position,    1);
  Schedule(Auxiliary,   1);
  Schedule(Output,     1);

  modelLoaded = false;

  return result;
}

bool FGFDMExec::DeAllocate(void) {

  if ( Atmosphere != 0 )  delete Atmosphere;
  if ( FCS != 0 )         delete FCS;
  if ( Propulsion != 0)   delete Propulsion;
  if ( Aircraft != 0 )    delete Aircraft;
  if ( Translation != 0 ) delete Translation;
  if ( Rotation != 0 )    delete Rotation;
  if ( Position != 0 )    delete Position;
  if ( Auxiliary != 0 )   delete Auxiliary;
  if ( Output != 0 )      delete Output;
  if ( State != 0 )       delete State;

  FirstModel  = 0L;
  Error       = 0;

  State       = 0;
  Atmosphere  = 0;
  FCS         = 0;
  Propulsion  = 0;
  Aircraft    = 0;
  Translation = 0;
  Rotation    = 0;
  Position    = 0;
  Auxiliary   = 0;
  Output      = 0;

  modelLoaded = false;
  return modelLoaded;
}


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


bool FGFDMExec::Run(void)
{
  FGModel* model_iterator;

  if (frozen) return true;

  model_iterator = FirstModel;
  if (model_iterator == 0L) return false;

  while (!model_iterator->Run())
  {
    model_iterator = model_iterator->NextModel;
    if (model_iterator == 0L) break;
  }

  State->IncrTime();

  return true;
}


bool FGFDMExec::RunIC(FGInitialCondition *fgic)
{
  State->Suspend();
  State->Initialize(fgic);
  Run();
  State->Resume();
  return true;
}


bool FGFDMExec::LoadModel(string APath, string EPath, string model)
{
  bool result = false;
  if (modelLoaded) {
    DeAllocate();
    Allocate();
  }
  AircraftPath = APath;
  EnginePath   = EPath;
  result = Aircraft->LoadAircraft(AircraftPath, EnginePath, model);

  if (result) {
    modelLoaded = true;
  } else {
    cerr << fgred
      << "FGFDMExec: Failed to load aircraft and/or engine model"
      << fgdef << endl;
  }

  cout << "\n\nJSBSim startup complete\n\n";
  return result;
}


bool FGFDMExec::RunScript(string script)
{
  FGConfigFile Script(script);
  if (!Script.IsOpen()) return false;

  string token = Script.GetValue();
  string scratch;
  ScriptName = Script.GetValue("NAME");


  return true;
}

