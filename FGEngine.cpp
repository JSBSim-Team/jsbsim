/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGEngine.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft
 
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
See header file.
 
HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created
09/03/99   JSB   Changed Rocket thrust equation to correct -= Thrust instead of
                 += Thrust (thanks to Tony Peden)
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
#else
#  include <fstream>
#endif

#include "FGEngine.h"
#include "FGTank.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGEngine.cpp,v 1.22 2001/01/04 13:42:31 jsb Exp $";
static const char *IdHdr = "ID_ENGINE";

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGEngine::FGEngine(FGFDMExec* exec) {
 FDMExec      = exec;
  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Propulsion  = FDMExec->GetPropulsion();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

//  EngineNumber = num;
  Thrust = PctPower = 0.0;
  Starved = Flameout = false;
  Running = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This base class Calculate() function should be called from within the
// derived class' Calculate() function before any other calculations are done.
// This base class method removes fuel from the fuel tanks as appropriate,
// and sets the starved flag if necessary.

float FGEngine::Calculate(void) {
  float Fshortage, Oshortage;
  FGTank* Tank;
  
  Fshortage = Oshortage = 0.0;
  for (int i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(i);
    if (Tank->GetType() == FGTank::ttFUEL) {
      Fshortage += Tank->Reduce(CalcFuelNeed()/Propulsion->GetnumSelectedFuelTanks());
    } else {
      Oshortage += Tank->Reduce(CalcOxidizerNeed()/Propulsion->GetnumSelectedOxiTanks());
    }
  }

  if (Fshortage < 0.00 || Oshortage < 0.00) {
    Starved = true;
  } else {
    Starved = false;
  }

  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGEngine::CalcFuelNeed(void) {
  FuelNeed = SLFuelFlowMax*PctPower*State->Getdt()*Propulsion->GetRate();
  return FuelNeed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGEngine::CalcOxidizerNeed(void) {
  OxidizerNeed = SLOxiFlowMax*PctPower*State->Getdt()*Propulsion->GetRate();
  return OxidizerNeed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::SetPlacement(float x, float y, float z, float pitch, float yaw) {
  X = x;
  Y = y;
  Z = z;
  EnginePitch = pitch;
  EngineYaw = yaw;
}
