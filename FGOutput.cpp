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

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.38 2001/09/28 02:33:44 jberndt Exp $";
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
  Filename = "JSBSim.out";
  SubSystems = 0;
  enabled = true;
  
#ifdef FG_WITH_JSBSIM_SOCKET
  socket = new FGfdmSocket("localhost",1138);
#endif
  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutput::~FGOutput()
{
  if (socket) delete socket;
  if (debug_lvl & 2) cout << "Destroyed:    FGOutput" << endl;
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

    } else {
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
  } else if (type != "NONE"){
    Type = otUnknown;
    cerr << "Unknown type of output specified in config file" << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::DelimitedOutput(string fname)
{
# if defined(sgi) && !defined(__GNUC__)
  ostream_withassign outstream;
# else
  _IO_ostream_withassign outstream;
# endif

  if (fname == "COUT" || fname == "cout") {
    outstream = cout;
  } else {
    datafile.open(fname.c_str());
    outstream = datafile;
  }

  if (dFirstPass) {
    outstream << "Time";
    if (SubSystems & FGAircraft::ssSimulation) {
      // Nothing here, yet
    }
    if (SubSystems & FGAircraft::ssAerosurfaces) {
      outstream << ", ";
      outstream << "Throttle, ";
      outstream << "Aileron Cmd, ";
      outstream << "Elevator Cmd, ";
      outstream << "Rudder Cmd, ";
      outstream << "Aileron Pos, ";
      outstream << "Elevator Pos, ";
      outstream << "Rudder Pos";
    }
    if (SubSystems & FGAircraft::ssRates) {
      outstream << ", ";
      outstream << "P, Q, R";
    }
    if (SubSystems & FGAircraft::ssVelocities) {
      outstream << ", ";
      outstream << "QBar, ";
      outstream << "Vtotal, ";
      outstream << "UBody, VBody, WBody, ";
      outstream << "UAero, VAero, WAero, ";
      outstream << "Vn, Ve, Vd";
    }
    if (SubSystems & FGAircraft::ssForces) {
      outstream << ", ";
      outstream << "Drag, Side, Lift, ";
      outstream << "L/D, ";
      outstream << "Xforce, Yforce, Zforce";
    }
    if (SubSystems & FGAircraft::ssMoments) {
      outstream << ", ";
      outstream << "L, M, N";
    }
    if (SubSystems & FGAircraft::ssAtmosphere) {
      outstream << ", ";
      outstream << "Rho";
    }
    if (SubSystems & FGAircraft::ssMassProps) {
      outstream << ", ";
      outstream << "Ixx, ";
      outstream << "Iyy, ";
      outstream << "Izz, ";
      outstream << "Ixz, ";
      outstream << "Mass, ";
      outstream << "Xcg, Ycg, Zcg";
    }
    if (SubSystems & FGAircraft::ssPosition) {
      outstream << ", ";
      outstream << "Altitude, ";
      outstream << "Phi, Tht, Psi, ";
      outstream << "Alpha, ";
      outstream << "Latitude, ";
      outstream << "Longitude, ";
      outstream << "Distance AGL, ";
      outstream << "Runway Radius";
    }
    if (SubSystems & FGAircraft::ssCoefficients) {
      outstream << ", ";
      outstream << Aerodynamics->GetCoefficientStrings();
    }
    if (SubSystems & FGAircraft::ssGroundReactions) {
      outstream << ", ";
      outstream << GroundReactions->GetGroundReactionStrings();
    }
    if (SubSystems & FGAircraft::ssPropulsion) {
      outstream << ", ";
      outstream << Propulsion->GetPropulsionStrings();
    }

    outstream << endl;
    dFirstPass = false;
  }

  outstream << State->Getsim_time();
  if (SubSystems & FGAircraft::ssSimulation) {
  }
  if (SubSystems & FGAircraft::ssAerosurfaces) {
    outstream << ", ";
    outstream << FCS->GetThrottlePos(0) << ", ";
    outstream << FCS->GetDaCmd() << ", ";
    outstream << FCS->GetDeCmd() << ", ";
    outstream << FCS->GetDrCmd() << ", ";
    outstream << FCS->GetDaPos() << ", ";
    outstream << FCS->GetDePos() << ", ";
    outstream << FCS->GetDrPos();
  }
  if (SubSystems & FGAircraft::ssRates) {
    outstream << ", ";
    outstream << Rotation->GetPQR();
  }
  if (SubSystems & FGAircraft::ssVelocities) {
    outstream << ", ";
    outstream << Translation->Getqbar() << ", ";
    outstream << Translation->GetVt() << ", ";
    outstream << Translation->GetUVW() << ", ";
    outstream << Translation->GetvAero() << ", ";
    outstream << Position->GetVel();
  }
  if (SubSystems & FGAircraft::ssForces) {
    outstream << ", ";
    outstream << Aerodynamics->GetvFs() << ", ";
    outstream << Aerodynamics->GetLoD() << ", ";
    outstream << Aircraft->GetForces();
  }
  if (SubSystems & FGAircraft::ssMoments) {
    outstream << ", ";
    outstream << Aircraft->GetMoments();
  }
  if (SubSystems & FGAircraft::ssAtmosphere) {
    outstream << ", ";
    outstream << Atmosphere->GetDensity();
  }
  if (SubSystems & FGAircraft::ssMassProps) {
    outstream << ", ";
    outstream << MassBalance->GetIxx() << ", ";
    outstream << MassBalance->GetIyy() << ", ";
    outstream << MassBalance->GetIzz() << ", ";
    outstream << MassBalance->GetIxz() << ", ";
    outstream << MassBalance->GetMass() << ", ";
    outstream << MassBalance->GetXYZcg();
  }
  if (SubSystems & FGAircraft::ssPosition) {
    outstream << ", ";
    outstream << Position->Geth() << ", ";
    outstream << Rotation->GetEuler() << ", ";
    outstream << Translation->Getalpha() << ", ";
    outstream << Position->GetLatitude() << ", ";
    outstream << Position->GetLongitude() << ", ";
    outstream << Position->GetDistanceAGL() << ", ";
    outstream << Position->GetRunwayRadius();
  }
  if (SubSystems & FGAircraft::ssCoefficients) {
    outstream << ", ";
    outstream << Aerodynamics->GetCoefficientValues();
  }
  if (SubSystems & FGAircraft::ssGroundReactions) {
    outstream << ", ";
    outstream << GroundReactions->GetGroundReactionValues();
  }
  if (SubSystems & FGAircraft::ssPropulsion) {
    outstream << ", ";
    outstream << Propulsion->GetPropulsionValues();
  }

  outstream << endl;
  outstream.flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketOutput(void)
{
  string asciiData;
  /*
  if (socket == NULL) return;

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
    socket->Append("Throttle");
    socket->Append("Aileron");
    socket->Append("Elevator");
    socket->Append("Rudder");
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
  socket->Append(Translation->GetVt());
  socket->Append(Translation->GetUVW(eU));
  socket->Append(Translation->GetUVW(eV));
  socket->Append(Translation->GetUVW(eW));
  socket->Append(Translation->GetvAero(eU));
  socket->Append(Translation->GetvAero(eV));
  socket->Append(Translation->GetvAero(eW));
  socket->Append(Position->GetVn());
  socket->Append(Position->GetVe());
  socket->Append(Position->GetVd());
  socket->Append(Translation->GetUdot());
  socket->Append(Translation->GetVdot());
  socket->Append(Translation->GetWdot());
  socket->Append(Rotation->GetP());
  socket->Append(Rotation->GetQ());
  socket->Append(Rotation->GetR());
  socket->Append(Rotation->GetPdot());
  socket->Append(Rotation->GetQdot());
  socket->Append(Rotation->GetRdot());
  socket->Append(Aircraft->GetFx());
  socket->Append(Aircraft->GetFy());
  socket->Append(Aircraft->GetFz());
  socket->Append(Position->GetLatitude());
  socket->Append(Position->GetLongitude());
  socket->Append(Translation->Getqbar());
  socket->Append(Translation->Getalpha());
  socket->Append(Aircraft->GetL());
  socket->Append(Aircraft->GetM());
  socket->Append(Aircraft->GetN());
  socket->Append(FCS->GetThrottle(0));
  socket->Append(FCS->GetDa());
  socket->Append(FCS->GetDe());
  socket->Append(FCS->GetDr());
  socket->Send(); */
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

void FGOutput::Debug(void)
{
    //TODO: Add your source code here
}

