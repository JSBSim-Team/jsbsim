/*******************************************************************************

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

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
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
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


// Constructor

FGFDMExec::FGFDMExec(void)
{
  FirstModel  = 0;
  Error       = 0;
  State       = 0;
  Atmosphere  = 0;
  FCS         = 0;
  Aircraft    = 0;
  Translation = 0;
  Rotation    = 0;
  Position    = 0;
  Auxiliary   = 0;
  Output      = 0;

  // Instantiate this FDM Executive's Models

  Atmosphere  = new FGAtmosphere(this);
  FCS         = new FGFCS(this);
  Aircraft    = new FGAircraft(this);
  Translation = new FGTranslation(this);
  Rotation    = new FGRotation(this);
  Position    = new FGPosition(this);
  Auxiliary   = new FGAuxiliary(this);
  Output      = new FGOutput(this);

  State       = new FGState(this);

  // Initialize models so they can communicate with each other

  if (!Atmosphere->InitModel()) {cerr << "Atmosphere model init failed"; Error+=1;}
  if (!FCS->InitModel())        {cerr << "FCS model init failed"; Error+=2;}
  if (!Aircraft->InitModel())   {cerr << "Aircraft model init failed"; Error+=4;}
  if (!Translation->InitModel()){cerr << "Translation model init failed"; Error+=8;}
  if (!Rotation->InitModel())   {cerr << "Rotation model init failed"; Error+=16;}
  if (!Position->InitModel())   {cerr << "Position model init failed"; Error+=32;}
  if (!Auxiliary->InitModel())  {cerr << "Auxiliary model init failed"; Error+=64;}
  if (!Output->InitModel())     {cerr << "Output model init failed"; Error+=128;}

  // Schedule a model. The second arg (the integer) is the pass number. For
  // instance, the atmosphere model gets executed every fifth pass it is called
  // by the executive. Everything else here gets executed each pass.

  Schedule(Atmosphere,  5);
  Schedule(FCS,         1);
  Schedule(Aircraft,    1);
  Schedule(Rotation,    1);
  Schedule(Translation, 1);
  Schedule(Position,    1);
  Schedule(Auxiliary,   1);
  Schedule(Output,     60);

  terminate = false;
  frozen = false;
}


FGFDMExec::~FGFDMExec(void)
{
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
  float save_dt=State->Getdt();
  State->Setdt(0.0);
  State->Initialize(fgic);
  Run();
  State->Setdt(save_dt);
  return true;
}
  

