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

#if !defined(__GNUC__)
#  include <time>
#else
#  include <time.h>
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: JSBSim.cpp,v 1.87 2004/11/17 12:40:17 jberndt Exp $";

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

    JSBSim::FGTrim fgt(FDMExec, JSBSim::tFull);
    if ( !fgt.DoTrim() ) {
      cout << "Trim Failed" << endl;
    }
    fgt.Report();

  } else {
    cout << "  No Aircraft, Script, or Reset information given" << endl << endl;
    exit(-1);
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
