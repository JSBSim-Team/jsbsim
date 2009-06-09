/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropulsion.cpp
 Author:       Jon S. Berndt
 Date started: 08/20/00
 Purpose:      Encapsulates the set of engines and tanks associated
               with this aircraft

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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
The Propulsion class is the container for the entire propulsion system, which is
comprised of engines and tanks. Once the Propulsion class gets the config file,
it reads in information which is specific to a type of engine. Then:

1) The appropriate engine type instance is created
2) At least one tank object is created, and is linked to an engine.

At Run time each engines Calculate() method is called.

HISTORY
--------------------------------------------------------------------------------
08/20/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropulsion.h"
#include <models/propulsion/FGRocket.h>
#include <models/propulsion/FGTurbine.h>
#include <models/propulsion/FGPiston.h>
#include <models/propulsion/FGElectric.h>
#include <models/propulsion/FGTurboProp.h>
#include <input_output/FGPropertyManager.h>
#include <input_output/FGXMLParse.h>
#include <math/FGColumnVector3.h>
#include <sstream>

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropulsion.cpp,v 1.33 2009/06/09 03:23:55 jberndt Exp $";
static const char *IdHdr = ID_PROPULSION;

extern short debug_lvl;


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  Name = "FGPropulsion";

  InitializedEngines = 0;
  numSelectedFuelTanks = numSelectedOxiTanks = 0;
  numTanks = numEngines = 0;
  numOxiTanks = numFuelTanks = 0;
  ActiveEngine = -1; // -1: ALL, 0: Engine 1, 1: Engine 2 ...
  tankJ.InitMatrix();
  refuel = dump = false;
  DumpRate = 0.0;
  fuel_freeze = false;
  TotalFuelQuantity = 0.0;
  IsBound =
  HavePistonEngine =
  HaveTurbineEngine =
  HaveRocketEngine =
  HaveTurboPropEngine =
  HaveElectricEngine = false;
  HasInitializedEngines = false;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropulsion::~FGPropulsion()
{
  for (unsigned int i=0; i<Engines.size(); i++) delete Engines[i];
  Engines.clear();
  for (unsigned int i=0; i<Tanks.size(); i++) delete Tanks[i];
  Tanks.clear();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  for (unsigned int i=0; i<numTanks; i++) Tanks[i]->ResetToIC();

  for (unsigned int i=0; i<numEngines; i++) {
    switch (Engines[i]->GetType()) {
      case FGEngine::etPiston:
        ((FGPiston*)Engines[i])->ResetToIC();
        if (HasInitializedEngines && (InitializedEngines & i)) InitRunning(i);
        break;
      case FGEngine::etTurbine:
        ((FGTurbine*)Engines[i])->ResetToIC();
        if (HasInitializedEngines && (InitializedEngines & i)) InitRunning(i);
        break;
      default:
        break;
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Run(void)
{
  unsigned int i;

  if (FGModel::Run()) return true;
  if (FDMExec->Holding()) return false;

  double dt = State->Getdt();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (i=0; i<numEngines; i++) {
    Engines[i]->Calculate();
    vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
    vMoments += Engines[i]->GetMoments();     // sum body frame moments
  }

  TotalFuelQuantity = 0.0;
  for (i=0; i<numTanks; i++) {
    Tanks[i]->Calculate( dt * rate );
    if (Tanks[i]->GetType() == FGTank::ttFUEL) {
      TotalFuelQuantity += Tanks[i]->GetContents();
    }
  }

  if (refuel) DoRefuel( dt * rate );
  if (dump) DumpFuel( dt * rate );

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::GetSteadyState(void)
{
  double currentThrust = 0, lastThrust = -1;
  int steady_count = 0, j = 0;
  bool steady = false;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
//      cout << "  Finding steady state for engine " << i << endl;
      Engines[i]->SetTrimMode(true);
      steady=false;
      steady_count=0;
      j=0;
      while (!steady && j < 6000) {
        Engines[i]->Calculate();
        lastThrust = currentThrust;
        currentThrust = Engines[i]->GetThruster()->GetThrust();
        if (fabs(lastThrust-currentThrust) < 0.0001) {
          steady_count++;
          if (steady_count > 120) {
            steady=true;
//            cout << "    Steady state found at thrust: " << currentThrust << " lbs." << endl;
          }
        } else {
          steady_count=0;
        }
        j++;
      }
//      if (j >= 6000) {
//        cout << "    Could not find a steady state for this engine." << endl;
//      }
      vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
      vMoments += Engines[i]->GetMoments();     // sum body frame moments
      Engines[i]->SetTrimMode(false);
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::InitRunning(int n)
{
  if (n > 0) { // A specific engine is supposed to be initialized

    if (n >= (int)GetNumEngines() ) {
      cerr << "Tried to initialize a non-existent engine!" << endl;
      throw;
    }
    FCS->SetThrottleCmd(n,1);
    FCS->SetMixtureCmd(n,1);
    GetEngine(n)->InitRunning();
    GetSteadyState();

    InitializedEngines = 1 << n;
    HasInitializedEngines = true;

  } else if (n < 0) { // -1 refers to "All Engines"

    for (unsigned int i=0; i<GetNumEngines(); i++) {
      FCS->SetThrottleCmd(i,1);
      FCS->SetMixtureCmd(i,1);
      GetEngine(i)->InitRunning();
    }
    GetSteadyState();
    InitializedEngines = -1;
    HasInitializedEngines = true;

  } else if (n == 0) { // No engines are to be initialized
    // Do nothing
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Load(Element* el)
{
  string type, engine_filename;
  bool ThrottleAdded = false;

  Debug(2);

  FGModel::Load(el); // Perform base class Load.

  Element* engine_element = el->FindElement("engine");
  while (engine_element) {
    engine_filename = engine_element->GetAttributeValue("file");

    if (engine_filename.empty()) {
      cerr << "Engine definition did not supply an engine file." << endl;
      return false;
    }

    engine_filename = FindEngineFullPathname(engine_filename);
    document = LoadXMLDocument(engine_filename);
    document->SetParent(engine_element);

    type = document->GetName();
    if (type == "piston_engine") {
      HavePistonEngine = true;
      if (!IsBound) bind();
      Engines.push_back(new FGPiston(FDMExec, document, numEngines));
    } else if (type == "turbine_engine") {
      HaveTurbineEngine = true;
      if (!IsBound) bind();
      Engines.push_back(new FGTurbine(FDMExec, document, numEngines));
    } else if (type == "turboprop_engine") {
      HaveTurboPropEngine = true;
      if (!IsBound) bind();
      Engines.push_back(new FGTurboProp(FDMExec, document, numEngines));
    } else if (type == "rocket_engine") {
      HaveRocketEngine = true;
      if (!IsBound) bind();
      Engines.push_back(new FGRocket(FDMExec, document, numEngines));
    } else if (type == "electric_engine") {
      HaveElectricEngine = true;
      if (!IsBound) bind();
      Engines.push_back(new FGElectric(FDMExec, document, numEngines));
    } else {
      cerr << "Unknown engine type: " << type << endl;
      exit(-5);
    }

    FCS->AddThrottle();
    ThrottleAdded = true;

    numEngines++;

    engine_element = el->FindNextElement("engine");
    ResetParser();
  }

  // Process tank definitions

  Element* tank_element = el->FindElement("tank");
  while (tank_element) {
    Tanks.push_back(new FGTank(FDMExec, tank_element, numTanks));
    if (Tanks.back()->GetType() == FGTank::ttFUEL) numFuelTanks++;
    else if (Tanks.back()->GetType() == FGTank::ttOXIDIZER) numOxiTanks++;
    else {cerr << "Unknown tank type specified." << endl; return false;}
    numTanks++;
    tank_element = el->FindNextElement("tank");
  }
  numSelectedFuelTanks = numFuelTanks;
  numSelectedOxiTanks  = numOxiTanks;

  CalculateTankInertias();
  if (!ThrottleAdded) FCS->AddThrottle(); // need to have at least one throttle

  // Process fuel dump rate
  if (el->FindElement("dump-rate"))
    DumpRate = el->FindElementValueAsNumberConvertTo("dump-rate", "LBS/MIN");


  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::FindEngineFullPathname(string engine_filename)
{
  string fullpath, localpath;
  string enginePath = FDMExec->GetEnginePath();
  string aircraftPath = FDMExec->GetFullAircraftPath();
  ifstream engine_file;

  string separator = "/";

  fullpath = enginePath + separator;
  localpath = aircraftPath + separator + "Engines" + separator;

  engine_file.open(string(fullpath + engine_filename + ".xml").c_str());
  if ( !engine_file.is_open()) {
    engine_file.open(string(localpath + engine_filename + ".xml").c_str());
      if ( !engine_file.is_open()) {
        cerr << " Could not open engine file: " << engine_filename << " in path "
             << fullpath << " or " << localpath << endl;
        return string("");
      } else {
        return string(localpath + engine_filename + ".xml");
      }
  }
  return string(fullpath + engine_filename + ".xml");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ifstream* FGPropulsion::FindEngineFile(string engine_filename)
{
  string fullpath, localpath;
  string enginePath = FDMExec->GetEnginePath();
  string aircraftPath = FDMExec->GetFullAircraftPath();
  ifstream* engine_file = new ifstream();

  string separator = "/";

  fullpath = enginePath + separator;
  localpath = aircraftPath + separator + "Engines" + separator;

  engine_file->open(string(fullpath + engine_filename + ".xml").c_str());
  if ( !engine_file->is_open()) {
    engine_file->open(string(localpath + engine_filename + ".xml").c_str());
      if ( !engine_file->is_open()) {
        cerr << " Could not open engine file: " << engine_filename << " in path "
             << fullpath << " or " << localpath << endl;
      }
  }
  return engine_file;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionStrings(string delimeter)
{
  unsigned int i;

  string PropulsionStrings = "";
  bool firstime = true;
  stringstream buf;

  for (i=0; i<Engines.size(); i++) {
    if (firstime)  firstime = false;
    else           PropulsionStrings += delimeter;

    PropulsionStrings += Engines[i]->GetEngineLabels(delimeter);
  }
  for (i=0; i<Tanks.size(); i++) {
    if (Tanks[i]->GetType() == FGTank::ttFUEL) buf << delimeter << "Fuel Tank " << i;
    else if (Tanks[i]->GetType() == FGTank::ttOXIDIZER) buf << delimeter << "Oxidizer Tank " << i;
  }

  return PropulsionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionValues(string delimeter)
{
  unsigned int i;

  string PropulsionValues = "";
  bool firstime = true;
  stringstream buf;

  for (i=0; i<Engines.size(); i++) {
    if (firstime)  firstime = false;
    else           PropulsionValues += delimeter;

    PropulsionValues += Engines[i]->GetEngineValues(delimeter);
  }
  for (i=0; i<Tanks.size(); i++) {
    buf << delimeter;
    buf << Tanks[i]->GetContents();
  }

  return PropulsionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGPropulsion::GetTanksMoment(void)
{
  vXYZtank_arm.InitMatrix();
  for (unsigned int i=0; i<Tanks.size(); i++) {
    vXYZtank_arm(eX) += Tanks[i]->GetXYZ(eX) * Tanks[i]->GetContents();
    vXYZtank_arm(eY) += Tanks[i]->GetXYZ(eY) * Tanks[i]->GetContents();
    vXYZtank_arm(eZ) += Tanks[i]->GetXYZ(eZ) * Tanks[i]->GetContents();
  }
  return vXYZtank_arm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksWeight(void)
{
  double Tw = 0.0;

  for (unsigned int i=0; i<Tanks.size(); i++) Tw += Tanks[i]->GetContents();

  return Tw;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGPropulsion::CalculateTankInertias(void)
{
  unsigned int size;

  size = Tanks.size();
  if (size == 0) return tankJ;

  tankJ = FGMatrix33();

  for (unsigned int i=0; i<size; i++) {
    tankJ += MassBalance->GetPointmassInertia( lbtoslug * Tanks[i]->GetContents(),
                                               Tanks[i]->GetXYZ() );
    tankJ(1,1) += Tanks[i]->GetIxx();
    tankJ(2,2) += Tanks[i]->GetIyy();
    tankJ(3,3) += Tanks[i]->GetIzz();
  }

  return tankJ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetMagnetos(int setting)
{
  if (ActiveEngine < 0) {
    for (unsigned i=0; i<Engines.size(); i++) {
      // ToDo: first need to make sure the engine Type is really appropriate:
      //   do a check to see if it is of type Piston. This should be done for
      //   all of this kind of possibly across-the-board settings.
      ((FGPiston*)Engines[i])->SetMagnetos(setting);
    }
  } else {
    ((FGPiston*)Engines[ActiveEngine])->SetMagnetos(setting);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetStarter(int setting)
{
  if (ActiveEngine < 0) {
    for (unsigned i=0; i<Engines.size(); i++) {
      if (setting == 0)
        Engines[i]->SetStarter(false);
      else
        Engines[i]->SetStarter(true);
    }
  } else {
    if (setting == 0)
      Engines[ActiveEngine]->SetStarter(false);
    else
      Engines[ActiveEngine]->SetStarter(true);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetCutoff(int setting)
{
  if (ActiveEngine < 0) {
    for (unsigned i=0; i<Engines.size(); i++) {
      if (setting == 0)
        ((FGTurbine*)Engines[i])->SetCutoff(false);
      else
        ((FGTurbine*)Engines[i])->SetCutoff(true);
    }
  } else {
    if (setting == 0)
      ((FGTurbine*)Engines[ActiveEngine])->SetCutoff(false);
    else
      ((FGTurbine*)Engines[ActiveEngine])->SetCutoff(true);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetActiveEngine(int engine)
{
  if (engine >= (int)Engines.size() || engine < 0)
    ActiveEngine = -1;
  else
    ActiveEngine = engine;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::Transfer(int source, int target, double amount)
{
 double shortage, overage;

  if (source == -1) {
     shortage = 0.0;
  } else {
     shortage = Tanks[source]->Drain(amount);
  }
  if (target == -1) {
     overage = 0.0;
  } else {
     overage = Tanks[target]->Fill(amount - shortage);
  }
  return overage;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::DoRefuel(double time_slice)
{
  unsigned int i;

  double fillrate = 100 * time_slice;   // 100 lbs/sec = 6000 lbs/min
  int TanksNotFull = 0;

  for (i=0; i<numTanks; i++) {
    if (Tanks[i]->GetPctFull() < 99.99) ++TanksNotFull;
  }

  if (TanksNotFull) {
    for (i=0; i<numTanks; i++) {
      if (Tanks[i]->GetPctFull() < 99.99)
          Transfer(-1, i, fillrate/TanksNotFull);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::DumpFuel(double time_slice)
{
  unsigned int i;
  int TanksDumping = 0;

  for (i=0; i<numTanks; i++) {
    if (Tanks[i]->GetContents() > Tanks[i]->GetStandpipe()) ++TanksDumping;
  }

  if (TanksDumping == 0) return;

  double dump_rate_per_tank = DumpRate / 60.0 * time_slice / TanksDumping;

  for (i=0; i<numTanks; i++) {
    if (Tanks[i]->GetContents() > Tanks[i]->GetStandpipe()) {
      Transfer(i, -1, dump_rate_per_tank);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetFuelFreeze(bool f)
{
  fuel_freeze = f;
  for (unsigned int i=0; i<numEngines; i++) {
    Engines[i]->SetFuelFreeze(f);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::bind(void)
{
  typedef double (FGPropulsion::*PMF)(int) const;
  typedef int (FGPropulsion::*iPMF)(void) const;

  IsBound = true;
  PropertyManager->Tie("propulsion/set-running", this, (iPMF)0, &FGPropulsion::InitRunning, true);
  if (HaveTurbineEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, (iPMF)0, &FGPropulsion::SetStarter,  true);
    PropertyManager->Tie("propulsion/cutoff_cmd", this,  (iPMF)0, &FGPropulsion::SetCutoff,   true);
  }

  if (HavePistonEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, (iPMF)0, &FGPropulsion::SetStarter,  true);
    PropertyManager->Tie("propulsion/magneto_cmd", this, (iPMF)0, &FGPropulsion::SetMagnetos, true);
  }

  PropertyManager->Tie("propulsion/active_engine", this, (iPMF)&FGPropulsion::GetActiveEngine,
                        &FGPropulsion::SetActiveEngine, true);
  PropertyManager->Tie("propulsion/total-fuel-lbs", this, &FGPropulsion::GetTotalFuelQuantity);
  PropertyManager->Tie("propulsion/refuel", this, &FGPropulsion::GetRefuel,
                        &FGPropulsion::SetRefuel, true);
  PropertyManager->Tie("propulsion/fuel_dump", this, &FGPropulsion::GetFuelDump,
                        &FGPropulsion::SetFuelDump, true);
  PropertyManager->Tie("forces/fbx-prop-lbs", this,1,
                       (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fby-prop-lbs", this,2,
                       (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fbz-prop-lbs", this,3,
                       (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("moments/l-prop-lbsft", this,1,
                       (PMF)&FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/m-prop-lbsft", this,2,
                       (PMF)&FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/n-prop-lbsft", this,3,
                       (PMF)&FGPropulsion::GetMoments);

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

void FGPropulsion::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 2) { // Loader
      cout << endl << "  Propulsion:" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPropulsion" << endl;
    if (from == 1) cout << "Destroyed:    FGPropulsion" << endl;
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
