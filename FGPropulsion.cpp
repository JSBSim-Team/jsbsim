/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropulsion.cpp
 Author:       Jon S. Berndt
 Date started: 08/20/00
 Purpose:      Encapsulates the set of engines and tanks associated
               with this aircraft

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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
#include "FGRocket.h"
#include "FGTurbine.h"
#include "FGPiston.h"
#include "FGElectric.h"
#include "FGPropertyManager.h"
#include <sstream>

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropulsion.cpp,v 1.116 2004/12/06 03:59:52 dpculp Exp $";
static const char *IdHdr = ID_PROPULSION;

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
  refuel = false;
  fuel_freeze = false;

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropulsion::~FGPropulsion()
{
  for (unsigned int i=0; i<Engines.size(); i++) delete Engines[i];
  Engines.clear();
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Run(void)
{
  unsigned int i;

  if (FGModel::Run()) return true;

  double dt = State->Getdt();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (i=0; i<numEngines; i++) {
    Engines[i]->Calculate();
    vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
    vMoments += Engines[i]->GetMoments();     // sum body frame moments
  }

  for (i=0; i<numTanks; i++) {
    Tanks[i]->Calculate( dt * rate );
  }

  if (refuel) DoRefuel( dt * rate );
  
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::GetSteadyState(void)
{
  double currentThrust = 0, lastThrust=-1;
  int steady_count,j=0;
  bool steady=false;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
      Engines[i]->SetTrimMode(true);
      steady=false;
      steady_count=0;
      while (!steady && j < 6000) {
        Engines[i]->Calculate();
        lastThrust = currentThrust;
        currentThrust = Engines[i]->GetThrust();
        if (fabs(lastThrust-currentThrust) < 0.0001) {
          steady_count++;
          if (steady_count > 120) { steady=true; }
        } else {
          steady_count=0;
        }
        j++;
      }
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

bool FGPropulsion::ICEngineStart(void)
{
  int j;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (unsigned int i=0; i<numEngines; i++) {
    Engines[i]->SetTrimMode(true);
    j=0;
    while (!Engines[i]->GetRunning() && j < 2000) {
      Engines[i]->Calculate();
      j++;
    }
    vForces  += Engines[i]->GetBodyForces();  // sum body frame forces
    vMoments += Engines[i]->GetMoments();     // sum body frame moments
    Engines[i]->SetTrimMode(false);
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Load(FGConfigFile* AC_cfg)
{
  string token, fullpath, localpath;
  string engineFileName, engType;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  string aircraftPath = FDMExec->GetAircraftPath();
  double xLoc, yLoc, zLoc, Pitch, Yaw;
  int Feed;
  bool ThrottleAdded = false;
  FGConfigFile* Cfg_ptr = 0;

# ifndef macintosh
      fullpath = enginePath + "/";
      localpath = aircraftPath + "/Engines/";
# else
      fullpath = enginePath + ";";
      localpath = aircraftPath +  ";Engines;";
# endif

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/PROPULSION")) {

    if (token == "AC_ENGINE") {                   // ============ READING ENGINES

      engineFileName = AC_cfg->GetValue("FILE");

      // Look in the Aircraft/Engines directory first
      Cfg_ptr = 0;
      FGConfigFile Local_cfg(localpath + engineFileName + ".xml");
      FGConfigFile Eng_cfg(fullpath + engineFileName + ".xml");
      if (Local_cfg.IsOpen()) {
        Cfg_ptr = &Local_cfg;
        if (debug_lvl > 0) cout << "\n    Reading engine from file: " << localpath
                                                + engineFileName + ".xml"<< endl;
      } else {
        if (Eng_cfg.IsOpen()) {
          Cfg_ptr = &Eng_cfg;
          if (debug_lvl > 0) cout << "\n    Reading engine from file: " << fullpath
                                                + engineFileName + ".xml"<< endl;
        }
      }

      if (Cfg_ptr) {
        Cfg_ptr->GetNextConfigLine();
        engType = Cfg_ptr->GetValue();

        FCS->AddThrottle();
        ThrottleAdded = true;

        if (engType == "FG_ROCKET") {
          Engines.push_back(new FGRocket(FDMExec, Cfg_ptr, numEngines));
        } else if (engType == "FG_PISTON") {
          Engines.push_back(new FGPiston(FDMExec, Cfg_ptr, numEngines));
        } else if (engType == "FG_TURBINE") {
          Engines.push_back(new FGTurbine(FDMExec, Cfg_ptr, numEngines));
        } else if (engType == "FG_SIMTURBINE") {
          cerr << endl;
          cerr << "The FG_SIMTURBINE engine type has been renamed to FG_TURBINE." << endl;
          cerr << "To fix this problem, simply replace the FG_SIMTURBINE name " << endl;
          cerr << "in your engine file to FG_TURBINE." << endl;
          cerr << endl;
          Engines.push_back(new FGTurbine(FDMExec, Cfg_ptr, numEngines));
        } else if (engType == "FG_ELECTRIC") {
          Engines.push_back(new FGElectric(FDMExec, Cfg_ptr, numEngines));
        } else {
          cerr << fgred << "    Unrecognized engine type: " << underon << engType
                    << underoff << " found in config file." << fgdef << endl;
          return false;
        }

        AC_cfg->GetNextConfigLine();
        while ((token = AC_cfg->GetValue()) != string("/AC_ENGINE")) {
          *AC_cfg >> token;
          if      (token == "XLOC")  { *AC_cfg >> xLoc; }
          else if (token == "YLOC")  { *AC_cfg >> yLoc; }
          else if (token == "ZLOC")  { *AC_cfg >> zLoc; }
          else if (token == "PITCH") { *AC_cfg >> Pitch;}
          else if (token == "YAW")   { *AC_cfg >> Yaw;  }
          else if (token.find("AC_THRUSTER") != string::npos) {
            if (debug_lvl > 0) cout << "\n    Reading thruster definition" << endl;
              Engines.back()->LoadThruster(AC_cfg);
              AC_cfg->GetNextConfigLine();
          }
          else if (token == "FEED")  {
            *AC_cfg >> Feed;
            Engines[numEngines]->AddFeedTank(Feed);
            if (debug_lvl > 0) cout << "      Feed tank: " << Feed << endl;
          } else cerr << "Unknown identifier: " << token << " in engine file: "
                                                        << engineFileName << endl;
        }

        if (debug_lvl > 0)  {
          cout << "      X = " << xLoc << endl;
          cout << "      Y = " << yLoc << endl;
          cout << "      Z = " << zLoc << endl;
          cout << "      Pitch = " << Pitch << endl;
          cout << "      Yaw = " << Yaw << endl;
        }

        Engines[numEngines]->SetPlacement(xLoc, yLoc, zLoc, Pitch, Yaw);
        numEngines++;

      } else {

        cerr << fgred << "\n  Could not read engine config file: " << underon <<
                    engineFileName + ".xml" << underoff << fgdef << endl;
        return false;
      }

    } else if (token == "AC_TANK") {              // ============== READING TANKS

      if (debug_lvl > 0) cout << "\n    Reading tank definition" << endl;
      Tanks.push_back(new FGTank(AC_cfg, FDMExec));
      switch(Tanks[numTanks]->GetType()) {
      case FGTank::ttFUEL:
        numSelectedFuelTanks++;
        numFuelTanks++;
        break;
      case FGTank::ttOXIDIZER:
        numSelectedOxiTanks++;
        numOxiTanks++;
        break;
      }

      numTanks++;
    }
    AC_cfg->GetNextConfigLine();
  }

  CalculateTankInertias();
  if (!ThrottleAdded) FCS->AddThrottle(); // need to have at least one throttle

  return true;
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
  iTank = Tanks.begin();
  vXYZtank_arm.InitMatrix();
  while (iTank < Tanks.end()) {
    vXYZtank_arm(eX) += (*iTank)->GetXYZ(eX)*(*iTank)->GetContents();
    vXYZtank_arm(eY) += (*iTank)->GetXYZ(eY)*(*iTank)->GetContents();
    vXYZtank_arm(eZ) += (*iTank)->GetXYZ(eZ)*(*iTank)->GetContents();
    iTank++;
  }
  return vXYZtank_arm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksWeight(void)
{
  double Tw = 0.0;

  iTank = Tanks.begin();
  while (iTank < Tanks.end()) {
    Tw += (*iTank)->GetContents();
    iTank++;
  }
  return Tw;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGPropulsion::CalculateTankInertias(void)
{
  unsigned int size;

  size = Tanks.size();
  if (size == 0) return tankJ;

  tankJ = FGMatrix33();

  for (unsigned int i=0; i<size; i++)
    tankJ += MassBalance->GetPointmassInertia( lbtoslug * Tanks[i]->GetContents(),
                                               Tanks[i]->GetXYZ() );

  return tankJ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::SetMagnetos(int setting)
{
  if (ActiveEngine < 0) {
    for (unsigned i=0; i<Engines.size(); i++) {
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
  if (engine >= Engines.size() || engine < 0)
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
  double fillrate = 100 * time_slice;   // 100 lbs/sec = 6000 lbs/min
  int TanksNotFull = 0;

  for (unsigned int i=0; i<numTanks; i++) {
    if (Tanks[i]->GetPctFull() < 99.99) ++TanksNotFull;
  }

  if (TanksNotFull) {
    for (unsigned int i=0; i<numTanks; i++) {
      if (Tanks[i]->GetPctFull() < 99.99)
          Transfer(-1, i, fillrate/TanksNotFull);
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

  PropertyManager->Tie("propulsion/magneto_cmd", this,
                       (iPMF)0, &FGPropulsion::SetMagnetos, true);
  PropertyManager->Tie("propulsion/starter_cmd", this,
                       (iPMF)0, &FGPropulsion::SetStarter,  true);
  PropertyManager->Tie("propulsion/cutoff_cmd", this,
                       (iPMF)0, &FGPropulsion::SetCutoff,   true);

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

  PropertyManager->Tie("propulsion/active_engine", this,
           (iPMF)&FGPropulsion::GetActiveEngine, &FGPropulsion::SetActiveEngine, true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::unbind(void)
{
  PropertyManager->Untie("propulsion/magneto_cmd");
  PropertyManager->Untie("propulsion/starter_cmd");
  PropertyManager->Untie("propulsion/cutoff_cmd");
  PropertyManager->Untie("propulsion/active_engine");
  PropertyManager->Untie("forces/fbx-prop-lbs");
  PropertyManager->Untie("forces/fby-prop-lbs");
  PropertyManager->Untie("forces/fbz-prop-lbs");
  PropertyManager->Untie("moments/l-prop-lbsft");
  PropertyManager->Untie("moments/m-prop-lbsft");
  PropertyManager->Untie("moments/n-prop-lbsft");
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
    if (from == 0) { // Constructor

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
