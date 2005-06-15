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
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGInertial.h"

#include <fstream>
#include <iomanip>

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.3 2005/06/15 12:01:55 jberndt Exp $";
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
  SubSystems = 0;
  enabled = true;
  delimeter = ", ";
  Filename = "";

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
    if (FGModel::Run()) return true;
    if (Type == otSocket) {
      SocketOutput();
    } else if (Type == otCSV || Type == otTab) {
      DelimitedOutput(Filename);
    } else if (Type == otTerminal) {
      // Not done yet
    } else if (Type == otNone) {
      // Do nothing
    } else {
      // Not a valid type of output
    }
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SetType(string type)
{
  if (type == "CSV") {
    Type = otCSV;
    delimeter = ", ";
  } else if (type == "TABULAR") {
    Type = otTab;
    delimeter = "\t";
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
      outstream << delimeter;
      outstream << "Aileron Cmd" + delimeter;
      outstream << "Elevator Cmd" + delimeter;
      outstream << "Rudder Cmd" + delimeter;
      outstream << "Flap Cmd" + delimeter;
      outstream << "Left Aileron Pos" + delimeter;
      outstream << "Right Aileron Pos" + delimeter;
      outstream << "Elevator Pos" + delimeter;
      outstream << "Rudder Pos" + delimeter;
      outstream << "Flap Pos";
    }
    if (SubSystems & ssRates) {
      outstream << delimeter;
      outstream << "P" + delimeter + "Q" + delimeter + "R" + delimeter;
      outstream << "Pdot" + delimeter + "Qdot" + delimeter + "Rdot";
    }
    if (SubSystems & ssVelocities) {
      outstream << delimeter;
      outstream << "QBar" + delimeter;
      outstream << "Vtotal" + delimeter;
      outstream << "UBody" + delimeter + "VBody" + delimeter + "WBody" + delimeter;
      outstream << "UAero" + delimeter + "VAero" + delimeter + "WAero" + delimeter;
      outstream << "Vn" + delimeter + "Ve" + delimeter + "Vd";
    }
    if (SubSystems & ssForces) {
      outstream << delimeter;
      outstream << "Drag" + delimeter + "Side" + delimeter + "Lift" + delimeter;
      outstream << "L/D" + delimeter;
      outstream << "Xforce" + delimeter + "Yforce" + delimeter + "Zforce";
    }
    if (SubSystems & ssMoments) {
      outstream << delimeter;
      outstream << "L" + delimeter + "M" + delimeter + "N";
    }
    if (SubSystems & ssAtmosphere) {
      outstream << delimeter;
      outstream << "Rho" + delimeter;
      outstream << "SL pressure" + delimeter;
      outstream << "Ambient pressure" + delimeter;
      outstream << "NWind" + delimeter + "EWind" + delimeter + "DWind";
    }
    if (SubSystems & ssMassProps) {
      outstream << delimeter;
      outstream << "Ixx" + delimeter;
      outstream << "Ixy" + delimeter;
      outstream << "Ixz" + delimeter;
      outstream << "Iyx" + delimeter;
      outstream << "Iyy" + delimeter;
      outstream << "Iyz" + delimeter;
      outstream << "Izx" + delimeter;
      outstream << "Izy" + delimeter;
      outstream << "Izz" + delimeter;
      outstream << "Mass" + delimeter;
      outstream << "Xcg" + delimeter + "Ycg" + delimeter + "Zcg";
    }
    if (SubSystems & ssPropagate) {
      outstream << delimeter;
      outstream << "Altitude" + delimeter;
      outstream << "Phi" + delimeter + "Tht" + delimeter + "Psi" + delimeter;
      outstream << "Alpha" + delimeter;
      outstream << "Beta" + delimeter;
      outstream << "Latitude (Deg)" + delimeter;
      outstream << "Longitude (Deg)" + delimeter;
      outstream << "Distance AGL" + delimeter;
      outstream << "Runway Radius";
    }
    if (SubSystems & ssCoefficients) {
      scratch = Aerodynamics->GetCoefficientStrings(delimeter);
      if (scratch.length() != 0) outstream << delimeter << scratch;
    }
    if (SubSystems & ssFCS) {
      scratch = FCS->GetComponentStrings(delimeter);
      if (scratch.length() != 0) outstream << delimeter << scratch;
    }
    if (SubSystems & ssGroundReactions) {
      outstream << delimeter;
      outstream << GroundReactions->GetGroundReactionStrings(delimeter);
    }
    if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
      outstream << delimeter;
      outstream << Propulsion->GetPropulsionStrings(delimeter);
    }
    if (OutputProperties.size() > 0) {
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        outstream << delimeter << OutputProperties[i]->GetName();
      }
    }

    outstream << endl;
    dFirstPass = false;
  }

  outstream << State->Getsim_time();
  if (SubSystems & ssSimulation) {
  }
  if (SubSystems & ssAerosurfaces) {
    outstream << delimeter;
    outstream << FCS->GetDaCmd() << delimeter;
    outstream << FCS->GetDeCmd() << delimeter;
    outstream << FCS->GetDrCmd() << delimeter;
    outstream << FCS->GetDfCmd() << delimeter;
    outstream << FCS->GetDaLPos() << delimeter;
    outstream << FCS->GetDaRPos() << delimeter;
    outstream << FCS->GetDePos() << delimeter;
    outstream << FCS->GetDrPos() << delimeter;
    outstream << FCS->GetDfPos();
  }
  if (SubSystems & ssRates) {
    outstream << delimeter;
    outstream << Propagate->GetPQR().Dump(delimeter) << delimeter;
    outstream << Propagate->GetPQRdot().Dump(delimeter);
  }
  if (SubSystems & ssVelocities) {
    outstream << delimeter;
    outstream << Auxiliary->Getqbar() << delimeter;
    outstream << setprecision(12) << Auxiliary->GetVt() << delimeter;
    outstream << setprecision(12) << Propagate->GetUVW().Dump(delimeter) << delimeter;
    outstream << Auxiliary->GetAeroUVW().Dump(delimeter) << delimeter;
    outstream << Propagate->GetVel().Dump(delimeter);
  }
  if (SubSystems & ssForces) {
    outstream << delimeter;
    outstream << Aerodynamics->GetvFs() << delimeter;
    outstream << Aerodynamics->GetLoD() << delimeter;
    outstream << Aircraft->GetForces().Dump(delimeter);
  }
  if (SubSystems & ssMoments) {
    outstream << delimeter;
    outstream << Aircraft->GetMoments().Dump(delimeter);
  }
  if (SubSystems & ssAtmosphere) {
    outstream << delimeter;
    outstream << Atmosphere->GetDensity() << delimeter;
    outstream << Atmosphere->GetPressureSL() << delimeter;
    outstream << Atmosphere->GetPressure() << delimeter;
    outstream << Atmosphere->GetWindNED().Dump(delimeter);
  }
  if (SubSystems & ssMassProps) {
    outstream << delimeter;
    outstream << MassBalance->GetJ() << delimeter;
    outstream << MassBalance->GetMass() << delimeter;
    outstream << MassBalance->GetXYZcg();
  }
  if (SubSystems & ssPropagate) {
    outstream << delimeter;
    outstream << Propagate->Geth() << delimeter;
    outstream << Propagate->GetEuler().Dump(delimeter) << delimeter;
    outstream << Auxiliary->Getalpha(inDegrees) << delimeter;
    outstream << Auxiliary->Getbeta(inDegrees) << delimeter;
    outstream << Propagate->GetLocation().GetLatitudeDeg() << delimeter;
    outstream << Propagate->GetLocation().GetLongitudeDeg() << delimeter;
    outstream << Propagate->GetDistanceAGL() << delimeter;
    outstream << Propagate->GetRunwayRadius();
  }
  if (SubSystems & ssCoefficients) {
    scratch = Aerodynamics->GetCoefficientValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssGroundReactions) {
    outstream << delimeter;
    outstream << GroundReactions->GetGroundReactionValues(delimeter);
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    outstream << delimeter;
    outstream << Propulsion->GetPropulsionValues(delimeter);
  }

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    outstream << delimeter << OutputProperties[i]->getDoubleValue();
  }

  outstream << endl;
  outstream.flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketOutput(void)
{
  string asciiData, scratch;

  if (socket == NULL) return;
  if (!socket->GetConnectStatus()) return;

  socket->Clear();
  if (sFirstPass) {
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
        socket->Append("Phi");
        socket->Append("Tht");
        socket->Append("Psi");
        socket->Append("Alpha");
        socket->Append("Beta");
        socket->Append("Latitude (Deg)");
        socket->Append("Longitude (Deg)");
    }
    if (SubSystems & ssCoefficients) {
      scratch = Aerodynamics->GetCoefficientStrings(",");
      if (scratch.length() != 0) socket->Append(scratch);
    }
    if (SubSystems & ssFCS) {
      scratch = FCS->GetComponentStrings(",");
      if (scratch.length() != 0) socket->Append(scratch);
    }
    if (SubSystems & ssGroundReactions) {
      socket->Append(GroundReactions->GetGroundReactionStrings(","));
    }
    if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
      socket->Append(Propulsion->GetPropulsionStrings(","));
    }
    if (OutputProperties.size() > 0) {
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        socket->Append(OutputProperties[i]->GetName());
      }
    }

    sFirstPass = false;
    socket->Send();
  }

  socket->Clear();
  socket->Append(State->Getsim_time());

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
    socket->Append(Propagate->GetPQR(eP));
    socket->Append(Propagate->GetPQR(eQ));
    socket->Append(Propagate->GetPQR(eR));
    socket->Append(Propagate->GetPQRdot(eP));
    socket->Append(Propagate->GetPQRdot(eQ));
    socket->Append(Propagate->GetPQRdot(eR));
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
    socket->Append(Aerodynamics->GetvFs()(eDrag));
    socket->Append(Aerodynamics->GetvFs()(eSide));
    socket->Append(Aerodynamics->GetvFs()(eLift));
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
    socket->Append(Atmosphere->GetDensity());
    socket->Append(Atmosphere->GetPressureSL());
    socket->Append(Atmosphere->GetPressure());
    socket->Append(Atmosphere->GetWindNED().Dump(","));
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
    socket->Append(Propagate->Geth());
    socket->Append(Propagate->GetEuler(ePhi));
    socket->Append(Propagate->GetEuler(eTht));
    socket->Append(Propagate->GetEuler(ePsi));
    socket->Append(Auxiliary->Getalpha(inDegrees));
    socket->Append(Auxiliary->Getbeta(inDegrees));
    socket->Append(Propagate->GetLocation().GetLatitudeDeg());
    socket->Append(Propagate->GetLocation().GetLongitudeDeg());
  }
  if (SubSystems & ssCoefficients) {
    scratch = Aerodynamics->GetCoefficientValues(",");
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

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    socket->Append(OutputProperties[i]->getDoubleValue());
  }

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

