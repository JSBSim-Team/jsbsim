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
#include "FGPropertyManager.h"


static const char *IdSrc = "$Id: FGPropulsion.cpp,v 1.72 2002/03/27 02:44:09 jberndt Exp $";
static const char *IdHdr = ID_PROPULSION;

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
        if(fabs(lastThrust-currentThrust) < 0.0001) {
          steady_count++;
          if(steady_count > 120) { steady=true; }
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
  string token, fullpath;
  string engineFileName, engType;
  string thrusterFileName, thrType;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  double xLoc, yLoc, zLoc, Pitch, Yaw;
  double P_Factor = 0, Sense = 0.0;
  int Feed;
  bool ThrottleAdded = false;

# ifndef macintosh
      fullpath = enginePath + "/";
# else
      fullpath = enginePath + ";";
# endif

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/PROPULSION")) {

    if (token == "AC_ENGINE") {                   // ============ READING ENGINES

      engineFileName = AC_cfg->GetValue("FILE");

      if (debug_lvl > 0) cout << "\n    Reading engine from file: " << fullpath
                                                + engineFileName + ".xml"<< endl;
      FGConfigFile Eng_cfg(fullpath + engineFileName + ".xml");

      if (Eng_cfg.IsOpen()) {
        Eng_cfg.GetNextConfigLine();
        engType = Eng_cfg.GetValue();

        FCS->AddThrottle();
        ThrottleAdded = true;

        if (engType == "FG_ROCKET") {
          Engines.push_back(new FGRocket(FDMExec, &Eng_cfg));
        } else if (engType == "FG_PISTON") {
          Engines.push_back(new FGPiston(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOJET") {
          Engines.push_back(new FGTurboJet(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOSHAFT") {
          Engines.push_back(new FGTurboShaft(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOPROP") {
          Engines.push_back(new FGTurboProp(FDMExec, &Eng_cfg));
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
                    fullpath + engineFileName + ".xml" << underoff << fgdef << endl;
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

      if (debug_lvl > 0) cout << "\n    Reading thruster from file: " <<
                                    fullpath + thrusterFileName + ".xml" << endl;
      FGConfigFile Thruster_cfg(fullpath + thrusterFileName + ".xml");

      if (Thruster_cfg.IsOpen()) {
        Thruster_cfg.GetNextConfigLine();
        thrType = Thruster_cfg.GetValue();

        if (thrType == "FG_PROPELLER") {
          Thrusters.push_back(new FGPropeller(FDMExec, &Thruster_cfg));
        } else if (thrType == "FG_NOZZLE") {
          Thrusters.push_back(new FGNozzle(FDMExec, &Thruster_cfg));
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
    case FGEngine::etTurboJet:
    case FGEngine::etTurboProp:
    case FGEngine::etTurboShaft:
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
    case FGEngine::etTurboJet:
    case FGEngine::etTurboProp:
    case FGEngine::etTurboShaft:
      break;
    }

    PropulsionValues += ", ";

    switch(Thrusters[i]->GetType()) {
    case FGThruster::ttNozzle:
      PropulsionValues += (string(gcvt(((FGNozzle*)Thrusters[i])->GetThrust(), 10, buff)));
      break;
    case FGThruster::ttRotor:
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

FGColumnVector3& FGPropulsion::GetTanksCG(void)
{
  iTank = Tanks.begin();
  vXYZtank.InitMatrix();
  while (iTank < Tanks.end()) {
    vXYZtank(eX) += (*iTank)->GetX()*(*iTank)->GetContents();
    vXYZtank(eY) += (*iTank)->GetY()*(*iTank)->GetContents();
    vXYZtank(eZ) += (*iTank)->GetZ()*(*iTank)->GetContents();
    iTank++;
  }
  return vXYZtank;
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

double FGPropulsion::GetTanksIxx(const FGColumnVector3& vXYZcg)
{
  double I = 0.0;
  iTank = Tanks.begin();
  while (iTank < Tanks.end()) {
    I += ((*iTank)->GetX() - vXYZcg(eX))*((*iTank)->GetX() - vXYZcg(eX)) * (*iTank)->GetContents()/(144.0*Inertial->gravity());
    iTank++;
  }
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksIyy(const FGColumnVector3& vXYZcg)
{
  double I = 0.0;
  iTank = Tanks.begin();
  while (iTank < Tanks.end()) {
    I += ((*iTank)->GetY() - vXYZcg(eY))*((*iTank)->GetY() - vXYZcg(eY)) * (*iTank)->GetContents()/(144.0*Inertial->gravity());
    iTank++;
  }
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksIzz(const FGColumnVector3& vXYZcg)
{
  double I = 0.0;
  iTank = Tanks.begin();
  while (iTank < Tanks.end()) {
    I += ((*iTank)->GetZ() - vXYZcg(eZ))*((*iTank)->GetZ() - vXYZcg(eZ)) * (*iTank)->GetContents()/(144.0*Inertial->gravity());
    iTank++;
  }
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksIxz(const FGColumnVector3& vXYZcg)
{
  double I = 0.0;
  iTank = Tanks.begin();
  while (iTank < Tanks.end()) {
    I += ((*iTank)->GetX() - vXYZcg(eX))*((*iTank)->GetZ() - vXYZcg(eZ)) * (*iTank)->GetContents()/(144.0*Inertial->gravity());
    iTank++;
  }
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropulsion::GetTanksIxy(const FGColumnVector3& vXYZcg)
{
  double I = 0.0;
  iTank = Tanks.begin();
  while (iTank != Tanks.end()) {
    I += ((*iTank)->GetX() - vXYZcg(eX))*((*iTank)->GetY() - vXYZcg(eY)) * (*iTank)->GetContents()/(144.0*Inertial->gravity());
    iTank++;
  }
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::bind(void){
  /* PropertyManager->Tie("propulsion/num-engines", this,
                       &FGPropulsion::GetNumEngines);
  PropertyManager->Tie("propulsion/num-tanks", this,
                       &FGPropulsion::GetNumTanks); */
  PropertyManager->Tie("propulsion/num-sel-fuel-tanks", this,
                       &FGPropulsion::GetnumSelectedFuelTanks);
  PropertyManager->Tie("propulsion/num-sel-ox-tanks", this,
                       &FGPropulsion::GetnumSelectedOxiTanks);
  PropertyManager->Tie("forces/fbx-prop-lbs", this,1,
                       &FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fby-prop-lbs", this,2,
                       &FGPropulsion::GetForces);
  PropertyManager->Tie("forces/fbz-prop-lbs", this,3,
                       &FGPropulsion::GetForces);
  PropertyManager->Tie("moments/l-prop-lbsft", this,1,
                       &FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/m-prop-lbsft", this,2,
                       &FGPropulsion::GetMoments);
  PropertyManager->Tie("moments/n-prop-lbsft", this,3,
                       &FGPropulsion::GetMoments);
  //PropertyManager->Tie("propulsion/tanks-weight-lbs", this,
  //                     &FGPropulsion::GetTanksWeight);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::unbind(void){
  /* PropertyManager->Untie("propulsion/num-engines");
  PropertyManager->Untie("propulsion/num-tanks"); */
  PropertyManager->Untie("propulsion/num-sel-fuel-tanks");
  PropertyManager->Untie("propulsion/num-sel-ox-tanks");
  PropertyManager->Untie("forces/fbx-prop-lbs");
  PropertyManager->Untie("forces/fby-prop-lbs");
  PropertyManager->Untie("forces/fbz-prop-lbs");
  PropertyManager->Untie("moments/l-prop-lbsft");
  PropertyManager->Untie("moments/m-prop-lbsft");
  PropertyManager->Untie("moments/n-prop-lbsft");
  //PropertyManager->Untie("propulsion/tanks-weight-lbs");
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

