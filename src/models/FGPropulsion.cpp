/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropulsion.cpp
 Author:       Jon S. Berndt
 Date started: 08/20/00
 Purpose:      Encapsulates the set of engines and tanks associated
               with this aircraft

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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

#include <iomanip>

#include "FGFDMExec.h"
#include "FGPropulsion.h"
#include "models/FGMassBalance.h"
#include "models/propulsion/FGRocket.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGElectric.h"
#include "models/propulsion/FGTurboProp.h"
#include "models/propulsion/FGTank.h"
#include "input_output/FGModelLoader.h"

using namespace std;

namespace JSBSim {

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  Name = "FGPropulsion";

  numSelectedFuelTanks = numSelectedOxiTanks = 0;
  numTanks = numEngines = 0;
  numOxiTanks = numFuelTanks = 0;
  ActiveEngine = -1; // -1: ALL, 0: Engine 1, 1: Engine 2 ...
  tankJ.InitMatrix();
  DumpRate = 0.0; 
  RefuelRate = 6000.0;
  FuelFreeze = false;
  IsBound =
  HavePistonEngine =
  HaveTurbineEngine =
  HaveRocketEngine =
  HaveTurboPropEngine =
  HaveElectricEngine = false;

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
  bool result = true;

  if (!FGModel::InitModel()) return false;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (unsigned int i=0; i<numTanks; i++) Tanks[i]->ResetToIC();
  TotalFuelQuantity = 0.0;
  TotalOxidizerQuantity = 0.0;
  refuel = dump = false;

