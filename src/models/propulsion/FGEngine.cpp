/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGEngine.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

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
#include "FGTank.h"
#include "FGPropeller.h"
#include "FGNozzle.h"
#include <input_output/FGXMLParse.h>
#include <math/FGColumnVector3.h>
#include <fstream>

namespace JSBSim {

static const char *IdSrc = "$Id: FGEngine.cpp,v 1.25 2009/02/05 10:22:49 jberndt Exp $";
static const char *IdHdr = ID_ENGINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGEngine::FGEngine(FGFDMExec* exec, Element* engine_element, int engine_number)
                      : EngineNumber(engine_number)
{
  Element* local_element;
  FGColumnVector3 location, orientation;

  Name = "";
  Type = etUnknown;
  X = Y = Z = 0.0;
  EnginePitch = EngineYaw = 0.0;
  SLFuelFlowMax = 0.0;
  MaxThrottle = 1.0;
  MinThrottle = 0.0;

  ResetToIC(); // initialize dynamic terms

  FDMExec = exec;
  State = FDMExec->GetState();
  Atmosphere = FDMExec->GetAtmosphere();
  FCS = FDMExec->GetFCS();
  Propulsion = FDMExec->GetPropulsion();
  Aircraft = FDMExec->GetAircraft();
  Propagate = FDMExec->GetPropagate();
  Auxiliary = FDMExec->GetAuxiliary();

  PropertyManager = FDMExec->GetPropertyManager();

  Name = engine_element->GetAttributeValue("name");

// Find and set engine location

  local_element = engine_element->GetParent()->FindElement("location");
  if (local_element)  location = local_element->FindElementTripletConvertTo("IN");
  else      cerr << "No engine location found for this engine." << endl;

  local_element = engine_element->GetParent()->FindElement("orient");
  if (local_element)  orientation = local_element->FindElementTripletConvertTo("RAD");
//  else          cerr << "No engine orientation found for this engine." << endl;
// Jon: The engine orientation has a default and is not normally used.

  SetPlacement(location, orientation);

  // Load thruster
  local_element = engine_element->GetParent()->FindElement("thruster");
  if (local_element) {
    if (!LoadThruster(local_element)) exit(-1);
  } else {
    cerr << "No thruster definition supplied with engine definition." << endl;
  }

  // Load feed tank[s] references
  local_element = engine_element->GetParent()->FindElement("feed");
  if (local_element) {
    while (local_element) {
      AddFeedTank((int)local_element->GetDataAsNumber());
      local_element = engine_element->GetParent()->FindNextElement("feed");
    }
  } else {
    cerr << "No feed tank specified in engine definition." << endl;
  }

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  property_name = base_property_name + "/set-running";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetRunning, &FGEngine::SetRunning );
  property_name = base_property_name + "/thrust-lbs";
  PropertyManager->Tie( property_name.c_str(), Thruster, &FGThruster::GetThrust);
  property_name = base_property_name + "/fuel-flow-rate-pps";
  PropertyManager->Tie( property_name.c_str(), this, &FGEngine::GetFuelFlowRate);

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
  Throttle = 0.0;
  Mixture = 1.0;
  Starter = false;
  FuelExpended = 0.0;
  Starved = Running = Cranking = false;
  PctPower = 0.0;
  TrimMode = false;
  FuelFlow_gph = 0.0;
  FuelFlow_pph = 0.0;
  FuelFreeze = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This base class function should be called from within the
// derived class' Calculate() function before any other calculations are done.
// This base class method removes fuel from the fuel tanks as appropriate,
// and sets the starved flag if necessary.
// This version of the fuel consumption code should never see an oxidizer tank.

void FGEngine::ConsumeFuel(void)
{
  if (FuelFreeze) return;
  if (TrimMode) return;

  unsigned int i;
  double Fshortage, TanksWithFuel;
  FGTank* Tank;
  Fshortage = TanksWithFuel = 0.0;

  // count how many assigned tanks have fuel
  for (i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(SourceTanks[i]);
    if (Tank->GetType() == FGTank::ttFUEL){
      if (Tank->GetContents() > 0.0) ++TanksWithFuel;
    } else {
       cerr << "No oxidizer tanks should be used for this engine type." << endl;
    }
  }
  if (TanksWithFuel==0) {
    Starved = true;
    return;
  }

  for (i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(SourceTanks[i]);
    if (Tank->GetType() == FGTank::ttFUEL) {
       Fshortage += Tank->Drain(CalcFuelNeed()/TanksWithFuel);
    } else {
       cerr << "No oxidizer tanks should be used for this engine type." << endl;
    }
  }

  if (Fshortage < 0.00) Starved = true;
  else Starved = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGEngine::CalcFuelNeed(void)
{
  double dT = State->Getdt()*Propulsion->GetRate();
  FuelFlowRate = SLFuelFlowMax*PctPower;
  FuelExpended = FuelFlowRate*dT;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::SetPlacement(FGColumnVector3& location, FGColumnVector3& orientation)
{
  X = location(eX);
  Y = location(eY);
  Z = location(eZ);
  EnginePitch = orientation(ePitch);
  EngineYaw = orientation (eYaw);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::AddFeedTank(int tkID)
{
  SourceTanks.push_back(tkID);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGEngine::GetBodyForces(void)
{
  return Thruster->GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGEngine::GetMoments(void)
{
  return Thruster->GetMoments();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGEngine::LoadThruster(Element *thruster_element)
{
  string token, fullpath, localpath;
  string thruster_filename, thruster_fullpathname, thrType;
  double P_Factor = 0, Sense = 0.0;
  string enginePath = FDMExec->GetEnginePath();
  string aircraftPath = FDMExec->GetFullAircraftPath();
  ifstream thruster_file;
  FGColumnVector3 location, orientation;
  string separator = "/";

  fullpath = enginePath + separator;
  localpath = aircraftPath + separator + "Engines" + separator;

  thruster_filename = thruster_element->GetAttributeValue("file");
  if ( !thruster_filename.empty()) {
    thruster_fullpathname = fullpath + thruster_filename + ".xml";
    thruster_file.open(thruster_fullpathname.c_str());
    if ( !thruster_file.is_open()) {
      thruster_fullpathname = localpath + thruster_filename + ".xml";
      thruster_file.open(thruster_fullpathname.c_str());
      if ( !thruster_file.is_open()) {
        cerr << "Could not open thruster file: " << thruster_filename << ".xml" << endl;
        return false;
      } else {
        thruster_file.close();
      }
    } else {
      thruster_file.close();
    }
  } else {
    cerr << "No thruster filename given." << endl;
    return false;
  }

  document = LoadXMLDocument(thruster_fullpathname);
  document->SetParent(thruster_element);

  thrType = document->GetName();

  if (thrType == "propeller") {
    Thruster = new FGPropeller(FDMExec, document, EngineNumber);
  } else if (thrType == "nozzle") {
    Thruster = new FGNozzle(FDMExec, document, EngineNumber);
  } else if (thrType == "direct") {
    Thruster = new FGThruster( FDMExec, document, EngineNumber);
  }

  Thruster->SetdeltaT(State->Getdt() * Propulsion->GetRate());

  Debug(2);
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
