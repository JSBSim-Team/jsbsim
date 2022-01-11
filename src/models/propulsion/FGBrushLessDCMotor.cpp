/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGBrushLessDCMotor.cpp
 Autor         Paolo Becchi
 1st release   1/1/2022
 Purpose:      This module models an BLDC electric motor

  ------------- Copyright (C) 2022  Paolo Becchi (pbecchi@aerobusinees.it) -------------

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  Further information about the GNU Lesser General Public License can also be
  found on the world wide web at http://www.gnu.org

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Following code represent a new BrushLess DC motor to be used as alternative 
to basic electric motor. 
BLDC motor code is based on basic "3 constant motor equations"
It require 3 basic physical motor properties:
Kv speed motor constant      [RPM/Volt]
Rm internal coil resistance  [Ohms]
I0 no load current           [Amperes]

HISTORY
--------------------------------------------------------------------------------
1/01/2022    Created


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>
#include <math.h>

#include "FGFDMExec.h"
#include "FGBrushLessDCMotor.h"
#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGBrushLessDCMotor::FGBrushLessDCMotor(FGFDMExec* exec, Element* el, int engine_number, struct FGEngine::Inputs& input)
  : FGEngine(engine_number, input)
{
  Load(exec, el);

  Type = etElectric;



  if (el->FindElement("maxvolts"))
    MaxVolts = el->FindElementValueAsNumberConvertTo("maxvolts", "VOLTS");
  else {
    cerr << el->ReadFrom()
         << "<maxvolts> is a mandatory parameter" << endl;
    throw JSBBaseException("Missing parameter");
  }

  if (el->FindElement("velocityconstant"))
    VelocityConstant = el->FindElementValueAsNumber("velocityconstant");
  else {
    cerr << el->ReadFrom()
         << "<velocityconstant> is a mandatory parameter" << endl;
    throw JSBBaseException("Missing parameter");
  }

  if (el->FindElement("coilresistance"))
    CoilResistance = el->FindElementValueAsNumberConvertTo("coilresistance", "OHMS");
  else {
    cerr << el->ReadFrom()
         << "<coilresistance> is a mandatory parameter" << endl;
    throw JSBBaseException("Missing parameter");
  }
  if (el->FindElement("noloadcurrent"))
    NoLoadCurrent = el->FindElementValueAsNumberConvertTo("noloadcurrent", "AMPERES");
  else {
    cerr << el->ReadFrom()
         << "<noloadcurrent> is a mandatory parameter" << endl;
    throw JSBBaseException("Missing parameter");
  }
  if (el->FindElement("deceleration_factor"))
    DecelerationFactor = el->FindElementValueAsNumber("deceleration_factor");
  else {
    cout << el->ReadFrom()
         << "Using default value " << DecelerationFactor << " for <deceleration_factor>" << endl;
  }

  MaxCurrent = MaxVolts / CoilResistance + NoLoadCurrent;

  PowerWatts = MaxCurrent * MaxVolts;

  string base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  exec->GetPropertyManager()->Tie(base_property_name + "/power-hp", &HP);

  exec->GetPropertyManager()->Tie(base_property_name + "/current-amperes", &CurrentRequired);

  Debug(0); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGBrushLessDCMotor::~FGBrushLessDCMotor()
{
  Debug(1); // Call Debug() routine from constructor if needed
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGBrushLessDCMotor::Calculate(void)
{
  RunPreFunctions();

  if (Thruster->GetType() == FGThruster::ttPropeller) {
    ((FGPropeller*)Thruster)->SetAdvance(in.PropAdvance[EngineNumber]);
    ((FGPropeller*)Thruster)->SetFeather(in.PropFeather[EngineNumber]);
  }

  RPM = Thruster->GetRPM();
  TorqueRequired = abs(((FGPropeller*)Thruster)->GetTorque()); //units [#*ft]

  CurrentRequired = (TorqueRequired * VelocityConstant) / TorqueConstant;

  //  total current required must include no load current i0
  CurrentRequired = CurrentRequired + NoLoadCurrent;
 
  V = MaxVolts * in.ThrottlePos[EngineNumber];
  
  //  Delta RPM = (input voltage - currentRequired * coil resistance) * velocity costant
  DeltaRPM = round((V - CurrentRequired * CoilResistance) * VelocityConstant);

  //  Torque is MaxTorque (stall torque) at 0 RPM and linearly go to 0 at max RPM (MaxVolts*VelocityCostant)
  //  MaxTorque = MaxCurrent*torqueconstant/velocityconstant*(1-RPM/maxRPM)

  MaxTorque = MaxCurrent / VelocityConstant * TorqueConstant * (1 - RPM / (MaxVolts* VelocityConstant));

  TorqueAvailable = MaxTorque - TorqueRequired;
  InertiaTorque = (((DeltaRPM/60)*(2.0 * M_PI))/(max(0.00001, in.TotalDeltaT))) * ((FGPropeller*)Thruster)->GetIxx();
 
  //  compute acceleration and deceleration phases:
  //  Acceleration is due to the max delta torque available and is limited to the inertial forces

  if (DeltaRPM >= 0) {
    TargetTorque = min(InertiaTorque, TorqueAvailable) + TorqueRequired;
  } else {
  //  Deceleration is due to braking force given by the ESC and set by parameter deceleration_time 
    TargetTorque = TorqueRequired - min(abs(InertiaTorque)/(max(DecelerationFactor,0.01)*30),RPM*TorqueConstant/VelocityConstant/VelocityConstant/CoilResistance);
  }

  EnginePower = ((2 * M_PI) * max(RPM, 0.0001) * TargetTorque) / 60;   //units [#*ft/s]
  HP = EnginePower /hptowatts*NMtoftpound;                             // units[HP]
  LoadThrusterInputs();
  Thruster->Calculate(EnginePower);

  RunPostFunctions();
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGBrushLessDCMotor::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << " HP (engine " << EngineNumber << ")" << delimiter
    << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGBrushLessDCMotor::GetEngineValues(const string& delimiter)
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

void FGBrushLessDCMotor::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      Power Watts: "         << PowerWatts << endl;

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGBrushLessDCMotor" << endl;
    if (from == 1) cout << "Destroyed:    FGBrushLessDCMotor" << endl;
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
