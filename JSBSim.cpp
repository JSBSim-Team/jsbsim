/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       JSBSim.cpp
 Author:       Jon S. Berndt
 Date started: 08/17/99
 Purpose:      Standalone version of JSBSim.
 Called by:    The USER.

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

This class Handles calling JSBSim standalone. It is set up for compilation under
Borland C+Builder or other compiler.

HISTORY
--------------------------------------------------------------------------------
08/17/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"
#include "FGScript.h"
#include "FGJSBBase.h"
#include "FGTrim.h"
#include "FGLGear.h"

#if !defined(__GNUC__)
#  include <time>
#else
#  include <time.h>
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: JSBSim.cpp,v 1.89 2005/01/04 12:40:51 jberndt Exp $";

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

string ScriptName;
string AircraftName;
string ResetName;
string LogOutputName;
string LogDirectiveName;
JSBSim::FGFDMExec* FDMExec;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void options(int, char**);
void convert(JSBSim::FGFDMExec* FDMExec);
bool bConvert;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** \mainpage JSBSim
 * The Open Source, Object-Oriented, Cross-Platform Flight Dynamics Model in C++

 * \section intro Introduction
 *
 * JSBSim is an open source, multi-platform, object-oriented flight dynamics
 * model (FDM) framework written in the C++ programming language.  It is
 * designed to support simulation modeling of any aerospace craft without the
 * need for specific compiled and linked program code, instead relying on a
 * relatively simple model specification written in a pseudo-XML format.
 *
 * JSBSim (www.jsbsim.org) was created initially for the open source FlightGear
 * flight simulator (www.flightgear.org), where it replaced LaRCSim (Langley
 * Research Center Simulation) as the default FDM.  JSBSim also maintains the
 * ability to run in a standalone, batch mode.  This is useful for running tests
 * or sets of tests automatically using the internal scripting capability.
 *
 * Differently from LaRCSim, JSBSim does not model specific aircraft in program
 * code. The aircraft itself is defined in a file written in an XML-based format
 * where the aircraft mass and geometric properties are specified.  Additional
 * statements define:
 *
 * - Landing gear location and properties.
 * - Pilot eyepoint
 * - Additional point masses (passengers, cargo, etc.)
 * - Propulsion system (engines, fuel tanks, and "thrusters")
 * - Flight control system
 * - Autopilot
 * - Aerodynamic stability derivatives and coefficients
 *
 * The configuration file format is set up to be easily comprehensible, for
 * instance featuring textbook-like coefficients, which enables newcomers to
 * become immediately fluent in describing vehicles, and requiring only prior
 * basic theoretical aero knowledge.
 *
 * One of the more unique features of JSBSim is its method of modeling flight
 * control systems and the autopilot.  These are modeled by assembling strings
 * of components that represent filters, switches, summers, gains, etc.  The
 * components are listed in the configuration file in the order they are to be
 * executed.
 *
 * Another unique feature is displayed in the use of "properties".  Properties
 * essentially expose chosen variables as nodes in a tree, in a directory-like
 * hierarchy.  This approach facilitates plugging in different FDMs into
 * FlightGear, but it also is a fundamental tool in allowing a wide range of
 * aircraft to be modeled, each having its own unique control system,
 * aerosurfaces, and flight deck instrument panel.  The use of properties allows
 * all these items for a craft to be modeled and integrated without the need for
 * specific and unique program source code.
 *
 * The equations of motion are modeled essentially as they are presented in
 * aerospace textbooks for the benefit of student users, but quaternions are
 * used to track orientation, avoiding "gimbal lock". While JSBSim is currently
 * designed to model primarily atmospheric flight at lower speeds, coriolis and
 * centripetal accelerations will be incorporated into the EOM in the future to
 * permit a wider range of vehicles to be simulated.
 *
 * Currently under development is an expansion of the atmospheric modeling for
 * JSBSim.  The existing model approximates the standard atmosphere of 1976.
 * Recently, source code for the NRLMSISE-00 model was obtained and this is
 * being implemented as a C++ class that can optionally be used.  Also, a simple
 *  Mars atmosphere is being implemented.
 *
 * JSBSim can output (log) data in a configurable way.  Sets of data that are
 * logically related can be selected to be output at a chosen rate, and
 * individual properties can be selected for output.  The output can be streamed
 * to the console, or to a file, and can also be transmitted through a socket.
 *
 * JSBSim has been used in a variety of ways:
 *
 * - For developing control laws for a sounding rocket
 * - For crafting an aircraft autopilot as part of a thesis project
 * - As a flight model for FlightGear
 * - As an FDM that drives motion base simulators for some
 *   commercial/entertainment simulators
 *
 * \section Supported Platforms:
 * JSBSim has been built on the following platforms:
 *
 *   - Linux (x86)
 *   - Windows (MSVC, Cygwin, Mingwin)
 *   - SGI (native compilers)
 *   - Mac OS X
 *   - FreeBSD
 *
 * \section depends Dependencies
 *
 * JSBSim has no dependencies at present.
 *
 * \section license Licensing
 *
 * JSBSim is licensed under the terms of the GPL
 *
 * \section website Website
 *
 * For more information, see the JSBSim web site: www.jsbsim.org.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

