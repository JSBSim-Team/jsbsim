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

REFERENCE:
http://web.mit.edu/drela/Public/web/qprop/motor1_theory.pdf

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
    throw BaseException("Missing parameter");
  }

  if (el->FindElement("velocityconstant"))
    Kv = el->FindElementValueAsNumber("velocityconstant");
  else {
    cerr << el->ReadFrom()
         << "<velocityconstant> is a mandatory parameter" << endl;
    throw BaseException("Missing parameter");
  }

  if (el->FindElement("coilresistance"))
    CoilResistance = el->FindElementValueAsNumberConvertTo("coilresistance", "OHMS");
  else {
    cerr << el->ReadFrom()
         << "<coilresistance> is a mandatory parameter" << endl;
    throw BaseException("Missing parameter");
  }
  if (el->FindElement("noloadcurrent"))
    ZeroTorqueCurrent = el->FindElementValueAsNumberConvertTo("noloadcurrent", "AMPERES");
  else {
    cerr << el->ReadFrom()
         << "<noloadcurrent> is a mandatory parameter" << endl;
    throw BaseException("Missing parameter");
  }

  double MaxCurrent = MaxVolts / CoilResistance + ZeroTorqueCurrent;

  PowerWatts = MaxCurrent * MaxVolts;

  string base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  auto pm = exec->GetPropertyManager();
  pm->Tie(base_property_name + "/power-hp", &HP);
  pm->Tie(base_property_name + "/current-amperes", &Current);

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

  double RPM = Thruster->GetRPM();
  double V = MaxVolts * in.ThrottlePos[EngineNumber];

  Current = (V - RPM / Kv) / CoilResistance; // Equation (4) from Drela's document

  // Compute torque from current with Kq=1/Kv considering NoLoadCurrent deadband
  // The "zero torque current" is by definition the current necessary for the
  // motor to overcome internal friction : it is always resisting the torque and
  // consequently has an opposite to the current.

  double Torque = 0;

  if (Current >= ZeroTorqueCurrent)
    Torque = (Current - ZeroTorqueCurrent) / Kv * WattperRPMtoftpound;
  if (Current<=-ZeroTorqueCurrent)
    Torque = (Current + ZeroTorqueCurrent) / Kv * WattperRPMtoftpound;

  // EnginePower must be non zero when accelerating from RPM == 0.0
  double EnginePower = ((2 * M_PI) * max(RPM, 0.0001) * Torque) / 60;  //units [#*ft/s]
  HP = EnginePower / hptowatts * NMtoftpound;  // units[HP]
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

      cout << "\n    Engine Name:        " << Name << endl;
      cout << "      Power Watts:        " << PowerWatts << endl;
      cout << "      Speed Factor:       " << Kv << endl;
      cout << "      Coil Resistance:    " << CoilResistance << endl;
      cout << "      NoLoad Current:     " << ZeroTorqueCurrent << endl;
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
