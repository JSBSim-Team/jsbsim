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

#if __BCPLUSPLUS__  == 0x0540   // If compiling under Borland C++Builder

#pragma hdrstop
#include <condefs.h>

USEUNIT("FGAerodynamics.cpp");
USEUNIT("FGAircraft.cpp");
USEUNIT("FGAtmosphere.cpp");
USEUNIT("FGAuxiliary.cpp");
USEUNIT("FGCoefficient.cpp");
USEUNIT("FGConfigFile.cpp");
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
USEUNIT("FGUtility.cpp");
USEUNIT("filtersjb\FGSwitch.cpp");
USEUNIT("filtersjb\FGFCSComponent.cpp");
USEUNIT("filtersjb\FGFilter.cpp");
USEUNIT("filtersjb\FGFlaps.cpp");
USEUNIT("filtersjb\FGGain.cpp");
USEUNIT("filtersjb\FGGradient.cpp");
USEUNIT("filtersjb\FGSummer.cpp");
USEUNIT("filtersjb\FGDeadBand.cpp");
USERES("JSBSim.res");
//---------------------------------------------------------------------------
#pragma argsused
#endif

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

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/JSBSim.cpp,v 1.42 2001/02/02 03:57:54 jsb Exp $";

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Standalone JSBSim main program
    This is the wrapper program used to instantiate the JSBSim system and control
    it. Use this program to build a version of JSBSim that can be run from the
    command line. This program is also designed to be built using Borland C++
    Builder, v4.0 or greater.
    @author Jon S. Berndt
    @version $Id: JSBSim.cpp,v 1.42 2001/02/02 03:57:54 jsb Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

int main(int argc, char** argv)
{
  FGFDMExec* FDMExec;
  bool result = false;

  if (argc != 3) {
    cout << endl
         << "  You must enter the name of a registered aircraft and reset point:"
         << endl << endl << "  FDM <aircraft name> <reset file>" << endl;
    exit(0);
  }

  FDMExec = new FGFDMExec();

  result = FDMExec->LoadModel("aircraft", "engine", string(argv[1]));
  
  if (!result) {
  	cerr << "Aircraft file " << argv[1] << " was not found" << endl;
	  exit(-1);
  }
  
  if ( ! FDMExec->GetState()->Reset("aircraft", string(argv[1]), string(argv[2])))
    FDMExec->GetState()->Initialize(2000,0,0,0,0,0,0.5,0.5,40000);

  float cmd = 0.0;

  while (FDMExec->GetState()->Getsim_time() <= 10.0)
  {
    // Fake an elevator ramp here after 1 second, hold for one second, ramp down
    
    if (FDMExec->GetState()->Getsim_time() >= 1.00 &&
        FDMExec->GetState()->Getsim_time() < 2.0)
    {
      cmd = -(FDMExec->GetState()->Getsim_time() - 1.00)/4.0;
      FDMExec->GetFCS()->SetThrottleCmd(0,FDMExec->GetState()->Getsim_time() - 1.00);
    } else if (FDMExec->GetState()->Getsim_time() >= 2.00 &&
        FDMExec->GetState()->Getsim_time() < 6.0)
    {
      cmd = -1.00/4.0;
    } else if (FDMExec->GetState()->Getsim_time() >= 6.00 &&
        FDMExec->GetState()->Getsim_time() < 7.0)
    {
      cmd = -(7.0 - FDMExec->GetState()->Getsim_time())/4.0;
    } else {
      cmd = 0.00;
    }
    FDMExec->GetFCS()->SetDeCmd(cmd);    // input between -1 and 1
    
    FDMExec->Run();
  }

  delete FDMExec;

  return 0;
}
