/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTurboProp.cpp
 Author:       Jiri "Javky" Javurek
               based on SimTurbine and Turbine engine from David Culp
 Date started: 05/14/2004
 Purpose:      This module models a turbo propeller engine.

 ------------- Copyright (C) 2004  (javky@email.cz) ---------

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

This class descends from the FGEngine class and models a Turbo propeller engine
based on parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
05/14/2004  Created
02/08/2011  T. Kreitler, added rotor support

//JVK (mark)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>

#include "FGTurboProp.h"
#include "FGPropeller.h"
#include "FGRotor.h"
#include "math/FGFunction.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTurboProp::FGTurboProp(FGFDMExec* exec, Element *el, int engine_number, struct Inputs& input)
  : FGEngine(engine_number, input)
{
  SetDefaults();
  Load(exec, el);
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTurboProp::Load(FGFDMExec* exec, Element *el)
{
  MaxStartingTime = 999999; //very big timeout -> infinite
  Ielu_max_torque=-1;

  Element* function_element = el->FindElement("function");

  while(function_element) {
    string name = function_element->GetAttributeValue("name");
    if (name == "EnginePowerVC")
      function_element->SetAttributeValue("name", string("propulsion/engine[#]/") + name);

    function_element = el->FindNextElement("function");
  }

  FGEngine::Load(exec, el);
  thrusterType = Thruster->GetType();

  string property_prefix = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  EnginePowerVC = GetPreFunction(property_prefix+"/EnginePowerVC");


// ToDo: Need to make sure units are properly accounted for below.

  if (el->FindElement("idlen1"))
    IdleN1 = el->FindElementValueAsNumber("idlen1");
  if (el->FindElement("maxn1"))
    MaxN1 = el->FindElementValueAsNumber("maxn1");
  if (el->FindElement("betarangeend"))
    BetaRangeThrottleEnd = el->FindElementValueAsNumber("betarangeend")/100.0;
  BetaRangeThrottleEnd = Constrain(0.0, BetaRangeThrottleEnd, 0.99999);
  if (el->FindElement("reversemaxpower"))
    ReverseMaxPower = el->FindElementValueAsNumber("reversemaxpower")/100.0;

  if (el->FindElement("maxpower"))
    MaxPower = el->FindElementValueAsNumber("maxpower");
  if (el->FindElement("idlefuelflow")) {
    cerr << el->ReadFrom() << "Note: 'idlefuelflow' is obsolete, "
         << "use the 'CombustionEfficiency_N1' table instead." << endl;
  }
  if (el->FindElement("psfc"))
    PSFC = el->FindElementValueAsNumber("psfc");
  if (el->FindElement("n1idle_max_delay"))
    Idle_Max_Delay = el->FindElementValueAsNumber("n1idle_max_delay");
  if (el->FindElement("maxstartingtime"))
    MaxStartingTime = el->FindElementValueAsNumber("maxstartingtime");
  if (el->FindElement("startern1"))
    StarterN1 = el->FindElementValueAsNumber("startern1");
  if (el->FindElement("ielumaxtorque"))
    Ielu_max_torque = el->FindElementValueAsNumber("ielumaxtorque");
  if (el->FindElement("itt_delay"))
    ITT_Delay = el->FindElementValueAsNumber("itt_delay");

  Element *table_element = el->FindElement("table");
  auto PropertyManager = exec->GetPropertyManager();

  while (table_element) {
    string name = table_element->GetAttributeValue("name");
    if (!EnginePowerVC && name == "EnginePowerVC") {
      // Get a different name for each engines otherwise FGTable::bind() will
      // complain that the property 'EnginePowerVC' is already bound. This is a
      // ugly hack but the functionality is obsolete and will be removed some
      // time in the future.
      table_element->SetAttributeValue("name", string("propulsion/engine[#]/") + name);
      EnginePowerVC = std::make_shared<FGTable>(PropertyManager, table_element,
                                  to_string((int)EngineNumber));
      table_element->SetAttributeValue("name", name);
      cerr << table_element->ReadFrom()
           <<"Note: Using the EnginePowerVC without enclosed <function> tag is deprecated"
           << endl;
    } else if (name == "EnginePowerRPM_N1") {
      EnginePowerRPM_N1 = std::make_unique<FGTable>(PropertyManager, table_element);
    } else if (name == "ITT_N1") {
      ITT_N1 = std::make_unique<FGTable>(PropertyManager, table_element);
    } else if (name == "CombustionEfficiency_N1") {
      CombustionEfficiency_N1 = std::make_unique<FGTable>(PropertyManager, table_element);
    } else {
      cerr << el->ReadFrom() << "Unknown table type: " << name
           << " in turboprop definition." << endl;
    }
    table_element = el->FindNextElement("table");
  }

  // Pre-calculations and initializations

  delay=1;
  N1_factor = MaxN1 - IdleN1;
  OilTemp_degK = in.TAT_c + 273.0;

  // default table based on '9.333 - (N1)/12.0' approximation
  // gives 430%Fuel at 60%N1
  if (! CombustionEfficiency_N1) {
    CombustionEfficiency_N1 = std::make_unique<FGTable>(6);
    *CombustionEfficiency_N1 <<  60.0 << 12.0/52.0;
    *CombustionEfficiency_N1 <<  82.0 << 12.0/30.0;
    *CombustionEfficiency_N1 <<  96.0 << 12.0/16.0;
    *CombustionEfficiency_N1 << 100.0 << 1.0;
    *CombustionEfficiency_N1 << 104.0 << 1.5;
    *CombustionEfficiency_N1 << 110.0 << 6.0;
  }

  bindmodel(PropertyManager.get());
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The main purpose of Calculate() is to determine what phase the engine should
// be in, then call the corresponding function.

void FGTurboProp::Calculate(void)
{
  RunPreFunctions();

  ThrottlePos = in.ThrottlePos[EngineNumber];

  /* The thruster controls the engine RPM because it encapsulates the gear ratio
     and other transmission variables */
  RPM = Thruster->GetEngineRPM();
  if (thrusterType == FGThruster::ttPropeller) {
    ((FGPropeller*)Thruster)->SetAdvance(in.PropAdvance[EngineNumber]);
    ((FGPropeller*)Thruster)->SetFeather(in.PropFeather[EngineNumber]);
    ((FGPropeller*)Thruster)->SetReverse(Reversed);
    if (Reversed) {
      ((FGPropeller*)Thruster)->SetReverseCoef(ThrottlePos);
    } else {
      ((FGPropeller*)Thruster)->SetReverseCoef(0.0);
    }

    if (Reversed) {
      if (ThrottlePos < BetaRangeThrottleEnd) {
          ThrottlePos = 0.0;  // idle when in Beta-range
      } else {
        // when reversed:
        ThrottlePos = (ThrottlePos-BetaRangeThrottleEnd)/(1-BetaRangeThrottleEnd) * ReverseMaxPower;
      }
    }
  }

  // When trimming is finished check if user wants engine OFF or RUNNING
  if ((phase == tpTrim) && (in.TotalDeltaT > 0)) {
    if (Running && !Starved) {
      phase = tpRun;
      N1 = IdleN1;
      OilTemp_degK = 366.0;
      Cutoff = false;
    } else {
      phase = tpOff;
      Cutoff = true;
      Eng_ITT_degC = in.TAT_c;
      Eng_Temperature = in.TAT_c;
      OilTemp_degK = in.TAT_c+273.15;
    }
  }

  if (!Running && Starter) {
    if (phase == tpOff) {
      phase = tpSpinUp;
      if (StartTime < 0) StartTime=0;
    }
  }
  if (!Running && !Cutoff && (N1 > 15.0)) {
    phase = tpStart;
    StartTime = -1;
  }
  if (Cutoff && (phase != tpSpinUp)) phase = tpOff;
  if (in.TotalDeltaT == 0) phase = tpTrim;
  if (Starved) phase = tpOff;
  if (Condition >= 10) {
    phase = tpOff;
    StartTime=-1;
  }

  // limiter intervention wanted?
  if (Ielu_max_torque > 0.0) {
    double torque = 0.0;

    if (thrusterType == FGThruster::ttPropeller) {
      torque = ((FGPropeller*)(Thruster))->GetTorque();
    } else if (thrusterType == FGThruster::ttRotor) {
      torque = ((FGRotor*)(Thruster))->GetTorque();
    }

    if (Condition < 1) {
      if ( abs(torque) > Ielu_max_torque && ThrottlePos >= OldThrottle ) {
        ThrottlePos = OldThrottle - 0.1 * in.TotalDeltaT; //IELU down
        Ielu_intervent = true;
      } else if ( Ielu_intervent && ThrottlePos >= OldThrottle) {
        ThrottlePos = OldThrottle + 0.05 * in.TotalDeltaT; //IELU up
        Ielu_intervent = true;
      } else {
        Ielu_intervent = false;
      }
    } else {
      Ielu_intervent = false;
    }
    OldThrottle = ThrottlePos;
  }

  switch (phase) {
    case tpOff:    HP = Off(); break;
    case tpRun:    HP = Run(); break;
    case tpSpinUp: HP = SpinUp(); break;
    case tpStart:  HP = Start(); break;
    default: HP = 0;
  }

  LoadThrusterInputs();
  // Filters out negative powers when the propeller is not rotating.
  double power = HP * hptoftlbssec;
  if (RPM <= 0.1) power = max(power, 0.0);
  Thruster->Calculate(power);

  RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::Off(void)
{
  Running = false; EngStarting = false;

  FuelFlow_pph = Seek(&FuelFlow_pph, 0, 800.0, 800.0);

  //allow the air turn with generator
  N1 = ExpSeek(&N1, in.qbar/15.0, Idle_Max_Delay*2.5, Idle_Max_Delay * 5);

  OilTemp_degK = ExpSeek(&OilTemp_degK,273.15 + in.TAT_c, 400 , 400);

  Eng_Temperature = ExpSeek(&Eng_Temperature,in.TAT_c,300,400);
  double ITT_goal = ITT_N1->GetValue(N1,0.1) + ((N1>20) ? 0.0 : (20-N1)/20.0 * Eng_Temperature);
  Eng_ITT_degC  = ExpSeek(&Eng_ITT_degC,ITT_goal,ITT_Delay,ITT_Delay*1.2);

  OilPressure_psi = (N1/100.0*0.25+(0.1-(OilTemp_degK-273.15)*0.1/80.0)*N1/100.0) / 7692.0e-6; //from MPa to psi

  if (RPM>5) return -0.012; // friction in engine when propeller spining (estimate)
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::Run(void)
{
  double EngPower_HP;

  Running = true; Starter = false; EngStarting = false;

//---
  double old_N1 = N1;
  N1 = ExpSeek(&N1, IdleN1 + ThrottlePos * N1_factor, Idle_Max_Delay, Idle_Max_Delay * 2.4);

  EngPower_HP = EnginePowerRPM_N1->GetValue(RPM,N1);
  EngPower_HP *= EnginePowerVC->GetValue();
  if (EngPower_HP > MaxPower) EngPower_HP = MaxPower;

  CombustionEfficiency = CombustionEfficiency_N1->GetValue(N1);
  FuelFlow_pph = PSFC / CombustionEfficiency * EngPower_HP;

  Eng_Temperature = ExpSeek(&Eng_Temperature,Eng_ITT_degC,300,400);
  double ITT_goal = ITT_N1->GetValue((N1-old_N1)*300+N1,1);
  Eng_ITT_degC  = ExpSeek(&Eng_ITT_degC,ITT_goal,ITT_Delay,ITT_Delay*1.2);

  OilPressure_psi = (N1/100.0*0.25+(0.1-(OilTemp_degK-273.15)*0.1/80.0)*N1/100.0) / 7692.0e-6; //from MPa to psi
//---

  OilTemp_degK = Seek(&OilTemp_degK, 353.15, 0.4-N1*0.001, 0.04);

  if (Cutoff) phase = tpOff;
  if (Starved) phase = tpOff;

  return EngPower_HP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::SpinUp(void)
{
  double EngPower_HP;
  Running = false; EngStarting = true;
  FuelFlow_pph = 0.0;

  if (!GeneratorPower) {
    EngStarting=false;
    phase=tpOff;
    StartTime = -1;
    return 0.0;
  }

  N1 = ExpSeek(&N1, StarterN1, Idle_Max_Delay * 6, Idle_Max_Delay * 2.4);

  Eng_Temperature = ExpSeek(&Eng_Temperature,in.TAT_c,300,400);
  double ITT_goal = ITT_N1->GetValue(N1,0.1) + ((N1>20) ? 0.0 : (20-N1)/20.0 * Eng_Temperature);
  Eng_ITT_degC  = ExpSeek(&Eng_ITT_degC,ITT_goal,ITT_Delay,ITT_Delay*1.2);

  OilTemp_degK = ExpSeek(&OilTemp_degK,273.15 + in.TAT_c, 400 , 400);

  OilPressure_psi = (N1/100.0*0.25+(0.1-(OilTemp_degK-273.15)*0.1/80.0)*N1/100.0) / 7692.0e-6; //from MPa to psi

  EngPower_HP = EnginePowerRPM_N1->GetValue(RPM,N1);
  EngPower_HP *= EnginePowerVC->GetValue();
  if (EngPower_HP > MaxPower) EngPower_HP = MaxPower;

  if (StartTime>=0) StartTime+=in.TotalDeltaT;
  if (StartTime > MaxStartingTime && MaxStartingTime > 0) { //start failed due timeout
    phase = tpOff;
    StartTime = -1;
  }

  return EngPower_HP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::Start(void)
{
  double EngPower_HP = 0.0;

  EngStarting = false;
  if ((N1 > 15.0) && !Starved) {       // minimum 15% N1 needed for start
    double old_N1 = N1;
    Cranking = true;                   // provided for sound effects signal
    if (N1 < IdleN1) {
      EngPower_HP = EnginePowerRPM_N1->GetValue(RPM,N1);
      EngPower_HP *= EnginePowerVC->GetValue();
      if (EngPower_HP > MaxPower) EngPower_HP = MaxPower;
      N1 = ExpSeek(&N1, IdleN1*1.1, Idle_Max_Delay*4, Idle_Max_Delay * 2.4);
      CombustionEfficiency = CombustionEfficiency_N1->GetValue(N1);
      FuelFlow_pph = PSFC / CombustionEfficiency * EngPower_HP;
      Eng_Temperature = ExpSeek(&Eng_Temperature,Eng_ITT_degC,300,400);
      double ITT_goal = ITT_N1->GetValue((N1-old_N1)*300+N1,1);
      Eng_ITT_degC  = ExpSeek(&Eng_ITT_degC,ITT_goal,ITT_Delay,ITT_Delay*1.2);

      OilPressure_psi = (N1/100.0*0.25+(0.1-(OilTemp_degK-273.15)*0.1/80.0)*N1/100.0) / 7692.0e-6; //from MPa to psi
      OilTemp_degK = Seek(&OilTemp_degK, 353.15, 0.4-N1*0.001, 0.04);

    } else {
      phase = tpRun;
      Running = true;
      Starter = false;
      Cranking = false;
      FuelFlow_pph = 0;
    }
  } else {                 // no start if N1 < 15% or Starved
    phase = tpOff;
    Starter = false;
  }

  return EngPower_HP;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::CalcFuelNeed(void)
{
  FuelFlowRate = FuelFlow_pph / 3600.0;
  FuelExpended = FuelFlowRate * in.TotalDeltaT;
  if (!Starved) FuelUsedLbs += FuelExpended;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::Seek(double *var, double target, double accel, double decel)
{
  double v = *var;
  if (v > target) {
    v -= in.TotalDeltaT * decel;
    if (v < target) v = target;
  } else if (v < target) {
    v += in.TotalDeltaT * accel;
    if (v > target) v = target;
  }
  return v;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboProp::ExpSeek(double *var, double target, double accel_tau, double decel_tau)
{
// exponential delay instead of the linear delay used in Seek
  double v = *var;
  if (v > target) {
    v = (v - target) * exp ( -in.TotalDeltaT / decel_tau) + target;
  } else if (v < target) {
    v = (target - v) * (1 - exp ( -in.TotalDeltaT / accel_tau)) + v;
  }
  return v;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurboProp::SetDefaults(void)
{
//  Name = "Not defined";
  N1 = 0.0;
  HP = 0.0;
  Type = etTurboprop;
  IdleN1 = 30.0;
  MaxN1 = 100.0;
  Reversed = false;
  Cutoff = true;
  phase = tpOff;
  Eng_ITT_degC = 0.0;

  GeneratorPower=true;
  Condition = 0;
  Ielu_intervent=false;

  Idle_Max_Delay = 1.0;

  ThrottlePos = OldThrottle = 0.0;
  ITT_Delay = 0.05;
  ReverseMaxPower = 0.0;
  BetaRangeThrottleEnd = 0.0;
  CombustionEfficiency = 1.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


string FGTurboProp::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << "_N1[" << EngineNumber << "]" << delimiter
      << Name << "_PwrAvail[" << EngineNumber << "]" << delimiter
      << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGTurboProp::GetEngineValues(const string& delimiter)
{
  std::ostringstream buf;

  buf << N1 << delimiter
      << HP << delimiter
      << Thruster->GetThrusterValues(EngineNumber,delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGTurboProp::InitRunning(void)
{
  double dt = in.TotalDeltaT;
  in.TotalDeltaT = 0.0;
  Cutoff=false;
  Running=true;
  Calculate();
  in.TotalDeltaT = dt;
  return phase==tpRun;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurboProp::bindmodel(FGPropertyManager* PropertyManager)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  property_name = base_property_name + "/n1";
  PropertyManager->Tie( property_name.c_str(), &N1);
  property_name = base_property_name + "/reverser";
  PropertyManager->Tie( property_name.c_str(), &Reversed);
  property_name = base_property_name + "/power-hp";
  PropertyManager->Tie( property_name.c_str(), &HP);
  property_name = base_property_name + "/itt-c";
  PropertyManager->Tie( property_name.c_str(), &Eng_ITT_degC);
  property_name = base_property_name + "/engtemp-c";
  PropertyManager->Tie( property_name.c_str(), &Eng_Temperature);
  property_name = base_property_name + "/ielu_intervent";
  PropertyManager->Tie( property_name.c_str(), &Ielu_intervent);
  property_name = base_property_name + "/combustion_efficiency";
  PropertyManager->Tie( property_name.c_str(), &CombustionEfficiency);
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

void FGTurboProp::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
    if (from == 2) { // called from Load()
      cout << "\n ****MUJ MOTOR TURBOPROP****\n";
      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      IdleN1:      "         << IdleN1 << endl;
      cout << "      MaxN1:       "         << MaxN1 << endl;

      cout << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTurboProp" << endl;
    if (from == 1) cout << "Destroyed:    FGTurboProp" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
