/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropulsion.cpp
 Author:       Jon S. Berndt
 Date started: 08/20/00
 Purpose:      Encapsulates the set of engines, tanks, and thrusters associated
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
comprised of engines, tanks, and "thrusters" (the device that transforms the
engine power into a force that acts on the aircraft, such as a nozzle or
propeller). Once the Propulsion class gets the config file, it reads in
information which is specific to a type of engine. Then:

1) The appropriate engine type instance is created
2) A thruster object is instantiated, and is linked to the engine
3) At least one tank object is created, and is linked to an engine.

At Run time each engines Calculate() method is called to return the excess power
generated during that iteration. The drag from the previous iteration is sub-
tracted to give the excess power available for thrust this pass. That quantity
is passed to the thrusters associated with a particular engine - perhaps with a
scaling mechanism (gearing?) to allow the engine to give its associated thrust-
ers specific distributed portions of the excess power.

HISTORY
--------------------------------------------------------------------------------
08/20/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropulsion.h"
#include "FGRocket.h"
#include "FGTurbine.h"
#include "FGPropeller.h"
#include "FGNozzle.h"
#include "FGPiston.h"
#include "FGElectric.h"
#include "FGPropertyManager.h"

#if defined (__APPLE__)
/* Not all systems have the gcvt function */
inline char* gcvt (double value, int ndigits, char *buf) {
    /* note that this is not exactly what gcvt is supposed to do! */
    snprintf (buf, ndigits+1, "%f", value);
    return buf;
}
#endif

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropulsion.cpp,v 1.103 2004/05/03 10:55:04 frohlich Exp $";
static const char *IdHdr = ID_PROPULSION;

