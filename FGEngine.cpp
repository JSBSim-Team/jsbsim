/*******************************************************************************
 
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
 
********************************************************************************
INCLUDES
*******************************************************************************/

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
#include "FGDefs.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGEngine::FGEngine(FGFDMExec* fdex, string enginePath, string engineName, int num) {
  string fullpath;
  string tag;

  FDMExec = fdex;

  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

  Name = engineName;
  fullpath = enginePath + "/" + engineName + ".dat";
  ifstream enginefile(fullpath.c_str());

  if (enginefile) {
    enginefile >> tag;

    if      (tag == "ROCKET")    Type = etRocket;
    else if (tag == "PISTON")    Type = etPiston;
    else if (tag == "TURBOPROP") Type = etTurboProp;
    else if (tag == "TURBOJET")  Type = etTurboJet;
    else                         Type = etUnknown;

    switch(Type) {
    case etUnknown:
      cerr << "Unknown engine type: " << tag << endl;
      break;
    case etPiston:
      enginefile >> X;
      enginefile >> Y;
      enginefile >> Z;
      enginefile >> EnginePitch;
      enginefile >> EngineYaw;
      enginefile >> BrakeHorsePower;
      enginefile >> MaxThrottle;
      enginefile >> MinThrottle;
      enginefile >> SLFuelFlowMax;
      enginefile >> SpeedSlope;
      enginefile >> SpeedIntercept;
      enginefile >> AltitudeSlope;

      break;
    case etRocket:
      enginefile >> X;
      enginefile >> Y;
      enginefile >> Z;
      enginefile >> EnginePitch;
      enginefile >> EngineYaw;
      enginefile >> SLThrustMax;
      enginefile >> VacThrustMax;
      enginefile >> MaxThrottle;
      enginefile >> MinThrottle;
      enginefile >> SLFuelFlowMax;
      enginefile >> SLOxiFlowMax;
      break;
    }

    enginefile.close();
  } else {
    cerr << "Unable to open engine definition file " << fullpath << endl;
  }

  EngineNumber = num;
  Thrust = PctPower = 0.0;
  Starved = Flameout = false;
}


FGEngine::~FGEngine(void) {}



float FGEngine::CalcRocketThrust(void) {
  float lastThrust;

  Throttle = FCS->GetThrottlePos(EngineNumber);
  lastThrust = Thrust;                 // last actual thrust

  if (Throttle < MinThrottle || Starved) {
    PctPower = Thrust = 0.0; // desired thrust
    Flameout = true;
  } else {
    PctPower = Throttle / MaxThrottle;
    Thrust = PctPower*((1.0 - Atmosphere->GetDensityRatio())*(VacThrustMax - SLThrustMax) +
                       SLThrustMax); // desired thrust
    Flameout = false;
  }


  Thrust -= 0.8*(Thrust - lastThrust); // actual thrust

  return Thrust;
}


float FGEngine::CalcPistonThrust(void) {
  float v,h,pa;

  Throttle = FCS->GetThrottlePos(EngineNumber);
  Throttle /= 100;

  v=Translation->GetVt();
  h=Position->Geth();
  if(v < 10)
    v=10;
  if(h < 0)
    h=0;
  
  pa=(SpeedSlope*v + SpeedIntercept)*(1 +AltitudeSlope*h)*BrakeHorsePower;
  
  Thrust= Throttle*(pa*HPTOFTLBSSEC)/v;

  return Thrust;
}


float FGEngine::CalcThrust(void) {
  switch(Type) {
  case etRocket:
    return CalcRocketThrust();
    // break;
  case etPiston:
    return CalcPistonThrust();
    // break;
  default:
    return 9999.0;
    // break;
  }

}

float FGEngine::CalcFuelNeed() {
  FuelNeed = SLFuelFlowMax*PctPower;
  return FuelNeed;
}


float FGEngine::CalcOxidizerNeed() {
  OxidizerNeed = SLOxiFlowMax*PctPower;
  return OxidizerNeed;
}

