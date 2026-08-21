/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTurboshaft.cpp
 Purpose:      This module models a turboshaft engine.

 ------------- Copyright (C) 2026 JSBSim contributors -------------

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

This class descends from the FGEngine class and models a turboshaft engine
based on parameters given in the engine config file for this class. See
FGTurboshaft.h for how this differs from FGTurboProp, the class it is derived
from.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>
#include <cmath>

#include "FGTurboshaft.h"
#include "FGRotor.h"
#include "FGGearbox.h"
#include "math/FGFunction.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTurboshaft::FGTurboshaft(FGFDMExec* exec, Element *el, int engine_number, struct Inputs& input)
  : FGEngine(engine_number, input)
{
  SetDefaults();
  Load(exec, el);
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTurboshaft::Load(FGFDMExec* exec, Element *el)
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

  string property_prefix = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  EnginePowerVC = GetPreFunction(property_prefix+"/EnginePowerVC");

  if (el->FindElement("idlen1"))
    IdleN1 = el->FindElementValueAsNumber("idlen1");
  if (el->FindElement("maxn1"))
    MaxN1 = el->FindElementValueAsNumber("maxn1");

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
      // Get a different name for each engine otherwise FGTable::bind() will
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
           << " in turboshaft definition." << endl;
    }
    table_element = el->FindNextElement("table");
  }

  // Pre-calculations and initializations

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

