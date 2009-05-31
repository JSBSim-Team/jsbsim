/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTurbine.cpp
 Author:       David Culp
 Date started: 03/11/2003
 Purpose:      This module models a turbine engine.

 ------------- Copyright (C) 2003  David Culp (davidculp2@comcast.net) ---------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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
#include <sstream>

#include "FGTurbine.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGTurbine.cpp,v 1.19 2009/05/31 19:54:57 dpculp Exp $";
static const char *IdHdr = ID_TURBINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGTurbine::FGTurbine(FGFDMExec* exec, Element *el, int engine_number)
  : FGEngine(exec, el, engine_number)
{
  Type = etTurbine;

  MilThrust = MaxThrust = 10000.0;
  TSFC = 0.8;
  ATSFC = 1.7;
  IdleN1 = 30.0;
  IdleN2 = 60.0;
  MaxN1 = MaxN2 = 100.0;
  Augmented = AugMethod = Injected = 0;
  BypassRatio = BleedDemand = 0.0;
  IdleThrustLookup = MilThrustLookup = MaxThrustLookup = InjectionLookup = 0;
  N1_spinup = 1.0; N2_spinup = 3.0; 

  ResetToIC();

  Load(exec, el);
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTurbine::~FGTurbine()
{
  delete IdleThrustLookup;
  delete MilThrustLookup;
  delete MaxThrustLookup;
  delete InjectionLookup;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::ResetToIC(void)
{
  N1 = N2 = 0.0;
  N2norm = 0.0;
  correctedTSFC = TSFC;
  ThrottlePos = AugmentCmd = 0.0;
  InletPosition = NozzlePosition = 1.0;
  Stalled = Seized = Overtemp = Fire = Augmentation = Injection = Reversed = false;
  Cutoff = true;
  phase = tpOff;
  EGT_degC = 0.0;
  OilTemp_degK = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556 + 273.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The main purpose of Calculate() is to determine what phase the engine should
// be in, then call the corresponding function.

double FGTurbine::Calculate(void)
{
  double thrust;

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
    case tpOff:    thrust = Off(); break;
    case tpRun:    thrust = Run(); break;
    case tpSpinUp: thrust = SpinUp(); break;
    case tpStart:  thrust = Start(); break;
    case tpStall:  thrust = Stall(); break;
    case tpSeize:  thrust = Seize(); break;
    case tpTrim:   thrust = Trim(); break;
    default: thrust = Off();
  }

  thrust = Thruster->Calculate(thrust); // allow thruster to modify thrust (i.e. reversing)

  return thrust;
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
  ConsumeFuel();  
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Run()
{
  double idlethrust, milthrust, thrust;
  double spoolup;                        // acceleration in pct/sec
  double sigma = Atmosphere->GetDensityRatio();
  double T = Atmosphere->GetTemperature();

  idlethrust = MilThrust * IdleThrustLookup->GetValue();
  milthrust = (MilThrust - idlethrust) * MilThrustLookup->GetValue();

  Running = true;
  Starter = false;

  // adjust acceleration for N2 and atmospheric density
  double n = N2norm + 0.1;
  if (n > 1) n = 1; 
  spoolup = delay / (1 + 3 * (1-n)*(1-n)*(1-n) + (1 - sigma));
  
  N2 = Seek(&N2, IdleN2 + ThrottlePos * N2_factor, spoolup, spoolup * 3.0);
  N1 = Seek(&N1, IdleN1 + ThrottlePos * N1_factor, spoolup, spoolup * 2.4);
  N2norm = (N2 - IdleN2) / N2_factor;
  thrust = idlethrust + (milthrust * N2norm * N2norm);
  EGT_degC = TAT + 363.1 + ThrottlePos * 357.1;
  OilPressure_psi = N2 * 0.62;
  OilTemp_degK = Seek(&OilTemp_degK, 366.0, 1.2, 0.1);

  if (!Augmentation) {
    correctedTSFC = TSFC * sqrt(T/389.7) * (0.84 + (1-N2norm)*(1-N2norm));
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
    thrust = MaxThrustLookup->GetValue() * MaxThrust;
    FuelFlow_pph = Seek(&FuelFlow_pph, thrust * ATSFC, 5000.0, 10000.0);
    NozzlePosition = Seek(&NozzlePosition, 1.0, 0.8, 0.8);
  }

  if (AugMethod == 2) {
    if (AugmentCmd > 0.0) {
      Augmentation = true;
      double tdiff = (MaxThrust * MaxThrustLookup->GetValue()) - thrust;
      thrust += (tdiff * AugmentCmd);
      FuelFlow_pph = Seek(&FuelFlow_pph, thrust * ATSFC, 5000.0, 10000.0);
      NozzlePosition = Seek(&NozzlePosition, 1.0, 0.8, 0.8);
    } else {
      Augmentation = false;
    }
  }

  if ((Injected == 1) && Injection) {
    InjectionTimer += dt;
    if (InjectionTimer < InjectionTime) {
       thrust = thrust * InjectionLookup->GetValue();
    } else {
       Injection = false;
    }
  }

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
  N2 = Seek(&N2, 25.18, N2_spinup, N2/2.0);
  N1 = Seek(&N1, 5.21, N1_spinup, N1/2.0);
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
      ConsumeFuel();
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
  ConsumeFuel();
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
    ConsumeFuel();
    OilPressure_psi = 0.0;
    OilTemp_degK = Seek(&OilTemp_degK, TAT + 273.0, 0, 0.2);
    Running = false;
    return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::Trim()
{
    double idlethrust, milthrust, thrust, tdiff;
    idlethrust = MilThrust * IdleThrustLookup->GetValue();
    milthrust = (MilThrust - idlethrust) * MilThrustLookup->GetValue();
    N2 = IdleN2 + ThrottlePos * N2_factor;
    N2norm = (N2 - IdleN2) / N2_factor;
    thrust = (idlethrust + (milthrust * N2norm * N2norm))
          * (1.0 - BleedDemand);

    if (AugMethod == 1) {
      if ((ThrottlePos > 0.99) && (N2 > 97.0)) {Augmentation = true;}
      else {Augmentation = false;}
    }

    if ((Augmented == 1) && Augmentation && (AugMethod < 2)) {
      thrust = MaxThrust * MaxThrustLookup->GetValue();
    }

    if (AugMethod == 2) {
      if (AugmentCmd > 0.0) {
        tdiff = (MaxThrust * MaxThrustLookup->GetValue()) - thrust;
        thrust += (tdiff * AugmentCmd);
      }
    }

    if ((Injected == 1) && Injection) {
      thrust = thrust * InjectionLookup->GetValue();
    }

    return thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurbine::CalcFuelNeed(void)
{
  double dT = State->Getdt() * Propulsion->GetRate();
  FuelFlowRate = FuelFlow_pph / 3600.0; // Calculates flow in lbs/sec from lbs/hr
  FuelExpended = FuelFlowRate * dT;     // Calculates fuel expended in this time step
  return FuelExpended;
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

bool FGTurbine::Load(FGFDMExec* exec, Element *el)
{
  string property_name, property_prefix;
  property_prefix = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  if (el->FindElement("milthrust"))
    MilThrust = el->FindElementValueAsNumberConvertTo("milthrust","LBS");
  if (el->FindElement("maxthrust"))
    MaxThrust = el->FindElementValueAsNumberConvertTo("maxthrust","LBS");
  if (el->FindElement("bypassratio"))
    BypassRatio = el->FindElementValueAsNumber("bypassratio");
  if (el->FindElement("bleed"))
    BleedDemand = el->FindElementValueAsNumber("bleed");
  if (el->FindElement("tsfc"))
    TSFC = el->FindElementValueAsNumber("tsfc");
  if (el->FindElement("atsfc"))
    ATSFC = el->FindElementValueAsNumber("atsfc");
  if (el->FindElement("idlen1"))
    IdleN1 = el->FindElementValueAsNumber("idlen1");
  if (el->FindElement("idlen2"))
    IdleN2 = el->FindElementValueAsNumber("idlen2");
  if (el->FindElement("maxn1"))
    MaxN1 = el->FindElementValueAsNumber("maxn1");
  if (el->FindElement("maxn2"))
    MaxN2 = el->FindElementValueAsNumber("maxn2");
  if (el->FindElement("n1spinup"))
    N1_spinup = el->FindElementValueAsNumber("n1spinup");
  if (el->FindElement("n2spinup"))
    N2_spinup = el->FindElementValueAsNumber("n2spinup");
  if (el->FindElement("augmented"))
    Augmented = (int)el->FindElementValueAsNumber("augmented");
  if (el->FindElement("augmethod"))
    AugMethod = (int)el->FindElementValueAsNumber("augmethod");
  if (el->FindElement("injected"))
    Injected = (int)el->FindElementValueAsNumber("injected");
  if (el->FindElement("injection-time"))
    InjectionTime = el->FindElementValueAsNumber("injection-time");

  Element *function_element;
  string name;
  FGPropertyManager* PropertyManager = exec->GetPropertyManager();

  while (true) {
    function_element = el->FindNextElement("function");
    if (!function_element) break;
    name = function_element->GetAttributeValue("name");
    if (name == "IdleThrust") {
      IdleThrustLookup = new FGFunction(PropertyManager, function_element, property_prefix);
    } else if (name == "MilThrust") {
      MilThrustLookup = new FGFunction(PropertyManager, function_element, property_prefix);
    } else if (name == "AugThrust") {
      MaxThrustLookup = new FGFunction(PropertyManager, function_element, property_prefix);
    } else if (name == "Injection") {
      InjectionLookup = new FGFunction(PropertyManager, function_element, property_prefix);
    } else {
      cerr << "Unknown function type: " << name << " in turbine definition." <<
      endl;
    }
  }

  // Pre-calculations and initializations

  delay = 90.0 / (BypassRatio + 3.0);
  N1_factor = MaxN1 - IdleN1;
  N2_factor = MaxN2 - IdleN2;
  OilTemp_degK = (Auxiliary->GetTotalTemperature() - 491.69) * 0.5555556 + 273.0;
  IdleFF = pow(MilThrust, 0.2) * 107.0;  // just an estimate

  bindmodel();
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGTurbine::GetEngineLabels(string delimeter)
{
  std::ostringstream buf;

  buf << Name << "_N1[" << EngineNumber << "]" << delimeter
      << Name << "_N2[" << EngineNumber << "]" << delimeter
      << Thruster->GetThrusterLabels(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGTurbine::GetEngineValues(string delimeter)
{
  std::ostringstream buf;

  buf << N1 << delimeter
      << N2 << delimeter
      << Thruster->GetThrusterValues(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurbine::bindmodel()
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  property_name = base_property_name + "/n1";
  PropertyManager->Tie( property_name.c_str(), &N1);
  property_name = base_property_name + "/n2";
  PropertyManager->Tie( property_name.c_str(), &N2);
  property_name = base_property_name + "/injection_cmd";
  PropertyManager->Tie( property_name.c_str(), (FGTurbine*)this, 
                        &FGTurbine::GetInjection, &FGTurbine::SetInjection);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGTurbine::InitRunning(void) {
  State->SuspendIntegration();
  Cutoff=false;
  Running=true;  
  N2=16.0;
  Calculate();
  State->ResumeIntegration();
  return phase==tpRun;
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
