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

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGRocket.cpp,v 1.9 2000/12/04 13:26:24 jsb Exp $";
static const char *IdHdr = ID_ROCKET;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRocket::FGRocket(FGFDMExec* exec, FGConfigFile* Eng_cfg) : FGEngine(exec) {
  *Eng_cfg >> SLThrustMax;
  *Eng_cfg >> VacThrustMax;
  *Eng_cfg >> MaxThrottle;
  *Eng_cfg >> MinThrottle;
  *Eng_cfg >> SLFuelFlowMax;
  *Eng_cfg >> SLOxiFlowMax;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRocket::~FGRocket(void) {
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGRocket::Calculate(void) {
  float lastThrust;

  FGEngine::Calculate();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  lastThrust = Thrust;                 // last actual thrust

  if (Throttle < MinThrottle || Starved) {
    PctPower = Thrust = 0.0; // desired thrust
    Flameout = true;
  } else {
    PctPower = Throttle / MaxThrottle;
    Thrust = PctPower*((1.0 - Atmosphere->GetPressureRatio())*(VacThrustMax - SLThrustMax) +
                       SLThrustMax); // desired thrust
    Flameout = false;
  }


  if(State->Getdt() > 0.0) {
    Thrust -= 0.8*(Thrust - lastThrust); // actual thrust
  }

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

