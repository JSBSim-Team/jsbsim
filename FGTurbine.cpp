/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTurbine.cpp
 Author:       David Culp
 Date started: 03/11/2003
 Purpose:      This module models a turbine engine.

 ------------- Copyright (C) 2003  David Culp (davidculp2@comcast.net) ---------

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

This class descends from the FGEngine class and models a turbine engine based
on parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
03/11/2003  DPC  Created
09/08/2003  DPC  Changed Calculate() and added engine phases

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGTurbine.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGTurbine.cpp,v 1.10 2004/05/03 16:22:40 dpculp Exp $";
static const char *IdHdr = ID_TURBINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGTurbine::FGTurbine(FGFDMExec* exec, FGConfigFile* cfg) : FGEngine(exec)
{
  SetDefaults();

  Load(cfg);
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTurbine::~FGTurbine()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The main purpose of Calculate() is to determine what phase the engine should
// be in, then call the corresponding function.

double FGTurbine::Calculate(double dummy)
{
  TAT = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556;
  dt = State->Getdt() * Propulsion->GetRate();
  ThrottlePos = FCS->GetThrottlePos(EngineNumber);
  if (ThrottlePos > 1.0) {
    AugmentCmd = ThrottlePos - 1.0;
    ThrottlePos -= AugmentCmd;
  } else {
    AugmentCmd = 0.0;
  }

  // When trimming is finished check if user wants engine OFF or RUNNING
  if ((phase == tpTrim) && (dt > 0)) {
    if (Running && !Starved) {
      phase = tpRun;
      N2 = IdleN2 + ThrottlePos * N2_factor;
      N1 = IdleN1 + ThrottlePos * N1_factor;
      OilTemp_degK = 366.0;
      Cutoff = false;
      }
    else {
      phase = tpOff;
      Cutoff = true;
      EGT_degC = TAT;
      }
    }

  if (!Running && Cutoff && Starter) {
     if (phase == tpOff) phase = tpSpinUp;
     }
  if (!Running && !Cutoff && (N2 > 15.0)) phase = tpStart;
  if (Cutoff && (phase != tpSpinUp)) phase = tpOff;
  if (dt == 0) phase = tpTrim;
  if (Starved) phase = tpOff;
  if (Stalled) phase = tpStall;
  if (Seized) phase = tpSeize;

  switch (phase) {
    case tpOff:    Thrust = Off(); break;
    case tpRun:    Thrust = Run(); break;
    case tpSpinUp: Thrust = SpinUp(); break;
    case tpStart:  Thrust = Start(); break;
    case tpStall:  Thrust = Stall(); break;
    case tpSeize:  Thrust = Seize(); break;
    case tpTrim:   Thrust = Trim(); break;
    default: Thrust = Off();
  }

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Off(void)
{
  double qbar = Auxiliary->Getqbar();
  Running = false;
  FuelFlow_pph = Seek(&FuelFlow_pph, 0, 1000.0, 10000.0);
  N1 = Seek(&N1, qbar/10.0, N1/2.0, N1/2.0);
  N2 = Seek(&N2, qbar/15.0, N2/2.0, N2/2.0);
  EGT_degC = Seek(&EGT_degC, TAT, 11.7, 7.3);
  OilTemp_degK = Seek(&OilTemp_degK, TAT + 273.0, 0.2, 0.2);
  OilPressure_psi = N2 * 0.62;
  NozzlePosition = Seek(&NozzlePosition, 1.0, 0.8, 0.8);
  EPR = Seek(&EPR, 1.0, 0.2, 0.2);
  Augmentation = false;
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Run(void)
{
  double idlethrust, milthrust, thrust;
  double N2norm;   // 0.0 = idle N2, 1.0 = maximum N2
  idlethrust = MilThrust * ThrustTables[0]->TotalValue();
  milthrust = (MilThrust - idlethrust) * ThrustTables[1]->TotalValue();

  Running = true;
  Starter = false;

  N2 = Seek(&N2, IdleN2 + ThrottlePos * N2_factor, delay, delay * 3.0);
  N1 = Seek(&N1, IdleN1 + ThrottlePos * N1_factor, delay, delay * 2.4);
  N2norm = (N2 - IdleN2) / N2_factor;
  thrust = idlethrust + (milthrust * N2norm * N2norm);
  EGT_degC = TAT + 363.1 + ThrottlePos * 357.1;
  OilPressure_psi = N2 * 0.62;
  OilTemp_degK = Seek(&OilTemp_degK, 366.0, 1.2, 0.1);

  if (!Augmentation) {
    double correctedTSFC = TSFC + TSFC - (N2norm * TSFC); 
    FuelFlow_pph = Seek(&FuelFlow_pph, thrust * correctedTSFC, 1000.0, 100000);
    if (FuelFlow_pph < IdleFF) FuelFlow_pph = IdleFF;
    NozzlePosition = Seek(&NozzlePosition, 1.0 - N2norm, 0.8, 0.8);
    thrust = thrust * (1.0 - BleedDemand);
    EPR = 1.0 + thrust/MilThrust;
  }

  if (AugMethod == 1) {
    if ((ThrottlePos > 0.99) && (N2 > 97.0)) {Augmentation = true;}
    else {Augmentation = false;}
  }

  if ((Augmented == 1) && Augmentation && (AugMethod < 2)) {
    thrust = MaxThrust * ThrustTables[2]->TotalValue();
    FuelFlow_pph = Seek(&FuelFlow_pph, thrust * ATSFC, 5000.0, 10000.0);
    NozzlePosition = Seek(&NozzlePosition, 1.0, 0.8, 0.8);
  }

  if ((AugmentCmd > 0.0) && (AugMethod == 2)) {
    Augmentation = true;
    double tdiff = (MaxThrust * ThrustTables[2]->TotalValue()) - thrust;
    thrust += (tdiff * AugmentCmd);
    FuelFlow_pph = Seek(&FuelFlow_pph, thrust * ATSFC, 5000.0, 10000.0);
    NozzlePosition = Seek(&NozzlePosition, 1.0, 0.8, 0.8);
  } else {
    Augmentation = false;
  }    

  if ((Injected == 1) && Injection)
    thrust = thrust * ThrustTables[3]->TotalValue();

  ConsumeFuel();
  if (Cutoff) phase = tpOff;
  if (Starved) phase = tpOff;

  return thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::SpinUp(void)
{
  Running = false;
  FuelFlow_pph = 0.0;
  N2 = Seek(&N2, 25.18, 3.0, N2/2.0);
  N1 = Seek(&N1, 5.21, 1.0, N1/2.0);
  EGT_degC = Seek(&EGT_degC, TAT, 11.7, 7.3);
  OilPressure_psi = N2 * 0.62;
  OilTemp_degK = Seek(&OilTemp_degK, TAT + 273.0, 0.2, 0.2);
  EPR = 1.0;
  NozzlePosition = 1.0;
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Start(void)
{
  if ((N2 > 15.0) && !Starved) {       // minimum 15% N2 needed for start
    Cranking = true;                   // provided for sound effects signal
    if (N2 < IdleN2) {
      N2 = Seek(&N2, IdleN2, 2.0, N2/2.0);
      N1 = Seek(&N1, IdleN1, 1.4, N1/2.0);
      EGT_degC = Seek(&EGT_degC, TAT + 363.1, 21.3, 7.3);
      FuelFlow_pph = Seek(&FuelFlow_pph, IdleFF, 103.7, 103.7);
      OilPressure_psi = N2 * 0.62;
      }
    else {
      phase = tpRun;
      Running = true;
      Starter = false;
      Cranking = false;
      }
    }
  else {                 // no start if N2 < 15%
    phase = tpOff;
    Starter = false;
    }

  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Stall(void)
{
  double qbar = Auxiliary->Getqbar();
  EGT_degC = TAT + 903.14;
  FuelFlow_pph = IdleFF;
  N1 = Seek(&N1, qbar/10.0, 0, N1/10.0);
  N2 = Seek(&N2, qbar/15.0, 0, N2/10.0);
  if (ThrottlePos < 0.01) phase = tpRun;        // clear the stall with throttle

  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Seize(void)
{
    double qbar = Auxiliary->Getqbar();
    N2 = 0.0;
    N1 = Seek(&N1, qbar/20.0, 0, N1/15.0);
    FuelFlow_pph = IdleFF;
    OilPressure_psi = 0.0;
    OilTemp_degK = Seek(&OilTemp_degK, TAT + 273.0, 0, 0.2);
    Running = false;
    return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Trim(void)
{
    double idlethrust, milthrust, thrust, tdiff;
    idlethrust = MilThrust * ThrustTables[0]->TotalValue();
    milthrust = (MilThrust - idlethrust) * ThrustTables[1]->TotalValue();
    thrust = (idlethrust + (milthrust * ThrottlePos * ThrottlePos)) * (1.0 - BleedDemand);
    if (AugmentCmd > 0.0) {
      tdiff = (MaxThrust * ThrustTables[2]->TotalValue()) - thrust;
      thrust += (tdiff * AugmentCmd);
      }     
    return thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::CalcFuelNeed(void)
{
  return FuelFlow_pph /3600 * State->Getdt() * Propulsion->GetRate();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::GetPowerAvailable(void) {
  if( ThrottlePos <= 0.77 )
    return 64.94*ThrottlePos;
  else
    return 217.38*ThrottlePos - 117.38;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Seek(double *var, double target, double accel, double decel) {
  double v = *var;
  if (v > target) {
    v -= dt * decel;
    if (v < target) v = target;
  } else if (v < target) {
    v += dt * accel;
    if (v > target) v = target;
  }
  return v;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::SetDefaults(void)
{
  Name = "Not defined";
  N1 = N2 = 0.0;
  Type = etTurbine;
  MilThrust = 10000.0;
  MaxThrust = 10000.0;
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
  ThrottlePos = 0.0;
  AugmentCmd = 0.0;
  InletPosition = 1.0;
  NozzlePosition = 1.0;
  Augmentation = false;
  Injection = false;
  Reversed = false;
  Cutoff = true;
  phase = tpOff;
  Stalled = false;
  Seized = false;
  Overtemp = false;
  Fire = false;
  EGT_degC = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTurbine::Load(FGConfigFile *Eng_cfg)
{
  string token;

  Name = Eng_cfg->GetValue("NAME");
  Eng_cfg->GetNextConfigLine();
  int counter=0;

  while (Eng_cfg->GetValue() != string("/FG_TURBINE")) {
    *Eng_cfg >> token;

    if (token[0] == '<') token.erase(0,1); // Tables are read "<TABLE"

    if      (token == "MILTHRUST") *Eng_cfg >> MilThrust;
    else if (token == "MAXTHRUST") *Eng_cfg >> MaxThrust;
    else if (token == "BYPASSRATIO") *Eng_cfg >> BypassRatio;
    else if (token == "BLEED") *Eng_cfg >> BleedDemand; 
    else if (token == "TSFC") *Eng_cfg >> TSFC;
    else if (token == "ATSFC") *Eng_cfg >> ATSFC;
    else if (token == "IDLEN1") *Eng_cfg >> IdleN1;
    else if (token == "IDLEN2") *Eng_cfg >> IdleN2;
    else if (token == "MAXN1") *Eng_cfg >> MaxN1;
    else if (token == "MAXN2") *Eng_cfg >> MaxN2;
    else if (token == "AUGMENTED") *Eng_cfg >> Augmented;
    else if (token == "AUGMETHOD") *Eng_cfg >> AugMethod;
    else if (token == "INJECTED") *Eng_cfg >> Injected;
    else if (token == "MINTHROTTLE") *Eng_cfg >> MinThrottle;
    else if (token == "TABLE") {
      if (counter++ == 0) Debug(2); // print engine specs prior to table read
      ThrustTables.push_back( new FGCoefficient(FDMExec) );
      ThrustTables.back()->Load(Eng_cfg);
    }
    else cerr << "Unhandled token in Engine config file: " << token << endl;
  }

  // Pre-calculations and initializations

  delay = 60.0 / (BypassRatio + 3.0);
  N1_factor = MaxN1 - IdleN1;
  N2_factor = MaxN2 - IdleN2;
  OilTemp_degK = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556 + 273.0;
  IdleFF = pow(MilThrust, 0.2) * 107.0;  // just an estimate

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

void FGTurbine::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
    if (from == 2) { // called from Load()
      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      MilThrust:   "         << MilThrust << endl;
      cout << "      MaxThrust:   "         << MaxThrust << endl;
      cout << "      BypassRatio: "         << BypassRatio << endl;
      cout << "      TSFC:        "         << TSFC << endl;
      cout << "      ATSFC:       "         << ATSFC << endl;
      cout << "      IdleN1:      "         << IdleN1 << endl;
      cout << "      IdleN2:      "         << IdleN2 << endl;
      cout << "      MaxN1:       "         << MaxN1 << endl;
      cout << "      MaxN2:       "         << MaxN2 << endl;
      cout << "      Augmented:   "         << Augmented << endl;
      cout << "      AugMethod:   "         << AugMethod << endl;
      cout << "      Injected:    "         << Injected << endl;
      cout << "      MinThrottle: "         << MinThrottle << endl;

      cout << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTurbine" << endl;
    if (from == 1) cout << "Destroyed:    FGTurbine" << endl;
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
