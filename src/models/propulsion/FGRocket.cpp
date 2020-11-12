/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRocket.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models a rocket engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) --------------

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
 found on the world wide web at http://www.gnu.org.

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

#include "FGFDMExec.h"
#include "FGRocket.h"
#include "FGThruster.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRocket::FGRocket(FGFDMExec* exec, Element *el, int engine_number, struct Inputs& input)
  : FGEngine(engine_number, input), isp_function(nullptr), FDMExec(exec)
{
  Load(exec, el);

  Type = etRocket;
  Element* thrust_table_element = nullptr;
  ThrustTable = nullptr;
  BurnTime = 0.0;
  previousFuelNeedPerTank = 0.0;
  previousOxiNeedPerTank = 0.0;
  PropellantFlowRate = 0.0;
  TotalPropellantExpended = 0.0;
  FuelFlowRate = FuelExpended = 0.0;
  OxidizerFlowRate = OxidizerExpended = 0.0;
  SLOxiFlowMax = SLFuelFlowMax = PropFlowMax = 0.0;
  MxR = 0.0;
  BuildupTime = 0.0;
  It = ItVac = 0.0;
  ThrustVariation = 0.0;
  TotalIspVariation = 0.0;
  VacThrust = 0.0;
  Flameout = false;

  // Defaults
   MinThrottle = 0.0;
   MaxThrottle = 1.0;

  std::stringstream strEngineNumber;
  strEngineNumber << EngineNumber;

  auto PropertyManager = exec->GetPropertyManager();
  bindmodel(PropertyManager.get()); // Bind model properties first, since they might be needed in functions.

  Element* isp_el = el->FindElement("isp");

  // Specific impulse may be specified as a constant value or as a function -
  // perhaps as a function of mixture ratio.
  if (isp_el) {
    Element* isp_func_el = isp_el->FindElement("function");
    if (isp_func_el) {
      isp_function = new FGFunction(exec, isp_func_el, strEngineNumber.str());
    } else {
      Isp = el->FindElementValueAsNumber("isp");
    }
  } else {
    throw("Specific Impulse <isp> must be specified for a rocket engine");
  }
  
  if (el->FindElement("builduptime"))
    BuildupTime = el->FindElementValueAsNumber("builduptime");
  if (el->FindElement("maxthrottle"))
    MaxThrottle = el->FindElementValueAsNumber("maxthrottle");
  if (el->FindElement("minthrottle"))
    MinThrottle = el->FindElementValueAsNumber("minthrottle");

  if (el->FindElement("slfuelflowmax")) {
    SLFuelFlowMax = el->FindElementValueAsNumberConvertTo("slfuelflowmax", "LBS/SEC");
    if (el->FindElement("sloxiflowmax")) {
    SLOxiFlowMax = el->FindElementValueAsNumberConvertTo("sloxiflowmax", "LBS/SEC");
    }
    PropFlowMax = SLOxiFlowMax + SLFuelFlowMax;
    MxR = SLOxiFlowMax/SLFuelFlowMax;
  } else if (el->FindElement("propflowmax")) {
    PropFlowMax = el->FindElementValueAsNumberConvertTo("propflowmax", "LBS/SEC");
    // Mixture ratio may be specified here, but it can also be specified as a
    // function or via property
    if (el->FindElement("mixtureratio")) {
      MxR = el->FindElementValueAsNumber("mixtureratio");
    }
  }

  if (isp_function) Isp = isp_function->GetValue(); // cause Isp function to be executed if present.
  // If there is a thrust table element, this is a solid propellant engine.
  thrust_table_element = el->FindElement("thrust_table");
  if (thrust_table_element) {
    ThrustTable = new FGTable(PropertyManager, thrust_table_element);
    Element* variation_element = el->FindElement("variation");
    if (variation_element) {
      if (variation_element->FindElement("thrust")) {
        ThrustVariation = variation_element->FindElementValueAsNumber("thrust");
      }
      if (variation_element->FindElement("total_isp")) {
        TotalIspVariation = variation_element->FindElementValueAsNumber("total_isp");
      }
    }
  }


  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRocket::~FGRocket(void)
{
  delete ThrustTable;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGRocket::Calculate(void)
{
  if (FDMExec->IntegrationSuspended()) return;

  RunPreFunctions();

  PropellantFlowRate = (FuelExpended + OxidizerExpended)/in.TotalDeltaT;
  TotalPropellantExpended += FuelExpended + OxidizerExpended;
  // If Isp has been specified as a function, override the value of Isp to that,
  // otherwise assume a constant value is given.
  if (isp_function) Isp = isp_function->GetValue();

  // If there is a thrust table, it is a function of propellant burned. The
  // engine is started when the throttle is advanced to 1.0. After that, it
  // burns without regard to throttle setting.

  if (ThrustTable != 0L) { // Thrust table given -> Solid fuel used

    if ((in.ThrottlePos[EngineNumber] == 1 || BurnTime > 0.0 ) && !Starved) {

      VacThrust = ThrustTable->GetValue(TotalPropellantExpended)
                * (ThrustVariation + 1)
                * (TotalIspVariation + 1);
      if (BurnTime <= BuildupTime && BuildupTime > 0.0) {
        VacThrust *= sin((BurnTime/BuildupTime)*M_PI/2.0);
        // VacThrust *= (1-cos((BurnTime/BuildupTime)*M_PI))/2.0; // 1 - cos approach
      }
      BurnTime += in.TotalDeltaT; // Increment burn time
    } else {
      VacThrust = 0.0;
    }

  } else { // liquid fueled rocket assumed

    if (in.ThrottlePos[EngineNumber] < MinThrottle || Starved) { // Combustion not supported

      PctPower = 0.0; // desired thrust
      Flameout = true;
      VacThrust = 0.0;

    } else { // Calculate thrust

      // PctPower = Throttle / MaxThrottle; // Min and MaxThrottle range from 0.0 to 1.0, normally.
      
      PctPower = in.ThrottlePos[EngineNumber];
      Flameout = false;
      VacThrust = Isp * PropellantFlowRate;

    }

  } // End thrust calculations

  LoadThrusterInputs();
  It += Thruster->Calculate(VacThrust) * in.TotalDeltaT;
  ItVac += VacThrust * in.TotalDeltaT;

  RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 
// The FuelFlowRate can be affected by the TotalIspVariation value (settable
// in a config file or via properties). The TotalIspVariation parameter affects
// thrust, but the thrust determines fuel flow rate, so it must be adjusted
// for Total Isp Variation.

double FGRocket::CalcFuelNeed(void)
{
  if (ThrustTable != 0L) {          // Thrust table given - infers solid fuel
    FuelFlowRate = VacThrust/Isp;   // This calculates wdot (weight flow rate in lbs/sec)
    FuelFlowRate /= (1 + TotalIspVariation);
  } else {
    SLFuelFlowMax = PropFlowMax / (1 + MxR);
    FuelFlowRate = SLFuelFlowMax * PctPower;
  }

  FuelExpended = FuelFlowRate * in.TotalDeltaT; // For this time step ...
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRocket::CalcOxidizerNeed(void)
{
  SLOxiFlowMax = PropFlowMax * MxR / (1 + MxR);
  OxidizerFlowRate = SLOxiFlowMax * PctPower;
  OxidizerExpended = OxidizerFlowRate * in.TotalDeltaT;
  return OxidizerExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRocket::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << " Total Impulse (engine " << EngineNumber << " in lbf)" << delimiter
      << Name << " Total Vacuum Impulse (engine " << EngineNumber << " in lbf)" << delimiter
      << Name << " Roll Moment (engine " << EngineNumber << " in ft-lbf)" << delimiter
      << Name << " Pitch Moment (engine " << EngineNumber << " in ft-lbf)" << delimiter
      << Name << " Yaw Moment (engine " << EngineNumber << " in ft-lbf)" << delimiter
      << Name << " X Force (engine " << EngineNumber << " in lbf)" << delimiter
      << Name << " Y Force (engine " << EngineNumber << " in lbf)" << delimiter
      << Name << " Z Force (engine " << EngineNumber << " in lbf)" << delimiter
      << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRocket::GetEngineValues(const string& delimiter)
{
  std::ostringstream buf;

  buf << It << delimiter 
      << ItVac << delimiter 
      << GetMoments().Dump(delimiter) << delimiter
      << Thruster->GetBodyForces().Dump(delimiter) << delimiter
      << Thruster->GetThrusterValues(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This function should tie properties to rocket engine specific properties
// that are not bound in the base class (FGEngine) code.
//
void FGRocket::bindmodel(FGPropertyManager* PropertyManager)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  property_name = base_property_name + "/total-impulse";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetTotalImpulse);
  property_name = base_property_name + "/total-vac-impulse";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetVacTotalImpulse);
  property_name = base_property_name + "/vacuum-thrust_lbs";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetVacThrust);

  if (ThrustTable) { // Solid rocket motor
    property_name = base_property_name + "/thrust-variation_pct";
    PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetThrustVariation,
                                                       &FGRocket::SetThrustVariation);
    property_name = base_property_name + "/total-isp-variation_pct";
    PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetTotalIspVariation,
                                                       &FGRocket::SetTotalIspVariation);
  } else { // Liquid rocket motor
    property_name = base_property_name + "/oxi-flow-rate-pps";
    PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetOxiFlowRate);
    property_name = base_property_name + "/mixture-ratio";
    PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetMixtureRatio,
                                                       &FGRocket::SetMixtureRatio);
    property_name = base_property_name + "/isp";
    PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetIsp,
                                                       &FGRocket::SetIsp);
  }
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
      cout << "      Vacuum Isp = " << Isp << endl;
      cout << "      Maximum Throttle = " << MaxThrottle << endl;
      cout << "      Minimum Throttle = " << MinThrottle << endl;
      cout << "      Fuel Flow (max) = " << SLFuelFlowMax << endl;
      cout << "      Oxidizer Flow (max) = " << SLOxiFlowMax << endl;
      if (SLFuelFlowMax > 0)
        cout << "      Mixture ratio = " << SLOxiFlowMax/SLFuelFlowMax << endl;
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
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
