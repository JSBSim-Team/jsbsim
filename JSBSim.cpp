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

#ifdef FGFS
#include <simgear/compiler.h>
#include STL_IOSTREAM
#  ifdef FG_HAVE_STD_INCLUDES
#    include <ctime>
#  else
#    include <time.h>
#  endif
#else
#include <iostream>
#include <ctime>
#endif

#if __BORLANDC__ > 0x540
#include <condefs.h>
USEUNIT("FGUtility.cpp");
USEUNIT("FGAircraft.cpp");
USEUNIT("FGAtmosphere.cpp");
USEUNIT("FGAuxiliary.cpp");
USEUNIT("FGCoefficient.cpp");
USEUNIT("FGConfigFile.cpp");
USEUNIT("FGControls.cpp");
USEUNIT("FGEngine.cpp");
USEUNIT("FGFCS.cpp");
USEUNIT("FGFDMExec.cpp");
USEUNIT("FGfdmSocket.cpp");
USEUNIT("FGForce.cpp");
USEUNIT("FGGroundReactions.cpp");
USEUNIT("FGInertial.cpp");
USEUNIT("FGInitialCondition.cpp");
USEUNIT("FGLGear.cpp");
USEUNIT("FGMassBalance.cpp");
USEUNIT("FGMatrix.cpp");
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
USEUNIT("filtersjb\FGFlaps.cpp");
USEUNIT("filtersjb\FGGain.cpp");
USEUNIT("filtersjb\FGGradient.cpp");
USEUNIT("filtersjb\FGSummer.cpp");
USEUNIT("filtersjb\FGDeadBand.cpp");
//---------------------------------------------------------------------------
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: JSBSim.cpp,v 1.49 2001/04/19 22:05:21 jberndt Exp $";

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Standalone JSBSim main program
    This is the wrapper program used to instantiate the JSBSim system and control
    it. Use this program to build a version of JSBSim that can be run from the
    command line. To get any use out of this, you will have to create a script
    to run a test case and specify what kind of output you would like.
    @author Jon S. Berndt
    @version $Id: JSBSim.cpp,v 1.49 2001/04/19 22:05:21 jberndt Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

int main(int argc, char** argv)
{
  FGFDMExec* FDMExec;
  float cmd = 0.0;
  bool result = false;
  bool scripted = false;

  if (argc == 2) {
    FGConfigFile testFile(argv[1]);

    if (!testFile.IsOpen()) {
      cout << "Script file not opened" << endl;
      exit(-1); 
    }

    testFile.GetNextConfigLine();
    if (testFile.GetValue("runscript").length() <= 0) {
      cout << "File: " << argv[1] << " is not a script file" << endl;
      exit(-1); 
    }
    scripted = true;
  } else if (argc != 3) {
    cout << endl
         << "  You must enter the name of a registered aircraft and reset point:"
         << endl << endl << "  FDM <aircraft name> <reset file>" << endl;
    cout << endl << "  Alternatively, you may specify only the name of a script file:"
         << endl << endl << "  FDM <script file>" << endl << endl;
    exit(0);
  }

  FDMExec = new FGFDMExec();

  if (scripted) {
    result = FDMExec->LoadScript(argv[1]);
    if (!result) {
      cerr << "Script file " << argv[1] << " was not successfully loaded" << endl;
      exit(-1);
    }
  } else {
    result = FDMExec->LoadModel("aircraft", "engine", string(argv[1]));
    if (!result) {
    	cerr << "Aircraft file " << argv[1] << " was not found" << endl;
	    exit(-1);
    }
    if ( ! FDMExec->GetState()->Reset("aircraft", string(argv[1]), string(argv[2])))
                   FDMExec->GetState()->Initialize(2000,0,0,0,0,0,0.5,0.5,40000);
  }

  while (FDMExec->Run()) {}

  delete FDMExec;

  return 0;
}