extern short debug_lvl;


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  Name = "FGPropulsion";

  numSelectedFuelTanks = numSelectedOxiTanks = 0;
  numTanks = numEngines = numThrusters = 0;
  numOxiTanks = numFuelTanks = 0;
  dt = 0.0;
  ActiveEngine = -1; // -1: ALL, 0: Engine 1, 1: Engine 2 ...
  tankJ.InitMatrix();

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
  double PowerAvailable;
  dt = State->Getdt();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
      Thrusters[i]->SetdeltaT(dt*rate);
      PowerAvailable = Engines[i]->Calculate(Thrusters[i]->GetPowerRequired());
      Thrusters[i]->Calculate(PowerAvailable);
      vForces  += Thrusters[i]->GetBodyForces();  // sum body frame forces
      vMoments += Thrusters[i]->GetMoments();     // sum body frame moments
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::GetSteadyState(void)
{
  double PowerAvailable;
  double currentThrust = 0, lastThrust=-1;
  dt = State->Getdt();
  int steady_count,j=0;
  bool steady=false;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
      Engines[i]->SetTrimMode(true);
      Thrusters[i]->SetdeltaT(dt*rate);
      steady=false;
      steady_count=0;
      while (!steady && j < 6000) {
        PowerAvailable = Engines[i]->Calculate(Thrusters[i]->GetPowerRequired());
        lastThrust = currentThrust;
        currentThrust = Thrusters[i]->Calculate(PowerAvailable);
        if (fabs(lastThrust-currentThrust) < 0.0001) {
          steady_count++;
          if (steady_count > 120) { steady=true; }
        } else {
          steady_count=0;
        }
        j++;
      }
      vForces  += Thrusters[i]->GetBodyForces();  // sum body frame forces
      vMoments += Thrusters[i]->GetMoments();     // sum body frame moments
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
  double PowerAvailable;
  int j;
  dt = State->Getdt();

  vForces.InitMatrix();
  vMoments.InitMatrix();

  for (unsigned int i=0; i<numEngines; i++) {
    Engines[i]->SetTrimMode(true);
    Thrusters[i]->SetdeltaT(dt*rate);
    j=0;
    while (!Engines[i]->GetRunning() && j < 2000) {
      PowerAvailable = Engines[i]->Calculate(Thrusters[i]->GetPowerRequired());
      Thrusters[i]->Calculate(PowerAvailable);
      j++;
    }
    vForces  += Thrusters[i]->GetBodyForces();  // sum body frame forces
    vMoments += Thrusters[i]->GetMoments();     // sum body frame moments
    Engines[i]->SetTrimMode(false);
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Load(FGConfigFile* AC_cfg)
{
  string token, fullpath, localpath;
  string engineFileName, engType;
  string thrusterFileName, thrType;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  string aircraftPath = FDMExec->GetAircraftPath();
  double xLoc, yLoc, zLoc, Pitch, Yaw;
  double P_Factor = 0, Sense = 0.0;
  int Feed;
  bool ThrottleAdded = false;
  FGConfigFile* Cfg_ptr = 0;

# ifndef macintosh
      fullpath = enginePath + "/";
      localpath = aircraftPath + "/" + FDMExec->GetModelName() + "/Engines/";
# else
      fullpath = enginePath + ";";
      localpath = aircraftPath + ";" + FDMExec->GetModelName() + ";Engines;";
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
          Engines.push_back(new FGRocket(FDMExec, Cfg_ptr));
        } else if (engType == "FG_PISTON") {
          Engines.push_back(new FGPiston(FDMExec, Cfg_ptr));
        } else if (engType == "FG_TURBINE") {
          Engines.push_back(new FGTurbine(FDMExec, Cfg_ptr));
        } else if (engType == "FG_SIMTURBINE") {
          cerr << endl;
          cerr << "The FG_SIMTURBINE engine type has been renamed to FG_TURBINE." << endl;
          cerr << "To fix this problem, simply replace the FG_SIMTURBINE name " << endl;
          cerr << "in your engine file to FG_TURBINE." << endl;
          cerr << endl;
        } else if (engType == "FG_ELECTRIC") {
          Engines.push_back(new FGElectric(FDMExec, Cfg_ptr));
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
          else if (token == "YAW")   { *AC_cfg >> Yaw;}
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
        Engines[numEngines]->SetEngineNumber(numEngines);
        numEngines++;

      } else {

        cerr << fgred << "\n  Could not read engine config file: " << underon <<
                    engineFileName + ".xml" << underoff << fgdef << endl;
        return false;
      }

    } else if (token == "AC_TANK") {              // ============== READING TANKS

      if (debug_lvl > 0) cout << "\n    Reading tank definition" << endl;
      Tanks.push_back(new FGTank(AC_cfg));
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

    } else if (token == "AC_THRUSTER") {          // ========== READING THRUSTERS

      thrusterFileName = AC_cfg->GetValue("FILE");

      // Look in the Aircraft/Engines directory first
      Cfg_ptr = 0;
      FGConfigFile Local_Thruster_cfg(localpath + thrusterFileName + ".xml");
      FGConfigFile Thruster_cfg(fullpath + thrusterFileName + ".xml");
      if (Local_Thruster_cfg.IsOpen()) {
        Cfg_ptr = &Local_Thruster_cfg;
        if (debug_lvl > 0) cout << "\n    Reading thruster from file: " << localpath
                                                + thrusterFileName + ".xml"<< endl;
      } else {
        if (Thruster_cfg.IsOpen()) {
          Cfg_ptr = &Thruster_cfg;
          if (debug_lvl > 0) cout << "\n    Reading thruster from file: " << fullpath
                                                + thrusterFileName + ".xml"<< endl;
        }
      }

      if (Cfg_ptr) {
        Cfg_ptr->GetNextConfigLine();
        thrType = Cfg_ptr->GetValue();

        if (thrType == "FG_PROPELLER") {
          Thrusters.push_back(new FGPropeller(FDMExec, Cfg_ptr));
        } else if (thrType == "FG_NOZZLE") {
          Thrusters.push_back(new FGNozzle(FDMExec, Cfg_ptr));
        } else if (thrType == "FG_DIRECT") {
          Thrusters.push_back(new FGThruster( FDMExec, Cfg_ptr) );
        }

        AC_cfg->GetNextConfigLine();
        while ((token = AC_cfg->GetValue()) != string("/AC_THRUSTER")) {
          *AC_cfg >> token;
          if (token == "XLOC") *AC_cfg >> xLoc;
          else if (token == "YLOC") *AC_cfg >> yLoc;
          else if (token == "ZLOC") *AC_cfg >> zLoc;
          else if (token == "PITCH") *AC_cfg >> Pitch;
          else if (token == "YAW") *AC_cfg >> Yaw;
          else if (token == "P_FACTOR") *AC_cfg >> P_Factor;
          else if (token == "SENSE")   *AC_cfg >> Sense;
          else cerr << "Unknown identifier: " << token << " in engine file: "
                                                        << engineFileName << endl;
        }

        Thrusters[numThrusters]->SetLocation(xLoc, yLoc, zLoc);
        Thrusters[numThrusters]->SetAnglesToBody(0, Pitch, Yaw);
        if (thrType == "FG_PROPELLER" && P_Factor > 0.001) {
          ((FGPropeller*)Thrusters[numThrusters])->SetPFactor(P_Factor);
          if (debug_lvl > 0) cout << "      P-Factor: " << P_Factor << endl;
          ((FGPropeller*)Thrusters[numThrusters])->SetSense(fabs(Sense)/Sense);
          if (debug_lvl > 0) cout << "      Sense: " << Sense <<  endl;
        }
        Thrusters[numThrusters]->SetdeltaT(dt*rate);
        Thrusters[numThrusters]->SetThrusterNumber(numThrusters);
        numThrusters++;

      } else {
        cerr << "Could not read thruster config file: " << fullpath
                                                + thrusterFileName + ".xml" << endl;
        return false;
      }

    }
    AC_cfg->GetNextConfigLine();
  }

  CalculateTankInertias();
  if (!ThrottleAdded) FCS->AddThrottle(); // need to have at least one throttle

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionStrings(void)
{
  string PropulsionStrings = "";
  bool firstime = true;
  char buffer[5];

  for (unsigned int i=0;i<Engines.size();i++) {
    if (firstime)  firstime = false;
    else           PropulsionStrings += ", ";

    sprintf(buffer, "%d", i);

    switch(Engines[i]->GetType()) {
    case FGEngine::etPiston:
      PropulsionStrings += (Engines[i]->GetName() + "_PwrAvail[" + buffer + "]");
      break;
    case FGEngine::etRocket:
      PropulsionStrings += (Engines[i]->GetName() + "_ChamberPress[" + buffer + "]");
      break;
    case FGEngine::etTurbine:
      PropulsionStrings += (Engines[i]->GetName() + "_N1[" + buffer + "], ");
      PropulsionStrings += (Engines[i]->GetName() + "_N2[" + buffer + "]");
      break;
    case FGEngine::etElectric:
      break;
    default:
      PropulsionStrings += "INVALID ENGINE TYPE";
      break;
    }

    PropulsionStrings += ", ";

    FGPropeller* Propeller = (FGPropeller*)Thrusters[i];
    switch(Thrusters[i]->GetType()) {
    case FGThruster::ttNozzle:
      PropulsionStrings += (Thrusters[i]->GetName() + "_Thrust[" + buffer + "]");
      break;
    case FGThruster::ttRotor:
      break;
    case FGThruster::ttPropeller:
      PropulsionStrings += (Thrusters[i]->GetName() + "_Torque[" + buffer + "], ");
      PropulsionStrings += (Thrusters[i]->GetName() + "_PFactor_Roll[" + buffer + "], ");
      PropulsionStrings += (Thrusters[i]->GetName() + "_PFactor_Pitch[" + buffer + "], ");
      PropulsionStrings += (Thrusters[i]->GetName() + "_PFactor_Yaw[" + buffer + "], ");
      PropulsionStrings += (Thrusters[i]->GetName() + "_Thrust[" + buffer + "], ");
      if (Propeller->IsVPitch())
        PropulsionStrings += (Thrusters[i]->GetName() + "_Pitch[" + buffer + "], ");
      PropulsionStrings += (Thrusters[i]->GetName() + "_RPM[" + buffer + "]");
      break;
    case FGThruster::ttDirect:
      PropulsionStrings += (Thrusters[i]->GetName() + "_Thrust[" + buffer + "]");
      break;
    default:
      PropulsionStrings += "INVALID THRUSTER TYPE";
      break;
    }
  }

  return PropulsionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropulsion::GetPropulsionValues(void)
{
  char buff[20];
  string PropulsionValues = "";
  bool firstime = true;

  for (unsigned int i=0;i<Engines.size();i++) {
    if (firstime)  firstime = false;
    else           PropulsionValues += ", ";

    switch(Engines[i]->GetType()) {
    case FGEngine::etPiston:
      PropulsionValues += (string(gcvt(((FGPiston*)Engines[i])->GetPowerAvailable(), 10, buff)));
      break;
    case FGEngine::etRocket:
      PropulsionValues += (string(gcvt(((FGRocket*)Engines[i])->GetChamberPressure(), 10, buff)));
      break;
    case FGEngine::etTurbine:
      PropulsionValues += (string(gcvt(((FGTurbine*)Engines[i])->GetN1(), 10, buff))) + ", ";
      PropulsionValues += (string(gcvt(((FGTurbine*)Engines[i])->GetN2(), 10, buff)));
      break;
    case FGEngine::etElectric:
      break;
    }

    PropulsionValues += ", ";

    switch(Thrusters[i]->GetType()) {
    case FGThruster::ttNozzle:
      PropulsionValues += (string(gcvt(((FGNozzle*)Thrusters[i])->GetThrust(), 10, buff)));
      break;
    case FGThruster::ttRotor:
      break;
    case FGThruster::ttDirect:
      PropulsionValues += (string(gcvt(((FGThruster*)Thrusters[i])->GetThrust(), 10, buff)));
      break;
    case FGThruster::ttPropeller:
      FGPropeller* Propeller = (FGPropeller*)Thrusters[i];
      FGColumnVector3 vPFactor = Propeller->GetPFactor();
      PropulsionValues += string(gcvt(Propeller->GetTorque(), 10, buff)) + ", ";
      PropulsionValues += string(gcvt(vPFactor(eRoll), 10, buff)) + ", ";
      PropulsionValues += string(gcvt(vPFactor(ePitch), 10, buff)) + ", ";
      PropulsionValues += string(gcvt(vPFactor(eYaw), 10, buff)) + ", ";
      PropulsionValues += string(gcvt(Propeller->GetThrust(), 10, buff)) + ", ";
      if (Propeller->IsVPitch())
        PropulsionValues += string(gcvt(Propeller->GetPitch(), 10, buff)) + ", ";
      PropulsionValues += string(gcvt(Propeller->GetRPM(), 10, buff));
      break;
    }
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
