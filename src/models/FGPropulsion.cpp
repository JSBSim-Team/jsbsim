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
#include <array>

#include "FGFDMExec.h"
#include "FGPropulsion.h"
#include "input_output/FGModelLoader.h"
#include "input_output/FGLog.h"
#include "models/FGMassBalance.h"
#include "models/propulsion/FGRocket.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGElectric.h"
#include "models/propulsion/FGTurboProp.h"
#include "models/propulsion/FGTank.h"
#include "models/propulsion/FGBrushLessDCMotor.h"
#include "models/FGFCS.h"


using namespace std;

namespace JSBSim {

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  Name = "FGPropulsion";

  ActiveEngine = -1; // -1: ALL, 0: Engine 1, 1: Engine 2 ...
  tankJ.InitMatrix();
  DumpRate = 0.0;
  RefuelRate = 6000.0;
  FuelFreeze = false;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropulsion::~FGPropulsion()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::InitModel(void)
{
  bool result = true;

  if (!FGModel::InitModel()) return false;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (auto& tank: Tanks) tank->ResetToIC();
  TotalFuelQuantity = 0.0;
  TotalOxidizerQuantity = 0.0;
  refuel = dump = false;

  for (auto& engine: Engines) engine->ResetToIC();

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  RunPreFunctions();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (auto& engine: Engines) {
    engine->Calculate();
    ConsumeFuel(engine.get());
    vForces  += engine->GetBodyForces();  // sum body frame forces
    vMoments += engine->GetMoments();     // sum body frame moments
  }

  TotalFuelQuantity = 0.0;
  TotalOxidizerQuantity = 0.0;
  for (auto& tank: Tanks) {
    tank->Calculate( in.TotalDeltaT, in.TAT_c);
    switch (tank->GetType()) {
    case FGTank::ttFUEL:
      TotalFuelQuantity += tank->GetContents();
      break;
    case FGTank::ttOXIDIZER:
      TotalOxidizerQuantity += tank->GetContents();
      break;
    default:
      break;
    }
  }

  if (refuel) DoRefuel( in.TotalDeltaT );
  if (dump) DumpFuel( in.TotalDeltaT );

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
  size_t numTanks = Tanks.size();

  // Process fuel tanks, if any
  while ((TanksWithFuel == 0) && (CurrentFuelTankPriority <= numTanks)) {
    for (unsigned int i=0; i<engine->GetNumSourceTanks(); i++) {
      unsigned int TankId = engine->GetSourceTank(i);
      const auto& Tank = Tanks[TankId];
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
        const auto& Tank = Tanks[TankId];
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
  for (const auto& feed: FeedListFuel)
    Tanks[feed]->Drain(FuelNeededPerTank);

  if (engine->GetType() == FGEngine::etRocket) {
    double OxidizerToBurn = engine->CalcOxidizerNeed();                // How much fuel does this engine need?
    double OxidizerNeededPerTank = 0;
    if (TanksWithOxidizer > 0) OxidizerNeededPerTank = OxidizerToBurn / TanksWithOxidizer; // Determine fuel needed per tank.
    for (const auto& feed: FeedListOxi)
      Tanks[feed]->Drain(OxidizerNeededPerTank);
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

    for (auto& engine: Engines) {
      steady=false;
      steady_count=0;
      j=0;
      while (!steady && j < 6000) {
        engine->Calculate();
        lastThrust = currentThrust;
        currentThrust = engine->GetThrust();
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
      vForces  += engine->GetBodyForces();  // sum body frame forces
      vMoments += engine->GetMoments();     // sum body frame moments
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
      LogException err(FDMExec->GetLogger());
      err << "Tried to initialize a non-existent engine!";
      throw err;
    }

    SetEngineRunning(n);

  } else if (n < 0) { // -1 refers to "All Engines"

    for (unsigned int i=0; i<GetNumEngines(); i++) {
      SetEngineRunning(i);
    }
  }

  GetSteadyState();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetEngineRunning(int engineIndex)
{
  in.ThrottleCmd[engineIndex] = in.ThrottlePos[engineIndex] = 1; // Set the throttle command and position
  in.MixtureCmd[engineIndex] = in.MixturePos[engineIndex] = 1;   // Set the mixture command and position
  FDMExec->GetFCS()->SetMixturePos(engineIndex, 1);    // Also set FCS values
  FDMExec->GetFCS()->SetMixtureCmd(engineIndex, 1);
  GetEngine(engineIndex)->InitRunning();
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
  unsigned int numTanks = 0;

  while (tank_element) {
    Tanks.push_back(make_shared<FGTank>(FDMExec, tank_element, numTanks));
    const auto& tank = Tanks.back();
    if (tank->GetType() == FGTank::ttFUEL)
      FuelDensity = tank->GetDensity();
    else if (tank->GetType() != FGTank::ttOXIDIZER) {
      FGXMLLogging log(FDMExec->GetLogger(), tank_element, LogLevel::ERROR);
      log << "Unknown tank type specified.\n";
      return false;
    }
    numTanks++;
    tank_element = el->FindNextElement("tank");
  }

  ReadingEngine = true;
  Element* engine_element = el->FindElement("engine");
  unsigned int numEngines = 0;

  while (engine_element) {
    if (!ModelLoader.Open(engine_element)) return false;

    try {
      // Locate the thruster definition
      Element* thruster_element = engine_element->FindElement("thruster");
      if (!thruster_element) {
        XMLLogException err(FDMExec->GetLogger(), engine_element);
        err << "No thruster definition supplied with engine definition.";
        throw err;
      }
      if (!ModelLoader.Open(thruster_element)) {
        XMLLogException err(FDMExec->GetLogger(), thruster_element);
        err << "Cannot open the thruster element.";
        throw err;
      }

      if (engine_element->FindElement("piston_engine")) {
        Element *element = engine_element->FindElement("piston_engine");
        Engines.push_back(make_shared<FGPiston>(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("turbine_engine")) {
        Element *element = engine_element->FindElement("turbine_engine");
        Engines.push_back(make_shared<FGTurbine>(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("turboprop_engine")) {
        Element *element = engine_element->FindElement("turboprop_engine");
        Engines.push_back(make_shared<FGTurboProp>(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("rocket_engine")) {
        Element *element = engine_element->FindElement("rocket_engine");
        Engines.push_back(make_shared<FGRocket>(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("electric_engine")) {
        Element *element = engine_element->FindElement("electric_engine");
        Engines.push_back(make_shared<FGElectric>(FDMExec, element, numEngines, in));
      } else if (engine_element->FindElement("brushless_dc_motor")) {
        Element *element = engine_element->FindElement("brushless_dc_motor");
        Engines.push_back(make_shared<FGBrushLessDCMotor>(FDMExec, element, numEngines, in));
      }
      else {
        FGXMLLogging log(FDMExec->GetLogger(), engine_element, LogLevel::ERROR);
        log << " Unknown engine type\n";
        return false;
      }
    } catch (XMLLogException& err) {
      err << "Cannot load " << Name << "\n";
      return false;
    } catch (LogException& e) {
      XMLLogException err(e, engine_element);
      err << "Cannot load " << Name << "\n";
      return false;
    } catch (const BaseException& e) {
      FGXMLLogging err(FDMExec->GetLogger(), engine_element, LogLevel::FATAL);
      err << "\n" << LogFormat::RED << e.what() << LogFormat::RESET
          << "\nCannot load " << Name << "\n";
      return false;
    }

    numEngines++;

    engine_element = el->FindNextElement("engine");
  }

  if (numEngines) bind();

  CalculateTankInertias();

  if (el->FindElement("dump-rate"))
    DumpRate = el->FindElementValueAsNumberConvertTo("dump-rate", "LBS/MIN");
  if (el->FindElement("refuel-rate"))
    RefuelRate = el->FindElementValueAsNumberConvertTo("refuel-rate", "LBS/MIN");

  for (auto& engine: Engines)
    engine->SetFuelDensity(FuelDensity);

  PostLoad(el, FDMExec);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPath FGPropulsion::FindFullPathName(const SGPath& path) const
{
  SGPath name = FGModel::FindFullPathName(path);
  if (!ReadingEngine && !name.isNull()) return name;

#ifdef _WIN32
  // Singular and plural are allowed for the folder names for consistency with
  // the default engine folder name "engine" and for backward compatibility
  // regarding the folder name "Engines".
  const array<string, 2> dir_names = {"Engines", "engine"};
#else
  // Allow alternative capitalization for case sensitive OSes.
  const array<string, 4> dir_names = {"Engines", "engines", "Engine", "engine"};
#endif

  for(const string& dir_name: dir_names) {
    name = CheckPathName(FDMExec->GetFullAircraftPath()/dir_name, path);
    if (!name.isNull()) return name;
  }

  return CheckPathName(FDMExec->GetEnginePath(), path);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionStrings(const string& delimiter) const
{
  unsigned int i = 0;

  string PropulsionStrings;
  bool firstime = true;
  stringstream buf;

  for (auto& engine: Engines) {
    if (firstime)  firstime = false;
    else           PropulsionStrings += delimiter;

    PropulsionStrings += engine->GetEngineLabels(delimiter);
  }
  for (auto& tank: Tanks) {
    if (tank->GetType() == FGTank::ttFUEL) buf << delimiter << "Fuel Tank " << i++;
    else if (tank->GetType() == FGTank::ttOXIDIZER) buf << delimiter << "Oxidizer Tank " << i++;

    const string& name = tank->GetName();
    if (!name.empty()) buf << " (" << name << ")";
  }

  PropulsionStrings += buf.str();

  return PropulsionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionValues(const string& delimiter) const
{
  string PropulsionValues;
  bool firstime = true;
  stringstream buf;

  for (const auto& engine: Engines) {
    if (firstime)  firstime = false;
    else           PropulsionValues += delimiter;

    PropulsionValues += engine->GetEngineValues(delimiter);
  }
  for (const auto& tank: Tanks) {
    buf << delimiter;
    buf << tank->GetContents();
  }

  PropulsionValues += buf.str();

  return PropulsionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionTankReport()
{
  string out;
  stringstream outstream;
  unsigned int i = 0;

  /*const FGMatrix33& mTkI =*/ CalculateTankInertias();

  for (const auto& tank: Tanks) {
    string tankdesc;
    const string& tankname = tank->GetName();
    if (!tankname.empty()) tankdesc = tankname + " (";
    if (tank->GetType() == FGTank::ttFUEL && tank->GetGrainType() != FGTank::gtUNKNOWN) {
      tankdesc += "Solid Fuel";
    } else if (tank->GetType() == FGTank::ttFUEL) {
      tankdesc += "Fuel";
    } else if (tank->GetType() == FGTank::ttOXIDIZER) {
      tankdesc += "Oxidizer";
    } else {
      tankdesc += "Unknown tank type";
    }
    if (!tankname.empty()) tankdesc += ")";
    outstream << highint << left << setw(4) << i++ << setw(30) << tankdesc << normint
      << right << setw(12) << tank->GetContents() << setw(8) << tank->GetXYZ(eX)
      << setw(8) << tank->GetXYZ(eY) << setw(8) << tank->GetXYZ(eZ)
      << setw(12) << tank->GetIxx() << setw(12) << tank->GetIyy()
      << setw(12) << tank->GetIzz() << "\n";
  }
  return outstream.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGPropulsion::GetTanksMoment(void)
{
  vXYZtank_arm.InitMatrix();
  for (const auto& tank: Tanks)
    vXYZtank_arm += tank->GetXYZ() * tank->GetContents();

  return vXYZtank_arm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksWeight(void) const
{
  double Tw = 0.0;

  for (const auto& tank: Tanks) Tw += tank->GetContents();

  return Tw;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGPropulsion::CalculateTankInertias(void)
{
  if (Tanks.empty()) return tankJ;

  tankJ.InitMatrix();

  for (const auto& tank: Tanks) {
    tankJ += FDMExec->GetMassBalance()->GetPointmassInertia( lbtoslug * tank->GetContents(),
                                                             tank->GetXYZ());
    tankJ(1,1) += tank->GetIxx();
    tankJ(2,2) += tank->GetIyy();
    tankJ(3,3) += tank->GetIzz();
  }

  return tankJ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetMagnetos(int setting)
{
  if (ActiveEngine < 0) {
    for (auto& engine: Engines) {
      // ToDo: first need to make sure the engine Type is really appropriate:
      //   do a check to see if it is of type Piston. This should be done for
      //   all of this kind of possibly across-the-board settings.
      if (engine->GetType() == FGEngine::etPiston)
        static_pointer_cast<FGPiston>(engine)->SetMagnetos(setting);
    }
  } else {
    auto engine = dynamic_pointer_cast<FGPiston>(Engines[ActiveEngine]);
    if (engine)
      engine->SetMagnetos(setting);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetStarter(int setting)
{
  if (ActiveEngine < 0) {
    for (auto& engine: Engines) {
      if (setting == 0)
        engine->SetStarter(false);
      else
        engine->SetStarter(true);
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

    for (auto& engine: Engines)
      starter &= engine->GetStarter();

    return starter ? 1 : 0;
  } else
    return Engines[ActiveEngine]->GetStarter() ? 1: 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetCutoff(int setting)
{
  bool bsetting = setting == 0 ? false : true;

  if (ActiveEngine < 0) {
    for (auto& engine: Engines) {
      switch (engine->GetType()) {
        case FGEngine::etTurbine:
          static_pointer_cast<FGTurbine>(engine)->SetCutoff(bsetting);
          break;
        case FGEngine::etTurboprop:
          static_pointer_cast<FGTurboProp>(engine)->SetCutoff(bsetting);
          break;
        default:
          break;
      }
    }
  } else {
    auto engine = Engines[ActiveEngine];
    switch (engine->GetType()) {
      case FGEngine::etTurbine:
        static_pointer_cast<FGTurbine>(engine)->SetCutoff(bsetting);
        break;
      case FGEngine::etTurboprop:
        static_pointer_cast<FGTurboProp>(engine)->SetCutoff(bsetting);
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

    for (auto& engine: Engines) {
      switch (engine->GetType()) {
      case FGEngine::etTurbine:
        cutoff &= static_pointer_cast<FGTurbine>(engine)->GetCutoff();
        break;
      case FGEngine::etTurboprop:
        cutoff &= static_pointer_cast<FGTurboProp>(engine)->GetCutoff();
        break;
      default:
        return -1;
      }
    }

    return cutoff ? 1 : 0;
  } else {
    auto engine = Engines[ActiveEngine];
    switch (engine->GetType()) {
    case FGEngine::etTurbine:
      return static_pointer_cast<FGTurbine>(engine)->GetCutoff() ? 1 : 0;
    case FGEngine::etTurboprop:
      return static_pointer_cast<FGTurboProp>(engine)->GetCutoff() ? 1 : 0;
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
  double fillrate = RefuelRate / 60.0 * time_slice;
  int TanksNotFull = 0;

  for (const auto& tank: Tanks) {
    if (tank->GetPctFull() < 99.99) ++TanksNotFull;
  }

  // adds fuel equally to all tanks that are not full
  if (TanksNotFull) {
    for (unsigned int i=0; i<Tanks.size(); i++) {
      if (Tanks[i]->GetPctFull() < 99.99)
          Transfer(-1, i, fillrate/TanksNotFull);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::DumpFuel(double time_slice)
{
  int TanksDumping = 0;

  for (const auto& tank: Tanks) {
    if (tank->GetContents() > tank->GetStandpipe()) ++TanksDumping;
  }

  if (TanksDumping == 0) return;

  double dump_rate_per_tank = DumpRate / 60.0 * time_slice / TanksDumping;

  for (unsigned int i=0; i<Tanks.size(); i++) {
    if (Tanks[i]->GetContents() > Tanks[i]->GetStandpipe()) {
      Transfer(i, -1, dump_rate_per_tank);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetFuelFreeze(bool f)
{
  FuelFreeze = f;
  for (auto& engine: Engines) engine->SetFuelFreeze(f);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::bind(void)
{
  bool HavePistonEngine = false;
  bool HaveTurboEngine = false;

  for (const auto& engine: Engines) {
    if (!HavePistonEngine && engine->GetType() == FGEngine::etPiston) HavePistonEngine = true;
    if (!HaveTurboEngine && engine->GetType() == FGEngine::etTurbine) HaveTurboEngine = true;
    if (!HaveTurboEngine && engine->GetType() == FGEngine::etTurboprop) HaveTurboEngine = true;
  }

  PropertyManager->Tie<FGPropulsion, int>("propulsion/set-running", this, nullptr,
                                          &FGPropulsion::InitRunning);
  if (HaveTurboEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, &FGPropulsion::GetStarter, &FGPropulsion::SetStarter);
    PropertyManager->Tie("propulsion/cutoff_cmd", this,  &FGPropulsion::GetCutoff, &FGPropulsion::SetCutoff);
  }

  if (HavePistonEngine) {
    PropertyManager->Tie("propulsion/starter_cmd", this, &FGPropulsion::GetStarter, &FGPropulsion::SetStarter);
    PropertyManager->Tie<FGPropulsion, int>("propulsion/magneto_cmd", this,
                                            nullptr, &FGPropulsion::SetMagnetos);
  }

  PropertyManager->Tie("propulsion/active_engine", this, &FGPropulsion::GetActiveEngine,
                        &FGPropulsion::SetActiveEngine);
  PropertyManager->Tie("forces/fbx-prop-lbs", this, eX, &FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fby-prop-lbs", this, eY, &FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fbz-prop-lbs", this, eZ, &FGPropulsion::GetForces);
  PropertyManager->Tie("moments/l-prop-lbsft", this, eX, &FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/m-prop-lbsft", this, eY, &FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/n-prop-lbsft", this, eZ, &FGPropulsion::GetMoments);
  PropertyManager->Tie("propulsion/total-fuel-lbs", &TotalFuelQuantity);
  PropertyManager->Tie("propulsion/total-oxidizer-lbs", &TotalOxidizerQuantity);
  PropertyManager->Tie("propulsion/refuel", &refuel);
  PropertyManager->Tie("propulsion/fuel_dump", &dump);
  PropertyManager->Tie<FGPropulsion, bool>("propulsion/fuel_freeze", this,
                                           nullptr, &FGPropulsion::SetFuelFreeze);
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
      FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
      log << "\n  Propulsion:\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGPropulsion\n";
    if (from == 1) log << "Destroyed:    FGPropulsion\n";
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
