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

static const char *IdSrc = "$Id: FGRocket.cpp,v 1.35 2001/12/12 18:31:08 jberndt Exp $";
static const char *IdHdr = ID_ROCKET;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRocket::FGRocket(FGFDMExec* exec, FGConfigFile* Eng_cfg) : FGEngine(exec)
{
  string token;

  Name = Eng_cfg->GetValue("NAME");
  Eng_cfg->GetNextConfigLine();

  while (Eng_cfg->GetValue() != string("/FG_ROCKET")) {
    *Eng_cfg >> token;
    if      (token == "SHR")           *Eng_cfg >> SHR;
    else if (token == "MAX_PC")        *Eng_cfg >> maxPC;
    else if (token == "PROP_EFF")      *Eng_cfg >> propEff;
    else if (token == "MAXTHROTTLE")   *Eng_cfg >> MaxThrottle;
    else if (token == "MINTHROTTLE")   *Eng_cfg >> MinThrottle;
    else if (token == "SLFUELFLOWMAX") *Eng_cfg >> SLFuelFlowMax;
    else if (token == "SLOXIFLOWMAX")  *Eng_cfg >> SLOxiFlowMax;
    else if (token == "VARIANCE")      *Eng_cfg >> Variance;
    else cerr << "Unhandled token in Engine config file: " << token << endl;
  }

  Debug(0);

  EngineNumber = 0;
  Type = etRocket;

  PC = 0.0;
  kFactor = (2.0*SHR*SHR/(SHR-1.0))*pow(2.0/(SHR+1), (SHR+1)/(SHR-1));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRocket::~FGRocket()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRocket::Calculate(double pe)
{
  double Cf=0;

  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);

  if (Throttle < MinThrottle || Starved) {
    PctPower = Thrust = 0.0; // desired thrust
    Flameout = true;
    PC = 0.0;
  } else {
    PctPower = Throttle / MaxThrottle;
    PC = maxPC*PctPower * (1.0 + Variance * ((double)rand()/(double)RAND_MAX - 0.5));
    Cf = sqrt(kFactor*(1 - pow(pe/(PC), (SHR-1)/SHR)));
    Flameout = false;
  }

  return Cf*maxPC*PctPower*propEff;
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

void FGRocket::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      Engine Name: " << Name << endl;
      cout << "      Specific Heat Ratio = " << SHR << endl;
      cout << "      Maximum Chamber Pressure = " << maxPC << endl;
      cout << "      Propulsive Efficiency = " << propEff << endl;
      cout << "      MaxThrottle = " << MaxThrottle << endl;
      cout << "      MinThrottle = " << MinThrottle << endl;
      cout << "      FuelFlowMax = " << SLFuelFlowMax << endl;
      cout << "      OxiFlowMax = " << SLOxiFlowMax << endl;
      cout << "      Variance = " << Variance << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGRocket" << endl;
    if (from == 1) cout << "Destroyed:    FGRocket" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
}

