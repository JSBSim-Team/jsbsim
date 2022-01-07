/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGEngine.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
See header file.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created
09/03/99   JSB   Changed Rocket thrust equation to correct -= Thrust instead of
                 += Thrust (thanks to Tony Peden)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"
#include "FGPropeller.h"
#include "FGNozzle.h"
#include "FGRotor.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGEngine::FGEngine(int engine_number, struct Inputs& input)
  : in(input), EngineNumber(engine_number)
{
  Type = etUnknown;
  SLFuelFlowMax = 0.0;
  FuelExpended = 0.0;
  MaxThrottle = 1.0;
  MinThrottle = 0.0;
  FuelDensity = 6.02;
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGEngine::~FGEngine()
{
  delete Thruster;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::ResetToIC(void)
{
  Starter = false;
  FuelExpended = 0.0;
  Starved = Running = Cranking = false;
  PctPower = 0.0;
  FuelFlow_gph = 0.0;
  FuelFlow_pph = 0.0;
  FuelFlowRate = 0.0;
  FuelFreeze = false;
  FuelUsedLbs = 0.0;
  Thruster->ResetToIC();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGEngine::CalcFuelNeed(void)
{
  FuelFlowRate = SLFuelFlowMax*PctPower;
  FuelExpended = FuelFlowRate*in.TotalDeltaT;
  if (!Starved) FuelUsedLbs += FuelExpended;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned int FGEngine::GetSourceTank(unsigned int i) const
{
  if (i < SourceTanks.size()) {
    return SourceTanks[i];
  } else {
    throw("No such source tank is available for this engine");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGEngine::GetThrust(void) const 
{
  return Thruster->GetThrust();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGEngine::GetBodyForces(void)
{
  return Thruster->GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGEngine::GetMoments(void)
{
  return Thruster->GetMoments();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::LoadThrusterInputs()
{
  Thruster->in.TotalDeltaT     = in.TotalDeltaT;
  Thruster->in.H_agl           = in.H_agl;
  Thruster->in.PQRi            = in.PQRi;
  Thruster->in.AeroPQR         = in.AeroPQR;
  Thruster->in.AeroUVW         = in.AeroUVW;
  Thruster->in.Density         = in.Density;
  Thruster->in.Pressure        = in.Pressure;
  Thruster->in.Soundspeed      = in.Soundspeed;
  Thruster->in.Alpha           = in.alpha;
  Thruster->in.Beta            = in.beta;
  Thruster->in.Vt              = in.Vt;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::LoadThruster(FGFDMExec* exec, Element *thruster_element)
{
  if (thruster_element->FindElement("propeller")) {
    Element *document = thruster_element->FindElement("propeller");
    Thruster = new FGPropeller(exec, document, EngineNumber);
  } else if (thruster_element->FindElement("nozzle")) {
    Element *document = thruster_element->FindElement("nozzle");
    Thruster = new FGNozzle(exec, document, EngineNumber);
  } else if (thruster_element->FindElement("rotor")) {
    Element *document = thruster_element->FindElement("rotor");
    Thruster = new FGRotor(exec, document, EngineNumber);
  } else if (thruster_element->FindElement("direct")) {
    Element *document = thruster_element->FindElement("direct");
    Thruster = new FGThruster(exec, document, EngineNumber);
  } else {
    cerr << thruster_element->ReadFrom() << " Unknown thruster type" << endl;
    throw("Failed to load the thruster");
  }

  Debug(2);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGEngine::Load(FGFDMExec *exec, Element *engine_element)
{
  Element* parent_element = engine_element->GetParent();
  Element* local_element;
  FGColumnVector3 location, orientation;

  auto PropertyManager = exec->GetPropertyManager();

  Name = engine_element->GetAttributeValue("name");

  // Call ModelFunctions loader
  FGModelFunctions::Load(engine_element, exec, to_string((int)EngineNumber));

  // If engine location and/or orientation is supplied issue a warning since they
  // are ignored. What counts is the location and orientation of the thruster.
  local_element = parent_element->FindElement("location");
  if (local_element)
    cerr << local_element->ReadFrom()
         << "Engine location ignored, only thruster location is used." << endl;

  local_element = parent_element->FindElement("orient");
  if (local_element)
    cerr << local_element->ReadFrom()
         << "Engine orientation ignored, only thruster orientation is used." << endl;

  // Load thruster
  local_element = parent_element->FindElement("thruster");
  if (local_element) {
    try {
      LoadThruster(exec, local_element);
    } catch (std::string& str) {
      throw("Error loading engine " + Name + ". " + str);
    }
  } else {
    cerr << "No thruster definition supplied with engine definition." << endl;
  }

  ResetToIC(); // initialize dynamic terms

  // Load feed tank[s] references
  local_element = parent_element->FindElement("feed");
  while (local_element) {
    int tankID = (int)local_element->GetDataAsNumber();
    SourceTanks.push_back(tankID);
    local_element = parent_element->FindNextElement("feed");
  }

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  property_name = base_property_name + "/set-running";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetRunning, &FGEngine::SetRunning );
  property_name = base_property_name + "/thrust-lbs";
  PropertyManager->Tie( property_name.c_str(), Thruster, &FGThruster::GetThrust);
  property_name = base_property_name + "/fuel-flow-rate-pps";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetFuelFlowRate);
  property_name = base_property_name + "/fuel-flow-rate-gph";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetFuelFlowRateGPH);
  property_name = base_property_name + "/fuel-used-lbs";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetFuelUsedLbs);

  PostLoad(engine_element, exec, to_string((int)EngineNumber));

  Debug(0);

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

void FGEngine::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
    if (from == 2) { // After thruster loading
      cout << "      X = " << Thruster->GetLocationX() << endl;
      cout << "      Y = " << Thruster->GetLocationY() << endl;
      cout << "      Z = " << Thruster->GetLocationZ() << endl;
      cout << "      Pitch = " << radtodeg*Thruster->GetAnglesToBody(ePitch) << " degrees" << endl;
      cout << "      Yaw = " << radtodeg*Thruster->GetAnglesToBody(eYaw) << " degrees" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGEngine" << endl;
    if (from == 1) cout << "Destroyed:    FGEngine" << endl;
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