void FGTurboshaft::Calculate(void)
{
  RunPreFunctions();

  ThrottlePos = in.ThrottlePos[EngineNumber];

  /* The thruster (gearbox, or a directly-attached rotor) controls the engine
     RPM because it encapsulates the gear ratio and other transmission
     variables. No propeller/beta/reverse handling here -- a turboshaft has
     none of that; see FGTurboshaft.h. */
  RPM = Thruster->GetEngineRPM();

  // When trimming is finished check if user wants engine OFF or RUNNING
  if ((phase == tsTrim) && (in.TotalDeltaT > 0)) {
    if (Running && !Starved) {
      phase = tsRun;
      N1 = IdleN1;
      OilTemp_degK = 366.0;
      Cutoff = false;
    } else {
      phase = tsOff;
      Cutoff = true;
      Eng_ITT_degC = in.TAT_c;
      Eng_Temperature = in.TAT_c;
      OilTemp_degK = in.TAT_c+273.15;
    }
  }

  if (!Running && Starter) {
    if (phase == tsOff) {
      phase = tsSpinUp;
      if (StartTime < 0) StartTime=0;
    }
  }
  if (!Running && !Cutoff && (N1 > 15.0)) {
    phase = tsStart;
    StartTime = -1;
  }
  if (Cutoff && (phase != tsSpinUp)) phase = tsOff;
  if (in.TotalDeltaT == 0) phase = tsTrim;
  if (Starved) phase = tsOff;
  if (Condition >= 10) {
    phase = tsOff;
    StartTime=-1;
  }

  // limiter intervention wanted? (IELU = Integrated Electronic Limiter Unit;
  // disabled by default, ielumaxtorque <= 0 -- see mi17_main_engine_1.xml's
  // own comment on why: torque = power/omega spikes enormously at low RPM,
  // real physics during spin-up, not something a single fixed threshold
  // should be tripping on.) Reads the thruster's own torque directly, same
  // as FGTurboProp.cpp, but only ever the FGRotor case: a turboshaft has no
  // propeller.
  if (Ielu_max_torque > 0.0) {
    double torque = 0.0;
    if (Thruster && Thruster->GetType() == FGThruster::ttRotor)
      torque = ((FGRotor*)(Thruster))->GetTorque();

    if (Condition < 1) {
      if ( fabs(torque) > Ielu_max_torque && ThrottlePos >= OldThrottle ) {
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
    case tsOff:    HP = Off(); break;
    case tsRun:    HP = Run(); break;
    case tsSpinUp: HP = SpinUp(); break;
    case tsStart:  HP = Start(); break;
    default: HP = 0;
  }

  LoadThrusterInputs();
  double power = HP * hptoftlbssec;
  if (RPM <= 0.1) power = max(power, 0.0);
  // A gearbox-fed engine hands its power to its channel instead of driving
  // its (shared) Thruster directly; see FGTurboProp.cpp's identical comment
  // and docs/interfaces/twin-engine-gearbox-interface.md section 3.
  if (FeedsGearbox())
    GetGearbox()->SetChannelPower(EngineNumber, power);
  else
    Thruster->Calculate(power);

  RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::Off(void)
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

  if (RPM>5) return -0.012; // friction in engine when shaft spinning (estimate)
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::Run(void)
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

  if (Cutoff) phase = tsOff;
  if (Starved) phase = tsOff;

  return EngPower_HP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::SpinUp(void)
{
  double EngPower_HP;
  Running = false; EngStarting = true;
  FuelFlow_pph = 0.0;

  if (!GeneratorPower) {
    EngStarting=false;
    phase=tsOff;
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
    phase = tsOff;
    StartTime = -1;
  }

  return EngPower_HP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::Start(void)
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
      phase = tsRun;
      Running = true;
      Starter = false;
      Cranking = false;
      FuelFlow_pph = 0;
    }
  } else {                 // no start if N1 < 15% or Starved
    phase = tsOff;
    Starter = false;
  }

  return EngPower_HP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::CalcFuelNeed(void)
{
  FuelFlowRate = FuelFlow_pph / 3600.0;
  FuelExpended = FuelFlowRate * in.TotalDeltaT;
  if (!Starved) FuelUsedLbs += FuelExpended;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTurboshaft::Seek(double *var, double target, double accel, double decel)
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

double FGTurboshaft::ExpSeek(double *var, double target, double accel_tau, double decel_tau)
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

void FGTurboshaft::SetDefaults(void)
{
  N1 = 0.0;
  HP = 0.0;
  Type = etTurboprop;   // no etTurboshaft type exists in FGEngine; this only
                         // affects diagnostic labeling elsewhere, not behavior.
  IdleN1 = 30.0;
  MaxN1 = 100.0;
  Cutoff = true;
  phase = tsOff;
  Eng_ITT_degC = 0.0;

  GeneratorPower=true;
  Condition = 0;
  Ielu_intervent=false;

  Idle_Max_Delay = 1.0;

  ThrottlePos = OldThrottle = 0.0;
  ITT_Delay = 0.05;
  CombustionEfficiency = 1.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGTurboshaft::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << "_N1[" << EngineNumber << "]" << delimiter
      << Name << "_PwrAvail[" << EngineNumber << "]" << delimiter
      << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGTurboshaft::GetEngineValues(const string& delimiter)
{
  std::ostringstream buf;

  buf << N1 << delimiter
      << HP << delimiter
      << Thruster->GetThrusterValues(EngineNumber,delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGTurboshaft::InitRunning(void)
{
  double dt = in.TotalDeltaT;
  in.TotalDeltaT = 0.0;
  Cutoff=false;
  Running=true;
  Calculate();
  in.TotalDeltaT = dt;
  return phase==tsRun;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTurboshaft::bindmodel(FGPropertyManager* PropertyManager)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  property_name = base_property_name + "/n1";
  PropertyManager->Tie( property_name.c_str(), &N1);
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
  // The functional additions over FGTurboProp: without these, nothing can
  // ever move this engine off the base FGEngine::Starter flag or the
  // Cutoff flag, so the realistic tsOff->tsSpinUp->tsStart->tsRun sequence
  // in Calculate() above is unreachable from a script; see this file's
  // header comment. Both are needed, matching a real turboshaft's start
  // procedure: starter-cmd alone only motors the shaft (SpinUp() with no
  // fuel), asymptoting toward StarterN1 and no further -- confirmed the
  // hard way, running a starter-only diagnostic that N1 plateaus at
  // StarterN1 forever, because SpinUp() never touches Cutoff and the
  // tsSpinUp->tsStart transition is explicitly gated on "!Cutoff" (see
  // Calculate()). Releasing cutoff-cmd (0 = fuel/ignition introduced) is
  // what actually lets N1 climb past StarterN1 into a self-sustaining
  // light-off, exactly like moving a real condition lever out of CUTOFF
  // once the starter has the engine motoring.
  property_name = base_property_name + "/starter-cmd";
  PropertyManager->Tie( property_name.c_str(), static_cast<FGEngine*>(this),
                         &FGEngine::GetStarter, &FGEngine::SetStarter );
  property_name = base_property_name + "/cutoff-cmd";
  PropertyManager->Tie( property_name.c_str(), this,
                         &FGTurboshaft::GetCutoff, &FGTurboshaft::SetCutoff );
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

void FGTurboshaft::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 2) { // called from Load()
      cout << "\n    Turboshaft Engine\n";
      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      IdleN1:      "         << IdleN1 << endl;
      cout << "      MaxN1:       "         << MaxN1 << endl;
      cout << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTurboshaft" << endl;
    if (from == 1) cout << "Destroyed:    FGTurboshaft" << endl;
  }
}
}
