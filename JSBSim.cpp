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
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGConfigFile.h"
#include "FGScript.h"
#include "FGJSBBase.h"

#ifdef FGFS
#include <simgear/compiler.h>
#include STL_IOSTREAM
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <iostream.h>
#  else
#    include <iostream>
#  endif
#endif

#if __BCPLUSPLUS__ == 0x540
#pragma hdrstop
#include <condefs.h>
//---------------------------------------------------------------------------
USERES("JSBSim.res");
USEUNIT("FGUtility.cpp");
USEUNIT("FGAircraft.cpp");
USEUNIT("FGAtmosphere.cpp");
USEUNIT("FGAuxiliary.cpp");
USEUNIT("FGCoefficient.cpp");
USEUNIT("FGColumnVector3.cpp");
USEUNIT("FGColumnVector4.cpp");
USEUNIT("FGConfigFile.cpp");
USEUNIT("FGEngine.cpp");
USEUNIT("FGFactorGroup.cpp");
USEUNIT("FGFCS.cpp");
USEUNIT("FGFDMExec.cpp");
USEUNIT("FGfdmSocket.cpp");
USEUNIT("FGForce.cpp");
USEUNIT("FGGroundReactions.cpp");
USEUNIT("FGInertial.cpp");
USEUNIT("FGInitialCondition.cpp");
USEUNIT("FGJSBBase.cpp");
USEUNIT("FGLGear.cpp");
USEUNIT("FGMassBalance.cpp");
USEUNIT("FGMatrix33.cpp");
USEUNIT("FGModel.cpp");
USEUNIT("FGNozzle.cpp");
USEUNIT("FGOutput.cpp");
USEUNIT("FGPiston.cpp");
USEUNIT("FGPosition.cpp");
USEUNIT("FGPropeller.cpp");
USEUNIT("FGPropulsion.cpp");
USEUNIT("FGRocket.cpp");
USEUNIT("FGRotation.cpp");
USEUNIT("FGRotor.cpp");
USEUNIT("FGScript.cpp");
USEUNIT("FGState.cpp");
USEUNIT("FGTable.cpp");
USEUNIT("FGTank.cpp");
USEUNIT("FGThruster.cpp");
USEUNIT("FGTranslation.cpp");
USEUNIT("FGTrim.cpp");
USEUNIT("FGTrimAxis.cpp");
USEUNIT("FGTurboJet.cpp");
USEUNIT("FGTurboProp.cpp");
USEUNIT("FGTurboShaft.cpp");
USEUNIT("FGAerodynamics.cpp");
USEUNIT("filtersjb\FGSwitch.cpp");
USEUNIT("filtersjb\FGFCSComponent.cpp");
USEUNIT("filtersjb\FGFilter.cpp");
USEUNIT("filtersjb\FGGain.cpp");
USEUNIT("filtersjb\FGGradient.cpp");
USEUNIT("filtersjb\FGKinemat.cpp");
USEUNIT("filtersjb\FGSummer.cpp");
USEUNIT("filtersjb\FGDeadBand.cpp");
//---------------------------------------------------------------------------
#pragma argsused
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: JSBSim.cpp,v 1.73 2002/09/07 22:41:44 apeden Exp $";

string ScriptName;
string AircraftName;
string ResetName;
string LogOutputName;
string LogDirectiveName;
FGFDMExec* FDMExec;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void options(int, char**);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Standalone JSBSim main program
    This is the wrapper program used to instantiate the JSBSim system and control
    it. Use this program to build a version of JSBSim that can be run from the
    command line. To get any use out of this, you will have to create a script
    to run a test case and specify what kind of output you would like.
    @author Jon S. Berndt
    @version $Id: JSBSim.cpp,v 1.73 2002/09/07 22:41:44 apeden Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/JSBSim.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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
  FGScript* Script;

  options(argc, argv);

  FDMExec = new FGFDMExec();
  FDMExec->SetAircraftPath("aircraft");
  FDMExec->SetEnginePath("engine");
  
  if (!ScriptName.empty()) { // SCRIPTED CASE

    Script = new FGScript(FDMExec);
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
    
    FGInitialCondition *IC=FDMExec->GetIC();
    if ( ! IC->Load(ResetName)) {
    	cerr << "Initialization unsuccessful" << endl;
      exit(-1);
    }
  } else {
    cout << "  No Aircraft, Script, or Reset information given" << endl << endl;
    exit(-1);
  }

//
// RUN loop. MESSAGES are read inside the Run() loop and output as necessary.
//

  FGJSBBase::Message* msg;
  result = FDMExec->Run();
  while (result) {
    while (FDMExec->ReadMessage()) {
      msg = FDMExec->ProcessMessage();
      switch (msg->type) {
      case FGJSBBase::Message::eText:
        cout << msg->messageId << ": " << msg->text << endl;
        break;
      case FGJSBBase::Message::eBool:
        cout << msg->messageId << ": " << msg->text << " " << msg->bVal << endl;
        break;
      case FGJSBBase::Message::eInteger:
        cout << msg->messageId << ": " << msg->text << " " << msg->iVal << endl;
        break;
      case FGJSBBase::Message::eDouble:
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

