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

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.35 2001/08/14 20:31:49 jberndt Exp $";
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
        if (Filename != "COUT" && Filename != "cout" && Filename.size() > 0) {
          DelimitedOutput(Filename);
        } else {
          DelimitedOutput();
        }
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

void FGOutput::DelimitedOutput(void)
{
  if (dFirstPass) {
    cout << "Time";
    if (SubSystems & FGAircraft::ssSimulation) {
      // Nothing here, yet
    }
    if (SubSystems & FGAircraft::ssAerosurfaces) {
      cout << ", ";
      cout << "Throttle, ";
      cout << "Aileron Cmd, ";
      cout << "Elevator Cmd, ";
      cout << "Rudder Cmd, ";
      cout << "Aileron Pos, ";
      cout << "Elevator Pos, ";
      cout << "Rudder Pos";
    }
    if (SubSystems & FGAircraft::ssRates) {
      cout << ", ";
      cout << "P, Q, R";
    }
    if (SubSystems & FGAircraft::ssVelocities) {
      cout << ", ";
      cout << "QBar, ";
      cout << "Vtotal, ";
      cout << "UBody, VBody, WBody, ";
      cout << "UAero, VAero, WAero, ";
      cout << "Vn, Ve, Vd";
    }
    if (SubSystems & FGAircraft::ssForces) {
      cout << ", ";
      cout << "Drag, Side, Lift, ";
      cout << "L/D, ";
      cout << "Xforce, Yforce, Zforce";
    }
    if (SubSystems & FGAircraft::ssMoments) {
      cout << ", ";
      cout << "L, M, N";
    }
    if (SubSystems & FGAircraft::ssAtmosphere) {
      cout << ", ";
      cout << "Rho";
    }
    if (SubSystems & FGAircraft::ssMassProps) {
      cout << ", ";
      cout << "Ixx, ";
      cout << "Iyy, ";
      cout << "Izz, ";
      cout << "Ixz, ";
      cout << "Mass, ";
      cout << "Xcg, Ycg, Zcg";
    }
    if (SubSystems & FGAircraft::ssPosition) {
      cout << ", ";
      cout << "Altitude, ";
      cout << "Phi, Tht, Psi, ";
      cout << "Alpha, ";
      cout << "Latitude, ";
      cout << "Longitude, ";
      cout << "Distance AGL, ";
      cout << "Runway Radius";
    }
    if (SubSystems & FGAircraft::ssCoefficients) {
      cout << ", ";
      cout << Aerodynamics->GetCoefficientStrings();
    }
    if (SubSystems & FGAircraft::ssGroundReactions) {
      cout << ", ";
      cout << GroundReactions->GetGroundReactionStrings();
    }
    if (SubSystems & FGAircraft::ssPropulsion) {
      cout << ", ";
      cout << Propulsion->GetPropulsionStrings();
    }

    cout << endl;
    dFirstPass = false;
  }

  cout << State->Getsim_time();
  if (SubSystems & FGAircraft::ssSimulation) {
  }
  if (SubSystems & FGAircraft::ssAerosurfaces) {
    cout << ", ";
    cout << FCS->GetThrottlePos(0) << ", ";
    cout << FCS->GetDaCmd() << ", ";
    cout << FCS->GetDeCmd() << ", ";
    cout << FCS->GetDrCmd() << ", ";
    cout << FCS->GetDaPos() << ", ";
    cout << FCS->GetDePos() << ", ";
    cout << FCS->GetDrPos();
  }
  if (SubSystems & FGAircraft::ssRates) {
    cout << ", ";
    cout << Rotation->GetPQR();
  }
  if (SubSystems & FGAircraft::ssVelocities) {
    cout << ", ";
    cout << Translation->Getqbar() << ", ";
    cout << Translation->GetVt() << ", ";
    cout << Translation->GetUVW() << ", ";
    cout << Translation->GetvAero() << ", ";
    cout << Position->GetVel();
  }
  if (SubSystems & FGAircraft::ssForces) {
    cout << ", ";
    cout << Aerodynamics->GetvFs() << ", ";
    cout << Aerodynamics->GetLoD() << ", ";
    cout << Aircraft->GetForces();
  }
  if (SubSystems & FGAircraft::ssMoments) {
    cout << ", ";
    cout << Aircraft->GetMoments();
  }
  if (SubSystems & FGAircraft::ssAtmosphere) {
    cout << ", ";
    cout << Atmosphere->GetDensity();
  }
  if (SubSystems & FGAircraft::ssMassProps) {
    cout << ", ";
    cout << MassBalance->GetIxx() << ", ";
    cout << MassBalance->GetIyy() << ", ";
    cout << MassBalance->GetIzz() << ", ";
    cout << MassBalance->GetIxz() << ", ";
    cout << MassBalance->GetMass() << ", ";
    cout << MassBalance->GetXYZcg();
  }
  if (SubSystems & FGAircraft::ssPosition) {
    cout << ", ";
    cout << Position->Geth() << ", ";
    cout << Rotation->GetEuler() << ", ";
    cout << Translation->Getalpha() << ", ";
    cout << Position->GetLatitude() << ", ";
    cout << Position->GetLongitude() << ", ";
    cout << Position->GetDistanceAGL() << ", ";
    cout << Position->GetRunwayRadius();
  }
  if (SubSystems & FGAircraft::ssCoefficients) {
    cout << ", ";
    cout << Aerodynamics->GetCoefficientValues();
  }
  if (SubSystems & FGAircraft::ssGroundReactions) {
    cout << ", ";
    cout << GroundReactions->GetGroundReactionValues();
  }
  if (SubSystems & FGAircraft::ssPropulsion) {
    cout << ", ";
    cout << Propulsion->GetPropulsionValues();
  }

  cout << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::DelimitedOutput(string fname)
{
  if (sFirstPass) {
    datafile.open(fname.c_str());
    datafile << "Time";
    if (SubSystems & FGAircraft::ssSimulation) {
      // Nothing here, yet
    }
    if (SubSystems & FGAircraft::ssAerosurfaces) {
      datafile << ", ";
      datafile << "Throttle, ";
      datafile << "Aileron Cmd, ";
      datafile << "Elevator Cmd, ";
      datafile << "Rudder Cmd, ";
      datafile << "Aileron Pos, ";
      datafile << "Elevator Pos, ";
      datafile << "Rudder Pos";
    }
    if (SubSystems & FGAircraft::ssRates) {
      datafile << ", ";
      datafile << "P, Q, R";
    }
    if (SubSystems & FGAircraft::ssVelocities) {
      datafile << ", ";
      datafile << "QBar, ";
      datafile << "Vtotal, ";
      datafile << "UBody, VBody, WBody, ";
      datafile << "UAero, VAero, WAero, ";
      datafile << "Vn, Ve, Vd";
    }
    if (SubSystems & FGAircraft::ssForces) {
      datafile << ", ";
      datafile << "Drag, Side, Lift, ";
      datafile << "L/D, ";
      datafile << "Xforce, Yforce, Zforce";
    }
    if (SubSystems & FGAircraft::ssMoments) {
      datafile << ", ";
      datafile << "L, M, N";
    }
    if (SubSystems & FGAircraft::ssAtmosphere) {
      datafile << ", ";
      datafile << "Rho";
    }
    if (SubSystems & FGAircraft::ssMassProps) {
      datafile << ", ";
      datafile << "Ixx, ";
      datafile << "Iyy, ";
      datafile << "Izz, ";
      datafile << "Ixz, ";
      datafile << "Mass, ";
      datafile << "Xcg, Ycg, Zcg";
    }
    if (SubSystems & FGAircraft::ssPosition) {
      datafile << ", ";
      datafile << "Altitude, ";
      datafile << "Phi, Tht, Psi, ";
      datafile << "Alpha, ";
      datafile << "Latitude, ";
      datafile << "Longitude, ";
      datafile << "Distance AGL, ";
      datafile << "Runway Radius";
    }
    if (SubSystems & FGAircraft::ssCoefficients) {
      datafile << ", ";
      datafile << Aerodynamics->GetCoefficientStrings();
    }
    if (SubSystems & FGAircraft::ssGroundReactions) {
      datafile << ", ";
      datafile << GroundReactions->GetGroundReactionStrings();
    }
    if (SubSystems & FGAircraft::ssFCS) {
      datafile << ", ";
      datafile << FCS->GetComponentStrings();
    }
    if (SubSystems & FGAircraft::ssPropulsion) {
      datafile << ", ";
      datafile << Propulsion->GetPropulsionStrings();
    }
    datafile << endl;
    sFirstPass = false;
  }

  datafile << State->Getsim_time();
  if (SubSystems & FGAircraft::ssSimulation) {
  }
  if (SubSystems & FGAircraft::ssAerosurfaces) {
    datafile << ", ";
    datafile << FCS->GetThrottlePos(0) << ", ";
    datafile << FCS->GetDaCmd() << ", ";
    datafile << FCS->GetDeCmd() << ", ";
    datafile << FCS->GetDrCmd() << ", ";
    datafile << FCS->GetDaPos() << ", ";
    datafile << FCS->GetDePos() << ", ";
    datafile << FCS->GetDrPos();
  }
  if (SubSystems & FGAircraft::ssRates) {
    datafile << ", ";
    datafile << Rotation->GetPQR();
  }
  if (SubSystems & FGAircraft::ssVelocities) {
    datafile << ", ";
    datafile << Translation->Getqbar() << ", ";
    datafile << Translation->GetVt() << ", ";
    datafile << Translation->GetUVW() << ", ";
    datafile << Translation->GetvAero() << ", ";
    datafile << Position->GetVel();
  }
  if (SubSystems & FGAircraft::ssForces) {
    datafile << ", ";
    datafile << Aerodynamics->GetvFs() << ", ";
    datafile << Aerodynamics->GetLoD() << ", ";
    datafile << Aircraft->GetForces();
  }
  if (SubSystems & FGAircraft::ssMoments) {
    datafile << ", ";
    datafile << Aircraft->GetMoments();
  }
  if (SubSystems & FGAircraft::ssAtmosphere) {
    datafile << ", ";
    datafile << Atmosphere->GetDensity();
  }
  if (SubSystems & FGAircraft::ssMassProps) {
    datafile << ", ";
    datafile << MassBalance->GetIxx() << ", ";
    datafile << MassBalance->GetIyy() << ", ";
    datafile << MassBalance->GetIzz() << ", ";
    datafile << MassBalance->GetIxz() << ", ";
    datafile << MassBalance->GetMass() << ", ";
    datafile << MassBalance->GetXYZcg();
  }
  if (SubSystems & FGAircraft::ssPosition) {
    datafile << ", ";
    datafile << Position->Geth() << ", ";
    datafile << Rotation->GetEuler() << ", ";
    datafile << Translation->Getalpha() << ", ";
    datafile << Position->GetLatitude() << ", ";
    datafile << Position->GetLongitude() << ", ";
    datafile << Position->GetDistanceAGL() << ", ";
    datafile << Position->GetRunwayRadius();
  }
  if (SubSystems & FGAircraft::ssCoefficients) {
    datafile << ", ";
    datafile << Aerodynamics->GetCoefficientValues();
  }
  if (SubSystems & FGAircraft::ssGroundReactions) {
    datafile << ", ";
    datafile << GroundReactions->GetGroundReactionValues();
  }
  if (SubSystems & FGAircraft::ssFCS) {
    datafile << ", ";
    datafile << FCS->GetComponentValues();
  }
  if (SubSystems & FGAircraft::ssPropulsion) {
    datafile << ", ";
    datafile << Propulsion->GetPropulsionValues();
  }
  datafile << endl;
  datafile.flush();
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

