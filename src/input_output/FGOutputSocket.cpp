/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputSocket.cpp
 Author:       Bertrand Coconnier
 Date started: 09/10/11
 Purpose:      Manage output of sim parameters to a socket
 Called by:    FGOutput

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
09/10/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>
#include <cstdlib>

#include "FGOutputSocket.h"
#include "FGFDMExec.h"
#include "models/FGAerodynamics.h"
#include "models/FGAccelerations.h"
#include "models/FGAircraft.h"
#include "models/FGAtmosphere.h"
#include "models/FGAuxiliary.h"
#include "models/FGPropulsion.h"
#include "models/FGMassBalance.h"
#include "models/FGPropagate.h"
#include "models/FGGroundReactions.h"
#include "models/FGFCS.h"
#include "models/atmosphere/FGWinds.h"
#include "input_output/FGXMLElement.h"
#include "math/FGPropertyValue.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutputSocket::FGOutputSocket(FGFDMExec* fdmex) :
  FGOutputType(fdmex),
  socket(0)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutputSocket::~FGOutputSocket()
{
  delete socket;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputSocket::SetOutputName(const string& fname)
{
  // tokenize the output name
  size_t dot_pos = fname.find(':', 0);
  size_t slash_pos = fname.find('/', 0);

  string name = fname.substr(0, dot_pos);

  string proto = "TCP";
  if(dot_pos + 1 < slash_pos)
    proto = fname.substr(dot_pos + 1, slash_pos - dot_pos - 1);

  string port = "1138";
  if(slash_pos < string::npos)
    port = fname.substr(slash_pos + 1, string::npos);

  // set the model name
  Name = name + ":" + port + "/" + proto;

  // set the socket params
  SockName = name;

  SockPort = atoi(port.c_str());

  if (to_upper(proto) == "UDP")
    SockProtocol = FGfdmSocket::ptUDP;
  else // Default to TCP
    SockProtocol = FGfdmSocket::ptTCP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputSocket::Load(Element* el)
{
  if (!FGOutputType::Load(el))
    return false;

  SetOutputName(el->GetAttributeValue("name") + ":" +
                el->GetAttributeValue("protocol") + "/" +
                el->GetAttributeValue("port"));

  // Check if output precision for doubles has been specified, default to 7 if not
  if(el->HasAttribute("precision"))
    precision = (int)el->GetAttributeValueAsNumber("precision");
  else
    precision = 7;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputSocket::InitModel(void)
{
  if (FGOutputType::InitModel()) {
    delete socket;
    socket = new FGfdmSocket(SockName, SockPort, SockProtocol, precision);

    if (socket == 0) return false;
    if (!socket->GetConnectStatus()) return false;

    PrintHeaders();

    return true;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputSocket::PrintHeaders(void)
{
  string scratch;

  socket->Clear();
  socket->Clear("<LABELS>");
  socket->Append("Time");

  if (SubSystems & ssAerosurfaces) {
    socket->Append("Aileron Command");
    socket->Append("Elevator Command");
    socket->Append("Rudder Command");
    socket->Append("Flap Command");
    socket->Append("Left Aileron Position");
    socket->Append("Right Aileron Position");
    socket->Append("Elevator Position");
    socket->Append("Rudder Position");
    socket->Append("Flap Position");
  }

  if (SubSystems & ssRates) {
    socket->Append("P");
    socket->Append("Q");
    socket->Append("R");
    socket->Append("PDot");
    socket->Append("QDot");
    socket->Append("RDot");
  }

  if (SubSystems & ssVelocities) {
    socket->Append("QBar");
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
  }

  if (SubSystems & ssForces) {
    socket->Append("F_Drag");
    socket->Append("F_Side");
    socket->Append("F_Lift");
    socket->Append("LoD");
    socket->Append("Fx");
    socket->Append("Fy");
    socket->Append("Fz");
  }

  if (SubSystems & ssMoments) {
    socket->Append("L");
    socket->Append("M");
    socket->Append("N");
  }

  if (SubSystems & ssAtmosphere) {
    socket->Append("Rho");
    socket->Append("SL pressure");
    socket->Append("Ambient pressure");
    socket->Append("Turbulence Magnitude");
    socket->Append("Turbulence Direction");
    socket->Append("NWind");
    socket->Append("EWind");
    socket->Append("DWind");
  }

  if (SubSystems & ssMassProps) {
    socket->Append("Ixx");
    socket->Append("Ixy");
    socket->Append("Ixz");
    socket->Append("Iyx");
    socket->Append("Iyy");
    socket->Append("Iyz");
    socket->Append("Izx");
    socket->Append("Izy");
    socket->Append("Izz");
    socket->Append("Mass");
    socket->Append("Xcg");
    socket->Append("Ycg");
    socket->Append("Zcg");
  }

  if (SubSystems & ssPropagate) {
    socket->Append("Altitude");
    socket->Append("Phi (deg)");
    socket->Append("Tht (deg)");
    socket->Append("Psi (deg)");
    socket->Append("Alpha (deg)");
    socket->Append("Beta (deg)");
    socket->Append("Latitude (deg)");
    socket->Append("Longitude (deg)");
  }

  if (SubSystems & ssAeroFunctions) {
    scratch = Aerodynamics->GetAeroFunctionStrings(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }

  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentStrings(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }

  if (SubSystems & ssGroundReactions)
    socket->Append(GroundReactions->GetGroundReactionStrings(","));

  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0)
    socket->Append(Propulsion->GetPropulsionStrings(","));

  for (unsigned int i=0;i<OutputParameters.size();++i) {
    if (!OutputCaptions[i].empty())
      socket->Append(OutputCaptions[i]);
    else
      socket->Append(OutputParameters[i]->GetPrintableName());
  }

  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputSocket::Print(void)
{
  string asciiData, scratch;

  if (socket == 0) return;
  if (!socket->GetConnectStatus()) return;

  socket->Clear();
  socket->Append(FDMExec->GetSimTime());

  if (SubSystems & ssAerosurfaces) {
    socket->Append(FCS->GetDaCmd());
    socket->Append(FCS->GetDeCmd());
    socket->Append(FCS->GetDrCmd());
    socket->Append(FCS->GetDfCmd());
    socket->Append(FCS->GetDaLPos());
    socket->Append(FCS->GetDaRPos());
    socket->Append(FCS->GetDePos());
    socket->Append(FCS->GetDrPos());
    socket->Append(FCS->GetDfPos());
  }
  if (SubSystems & ssRates) {
    socket->Append(radtodeg*Propagate->GetPQR(eP));
    socket->Append(radtodeg*Propagate->GetPQR(eQ));
    socket->Append(radtodeg*Propagate->GetPQR(eR));
    socket->Append(radtodeg*Accelerations->GetPQRdot(eP));
    socket->Append(radtodeg*Accelerations->GetPQRdot(eQ));
    socket->Append(radtodeg*Accelerations->GetPQRdot(eR));
  }
  if (SubSystems & ssVelocities) {
    socket->Append(Auxiliary->Getqbar());
    socket->Append(Auxiliary->GetVt());
    socket->Append(Propagate->GetUVW(eU));
    socket->Append(Propagate->GetUVW(eV));
    socket->Append(Propagate->GetUVW(eW));
    socket->Append(Auxiliary->GetAeroUVW(eU));
    socket->Append(Auxiliary->GetAeroUVW(eV));
    socket->Append(Auxiliary->GetAeroUVW(eW));
    socket->Append(Propagate->GetVel(eNorth));
    socket->Append(Propagate->GetVel(eEast));
    socket->Append(Propagate->GetVel(eDown));
  }
  if (SubSystems & ssForces) {
    socket->Append(Aerodynamics->GetvFw()(eDrag));
    socket->Append(Aerodynamics->GetvFw()(eSide));
    socket->Append(Aerodynamics->GetvFw()(eLift));
    socket->Append(Aerodynamics->GetLoD());
    socket->Append(Aircraft->GetForces(eX));
    socket->Append(Aircraft->GetForces(eY));
    socket->Append(Aircraft->GetForces(eZ));
  }
  if (SubSystems & ssMoments) {
    socket->Append(Aircraft->GetMoments(eL));
    socket->Append(Aircraft->GetMoments(eM));
    socket->Append(Aircraft->GetMoments(eN));
  }
  if (SubSystems & ssAtmosphere) {
    const auto Atmosphere = FDMExec->GetAtmosphere();
    socket->Append(Atmosphere->GetDensity());
    socket->Append(Atmosphere->GetPressureSL());
    socket->Append(Atmosphere->GetPressure());
    socket->Append(Winds->GetTurbMagnitude());
    socket->Append(Winds->GetTurbDirection());
    socket->Append(Winds->GetTotalWindNED().Dump(","));
  }
  if (SubSystems & ssMassProps) {
    socket->Append(MassBalance->GetJ()(1,1));
    socket->Append(MassBalance->GetJ()(1,2));
    socket->Append(MassBalance->GetJ()(1,3));
    socket->Append(MassBalance->GetJ()(2,1));
    socket->Append(MassBalance->GetJ()(2,2));
    socket->Append(MassBalance->GetJ()(2,3));
    socket->Append(MassBalance->GetJ()(3,1));
    socket->Append(MassBalance->GetJ()(3,2));
    socket->Append(MassBalance->GetJ()(3,3));
    socket->Append(MassBalance->GetMass());
    socket->Append(MassBalance->GetXYZcg()(eX));
    socket->Append(MassBalance->GetXYZcg()(eY));
    socket->Append(MassBalance->GetXYZcg()(eZ));
  }
  if (SubSystems & ssPropagate) {
    socket->Append(Propagate->GetAltitudeASL());
    socket->Append(radtodeg*Propagate->GetEuler(ePhi));
    socket->Append(radtodeg*Propagate->GetEuler(eTht));
    socket->Append(radtodeg*Propagate->GetEuler(ePsi));
    socket->Append(Auxiliary->Getalpha(inDegrees));
    socket->Append(Auxiliary->Getbeta(inDegrees));
    socket->Append(Propagate->GetLocation().GetLatitudeDeg());
    socket->Append(Propagate->GetLocation().GetLongitudeDeg());
  }
  if (SubSystems & ssAeroFunctions) {
    scratch = Aerodynamics->GetAeroFunctionValues(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }
  if (SubSystems & ssGroundReactions) {
    socket->Append(GroundReactions->GetGroundReactionValues(","));
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    socket->Append(Propulsion->GetPropulsionValues(","));
  }

  for (unsigned int i=0;i<OutputParameters.size();++i) {
    socket->Append(OutputParameters[i]->GetValue());
  }

  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputSocket::SocketStatusOutput(const string& out_str)
{
  string asciiData;

  if (socket == 0) return;

  socket->Clear();
  asciiData = string("<STATUS>") + out_str;
  socket->Append(asciiData.c_str());
  socket->Send();
}
}
