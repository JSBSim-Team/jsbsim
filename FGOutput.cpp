/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file or stdout
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGOutput.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAerodynamics.h"
#include "FGGroundReactions.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGInertial.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.78 2004/03/26 04:51:54 jberndt Exp $";
static const char *IdHdr = ID_OUTPUT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGOutput";
  sFirstPass = dFirstPass = true;
  socket = 0;
  Type = otNone;
  Filename = "";
  SubSystems = 0;
  enabled = true;
  outputInFileName = "";

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutput::~FGOutput()
{
  if (socket) delete socket;
  for (unsigned int i=0; i<OutputProperties.size(); i++) delete OutputProperties[i];

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Run(void)
{
  if (enabled) {
    if (!FGModel::Run()) {

      if (Type == otSocket) {
        SocketOutput();
      } else if (Type == otCSV) {
        DelimitedOutput(Filename);
      } else if (Type == otTerminal) {
        // Not done yet
      } else if (Type == otNone) {
        // Do nothing
      } else {
        // Not a valid type of output
      }
    return false;
    } else {
    return true;
    }
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SetType(string type)
{
  if (type == "CSV") {
    Type = otCSV;
  } else if (type == "TABULAR") {
    Type = otTab;
  } else if (type == "SOCKET") {
    Type = otSocket;
  } else if (type == "TERMINAL") {
    Type = otTerminal;
  } else if (type != string("NONE")) {
    Type = otUnknown;
    cerr << "Unknown type of output specified in config file" << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::DelimitedOutput(string fname)
{
  streambuf* buffer;
  string scratch = "";

  if (fname == "COUT" || fname == "cout") {
    buffer = cout.rdbuf();
  } else {
    datafile.open(fname.c_str());
    buffer = datafile.rdbuf();
  }

  ostream outstream(buffer);

  if (dFirstPass) {
    outstream << "Time";
    if (SubSystems & ssSimulation) {
      // Nothing here, yet
    }
    if (SubSystems & ssAerosurfaces) {
      outstream << ", ";
      outstream << "Aileron Cmd, ";
      outstream << "Elevator Cmd, ";
      outstream << "Rudder Cmd, ";
      outstream << "Flap Cmd, ";
      outstream << "Left Aileron Pos, ";
      outstream << "Right Aileron Pos, ";
      outstream << "Elevator Pos, ";
      outstream << "Rudder Pos, ";
      outstream << "Flap Pos";
    }
    if (SubSystems & ssRates) {
      outstream << ", ";
      outstream << "P, Q, R, ";
      outstream << "Pdot, Qdot, Rdot";
    }
    if (SubSystems & ssVelocities) {
      outstream << ", ";
      outstream << "QBar, ";
      outstream << "Vtotal, ";
      outstream << "UBody, VBody, WBody, ";
      outstream << "UAero, VAero, WAero, ";
      outstream << "Vn, Ve, Vd";
    }
    if (SubSystems & ssForces) {
      outstream << ", ";
      outstream << "Drag, Side, Lift, ";
      outstream << "L/D, ";
      outstream << "Xforce, Yforce, Zforce, ";
      outstream << "xGravity, yGravity, zGravity, ";
      outstream << "xCoriolis, yCoriolis, zCoriolis, ";
      outstream << "xCentrifugal, yCentrifugal, zCentrifugal";
    }
    if (SubSystems & ssMoments) {
      outstream << ", ";
      outstream << "L, M, N";
    }
    if (SubSystems & ssAtmosphere) {
      outstream << ", ";
      outstream << "Rho, ";
      outstream << "NWind, EWind, DWind";
    }
    if (SubSystems & ssMassProps) {
      outstream << ", ";
      outstream << "Ixx, ";
      outstream << "Ixy, ";
      outstream << "Ixz, ";
      outstream << "Iyx, ";
      outstream << "Iyy, ";
      outstream << "Iyz, ";
      outstream << "Izx, ";
      outstream << "Izy, ";
      outstream << "Izz, ";
      outstream << "Mass, ";
      outstream << "Xcg, Ycg, Zcg";
    }
    if (SubSystems & ssPosition) {
      outstream << ", ";
      outstream << "Altitude, ";
      outstream << "Phi, Tht, Psi, ";
      outstream << "Alpha, ";
      outstream << "Beta, ";
      outstream << "Latitude, ";
      outstream << "Longitude, ";
      outstream << "Distance AGL, ";
      outstream << "Runway Radius";
    }
    if (SubSystems & ssCoefficients) {
      scratch = Aerodynamics->GetCoefficientStrings();
      if (scratch.length() != 0) outstream << ", " << scratch;
    }
    if (SubSystems & ssFCS) {
      scratch = FCS->GetComponentStrings();
      if (scratch.length() != 0) outstream << ", " << scratch;
    }
    if (SubSystems & ssGroundReactions) {
      outstream << ", ";
      outstream << GroundReactions->GetGroundReactionStrings();
    }
    if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
      outstream << ", ";
      outstream << Propulsion->GetPropulsionStrings();
    }
    if (OutputProperties.size() > 0) {
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        outstream << ", " << OutputProperties[i]->GetName();
      }
    }

    outstream << endl;
    dFirstPass = false;
  }

  outstream << State->Getsim_time();
  if (SubSystems & ssSimulation) {
  }
  if (SubSystems & ssAerosurfaces) {
    outstream << ", ";
    outstream << FCS->GetDaCmd() << ", ";
    outstream << FCS->GetDeCmd() << ", ";
    outstream << FCS->GetDrCmd() << ", ";
    outstream << FCS->GetDfCmd() << ", ";
    outstream << FCS->GetDaLPos() << ", ";
    outstream << FCS->GetDaRPos() << ", ";
    outstream << FCS->GetDePos() << ", ";
    outstream << FCS->GetDrPos() << ", ";
    outstream << FCS->GetDfPos();
  }
  if (SubSystems & ssRates) {
    outstream << ", ";
    outstream << Rotation->GetPQR() << ", ";
    outstream << Rotation->GetPQRdot();
  }
  if (SubSystems & ssVelocities) {
    outstream << ", ";
    outstream << Auxiliary->Getqbar() << ", ";
    outstream << Auxiliary->GetVt() << ", ";
    outstream << Translation->GetUVW() << ", ";
    outstream << Auxiliary->GetAeroUVW() << ", ";
    outstream << Position->GetVel();
  }
  if (SubSystems & ssForces) {
    outstream << ", ";
    outstream << Aerodynamics->GetvFs() << ", ";
    outstream << Aerodynamics->GetLoD() << ", ";
    outstream << Aircraft->GetForces() << ", ";
    outstream << Inertial->GetGravity() << ", ";
    outstream << Inertial->GetCoriolis() << ", ";
    outstream << Inertial->GetCentrifugal();
  }
  if (SubSystems & ssMoments) {
    outstream << ", ";
    outstream << Aircraft->GetMoments();
  }
  if (SubSystems & ssAtmosphere) {
    outstream << ", ";
    outstream << Atmosphere->GetDensity() << ", ";
    outstream << Atmosphere->GetWindNED();
  }
  if (SubSystems & ssMassProps) {
    outstream << ", ";
    outstream << MassBalance->GetJ() << ", ";
    outstream << MassBalance->GetMass() << ", ";
    outstream << MassBalance->GetXYZcg();
  }
  if (SubSystems & ssPosition) {
    outstream << ", ";
    outstream << Position->Geth() << ", ";
    outstream << Rotation->GetEuler() << ", ";
    outstream << Auxiliary->Getalpha() << ", ";
    outstream << Auxiliary->Getbeta() << ", ";
    outstream << Position->GetLatitude() << ", ";
    outstream << Position->GetLongitude() << ", ";
    outstream << Position->GetDistanceAGL() << ", ";
    outstream << Position->GetRunwayRadius();
  }
  if (SubSystems & ssCoefficients) {
    scratch = Aerodynamics->GetCoefficientValues();
    if (scratch.length() != 0) outstream << ", " << scratch;
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues();
    if (scratch.length() != 0) outstream << ", " << scratch;
  }
  if (SubSystems & ssGroundReactions) {
    outstream << ", ";
    outstream << GroundReactions->GetGroundReactionValues();
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    outstream << ", ";
    outstream << Propulsion->GetPropulsionValues();
  }

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    outstream << ", " << OutputProperties[i]->getDoubleValue();
  }

  outstream << endl;
  outstream.flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketOutput(void)
{
  string asciiData;

  if (socket == NULL) return;
  if (!socket->GetConnectStatus()) return;

  socket->Clear();
  if (sFirstPass) {
    socket->Append("<LABELS>");
    socket->Append("Time");
    socket->Append("Altitude");
    socket->Append("Phi");
    socket->Append("Tht");
    socket->Append("Psi");
    socket->Append("Rho");
    socket->Append("Vtotal");
    socket->Append("UBody");
    socket->Append("VBody");
    socket->Append("WBody");
    socket->Append("UAero");
    socket->Append("VAero");
    socket->Append("WAero");
    socket->Append("Vn");
    socket->Append("Ve");
    socket->Append("Vd");
    socket->Append("Udot");
    socket->Append("Vdot");
    socket->Append("Wdot");
    socket->Append("P");
    socket->Append("Q");
    socket->Append("R");
    socket->Append("PDot");
    socket->Append("QDot");
    socket->Append("RDot");
    socket->Append("Fx");
    socket->Append("Fy");
    socket->Append("Fz");
    socket->Append("Latitude");
    socket->Append("Longitude");
    socket->Append("QBar");
    socket->Append("Alpha");
    socket->Append("L");
    socket->Append("M");
    socket->Append("N");
    socket->Append("Throttle Position");
    socket->Append("Left Aileron Position");
    socket->Append("Right Aileron Position");
    socket->Append("Elevator Position");
    socket->Append("Rudder Position");
    sFirstPass = false;
    socket->Send();
  }

  socket->Clear();
  socket->Append(State->Getsim_time());
  socket->Append(Position->Geth());
  socket->Append(Rotation->Getphi());
  socket->Append(Rotation->Gettht());
  socket->Append(Rotation->Getpsi());
  socket->Append(Atmosphere->GetDensity());
  socket->Append(Auxiliary->GetVt());
  socket->Append(Translation->GetUVW(eU));
  socket->Append(Translation->GetUVW(eV));
  socket->Append(Translation->GetUVW(eW));
  socket->Append(Auxiliary->GetAeroUVW(eU));
  socket->Append(Auxiliary->GetAeroUVW(eV));
  socket->Append(Auxiliary->GetAeroUVW(eW));
  socket->Append(Position->GetVn());
  socket->Append(Position->GetVe());
  socket->Append(Position->GetVd());
  socket->Append(Translation->GetUVWdot(eU));
  socket->Append(Translation->GetUVWdot(eV));
  socket->Append(Translation->GetUVWdot(eW));
  socket->Append(Rotation->GetPQR(eP));
  socket->Append(Rotation->GetPQR(eQ));
  socket->Append(Rotation->GetPQR(eR));
  socket->Append(Rotation->GetPQRdot(eP));
  socket->Append(Rotation->GetPQRdot(eQ));
  socket->Append(Rotation->GetPQRdot(eR));
  socket->Append(Aircraft->GetForces(eX));
  socket->Append(Aircraft->GetForces(eY));
  socket->Append(Aircraft->GetForces(eZ));
  socket->Append(Position->GetLatitude());
  socket->Append(Position->GetLongitude());
  socket->Append(Auxiliary->Getqbar());
  socket->Append(Auxiliary->Getalpha());
  socket->Append(Aircraft->GetMoments(eL));
  socket->Append(Aircraft->GetMoments(eM));
  socket->Append(Aircraft->GetMoments(eN));
  socket->Append(FCS->GetThrottlePos(0));
  socket->Append(FCS->GetDaLPos());
  socket->Append(FCS->GetDaRPos());
  socket->Append(FCS->GetDePos());
  socket->Append(FCS->GetDrPos());
  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketStatusOutput(string out_str)
{
  string asciiData;

  if (socket == NULL) return;

  socket->Clear();
  asciiData = string("<STATUS>") + out_str;
  socket->Append(asciiData.c_str());
  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Load(FGConfigFile* AC_cfg)
{
  string token="", parameter="", separator="";
  string name="", fname="";
  int OutRate = 0;
  FGConfigFile* Output_cfg;
  string property;

# ifndef macintosh
    separator = "/";
# else
    separator = ";";
# endif

  name = AC_cfg->GetValue("NAME");
  fname = AC_cfg->GetValue("FILE");
  token = AC_cfg->GetValue("TYPE");
  Output->SetType(token);

#if defined( FG_WITH_JSBSIM_SOCKET ) || !defined( FGFS )
  if (token == "SOCKET") {
    socket = new FGfdmSocket("localhost",1138);
  }
#endif

  if (!fname.empty()) {
    outputInFileName = FDMExec->GetAircraftPath() + separator
                        + FDMExec->GetModelName() + separator + fname + ".xml";
    Output_cfg = new FGConfigFile(outputInFileName);
    if (!Output_cfg->IsOpen()) {
      cerr << "Could not open file: " << outputInFileName << endl;
      return false;
    }
  } else {
    Output_cfg = AC_cfg;
  }
  Output->SetFilename(name);

  while ((token = Output_cfg->GetValue()) != string("/OUTPUT")) {
    *Output_cfg >> parameter;
    if (parameter == "RATE_IN_HZ") {
      *Output_cfg >> OutRate;
    }
    if (parameter == "SIMULATION") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssSimulation;
    }
    if (parameter == "AEROSURFACES") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssAerosurfaces;
    }
    if (parameter == "RATES") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssRates;
    }
    if (parameter == "VELOCITIES") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssVelocities;
    }
    if (parameter == "FORCES") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssForces;
    }
    if (parameter == "MOMENTS") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssMoments;
    }
    if (parameter == "ATMOSPHERE") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssAtmosphere;
    }
    if (parameter == "MASSPROPS") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssMassProps;
    }
    if (parameter == "POSITION") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssPosition;
    }
    if (parameter == "COEFFICIENTS") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssCoefficients;
    }
    if (parameter == "GROUND_REACTIONS") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssGroundReactions;
    }
    if (parameter == "FCS") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssFCS;
    }
    if (parameter == "PROPULSION") {
      *Output_cfg >> parameter;
      if (parameter == "ON") SubSystems += ssPropulsion;
    }
    if (parameter == "PROPERTY") {
      *Output_cfg >> property;
      OutputProperties.push_back(PropertyManager->GetNode(property));
    }

    if (parameter == "EOF") break;
  }

  OutRate = OutRate>120?120:(OutRate<0?0:OutRate);
  rate = (int)(0.5 + 1.0/(State->Getdt()*OutRate));

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

