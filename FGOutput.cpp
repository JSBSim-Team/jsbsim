/*******************************************************************************

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
later. Some machines may not support the ncurses console output. Borland is one
of those environments which does not, so the ncurses stuff is commented out.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGOutput.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGOutput";
  sFirstPass = dFirstPass = true;
  socket = 0;
#ifdef FG_WITH_JSBSIM_SOCKET
  socket = new FGfdmSocket("localhost",1138);
#endif
}


FGOutput::~FGOutput(void)
{
  if (socket) delete socket;
}


bool FGOutput::Run(void)
{
  if (!FGModel::Run()) {
//    SocketOutput();
//    DelimitedOutput("JSBSimData.csv");
//    DelimitedOutput();
  } else {
  }
  return false;
}


void FGOutput::DelimitedOutput(void)
{
  if (dFirstPass) {
    cout << "Time,";
    cout << "Altitude,";
    cout << "Phi,";
    cout << "Tht,";
    cout << "Psi,";
    cout << "Rho,";
    cout << "Vtotal,";
    cout << "U,";
    cout << "V,";
    cout << "W,";
    cout << "Vn,";
    cout << "Ve,";
    cout << "Vd,";
    cout << "Udot,";
    cout << "Vdot,";
    cout << "Wdot,";
    cout << "P,";
    cout << "Q,";
    cout << "R,";
    cout << "PDot,";
    cout << "QDot,";
    cout << "RDot,";
    cout << "Fx,";
    cout << "Fy,";
    cout << "Fz,";
    cout << "Latitude,";
    cout << "Longitude,";
    cout << "QBar,";
    cout << "Alpha,";
    cout << "L,";
    cout << "M,";
    cout << "N,";
    cout << "Throttle,";
    cout << "Aileron,";
    cout << "Elevator,";
    cout << "Rudder,";
    cout << "Ixx,";
    cout << "Iyy,";
    cout << "Izz,";
    cout << "Ixz,";
    cout << "Mass,";
    cout << "X CG";
    cout << endl;
    dFirstPass = false;
  }

  cout << State->Getsim_time() << ",";
  cout << State->Geth() << ",";
  cout << Rotation->Getphi() << ",";
  cout << Rotation->Gettht() << ",";
  cout << Rotation->Getpsi() << ",";
  cout << Atmosphere->GetDensity() << ",";
  cout << State->GetVt() << ",";
  cout << Translation->GetU() << ",";
  cout << Translation->GetV() << ",";
  cout << Translation->GetW() << ",";
  cout << Position->GetVn() << ",";
  cout << Position->GetVe() << ",";
  cout << Position->GetVd() << ",";
  cout << Translation->GetUdot() << ",";
  cout << Translation->GetVdot() << ",";
  cout << Translation->GetWdot() << ",";
  cout << Rotation->GetP() << ",";
  cout << Rotation->GetQ() << ",";
  cout << Rotation->GetR() << ",";
  cout << Rotation->GetPdot() << ",";
  cout << Rotation->GetQdot() << ",";
  cout << Rotation->GetRdot() << ",";
  cout << Aircraft->GetFx() << ",";
  cout << Aircraft->GetFy() << ",";
  cout << Aircraft->GetFz() << ",";
  cout << State->Getlatitude() << ",";
  cout << State->Getlongitude() << ",";
  cout << State->Getqbar() << ",";
  cout << Translation->Getalpha() << ",";
  cout << Aircraft->GetL() << ",";
  cout << Aircraft->GetM() << ",";
  cout << Aircraft->GetN() << ",";
  cout << FCS->GetThrottle(0) << ",";
  cout << FCS->GetDa() << ",";
  cout << FCS->GetDe() << ",";
  cout << FCS->GetDr() << ",";
  cout << Aircraft->GetIxx() << ",";
  cout << Aircraft->GetIyy() << ",";
  cout << Aircraft->GetIzz() << ",";
  cout << Aircraft->GetIxz() << ",";
  cout << Aircraft->GetMass() << ",";
  cout << Aircraft->GetXcg() << "";
  cout << endl;

}


void FGOutput::DelimitedOutput(string fname)
{
  if (sFirstPass) {
    datafile.open(fname.c_str());
    datafile << "Time,";
    datafile << "Altitude,";
    datafile << "Phi,";
    datafile << "Tht,";
    datafile << "Psi,";
    datafile << "Rho,";
    datafile << "Vtotal,";
    datafile << "U,";
    datafile << "V,";
    datafile << "W,";
    datafile << "Vn,";
    datafile << "Ve,";
    datafile << "Vd,";
    datafile << "Udot,";
    datafile << "Vdot,";
    datafile << "Wdot,";
    datafile << "P,";
    datafile << "Q,";
    datafile << "R,";
    datafile << "PDot,";
    datafile << "QDot,";
    datafile << "RDot,";
    datafile << "Fx,";
    datafile << "Fy,";
    datafile << "Fz,";
    datafile << "Latitude,";
    datafile << "Longitude,";
    datafile << "QBar,";
    datafile << "Alpha,";
    datafile << "L,";
    datafile << "M,";
    datafile << "N,";
    datafile << "Throttle,";
    datafile << "Aileron,";
    datafile << "Elevator,";
    datafile << "Rudder,";
    datafile << "Ixx,";
    datafile << "Iyy,";
    datafile << "Izz,";
    datafile << "Ixz,";
    datafile << "Mass,";
    datafile << "X CG";
    datafile << endl;
    sFirstPass = false;
  }

  datafile << State->Getsim_time() << ",";
  datafile << State->Geth() << ",";
  datafile << Rotation->Getphi() << ",";
  datafile << Rotation->Gettht() << ",";
  datafile << Rotation->Getpsi() << ",";
  datafile << Atmosphere->GetDensity() << ",";
  datafile << State->GetVt() << ",";
  datafile << Translation->GetU() << ",";
  datafile << Translation->GetV() << ",";
  datafile << Translation->GetW() << ",";
  datafile << Position->GetVn() << ",";
  datafile << Position->GetVe() << ",";
  datafile << Position->GetVd() << ",";
  datafile << Translation->GetUdot() << ",";
  datafile << Translation->GetVdot() << ",";
  datafile << Translation->GetWdot() << ",";
  datafile << Rotation->GetP() << ",";
  datafile << Rotation->GetQ() << ",";
  datafile << Rotation->GetR() << ",";
  datafile << Rotation->GetPdot() << ",";
  datafile << Rotation->GetQdot() << ",";
  datafile << Rotation->GetRdot() << ",";
  datafile << Aircraft->GetFx() << ",";
  datafile << Aircraft->GetFy() << ",";
  datafile << Aircraft->GetFz() << ",";
  datafile << State->Getlatitude() << ",";
  datafile << State->Getlongitude() << ",";
  datafile << State->Getqbar() << ",";
  datafile << Translation->Getalpha() << ",";
  datafile << Aircraft->GetL() << ",";
  datafile << Aircraft->GetM() << ",";
  datafile << Aircraft->GetN() << ",";
  datafile << FCS->GetThrottle(0) << ",";
  datafile << FCS->GetDa() << ",";
  datafile << FCS->GetDe() << ",";
  datafile << FCS->GetDr() << ",";
  datafile << Aircraft->GetIxx() << ",";
  datafile << Aircraft->GetIyy() << ",";
  datafile << Aircraft->GetIzz() << ",";
  datafile << Aircraft->GetIxz() << ",";
  datafile << Aircraft->GetMass() << ",";
  datafile << Aircraft->GetXcg() << "";
  datafile << endl;
  datafile.flush();
}

void FGOutput::SocketOutput(void)
{
  string asciiData;

  if (socket <= 0) return;

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
    socket->Append("U");
    socket->Append("V");
    socket->Append("W");
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
  socket->Append(State->Geth());
  socket->Append(Rotation->Getphi());
  socket->Append(Rotation->Gettht());
  socket->Append(Rotation->Getpsi());
  socket->Append(Atmosphere->GetDensity());
  socket->Append(State->GetVt());
  socket->Append(Translation->GetU());
  socket->Append(Translation->GetV());
  socket->Append(Translation->GetW());
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
  socket->Append(State->Getlatitude());
  socket->Append(State->Getlongitude());
  socket->Append(State->Getqbar());
  socket->Append(Translation->Getalpha());
  socket->Append(Aircraft->GetL());
  socket->Append(Aircraft->GetM());
  socket->Append(Aircraft->GetN());
  socket->Append(FCS->GetThrottle(0));
  socket->Append(FCS->GetDa());
  socket->Append(FCS->GetDe());
  socket->Append(FCS->GetDr());
  socket->Send();
}


void FGOutput::SocketStatusOutput(string out_str)
{
  string asciiData;

  if (socket <= 0) return;

  socket->Clear();
  asciiData = string("<STATUS>") + out_str;
  socket->Append(asciiData.c_str());
  socket->Send();
}