bool FGOutput::Load(Element* element)
{
  string type="", parameter="";
  string name="", fname="";
  int OutRate = 0;
  string property;
  unsigned int port;
  FGXMLParse output_file_parser;
  Element *document, *property_element;
  ifstream* output_file = new ifstream();

  string separator = "/";
# ifdef macintosh
  separator = ";";
# endif

  fname = element->GetAttributeValue("file");
  if (!fname.empty()) {
    output_file_name = FDMExec->GetAircraftPath() + separator
                        + FDMExec->GetModelName() + separator + fname + ".xml";

    output_file->open(output_file_name.c_str());
    readXML(*output_file, output_file_parser);
    delete output_file;
    document = output_file_parser.GetDocument();
  } else {
    document = element;
  }

  name = document->GetAttributeValue("name");
  type = document->GetAttributeValue("type");
  Output->SetType(type);
  if (!document->GetAttributeValue("port").empty() && type == string("SOCKET")) {
    port = atoi(document->GetAttributeValue("port").c_str());
    socket = new FGfdmSocket(name, port);
  } else {
    Filename = name;
  }

  if (!document->GetAttributeValue("rate").empty()) {
    OutRate = (int)document->GetAttributeValueAsNumber("rate");
  } else {
    OutRate = 1;
  }

  if (document->FindElementValue("simulation") == string("ON"))
    SubSystems += ssSimulation;
  if (document->FindElementValue("aerosurfaces") == string("ON"))
    SubSystems += ssAerosurfaces;
  if (document->FindElementValue("rates") == string("ON"))
    SubSystems += ssRates;
  if (document->FindElementValue("velocities") == string("ON"))
    SubSystems += ssVelocities;
  if (document->FindElementValue("forces") == string("ON"))
    SubSystems += ssForces;
  if (document->FindElementValue("moments") == string("ON"))
    SubSystems += ssMoments;
  if (document->FindElementValue("atmosphere") == string("ON"))
    SubSystems += ssAtmosphere;
  if (document->FindElementValue("massprops") == string("ON"))
    SubSystems += ssMassProps;
  if (document->FindElementValue("position") == string("ON"))
    SubSystems += ssPropagate;
  if (document->FindElementValue("coefficients") == string("ON"))
    SubSystems += ssCoefficients;
  if (document->FindElementValue("ground_reactions") == string("ON"))
    SubSystems += ssGroundReactions;
  if (document->FindElementValue("fcs") == string("ON"))
    SubSystems += ssFCS;
  if (document->FindElementValue("propulsion") == string("ON"))
    SubSystems += ssPropulsion;
  property_element = document->FindElement("property");
  while (property_element) {
    string property = property_element->GetDataLine();
    OutputProperties.push_back(PropertyManager->GetNode(property));
    property_element = document->FindNextElement("property");
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
      if (output_file_name.empty())
        cout << "  " << "Output parameters read inline" << endl;
      else
        cout << "    Output parameters read from file: " << output_file_name << endl;

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
      if (SubSystems & ssPropagate)       cout << "    Propagate parameters logged" << endl;
      if (SubSystems & ssGroundReactions) cout << "    Ground parameters logged" << endl;
      if (SubSystems & ssFCS)             cout << "    FCS parameters logged" << endl;
      if (SubSystems & ssPropulsion)      cout << "    Propulsion parameters logged" << endl;
      if (OutputProperties.size() > 0)    cout << "    Properties logged:" << endl;
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        cout << "      - " << OutputProperties[i]->GetName() << endl;
      }
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
