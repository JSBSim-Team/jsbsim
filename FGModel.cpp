/*******************************************************************************

 Module:       FGModel.cpp
 Author:       Jon Berndt
 Date started: 11/11/98
 Purpose:      Base class for all models
 Called by:    FGSimExec, et. al.

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
This base class for the FGAero, FGRotational, etc. classes defines methods
common to all models.

HISTORY
--------------------------------------------------------------------------------
11/11/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"
#include "FGState.h"
#include "FGFDMExec.h"
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

FGModel::FGModel(FGFDMExec* fdmex)
{
  FDMExec     = fdmex;
  NextModel   = 0L;
  
  State       = 0;
  Atmosphere  = 0;
  FCS         = 0;
  Aircraft    = 0;
  Translation = 0;
  Rotation    = 0;
  Position    = 0;
  Auxiliary   = 0;
  Output      = 0;

  exe_ctr     = 1;
}


FGModel::~FGModel()
{
}


bool FGModel::InitModel(void)
{
  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

  if (!State ||
      !Atmosphere ||
      !FCS ||
      !Aircraft ||
      !Translation ||
      !Rotation ||
      !Position ||
      !Auxiliary ||
      !Output) return(false);
  else return(true);
}


bool FGModel::Run()
{
  if (exe_ctr == 1) {
    if (exe_ctr++ >= rate) exe_ctr = 1;
    return false;
  } else {
    if (exe_ctr++ >= rate) exe_ctr = 1;
    return true;
  }
}


