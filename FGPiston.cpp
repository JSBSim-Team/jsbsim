/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPiston.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models a Piston engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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

This class descends from the FGEngine class and models a Piston engine based on
parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPiston.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPiston.cpp,v 1.10 2001/01/19 23:36:06 jsb Exp $";
static const char *IdHdr = ID_PISTON;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPiston::FGPiston(FGFDMExec* exec, FGConfigFile* Eng_cfg) : FGEngine(exec)
{
  string token;

  Name = Eng_cfg->GetValue("NAME");
  cout << "\n    Engine Name: " << Name << endl;
  Eng_cfg->GetNextConfigLine();
  while (Eng_cfg->GetValue() != "/FG_PISTON") {
    *Eng_cfg >> token;
    if (token == "BRAKEHORSEPOWER") {
      *Eng_cfg >> BrakeHorsePower;
      cout << "      BrakeHorsePower = " << BrakeHorsePower << endl;
    } else if (token == "MAXTHROTTLE") {
      *Eng_cfg >> MaxThrottle;
      cout << "      MaxThrottle = " << MaxThrottle << endl;
    } else if (token == "MINTHROTTLE") {
      *Eng_cfg >> MinThrottle;
      cout << "      MinThrottle = " << MinThrottle << endl;
    } else if (token == "SLFUELFLOWMAX") {
      *Eng_cfg >> SLFuelFlowMax;
      cout << "      SLFuelFlowMax = " << SLFuelFlowMax << endl;
    } else if (token == "SPEEDSLOPE") {
      *Eng_cfg >> SpeedSlope;
      cout << "      SpeedSlope = " << SpeedSlope << endl;
    } else if (token == "SPEEDINTERCEPT") {
      *Eng_cfg >> SpeedIntercept;
      cout << "      SpeedIntercept = " << SpeedIntercept << endl;
    } else if (token == "ALTITUDESLOPE") {
      *Eng_cfg >> AltitudeSlope;
      cout << "      AltitudeSlope = " << AltitudeSlope << endl;
    } else {
      cout << "Unhandled token in Engine config file: " << token << endl;
    }
  }

  EngineNumber = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPiston::Calculate(float PowerRequired)
{
  float v,h,pa;

  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  Throttle /= 100;

  v = Translation->GetVt();
  h = Position->Geth();

  if (v < 10)
    v = 10;
  if (h < 0)
    h = 0;

  pa = (SpeedSlope*v + SpeedIntercept)*(1 +AltitudeSlope*h)*BrakeHorsePower;

  Thrust = Throttle*(pa*HPTOFTLBSSEC)/v;

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPiston::GetPowerAvailable(void)
{

  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

