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
#include "FGTrim.h"

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

#include <iomanip>
#include <fstream>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: c172_vti.cpp,v 1.3 2001/12/19 10:29:38 apeden Exp $";

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
    @version $Id: c172_vti.cpp,v 1.3 2001/12/19 10:29:38 apeden Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

int main(int argc, char** argv)
{
  FGFDMExec* fdmex;
  bool result;
 fdmex = new FGFDMExec();

    result =fdmex->LoadModel("../aircraft", "../engine", "c172");
    if (!result) {
    	cerr << "Aircraft file " << argv[1] << " was not found" << endl;
	    exit(-1);
    }

  
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGPropulsion*   Propulsion;
  FGMassBalance*  MassBalance;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGAerodynamics* Aerodynamics;
  FGGroundReactions *GroundReactions;
  
  State           = fdmex->GetState();
  Atmosphere      = fdmex->GetAtmosphere();
  FCS             = fdmex->GetFCS();
  MassBalance     = fdmex->GetMassBalance();
  Propulsion      = fdmex->GetPropulsion();
  Aircraft        = fdmex->GetAircraft();
  Translation     = fdmex->GetTranslation();
  Rotation        = fdmex->GetRotation();
  Position        = fdmex->GetPosition();
  Auxiliary       = fdmex->GetAuxiliary();
  Aerodynamics    = fdmex->GetAerodynamics();
  GroundReactions = fdmex->GetGroundReactions();  
  
  FGInitialCondition* fgic = new FGInitialCondition(fdmex);
  
  //set speed, etc from reset file
  fgic->Load("../aircraft","c172","reset01");
  fdmex->RunIC(fgic);
  
  //compute cyo and clo from config file cno
  //assumes cyo and clo are set to 1 in config file
  //adjust cno and re-run until rudder required to trim
  //is zero.  When done add clo and cyo values to
  //config file, comment out the setgain calls
  //below and re-run to check.
  double b,lvx,lvz,cno,cyo,clo;
  lvx =15.7; //vertical tail x-axis moment arm, ft.
  lvz = 1.2; //vertical tail z-axis moment arm, ft.
  b = 35.8;  //wing span, ft.
  cno=Aerodynamics->GetCoefficient("Cno")->GetSD();
  cyo=-cno*b/lvx;
  clo=cyo*lvz/b;
  //cout << "cno, clo, cyo: " << cno << ", " << clo << ", " << cyo << endl;
  Aerodynamics->GetCoefficient("Clo")->setGain(clo);
  Aerodynamics->GetCoefficient("CYo")->setGain(cyo);
  fdmex->RunIC(fgic);

  int neng = Propulsion->GetNumEngines();

  for(int i=0;i<neng;i++) {
    FGEngine* e = Propulsion->GetEngine(i);
    FGThruster * thrust = Propulsion->GetThruster(i);

    e->SetRunning(false);
    e->SetMagnetos(3);
    FCS->SetThrottleCmd(i,0.25);
    FCS->SetMixtureCmd(i,1.0);
    e->SetStarter(true);
    fdmex->RunIC(fgic);
  }   
  Propulsion->ICEngineStart();
  
  for(int i=0;i<neng;i++) {  
    Propulsion->GetEngine(i)->SetStarter(false);
  }

  fdmex->RunIC(fgic);
  Propulsion->GetSteadyState();
 
  FGTrim *fgt = new FGTrim(fdmex,fgic,tFull);
 
  fgt->RemoveState(tHmgt);  // do not attempt to make ground track=heading
  fgt->EditState(tPdot,tBeta); // adjust sideslip to zero out roll accel
  
  fgt->DoTrim();
  State->ReportState();
  
  cout << "Cno: " << Aerodynamics->GetCoefficient("Cno")->GetSD() << endl;
  cout << "Clo: " << Aerodynamics->GetCoefficient("Clo")->GetSD() << endl;
  cout << "CYo: " << Aerodynamics->GetCoefficient("CYo")->GetSD() << endl;

  
  delete fgt;
  delete fgic;
  delete fdmex; 

  return 0;
}