void FGOutput::Debug(int from)
{
  string scratch="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
    if (from == 2) {
      if (outputInFileName.empty())
        cout << "  " << "Output parameters read inline" << endl;
      else
        cout << "    Output parameters read from file: " << outputInFileName << endl;

      if (Filename == "cout" || Filename == "COUT") {
        scratch = "    Log output goes to screen console";
      } else if (!Filename.empty()) {
        scratch = "    Log output goes to file: " + Filename;
      }
      switch (Type) {
      case otCSV:
        cout << scratch << " in CSV format output at rate " << 120/rate << " Hz" << endl;
        break;
      case otNone:
        cout << "  No log output" << endl;
        break;
      }

      if (SubSystems & ssSimulation)      cout << "    Simulation parameters logged" << endl;
      if (SubSystems & ssAerosurfaces)    cout << "    Aerosurface parameters logged" << endl;
      if (SubSystems & ssRates)           cout << "    Rate parameters logged" << endl;
      if (SubSystems & ssVelocities)      cout << "    Velocity parameters logged" << endl;
      if (SubSystems & ssForces)          cout << "    Force parameters logged" << endl;
      if (SubSystems & ssMoments)         cout << "    Moments parameters logged" << endl;
      if (SubSystems & ssAtmosphere)      cout << "    Atmosphere parameters logged" << endl;
      if (SubSystems & ssMassProps)       cout << "    Mass parameters logged" << endl;
      if (SubSystems & ssCoefficients)    cout << "    Coefficient parameters logged" << endl;
      if (SubSystems & ssPosition)        cout << "    Position parameters logged" << endl;
      if (SubSystems & ssGroundReactions) cout << "    Ground parameters logged" << endl;
      if (SubSystems & ssFCS)             cout << "    FCS parameters logged" << endl;
      if (SubSystems & ssPropulsion)      cout << "    Propulsion parameters logged" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGOutput" << endl;
    if (from == 1) cout << "Destroyed:    FGOutput" << endl;
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
