/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRocket.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models a rocket engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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

This class descends from the FGEngine class and models a rocket engine based on
parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sstream>

#include "FGRocket.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGRocket.cpp,v 1.10 2009/05/10 10:59:49 andgi Exp $";
static const char *IdHdr = ID_ROCKET;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGRocket::FGRocket(FGFDMExec* exec, Element *el, int engine_number)
  : FGEngine(exec, el, engine_number)
{
  Element* thrust_table_element = 0;
  ThrustTable = 0L;
  BurnTime = 0.0;
  previousFuelNeedPerTank = 0.0;
  previousOxiNeedPerTank = 0.0;
  PropellantFlowRate = 0.0;
  FuelFlowRate = 0.0;
  OxidizerFlowRate = 0.0;
  SLOxiFlowMax = 0.0;
  It = 0.0;

  // Defaults
   MinThrottle = 0.0;
   MaxThrottle = 1.0;

  if (el->FindElement("isp"))
    Isp = el->FindElementValueAsNumber("isp");
  if (el->FindElement("maxthrottle"))
    MaxThrottle = el->FindElementValueAsNumber("maxthrottle");
  if (el->FindElement("minthrottle"))
    MinThrottle = el->FindElementValueAsNumber("minthrottle");
  if (el->FindElement("slfuelflowmax"))
    SLFuelFlowMax = el->FindElementValueAsNumberConvertTo("slfuelflowmax", "LBS/SEC");
  if (el->FindElement("sloxiflowmax"))
    SLOxiFlowMax = el->FindElementValueAsNumberConvertTo("sloxiflowmax", "LBS/SEC");

  thrust_table_element = el->FindElement("thrust_table");
  if (thrust_table_element) {
    ThrustTable = new FGTable(PropertyManager, thrust_table_element);
  }

  bindmodel();

  Debug(0);

  Type = etRocket;
  Flameout = false;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRocket::~FGRocket(void)
{
  delete ThrustTable;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRocket::Calculate(void)
{
  double dT = State->Getdt()*Propulsion->GetRate();
  double thrust;

  if (!Flameout && !Starved) ConsumeFuel();

  PropellantFlowRate = (FuelExpended + OxidizerExpended)/dT;
  Throttle = FCS->GetThrottlePos(EngineNumber);

  // If there is a thrust table, it is a function of propellant remaining. The
  // engine is started when the throttle is advanced to 1.0. After that, it
  // burns without regard to throttle setting. The table returns a value between
  // zero and one, representing the percentage of maximum vacuum thrust being
  // applied.

  if (ThrustTable != 0L) { // Thrust table given -> Solid fuel used

    if ((Throttle == 1 || BurnTime > 0.0 ) && !Starved) {
      BurnTime += State->Getdt();
      double TotalEngineFuelAvailable=0.0;
      for (int i=0; i<(int)SourceTanks.size(); i++)
        TotalEngineFuelAvailable += Propulsion->GetTank(SourceTanks[i])->GetContents();

      VacThrust = ThrustTable->GetValue(TotalEngineFuelAvailable);
    } else {
      VacThrust = 0.0;
    }

  } else { // liquid fueled rocket assumed

    if (Throttle < MinThrottle || Starved) { // Combustion not supported

      PctPower = 0.0; // desired thrust
      Flameout = true;
      VacThrust = 0.0;

    } else { // Calculate thrust

      PctPower = Throttle / MaxThrottle; // Min and MaxThrottle range from 0.0 to 1.0, normally.
      Flameout = false;
      VacThrust = Isp * PropellantFlowRate;

    }

  } // End thrust calculations

  thrust = Thruster->Calculate(VacThrust);
  It += thrust * dT;

  return thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This overrides the base class ConsumeFuel() function, for special rocket
// engine processing.

void FGRocket::ConsumeFuel(void)
{
  unsigned int i;
  FGTank* Tank;
  bool haveOxTanks = false;
  double Fshortage=0, Oshortage=0, TanksWithFuel=0, TanksWithOxidizer=0;

  if (FuelFreeze) return;
  if (TrimMode) return;

  // Count how many assigned tanks have fuel for this engine at this time.
  // If there is/are fuel tanks but no oxidizer tanks, this indicates
  // a solid rocket is being modeled.

  for (i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(SourceTanks[i]);
    switch(Tank->GetType()) {
      case FGTank::ttFUEL:
        if (Tank->GetContents() > 0.0 && Tank->GetSelected()) ++TanksWithFuel;
        break;
      case FGTank::ttOXIDIZER:
        haveOxTanks = true;
        if (Tank->GetContents() > 0.0 && Tank->GetSelected()) ++TanksWithOxidizer;
        break;
    }
  }

  // If this engine has burned out, it is starved.

  if (TanksWithFuel==0 || (haveOxTanks && TanksWithOxidizer==0)) {
    Starved = true;
    return;
  }

  // Expend fuel from the engine's tanks if the tank is selected as a source
  // for this engine.

  double fuelNeedPerTank = CalcFuelNeed()/TanksWithFuel;
  double oxiNeedPerTank = CalcOxidizerNeed()/TanksWithOxidizer;

  for (i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(SourceTanks[i]);
    if ( ! Tank->GetSelected()) continue; // If this tank is not selected as a source, skip it.
    switch(Tank->GetType()) {
      case FGTank::ttFUEL:
        Fshortage += Tank->Drain(2.0*fuelNeedPerTank - previousFuelNeedPerTank);
        previousFuelNeedPerTank = fuelNeedPerTank;
        break;
      case FGTank::ttOXIDIZER:
        Oshortage += Tank->Drain(2.0*oxiNeedPerTank - previousOxiNeedPerTank);
        previousOxiNeedPerTank = oxiNeedPerTank;
        break;
    }
  }

  if (Fshortage < 0.00 || (haveOxTanks && Oshortage < 0.00)) Starved = true;
  else Starved = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRocket::CalcFuelNeed(void)
{
  double dT = State->Getdt()*Propulsion->GetRate();

  if (ThrustTable != 0L) {          // Thrust table given - infers solid fuel
    FuelFlowRate = VacThrust/Isp;   // This calculates wdot (weight flow rate in lbs/sec)
  } else {
    FuelFlowRate = SLFuelFlowMax*PctPower;
  }

  FuelExpended = FuelFlowRate*dT; // For this time step ...
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRocket::CalcOxidizerNeed(void)
{
  double dT = State->Getdt()*Propulsion->GetRate();
  OxidizerFlowRate = SLOxiFlowMax*PctPower;
  OxidizerExpended = OxidizerFlowRate*dT;
  return OxidizerExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRocket::GetEngineLabels(string delimeter)
{
  std::ostringstream buf;

  buf << Name << " Total Impulse (engine " << EngineNumber << " in psf)" << delimeter
      << Thruster->GetThrusterLabels(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRocket::GetEngineValues(string delimeter)
{
  std::ostringstream buf;

  buf << It << delimeter << Thruster->GetThrusterValues(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This funciton should tie properties to rocket engine specific properties
// that are not bound in the base class (FGEngine) code.
//
void FGRocket::bindmodel()
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  property_name = base_property_name + "/total-impulse";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetTotalImpulse);
  property_name = base_property_name + "/oxi-flow-rate-pps";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetOxiFlowRate);
  property_name = base_property_name + "/vacuum-thrust_lbs";
  PropertyManager->Tie( property_name.c_str(), this, &FGRocket::GetVacThrust);
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