  for (unsigned int i=0; i<numEngines; i++)
    Engines[i]->ResetToIC();

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Run(bool Holding)
{
  unsigned int i;

  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  RunPreFunctions();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (i=0; i<numEngines; i++) {
    Engines[i]->Calculate();
    ConsumeFuel(Engines[i]);
    vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
    vMoments += Engines[i]->GetMoments();     // sum body frame moments
  }

  TotalFuelQuantity = 0.0;
  TotalOxidizerQuantity = 0.0;
  for (i=0; i<numTanks; i++) {
    Tanks[i]->Calculate( in.TotalDeltaT, in.TAT_c);
    switch (Tanks[i]->GetType()) {
    case FGTank::ttFUEL:
      TotalFuelQuantity += Tanks[i]->GetContents();
      break;
    case FGTank::ttOXIDIZER:
      TotalOxidizerQuantity += Tanks[i]->GetContents();
      break;
    default:
      break;
    }
  }

  if (refuel.node() && refuel) DoRefuel( in.TotalDeltaT );
  if (dump.node() && dump) DumpFuel( in.TotalDeltaT );

  RunPostFunctions();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// The engine can tell us how much fuel it needs, but it is up to the propulsion
// subsystem manager class FGPropulsion to manage fuel flow amongst tanks. Engines
// May burn fuel from more than one tank at a time, and may burn from one tank
// before another - that is, may burn from one tank until the tank is depleted,
// then burn from the next highest priority tank. This can be accompished
// by defining a fuel management system, but this way of specifying priorities
// is more automatic from a user perspective.

void FGPropulsion::ConsumeFuel(FGEngine* engine)
{
  if (FuelFreeze) return;
  if (FDMExec->GetTrimStatus()) return;

  unsigned int TanksWithFuel=0, CurrentFuelTankPriority=1;
  unsigned int TanksWithOxidizer=0, CurrentOxidizerTankPriority=1;
  vector <int> FeedListFuel, FeedListOxi;
  bool Starved = true; // Initially set Starved to true. Set to false in code below.
  bool hasOxTanks = false;

  // For this engine,
  // 1) Count how many fuel tanks with the current priority level have fuel
  // 2) If there none, then try next lower priority (higher number) - that is,
  //    increment CurrentPriority.
  // 3) Build the feed list.
  // 4) Do the same for oxidizer tanks, if needed.

  // Process fuel tanks, if any
  while ((TanksWithFuel == 0) && (CurrentFuelTankPriority <= numTanks)) {
    for (unsigned int i=0; i<engine->GetNumSourceTanks(); i++) {
      unsigned int TankId = engine->GetSourceTank(i);
      FGTank* Tank = Tanks[TankId];
      unsigned int TankPriority = Tank->GetPriority();
      if (TankPriority != 0) {
        switch(Tank->GetType()) {
        case FGTank::ttFUEL:
          if ((Tank->GetContents() > Tank->GetUnusable()) && Tank->GetSelected() && (TankPriority == CurrentFuelTankPriority)) {
            TanksWithFuel++;
            Starved = false;
            FeedListFuel.push_back(TankId);
          } 
          break;
        case FGTank::ttOXIDIZER:
          // Skip this here (done below)
          break;
        }
      }
    }
    if (TanksWithFuel == 0) CurrentFuelTankPriority++; // No tanks at this priority, try next priority
  }

  bool FuelStarved = Starved;
  Starved = true;

  // Process Oxidizer tanks, if any
  if (engine->GetType() == FGEngine::etRocket) {
    while ((TanksWithOxidizer == 0) && (CurrentOxidizerTankPriority <= numTanks)) {
      for (unsigned int i=0; i<engine->GetNumSourceTanks(); i++) {
        unsigned int TankId = engine->GetSourceTank(i);
        FGTank* Tank = Tanks[TankId];
        unsigned int TankPriority = Tank->GetPriority();
        if (TankPriority != 0) {
          switch(Tank->GetType()) {
          case FGTank::ttFUEL:
            // Skip this here (done above)
            break;
          case FGTank::ttOXIDIZER:
            hasOxTanks = true;
            if (Tank->GetContents() > Tank->GetUnusable() && Tank->GetSelected() && TankPriority == CurrentOxidizerTankPriority) {
              TanksWithOxidizer++;
              if (TanksWithFuel > 0) Starved = false;
              FeedListOxi.push_back(TankId);
            }
            break;
          }
        }
      }
      if (TanksWithOxidizer == 0) CurrentOxidizerTankPriority++; // No tanks at this priority, try next priority
    }
  }

  bool OxiStarved = Starved;

  engine->SetStarved(FuelStarved || (hasOxTanks && OxiStarved)); // Tanks can be refilled, so be sure to reset engine Starved flag here.

  // No fuel or fuel/oxidizer found at any priority!
//  if (Starved) return;
  if (FuelStarved || (hasOxTanks && OxiStarved)) return;

  double FuelToBurn = engine->CalcFuelNeed();            // How much fuel does this engine need?
  double FuelNeededPerTank = FuelToBurn / TanksWithFuel; // Determine fuel needed per tank.  
  for (unsigned int i=0; i<FeedListFuel.size(); i++) {
    Tanks[FeedListFuel[i]]->Drain(FuelNeededPerTank); 
  }

  if (engine->GetType() == FGEngine::etRocket) {
    double OxidizerToBurn = engine->CalcOxidizerNeed();                // How much fuel does this engine need?
    double OxidizerNeededPerTank = 0;
    if (TanksWithOxidizer > 0) OxidizerNeededPerTank = OxidizerToBurn / TanksWithOxidizer; // Determine fuel needed per tank.  
    for (unsigned int i=0; i<FeedListOxi.size(); i++) {
      Tanks[FeedListOxi[i]]->Drain(OxidizerNeededPerTank); 
    }
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::GetSteadyState(void)
{
  double currentThrust = 0, lastThrust = -1;
  int steady_count = 0, j = 0;
  bool steady = false;
  bool TrimMode = FDMExec->GetTrimStatus();
  double TimeStep = FDMExec->GetDeltaT();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if (!FGModel::Run(false)) {
    FDMExec->SetTrimStatus(true);
    // This is a time marching algorithm so it needs a non-zero time step to
    // reach a steady state.
    in.TotalDeltaT = 0.5;

    for (unsigned int i=0; i<numEngines; i++) {
      steady=false;
      steady_count=0;
      j=0;
      while (!steady && j < 6000) {
        Engines[i]->Calculate();
        lastThrust = currentThrust;
        currentThrust = Engines[i]->GetThrust();
        if (fabs(lastThrust-currentThrust) < 0.0001) {
          steady_count++;
          if (steady_count > 120) {
            steady=true;
          }
        } else {
          steady_count=0;
        }
        j++;
      }
      vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
      vMoments += Engines[i]->GetMoments();     // sum body frame moments
    }

    FDMExec->SetTrimStatus(TrimMode);
    in.TotalDeltaT = TimeStep;

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::InitRunning(int n)
{
  if (n >= 0) { // A specific engine is supposed to be initialized

    if (n >= (int)GetNumEngines() ) {
      throw(string("Tried to initialize a non-existent engine!"));
    }

    in.ThrottleCmd[n] = in.ThrottlePos[n] = 1; // Set the throttle command and position
    in.MixtureCmd[n] = in.MixturePos[n] = 1;   // Set the mixture command and position

    GetEngine(n)->InitRunning();
    GetSteadyState();

  } else if (n < 0) { // -1 refers to "All Engines"

    for (unsigned int i=0; i<GetNumEngines(); i++) {
      in.ThrottleCmd[i] = in.ThrottlePos[i] = 1; // Set the throttle command and position
      in.MixtureCmd[i] = in.MixturePos[i] = 1;   // Set the mixture command and position
      GetEngine(i)->InitRunning();
    }

    GetSteadyState();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Load(Element* el)
{
  FGModelLoader ModelLoader(this);

  Debug(2);
  ReadingEngine = false;
  double FuelDensity = 6.0;

  Name = "Propulsion Model: " + el->GetAttributeValue("name");

  // Perform base class Pre-Load
  if (!FGModel::Upload(el, true))
    return false;

  // Process tank definitions first to establish the number of fuel tanks

  Element* tank_element = el->FindElement("tank");
  while (tank_element) {
    Tanks.push_back(new FGTank(FDMExec, tank_element, numTanks));
    if (Tanks.back()->GetType() == FGTank::ttFUEL) { 
      FuelDensity = Tanks[numFuelTanks]->GetDensity();
      numFuelTanks++;
      }
    else if (Tanks.back()->GetType() == FGTank::ttOXIDIZER) numOxiTanks++;
    else {cerr << "Unknown tank type specified." << endl; return false;}
    numTanks++;
    tank_element = el->FindNextElement("tank");
  }
  numSelectedFuelTanks = numFuelTanks;
  numSelectedOxiTanks  = numOxiTanks;

  ReadingEngine = true;
  Element* engine_element = el->FindElement("engine");
  while (engine_element) {
    if (!ModelLoader.Open(engine_element)) return false;

    try {
      // Locate the thruster definition
      Element* thruster_element = engine_element->FindElement("thruster");
      if (!thruster_element || !ModelLoader.Open(thruster_element))
        throw("No thruster definition supplied with engine definition.");

      if (engine_element->FindElement("piston_engine")) {
        HavePistonEngine = true;
        if (!IsBound) bind();
        Element *element = engine_element->FindElement("piston_engine");
        Engines.push_back(new FGPiston(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("turbine_engine")) {
        HaveTurbineEngine = true;
        if (!IsBound) bind();
        Element *element = engine_element->FindElement("turbine_engine");
        Engines.push_back(new FGTurbine(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("turboprop_engine")) {
        HaveTurboPropEngine = true;
        if (!IsBound) bind();
        Element *element = engine_element->FindElement("turboprop_engine");
        Engines.push_back(new FGTurboProp(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("rocket_engine")) {
        HaveRocketEngine = true;
        if (!IsBound) bind();
        Element *element = engine_element->FindElement("rocket_engine");
        Engines.push_back(new FGRocket(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("electric_engine")) {
        HaveElectricEngine = true;
        if (!IsBound) bind();
        Element *element = engine_element->FindElement("electric_engine");
        Engines.push_back(new FGElectric(FDMExec, element, numEngines, in));
      } else {
        cerr << engine_element->ReadFrom() << " Unknown engine type" << endl;
        return false;
      }
    } catch (std::string& str) {
      cerr << endl << fgred << str << reset << endl;
      return false;
    }

    numEngines++;

    engine_element = el->FindNextElement("engine");
  }

  CalculateTankInertias();

  if (el->FindElement("dump-rate"))
    DumpRate = el->FindElementValueAsNumberConvertTo("dump-rate", "LBS/MIN");
  if (el->FindElement("refuel-rate"))
    RefuelRate = el->FindElementValueAsNumberConvertTo("refuel-rate", "LBS/MIN");

  unsigned int i;
  for (i=0; i<Engines.size(); i++) {
    Engines[i]->SetFuelDensity(FuelDensity);
  }


  PostLoad(el, FDMExec);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPath FGPropulsion::FindFullPathName(const SGPath& path) const
{
  if (!ReadingEngine) return FGModel::FindFullPathName(path);

  SGPath name = CheckPathName(FDMExec->GetFullAircraftPath()/string("Engines"),
                              path);
  if (!name.isNull()) return name;

  return CheckPathName(FDMExec->GetEnginePath(), path);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionStrings(const string& delimiter) const
{
  unsigned int i;

  string PropulsionStrings = "";
  bool firstime = true;
  stringstream buf;

  for (i=0; i<Engines.size(); i++) {
    if (firstime)  firstime = false;
    else           PropulsionStrings += delimiter;

    PropulsionStrings += Engines[i]->GetEngineLabels(delimiter);
  }
  for (i=0; i<Tanks.size(); i++) {
    if (Tanks[i]->GetType() == FGTank::ttFUEL) buf << delimiter << "Fuel Tank " << i;
    else if (Tanks[i]->GetType() == FGTank::ttOXIDIZER) buf << delimiter << "Oxidizer Tank " << i;
  }

  PropulsionStrings += buf.str();
  buf.str("");

  return PropulsionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionValues(const string& delimiter) const
{
  unsigned int i;

  string PropulsionValues = "";
  bool firstime = true;
  stringstream buf;

  for (i=0; i<Engines.size(); i++) {
    if (firstime)  firstime = false;
    else           PropulsionValues += delimiter;

    PropulsionValues += Engines[i]->GetEngineValues(delimiter);
  }
  for (i=0; i<Tanks.size(); i++) {
    buf << delimiter;
    buf << Tanks[i]->GetContents();
  }

  PropulsionValues += buf.str();
  buf.str("");

  return PropulsionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionTankReport()
{
  string out="";
  stringstream outstream;

  /*const FGMatrix33& mTkI =*/ CalculateTankInertias();

  for (unsigned int i=0; i<numTanks; i++)
  {
    FGTank* tank = Tanks[i];
    string tankname="";
    if (tank->GetType() == FGTank::ttFUEL && tank->GetGrainType() != FGTank::gtUNKNOWN) {
      tankname = "Solid Fuel";
    } else if (tank->GetType() == FGTank::ttFUEL) {
      tankname = "Fuel";
    } else if (tank->GetType() == FGTank::ttOXIDIZER) {
      tankname = "Oxidizer";
    } else {
      tankname = "(Unknown tank type)";
    }
    outstream << highint << left << setw(4) << i << setw(30) << tankname << normint
      << right << setw(10) << tank->GetContents() << setw(8) << tank->GetXYZ(eX)
         << setw(8) << tank->GetXYZ(eY) << setw(8) << tank->GetXYZ(eZ)
         << setw(12) << tank->GetIxx() << setw(12) << tank->GetIyy()
         << setw(12) << tank->GetIzz() << endl;
  }
  return outstream.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGPropulsion::GetTanksMoment(void)
{
  vXYZtank_arm.InitMatrix();
  for (unsigned int i=0; i<Tanks.size(); i++) {
    vXYZtank_arm += Tanks[i]->GetXYZ() * Tanks[i]->GetContents();
  }
  return vXYZtank_arm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksWeight(void) const
{
  double Tw = 0.0;

  for (unsigned int i=0; i<Tanks.size(); i++) Tw += Tanks[i]->GetContents();

  return Tw;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGPropulsion::CalculateTankInertias(void)
{
  size_t size = Tanks.size();

  if (size == 0) return tankJ;

  tankJ.InitMatrix();

  for (unsigned int i=0; i<size; i++) {
    tankJ += FDMExec->GetMassBalance()->GetPointmassInertia( lbtoslug * Tanks[i]->GetContents(),
                                                             Tanks[i]->GetXYZ());
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
      if (Engines[i]->GetType() == FGEngine::etPiston)
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

int FGPropulsion::GetStarter(void) const
{
  if (ActiveEngine < 0) {
    bool starter = true;

    for (unsigned i=0; i<Engines.size(); i++)
      starter &= Engines[i]->GetStarter();

    return starter ? 1 : 0;
  } else
    return Engines[ActiveEngine]->GetStarter() ? 1: 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetCutoff(int setting)
{
  bool bsetting = setting == 0 ? false : true;

  if (ActiveEngine < 0) {
    for (unsigned i=0; i<Engines.size(); i++) {
      switch (Engines[i]->GetType()) { 
        case FGEngine::etTurbine:
          ((FGTurbine*)Engines[i])->SetCutoff(bsetting);
          break;
        case FGEngine::etTurboprop:
          ((FGTurboProp*)Engines[i])->SetCutoff(bsetting);
          break;
        default:
          break;
      }
    }
  } else {
    switch (Engines[ActiveEngine]->GetType()) { 
      case FGEngine::etTurbine:
        ((FGTurbine*)Engines[ActiveEngine])->SetCutoff(bsetting);
        break;
      case FGEngine::etTurboprop:
        ((FGTurboProp*)Engines[ActiveEngine])->SetCutoff(bsetting);
        break;
      default:
        break;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPropulsion::GetCutoff(void) const
{
  if (ActiveEngine < 0) {
    bool cutoff = true;

    for (unsigned i=0; i<Engines.size(); i++) {
      switch (Engines[i]->GetType()) { 
      case FGEngine::etTurbine:
        cutoff &= ((FGTurbine*)Engines[i])->GetCutoff();
        break;
      case FGEngine::etTurboprop:
        cutoff &= ((FGTurboProp*)Engines[i])->GetCutoff();
        break;
      default:
        return -1;
      }
    }

    return cutoff ? 1 : 0;
  } else {
    switch (Engines[ActiveEngine]->GetType()) {
    case FGEngine::etTurbine:
      return ((FGTurbine*)Engines[ActiveEngine])->GetCutoff() ? 1 : 0;
    case FGEngine::etTurboprop:
      return ((FGTurboProp*)Engines[ActiveEngine])->GetCutoff() ? 1 : 0;
    default:
      break;
    }
  }

  return -1;
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

  double fillrate = RefuelRate / 60.0 * time_slice;   
  int TanksNotFull = 0;

  for (i=0; i<numTanks; i++) {
    if (Tanks[i]->GetPctFull() < 99.99) ++TanksNotFull;
  }

  // adds fuel equally to all tanks that are not full
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
  FuelFreeze = f;
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
  PropertyManager->Tie("propulsion/set-running", this, (iPMF)0, &FGPropulsion::InitRunning, false);
  if (HaveTurbineEngine || HaveTurboPropEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, &FGPropulsion::GetStarter, &FGPropulsion::SetStarter);
    PropertyManager->Tie("propulsion/cutoff_cmd", this,  &FGPropulsion::GetCutoff, &FGPropulsion::SetCutoff);
  }

  if (HavePistonEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, &FGPropulsion::GetStarter, &FGPropulsion::SetStarter);
    PropertyManager->Tie("propulsion/magneto_cmd", this, (iPMF)0, &FGPropulsion::SetMagnetos, false);
  }

  PropertyManager->Tie("propulsion/active_engine", this, (iPMF)&FGPropulsion::GetActiveEngine,
                        &FGPropulsion::SetActiveEngine, true);
  PropertyManager->Tie("forces/fbx-prop-lbs", this, eX, (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fby-prop-lbs", this, eY, (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fbz-prop-lbs", this, eZ, (PMF)&FGPropulsion::GetForces);
  PropertyManager->Tie("moments/l-prop-lbsft", this, eX, (PMF)&FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/m-prop-lbsft", this, eY, (PMF)&FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/n-prop-lbsft", this, eZ, (PMF)&FGPropulsion::GetMoments);
  TotalFuelQuantity = PropertyManager->CreatePropertyObject<double>("propulsion/total-fuel-lbs");
  TotalOxidizerQuantity = PropertyManager->CreatePropertyObject<double>("propulsion/total-oxidizer-lbs");
  refuel = PropertyManager->CreatePropertyObject<bool>("propulsion/refuel");
  dump = PropertyManager->CreatePropertyObject<bool>("propulsion/fuel_dump");
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
    }
  }
}
}
