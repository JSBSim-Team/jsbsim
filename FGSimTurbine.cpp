/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSimTurbine.cpp
 Author:       David Culp
 Date started: 03/11/2003
 Purpose:      This module models a turbine engine.

 ------------- Copyright (C) 2003  David Culp (davidculp2@attbi.com) -----------

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

This class descends from the FGEngine class and models a Turbine engine based
on parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
03/11/2003  DPC  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGSimTurbine.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGSimTurbine.cpp,v 1.5 2003/06/07 08:48:29 ehofman Exp $";
static const char *IdHdr = ID_SIMTURBINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGSimTurbine::FGSimTurbine(FGFDMExec* exec, FGConfigFile* cfg) : FGEngine(exec)
{
  SetDefaults();
  FGEngine::Type=etSimTurbine;
  Load(cfg);
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSimTurbine::~FGSimTurbine()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGSimTurbine::Calculate(double dummy)
{
  double idlethrust, milthrust, thrust;
  double TAT = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556;
  dt = State->Getdt() * Propulsion->GetRate();

  // calculate virtual throttle position (actual +/- lag) based on
  // FCS Throttle value (except when trimming)
  if (dt > 0.0) {
    Running = !Starved;
    ThrottleCmd = FCS->GetThrottleCmd(EngineNumber);
    if ( ThrottleCmd > throttle ) {
      throttle += (dt * delay);
      if (throttle > ThrottleCmd ) throttle = ThrottleCmd;
      }
    else {
      throttle -= (dt * delay * 3.0);
      if (throttle < ThrottleCmd ) throttle = ThrottleCmd;
      }
    }
  else {
    Starved = false;
    throttle = ThrottleCmd = FCS->GetThrottleCmd(EngineNumber);
    }
    
  idlethrust = MaxMilThrust * ThrustTables[0]->TotalValue();
  milthrust = MaxMilThrust * ThrustTables[1]->TotalValue();

  if (Running) {
    thrust = milthrust * throttle * throttle;
    if (thrust < idlethrust) thrust = idlethrust;
    FuelFlow_pph = thrust * TSFC;
    thrust = thrust * (1.0 - BleedDemand);
    IdleFF = pow(MaxMilThrust, 0.2) * 107.0;
    if (FuelFlow_pph < IdleFF) FuelFlow_pph = IdleFF;
    N1 = IdleN1 + throttle * N1_factor;
    N2 = IdleN2 + throttle * N2_factor;
    EGT_degC = TAT + 363.1 + ThrottleCmd * 357.1;
    OilPressure_psi = N2 * 0.62;
    OilTemp_degK += dt * 1.2;
    if (OilTemp_degK > 366.0) OilTemp_degK = 366.0;
    EPR = 1.0 + thrust/MaxMilThrust;
    NozzlePosition = 1.0 - throttle;
    if (Reversed) thrust = thrust * -0.2;
    }
  else {
    thrust = 0.0;
    FuelFlow_pph = 0.000001;
    N1 -= (dt * 3.0);
    if (N1 < (Translation->Getqbar()/10.0)) N1 = Translation->Getqbar()/10.0;
    N2 -= (dt * 3.5);
    if (N2 < (Translation->Getqbar()/15.0)) N2 = Translation->Getqbar()/15.0;
    EGT_degC -= (dt * 11.7);
    if (EGT_degC < TAT) EGT_degC = TAT;
    OilPressure_psi = N2 * 0.62;
    OilTemp_degK -= (dt * 0.2);
    if (OilTemp_degK < (TAT + 273.0)) OilTemp_degK = (TAT + 273.0);
    EPR = 1.0;
    }

  if (AugMethod == 1) {
    if (throttle > 0.99) {Augmentation = true;} 
      else {Augmentation = false;}
    }

  if ((Augmented == 1) && Augmentation) {
    thrust = thrust * ThrustTables[2]->TotalValue();
    FuelFlow_pph = thrust * ATSFC;
    NozzlePosition = 1.0;
    }

  if ((Injected == 1) && Injection)
    thrust = thrust * ThrustTables[3]->TotalValue();  
  
  ConsumeFuel();
  
  return Thrust = thrust;
  
}
        
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGSimTurbine::CalcFuelNeed(void)
{
  return FuelFlow_pph /3600 * State->Getdt() * Propulsion->GetRate();
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSimTurbine::SetDefaults(void)
{
  Name = "None_Defined";
  MaxMilThrust = 10000.0;
  BypassRatio = 0.0;
  TSFC = 0.8;
  ATSFC = 1.7;
  IdleN1 = 30.0;
  IdleN2 = 60.0;
  MaxN1 = 100.0;
  MaxN2 = 100.0;
  Augmented = 0;
  AugMethod = 0;
  Injected = 0;
  BleedDemand = 0.0;
  throttle = 0.0;
  InletPosition = 1.0;
  NozzlePosition = 1.0;
  Augmentation = false;
  Injection = false;
  Reversed = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSimTurbine::Load(FGConfigFile *Eng_cfg)
{
  int i;
  string token;
  Name = Eng_cfg->GetValue("NAME");
  cout << Name << endl;
  Eng_cfg->GetNextConfigLine();
  *Eng_cfg >> token >> MaxMilThrust;
  *Eng_cfg >> token >> BypassRatio;
  *Eng_cfg >> token >> TSFC;
  *Eng_cfg >> token >> ATSFC;
  *Eng_cfg >> token >> IdleN1;
  *Eng_cfg >> token >> IdleN2;
  *Eng_cfg >> token >> MaxN1;
  *Eng_cfg >> token >> MaxN2;
  *Eng_cfg >> token >> Augmented;
  *Eng_cfg >> token >> AugMethod;
  *Eng_cfg >> token >> Injected;
  i=0;
  while( Eng_cfg->GetValue() != string("/FG_SIMTURBINE") && i < 10){
    ThrustTables.push_back( new FGCoefficient(FDMExec) );
    ThrustTables.back()->Load(Eng_cfg);
    i++;
  }
  
  // pre-calculations and initializations
  delay= 1.0 / (BypassRatio + 3.0);
  N1_factor = MaxN1 - IdleN1;
  N2_factor = MaxN2 - IdleN2;
  OilTemp_degK = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556 + 273.0;
  IdleFF = pow(MaxMilThrust, 0.2) * 107.0;  // just an estimate
  AddFeedTank(EngineNumber);   // engine[n] feeds from tank[n]
  return true;
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

void FGSimTurbine::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGSimTurbine" << endl;
    if (from == 1) cout << "Destroyed:    FGSimTurbine" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
