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

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPiston.cpp,v 1.19 2001/03/19 14:07:19 jberndt Exp $";
static const char *IdHdr = ID_PISTON;

extern short debug_lvl;

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

  if (debug_lvl & 2) cout << "Instantiated: FGPiston" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPiston::Calculate(float PowerRequired)
{
  float h,EngineMaxPower;

  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);

  h = Position->Geth();

  if (h < 0) h = 0;

  EngineMaxPower = (1 + AltitudeSlope*h)*BrakeHorsePower;
  PowerAvailable = Throttle*EngineMaxPower*HPTOFTLBSSEC - PowerRequired;
  
  return PowerAvailable;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPiston::Debug(void)
{
    //TODO: Add your source code here
}

