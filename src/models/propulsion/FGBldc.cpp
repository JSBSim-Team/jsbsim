/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGBldc.cpp
 Original Author:         Matt Vacanti
 Corrections and updates: Paolo Becchi
 Date started: 11/08/2020
 Corrected   : 26/12/2021
 Purpose:      This module models an DLDC electric motor

 --------- Copyright (C) 2020  Matt Vacanti -------------

UPDATE LICENSE

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

UPDATE FUNCTIONAL DESCRIPTION

HISTORY
--------------------------------------------------------------------------------
11/08/2020  MDV  Created


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>
#include <math.h>

#include "FGFDMExec.h"
#include "FGBldc.h"
#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGBldc::FGBldc(FGFDMExec* exec, Element *el, int engine_number, struct FGEngine::Inputs& input)
  : FGEngine(engine_number, input)
{
  Load(exec,el);

  Type = etElectric;
  PowerWatts = 745.7;
  hptowatts = 745.7;

// this property is not necessary since is computed using other properties
//  if (el->FindElement("maxcurrent"))
//    MaxCurrent= el->FindElementValueAsNumber("maxcurrent");

  if (el->FindElement("maxvolts"))
    MaxVolts= el->FindElementValueAsNumber("maxvolts");

  if (el->FindElement("velocityconstant"))
    VelocityConstant= el->FindElementValueAsNumber("velocityconstant");

  if (el->FindElement("torqueconstant"))
    TorqueConstant= el->FindElementValueAsNumber("torqueconstant");
  
  
  // added coilresistance and noload current properties
  
  if (el->FindElement("coilresistance"))
      coilResistance = el->FindElementValueAsNumber("coilresistance");
  if (el->FindElement("noloadcurrent"))
      noLoadCurrent = el->FindElementValueAsNumber("noloadcurrent");
  if (el->FindElement("decelerationTime"))
      deceleration_time = el->FindElementValueAsNumber("decelerationTime");
 
  MaxCurrent = MaxVolts / coilResistance + noLoadCurrent;
  // end of additions

  PowerWatts = MaxCurrent * MaxVolts;

  string base_property_name = CreateIndexedPropertyName("propulsion/engine",
                                                        EngineNumber);
  exec->GetPropertyManager()->Tie(base_property_name + "/power-hp", &HP);

  exec->GetPropertyManager()->Tie(base_property_name + "/current-a", &CurrentRequired);

  Debug(0); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGBldc::~FGBldc()
{
  Debug(1); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGBldc::Calculate(void)
{
  RunPreFunctions();

  if (Thruster->GetType() == FGThruster::ttPropeller) {
    ((FGPropeller*)Thruster)->SetAdvance(in.PropAdvance[EngineNumber]);
    ((FGPropeller*)Thruster)->SetFeather(in.PropFeather[EngineNumber]);
  }

  RPM = Thruster->GetRPM();
  //TODO Add gear ratio / transmission support.

  TorqueRequired = abs(((FGPropeller*)Thruster)->GetTorque());

  CurrentRequired = (TorqueRequired * VelocityConstant) / TorqueConstant;

// total current required must include no load current i0
  CurrentRequired = CurrentRequired + noLoadCurrent;
 
  V = MaxVolts * in.ThrottlePos[EngineNumber];
  
// Commanded RPM = (input voltage - currentRequired * coil resistance) * velocity costant
    CommandedRPM = (V - CurrentRequired * coilResistance) * VelocityConstant;
 
  
    DeltaRPM = round((CommandedRPM - RPM));


//     Torque is MaxTorque (stall torque) at 0 RPM and linearly go to 0 at max RPM (MaxVolts*VelocityCostant)
//     MaxTorque = MaxCurrent*torqueconstant/velocityconstant*(1-RPM/maxRPM)
    MaxTorque = MaxCurrent / VelocityConstant * TorqueConstant * (1 - RPM / (MaxVolts* VelocityConstant));
  
    TorqueAvailable = MaxTorque - TorqueRequired;
    DeltaTorque = (((DeltaRPM/60)*(2.0 * M_PI))/(max(0.00001, in.TotalDeltaT))) * ((FGPropeller*)Thruster)->GetIxx();
 
//      compute acceleration and deceleration phases:
//     Acceleration is due to the max delta torque available and is limited to the inertial forces
    if (DeltaRPM >= 0) {
      TargetTorque = min(DeltaTorque, TorqueAvailable) + TorqueRequired;
    } else {
//    Deceleration is due to braking force given by the ESC and set by parameter deceleration_time 
      TargetTorque = TorqueRequired  - min(abs(DeltaTorque)/(max(deceleration_time,0.01)*30),RPM*TorqueConstant/VelocityConstant/VelocityConstant/coilResistance);
    }

    EnginePower = ((2 * M_PI) * max(RPM, 0.0001) * TargetTorque) / 60;
    HP = EnginePower / 550;
    LoadThrusterInputs();
    Thruster->Calculate(EnginePower);

    RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGBldc::CalcFuelNeed(void)
{
  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGBldc::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << " HP (engine " << EngineNumber << ")" << delimiter
      << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGBldc::GetEngineValues(const string& delimiter)
{
  std::ostringstream buf;

  buf << HP << delimiter
     << Thruster->GetThrusterValues(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
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

void FGBldc::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      Power Watts: "         << PowerWatts << endl;

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGBldc" << endl;
    if (from == 1) cout << "Destroyed:    FGBldc" << endl;
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

} // namespace JSBSim
