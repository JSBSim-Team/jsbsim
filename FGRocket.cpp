/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRocket.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models a rocket engine

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

This class descends from the FGEngine class and models a rocket engine based on
parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGRocket.h"

static const char *IdSrc = "$Id: FGRocket.cpp,v 1.22 2001/03/23 23:30:55 jberndt Exp $";
static const char *IdHdr = ID_ROCKET;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRocket::FGRocket(FGFDMExec* exec, FGConfigFile* Eng_cfg) : FGEngine(exec)
{
  string token;

  Name = Eng_cfg->GetValue("NAME");
  cout << "      Engine Name: " << Name << endl;
  Eng_cfg->GetNextConfigLine();
  while (Eng_cfg->GetValue() != "/FG_ROCKET") {
    *Eng_cfg >> token;
    if (token == "SHR") {
      *Eng_cfg >> SHR;
      cout << "      Specific Heat Ratio = " << SHR << endl;
    } else if (token == "MAX_PC") {
      *Eng_cfg >> maxPC;
      cout << "      Maximum Chamber Pressure = " << maxPC << endl;
    } else if (token == "PROP_EFF") {
      *Eng_cfg >> propEff;
      cout << "      Propulsive Efficiency = " << propEff << endl;
    } else if (token == "MAXTHROTTLE") {
      *Eng_cfg >> MaxThrottle;
      cout << "      MaxThrottle = " << MaxThrottle << endl;
    } else if (token == "MINTHROTTLE") {
      *Eng_cfg >> MinThrottle;
      cout << "      MinThrottle = " << MinThrottle << endl;
    } else if (token == "SLFUELFLOWMAX") {
      *Eng_cfg >> SLFuelFlowMax;
      cout << "      FuelFlowMax = " << SLFuelFlowMax << endl;
    } else if (token == "SLOXIFLOWMAX") {
      *Eng_cfg >> SLOxiFlowMax;
      cout << "      OxiFlowMax = " << SLOxiFlowMax << endl;
    } else if (token == "VARIANCE") {
      *Eng_cfg >> Variance;
      cout << "      Variance = " << Variance << endl;
    } else {
      cout << "Unhandled token in Engine config file: " << token << endl;
    }
  }

  EngineNumber = 0;

  kFactor = (2.0*SHR*SHR/(SHR-1.0))*pow(2.0/(SHR+1), (SHR+1)/(SHR-1));

  if (debug_lvl & 2) cout << "Instantiated: FGRocket" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRocket::~FGRocket()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGRocket" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGRocket::Calculate(float pe)
{
  float lastThrust;
  float Cf;
  float chamberPress;

  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  lastThrust = Thrust;                 // last actual thrust

  if (Throttle < MinThrottle || Starved) {
    PctPower = Thrust = 0.0; // desired thrust
    Flameout = true;
  } else {
    PctPower = Throttle / MaxThrottle;
    chamberPress = maxPC*PctPower * (1.0 + Variance * ((float)rand()/(float)RAND_MAX - 0.5));
    Cf = sqrt(kFactor*(1 - pow(pe/(chamberPress), (SHR-1)/SHR)));
    Flameout = false;
  }

  if (State->Getdt() > 0.0) {  // actual thrust - if not in freeze
    Thrust -= 0.8*(Thrust - lastThrust);
  }

  return Cf*maxPC*PctPower;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGRocket::Debug(void)
{
    //TODO: Add your source code here
}