int main(int argc, char* argv[])
{

  ScriptName = "";
  AircraftName = "";
  ResetName = "";
  LogOutputName = "";
  LogDirectiveName = "";
  bool result = false;
  bool Scripted = false;
  bConvert = false;
  JSBSim::FGScript* Script;

  options(argc, argv);

  FDMExec = new JSBSim::FGFDMExec();
  FDMExec->SetAircraftPath("aircraft");
  FDMExec->SetEnginePath("engine");

  if (!ScriptName.empty()) { // SCRIPTED CASE

    Script = new JSBSim::FGScript(FDMExec);
    result = Script->LoadScript(ScriptName);

    if (!result) {
      cerr << "Script file " << ScriptName << " was not successfully loaded" << endl;
      exit(-1);
    }

    Scripted = true;

  } else if (!AircraftName.empty() || !ResetName.empty()) {        // form jsbsim <acname> <resetfile>

    if ( ! FDMExec->LoadModel("aircraft", "engine",AircraftName)) {
      cerr << "  JSBSim could not be started" << endl << endl;
      exit(-1);
    }

    JSBSim::FGInitialCondition *IC = FDMExec->GetIC();
    if ( ! IC->Load(ResetName)) {
      cerr << "Initialization unsuccessful" << endl;
      exit(-1);
    }
/*
    JSBSim::FGTrim fgt(FDMExec, JSBSim::tFull);
    if ( !fgt.DoTrim() ) {
      cout << "Trim Failed" << endl;
    }
    fgt.Report();
*/
  } else {
    cout << "  No Aircraft, Script, or Reset information given" << endl << endl;
    exit(-1);
  }

// if this is a conversion run, convert files, then exit.

  if (bConvert) {
    convert(FDMExec);
    exit(0);
  }

//
// RUN loop. MESSAGES are read inside the Run() loop and output as necessary.
//

  JSBSim::FGJSBBase::Message* msg;
  result = FDMExec->Run();
  while (result) {
    while (FDMExec->ReadMessage()) {
      msg = FDMExec->ProcessMessage();
      switch (msg->type) {
      case JSBSim::FGJSBBase::Message::eText:
        cout << msg->messageId << ": " << msg->text << endl;
        break;
      case JSBSim::FGJSBBase::Message::eBool:
        cout << msg->messageId << ": " << msg->text << " " << msg->bVal << endl;
        break;
      case JSBSim::FGJSBBase::Message::eInteger:
        cout << msg->messageId << ": " << msg->text << " " << msg->iVal << endl;
        break;
      case JSBSim::FGJSBBase::Message::eDouble:
        cout << msg->messageId << ": " << msg->text << " " << msg->dVal << endl;
        break;
      default:
        cerr << "Unrecognized message type." << endl;
        break;
      }
    }

    if (Scripted) {
      if (!Script->RunScript()) break;
    }

    result = FDMExec->Run();
  }

  delete FDMExec;
  cout << endl << "Seconds processor time used: " << clock()/CLOCKS_PER_SEC << " seconds" << endl;

  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void options(int count, char **arg)
{
  int i;

  if (count == 1) {
    cout << endl << "  JSBSim version " << FDMExec->GetVersion() << endl << endl;
    cout << "  Usage: jsbsim <options>" << endl << endl;
    cout << "  options:" << endl;
      cout << "    --help  returns this message" << endl;
      cout << "    --version  returns the version number" << endl;
      cout << "    --convert  converts files to the new v2.0 format from the original format" << endl;
      cout << "    --outputlogfile=<filename>  sets the name of the data output file" << endl;
      cout << "    --logdirectivefile=<filename>  specifies the name of the data logging directives file" << endl;
      cout << "    --aircraft=<filename>  specifies the name of the aircraft to be modeled" << endl;
      cout << "    --script=<filename>  specifies a script to run" << endl;
      cout << "    --initfile=<filename>  specifies an initilization file" << endl << endl;
    cout << "  NOTE: There can be no spaces around the = sign when" << endl;
    cout << "        an option is followed by a filename" << endl << endl;
  }

  for (i=1; i<count; i++) {
    string argument = string(arg[i]);
    int n=0;
    if (argument.find("--help") != string::npos) {
      cout << endl << "  JSBSim version " << FDMExec->GetVersion() << endl << endl;
      cout << "  Usage: jsbsim <options>" << endl << endl;
      cout << "  options:" << endl;
      cout << "    --help  returns this message" << endl;
      cout << "    --version  returns the version number" << endl;
      cout << "    --convert  converts files to the new v2.0 format from the original format" << endl;
      cout << "    --outputlogfile=<filename>  sets the name of the data output file" << endl;
      cout << "    --logdirectivefile=<filename>  specifies the name of the data logging directives file" << endl;
      cout << "    --aircraft=<filename>  specifies the name of the aircraft to be modeled" << endl;
      cout << "    --script=<filename>  specifies a script to run" << endl;
      cout << "    --initfile=<filename>  specifies an initilization file" << endl << endl;
      cout << "  NOTE: There can be no spaces around the = sign when" << endl;
      cout << "        an option is followed by a filename" << endl << endl;
      exit(0);
    } else if (argument.find("--version") != string::npos) {
      cout << endl << "  JSBSim Version: " << FDMExec->GetVersion() << endl << endl;
      exit (0);
    } else if (argument.find("--outputlogfile") != string::npos) {
      n = argument.find("=")+1;
      if (n > 0) {
        LogOutputName = argument.substr(argument.find("=")+1);
      } else {
        LogOutputName = "JSBout.csv";
        cerr << "  Output log file name not valid or not understood. Using JSBout.csv as default";
      }
    } else if (argument.find("--logdirectivefile") != string::npos) {
      n = argument.find("=")+1;
      if (n > 0) {
        LogDirectiveName = argument.substr(argument.find("=")+1);
      } else {
        cerr << "  Log directives file not valid or not understood." << endl << endl;
        exit(1);
      }
    } else if (argument.find("--convert") != string::npos) {
      bConvert = true;
    } else if (argument.find("--aircraft") != string::npos) {
      n = argument.find("=")+1;
      if (n > 0) {
        AircraftName = argument.substr(argument.find("=")+1);
      } else {
        cerr << "  Aircraft name not valid or not understood." << endl << endl;
        exit(1);
      }
    } else if (argument.find("--script") != string::npos) {
      n = argument.find("=")+1;
      if (n > 0) {
        ScriptName = argument.substr(argument.find("=")+1);
      } else {
        cerr << "  Script name not valid or not understood." << endl << endl;
        exit(1);
      }
    } else if (argument.find("--initfile") != string::npos) {
      n = argument.find("=")+1;
      if (n > 0) {
        ResetName = argument.substr(argument.find("=")+1);
      } else {
        cerr << "  Reset name not valid or not understood." << endl << endl;
        exit(1);
      }
    } else {
      cerr << endl << "  Parameter: " << argument << " not understood" << endl;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void convert(JSBSim::FGFDMExec* FDMExec)
{
  cout << "<?xml version=\"1.0\"?>" << endl;
  cout << "<?xml-stylesheet href=\"JSBSim.xsl\" type=\"application/xml\"?>" << endl;
  cout << "<FDM_CONFIG NAME=\"" << FDMExec->GetAircraft()->GetAircraftName() << "\" VERSION=\"2.0\" RELEASE=\"BETA\">" << endl << endl;
  cout << "    <FILEHEADER>" << endl;
  cout << "        <AUTHOR>Author Name</AUTHOR>" << endl;
  cout << "        <FILECREATIONDATE>Creation Date</FILECREATIONDATE>" << endl;
  cout << "        <DESCRIPTION>Description</DESCRIPTION>" << endl;
  cout << "        <VERSION>$Id: JSBSim.cpp,v 1.89 2005/01/04 12:40:51 jberndt Exp $</VERSION>" << endl;
  cout << "        <REFERENCE refID=\"None\" author=\"n/a\" title=\"n/a\" date=\"n/a\"/>" << endl;
  cout << "    </FILEHEADER>" << endl << endl;
  cout << "    <METRICS>" << endl;
  cout << "        <WINGAREA UNIT=\"FT2\"> " << FDMExec->GetAircraft()->GetWingArea() << " </WINGAREA>" << endl;
  cout << "        <WINGSPAN UNIT=\"FT\"> " << FDMExec->GetAircraft()->GetWingSpan() << " </WINGSPAN>" << endl;
  cout << "        <CHORD UNIT=\"FT\"> " << FDMExec->GetAircraft()->Getcbar() << " </CHORD>" << endl;
  cout << "        <HTAILAREA UNIT=\"FT2\"> " << FDMExec->GetAircraft()->GetHTailArea() << " </HTAILAREA>" << endl;
  cout << "        <HTAILARM UNIT=\"FT\"> " << FDMExec->GetAircraft()->GetHTailArm() << " </HTAILARM>" << endl;
  cout << "        <VTAILAREA UNIT=\"FT2\"> " << FDMExec->GetAircraft()->GetVTailArea() << " </VTAILAREA>" << endl;
  cout << "        <VTAILARM UNIT=\"FT\"> " << FDMExec->GetAircraft()->GetVTailArm() << " </VTAILARM>" << endl;
  cout << "        <LOCATION NAME=\"AERORP\" UNIT=\"IN\">" << endl;
  cout << "            <X> " << FDMExec->GetAircraft()->GetXYZrp(1) << " </X>" << endl;
  cout << "            <Y> " << FDMExec->GetAircraft()->GetXYZrp(2) << " </Y>" << endl;
  cout << "            <Z> " << FDMExec->GetAircraft()->GetXYZrp(3) << " </Z>" << endl;
  cout << "        </LOCATION>" << endl;
  cout << "        <LOCATION NAME=\"EYEPOINT\" UNIT=\"IN\">" << endl;
  cout << "            <X> " << FDMExec->GetAircraft()->GetXYZep(1) << " </X>" << endl;
  cout << "            <Y> " << FDMExec->GetAircraft()->GetXYZep(2) << " </Y>" << endl;
  cout << "            <Z> " << FDMExec->GetAircraft()->GetXYZep(3) << " </Z>" << endl;
  cout << "        </LOCATION>" << endl;
  cout << "        <LOCATION NAME=\"VRP\" UNIT=\"IN\">" << endl;
  cout << "            <X> " << FDMExec->GetAircraft()->GetXYZvrp(1) << " </X>" << endl;
  cout << "            <Y> " << FDMExec->GetAircraft()->GetXYZvrp(2) << " </Y>" << endl;
  cout << "            <Z> " << FDMExec->GetAircraft()->GetXYZvrp(3) << " </Z>" << endl;
  cout << "        </LOCATION>" << endl;
  cout << "    </METRICS>" << endl << endl;
  cout << "    <MASS_BALANCE>" << endl;
  cout << "        <IXX UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(1,1) << " </IXX>" << endl;
  cout << "        <IYY UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(2,2) << " </IYY>" << endl;
  cout << "        <IZZ UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(3,3) << " </IZZ>" << endl;
  cout << "        <IXZ UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(1,3) << " </IXZ>" << endl;
  cout << "        <IYZ UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(2,3) << " </IYZ>" << endl;
  cout << "        <IXY UNIT=\"SLUG*FT2\"> " << FDMExec->GetMassBalance()->GetAircraftBaseInertias()(1,2) << " </IXY>" << endl;
  cout << "        <EMPTYWT UNIT=\"LBS\"> " << FDMExec->GetMassBalance()->GetEmptyWeight() << " </EMPTYWT>" << endl;
  cout << "        <LOCATION NAME=\"CG\" UNIT=\"IN\">" << endl;
  cout << "            <X> " << FDMExec->GetMassBalance()->GetbaseXYZcg(1) << " </X>" << endl;
  cout << "            <Y> " << FDMExec->GetMassBalance()->GetbaseXYZcg(2) << " </Y>" << endl;
  cout << "            <Z> " << FDMExec->GetMassBalance()->GetbaseXYZcg(3) << " </Z>" << endl;
  cout << "        </LOCATION>" << endl;

  for (int i=0; i<FDMExec->GetMassBalance()->GetNumPointMasses(); i++) {
    cout << "        <POINTMASS NAME=\"name\">" << endl;
    cout << "            <WEIGHT UNIT=\"LBS\"> " << FDMExec->GetMassBalance()->GetPointMassWeight(i) << " </WEIGHT>" << endl;
    cout << "            <LOCATION NAME=\"POINTMASS\" UNIT=\"IN\">" << endl;
    cout << "                <X> " << FDMExec->GetMassBalance()->GetPointMassLoc(i)(1) << " </X>" << endl;
    cout << "                <Y> " << FDMExec->GetMassBalance()->GetPointMassLoc(i)(2) << " </Y>" << endl;
    cout << "                <Z> " << FDMExec->GetMassBalance()->GetPointMassLoc(i)(3) << " </Z>" << endl;
    cout << "            </LOCATION>" << endl;
    cout << "        </POINTMASS>" << endl;
  }
  cout << "    </MASS_BALANCE>" << endl << endl;

  cout << "    <GROUND_REACTIONS>" << endl;
  for (int i=0; i<FDMExec->GetGroundReactions()->GetNumGearUnits(); i++) {
    JSBSim::FGLGear* gear = FDMExec->GetGroundReactions()->GetGearUnit(i);
    cout << "        <CONTACT TYPE=\"BOGEY\" NAME=\"" << gear->GetName() << "\">" << endl;
    cout << "            <LOCATION UNIT=\"IN\">" << endl;
    cout << "                <X> " << gear->GetXYZ(1) << " </X>" << endl;
    cout << "                <Y> " << gear->GetXYZ(2) << " </Y>" << endl;
    cout << "                <Z> " << gear->GetXYZ(3) << " </Z>" << endl;
    cout << "            </LOCATION>" << endl;
    cout << "            <STATIC_FRICTION> " << gear->GetstaticFCoeff() << " </STATIC_FRICTION>" << endl;
    cout << "            <DYNAMIC_FRICTION> " << gear->GetdynamicFCoeff() << " </DYNAMIC_FRICTION>" << endl;
    cout << "            <ROLLING_FRICTION> " << gear->GetrollingFCoeff() << " </ROLLING_FRICTION>" << endl;
    cout << "            <SPRING_COEFF UNIT=\"LBS/FT\"> " << gear->GetkSpring() << " </SPRING_COEFF>" << endl;
    cout << "            <DAMPING_COEFF UNIT=\"LBS/FT/SEC\"> " << gear->GetbDamp() << " </DAMPING_COEFF>" << endl;
    if (gear->GetsSteerType() == "CASTERED" ) {
      cout << "            <MAX_STEER UNIT=\"DEG\"> 360.0 </MAX_STEER>" << endl;
    } else if (gear->GetsSteerType() == "FIXED" ) {
      cout << "            <MAX_STEER UNIT=\"DEG\"> 0.0 </MAX_STEER>" << endl;
    } else {
      cout << "            <MAX_STEER UNIT=\"DEG\"> " << gear->GetmaxSteerAngle() << " </MAX_STEER>" << endl;
    }
    cout << "            <BRAKE_GROUP> " << gear->GetsBrakeGroup() << " </BRAKE_GROUP>" << endl;
    if (gear->GetsRetractable() == "RETRACT")
      cout << "            <RETRACTABLE>1</RETRACTABLE>" << endl;
    else
      cout << "            <RETRACTABLE>0</RETRACTABLE>" << endl;

    cout << "        </CONTACT>" << endl;
  }
  cout << "    </GROUND_REACTIONS>" << endl;

}
