/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGAircraft.cpp
 Author:       Jon S. Berndt
 Date started: 12/12/98                                   
 Purpose:      Encapsulates an aircraft
 Called by:    FGFDMExec

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
Models the aircraft reactions and forces. This class is instantiated by the
FGFDMExec class and scheduled as an FDM entry. 
 
HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created
04/03/99   JSB   Changed Aero() method to correct body axis force calculation
                 from wind vector. Fix provided by Tony Peden.
05/03/99   JSB   Changed (for the better?) the way configurations are read in.
9/17/99     TP   Combined force and moment functions. Added aero reference 
                 point to config file. Added calculations for moments due to 
                 difference in cg and aero reference point
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sys/stat.h>
#include <sys/types.h>

#ifdef FGFS
#  ifndef __BORLANDC__
#    include <simgear/compiler.h>
#  endif
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  if defined (sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGInertial.h"
#include "FGGroundReactions.h"
#include "FGAerodynamics.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: FGAircraft.cpp,v 1.106 2001/12/21 13:24:24 jberndt Exp $";
static const char *IdHdr = ID_AIRCRAFT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAircraft";
  alphaclmin = alphaclmax = 0;
  HTailArea = VTailArea = HTailArm = VTailArm = 0.0;
  lbarh = lbarv = vbarh = vbarv = 0.0;
  WingIncidence=0;
  impending_stall = 0;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAircraft::~FGAircraft()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::Load(FGConfigFile* AC_cfg)
{
  string token;

  if (!ReadPrologue(AC_cfg)) return false;

  while ((AC_cfg->GetNextConfigLine() != string("EOF")) &&
         (token = AC_cfg->GetValue()) != string("/FDM_CONFIG")) {
    if (token == "METRICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Metrics" << fgdef << endl;
      if (!ReadMetrics(AC_cfg)) return false;
    } else if (token == "AERODYNAMICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Aerodynamics" << fgdef << endl;
      if (!ReadAerodynamics(AC_cfg)) return false;
    } else if (token == "UNDERCARRIAGE") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Landing Gear" << fgdef << endl;
      if (!ReadUndercarriage(AC_cfg)) return false;
    } else if (token == "PROPULSION") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Propulsion" << fgdef << endl;
      if (!ReadPropulsion(AC_cfg)) return false;
    } else if (token == "FLIGHT_CONTROL") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Flight Control" << fgdef << endl;
      if (!ReadFlightControls(AC_cfg)) return false;
    } else if (token == "OUTPUT") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Output directives" << fgdef << endl;
      if (!ReadOutput(AC_cfg)) return false;
    }
  }
  
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    vForces.InitMatrix();
    vForces += Aerodynamics->GetForces();
    vForces += Inertial->GetForces();
    vForces += Propulsion->GetForces();
    vForces += GroundReactions->GetForces();

    vMoments.InitMatrix();
    vMoments += Aerodynamics->GetMoments();
    vMoments += Propulsion->GetMoments();
    vMoments += GroundReactions->GetMoments();
    
    vBodyAccel = vForces/MassBalance->GetMass();
    
    vNcg = vBodyAccel/Inertial->gravity();

    vNwcg = State->GetTb2s() * vNcg;
    vNwcg(3) = -1*vNwcg(3) + 1;
    
    if (alphaclmax != 0) {
      if (Translation->Getalpha() > 0.85*alphaclmax) {
        impending_stall = 10*(Translation->Getalpha()/alphaclmax - 0.85);
      } else {
        impending_stall = 0;
      }
    }      
    
    return false;
  } else {                               // skip Run() execution this time
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGAircraft::GetNlf(void)
{
  return vNwcg(3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadPrologue(FGConfigFile* AC_cfg)
{
  string token = AC_cfg->GetValue();
  string scratch;
  AircraftName = AC_cfg->GetValue("NAME");
  if (debug_lvl > 0) cout << underon << "Reading Aircraft Configuration File"
            << underoff << ": " << highint << AircraftName << normint << endl;
  scratch = AC_cfg->GetValue("VERSION").c_str();

  CFGVersion = AC_cfg->GetValue("VERSION");

  if (debug_lvl > 0)
    cout << "                            Version: " << highint << CFGVersion
                                                             << normint << endl;
  if (CFGVersion != needed_cfg_version) {
    cerr << endl << fgred << "YOU HAVE AN INCOMPATIBLE CFG FILE FOR THIS AIRCRAFT."
            " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cerr << "Current version needed is: " << needed_cfg_version << endl;
    cerr << "         You have version: " << CFGVersion << endl << fgdef << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadMetrics(FGConfigFile* AC_cfg)
{
  string token = "";
  string parameter;
  double EW, bixx, biyy, bizz, bixy, bixz;
  double pmWt, pmX, pmY, pmZ;
  FGColumnVector3 vbaseXYZcg;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/METRICS")) {
    *AC_cfg >> parameter;
    if (parameter == "AC_WINGAREA") {
      *AC_cfg >> WingArea;
      if (debug_lvl > 0) cout << "    WingArea: " << WingArea  << endl;
    } else if (parameter == "AC_WINGSPAN") {
      *AC_cfg >> WingSpan;
      if (debug_lvl > 0) cout << "    WingSpan: " << WingSpan  << endl;
    } else if (parameter == "AC_WINGINCIDENCE") {
      *AC_cfg >> WingIncidence;
      if (debug_lvl > 0) cout << "    Chord: " << cbar << endl;
    } else if (parameter == "AC_CHORD") {
      *AC_cfg >> cbar;
      if (debug_lvl > 0) cout << "    Chord: " << cbar << endl;
    } else if (parameter == "AC_HTAILAREA") {
      *AC_cfg >> HTailArea;
      if (debug_lvl > 0) cout << "    H. Tail Area: " << HTailArea << endl;
    } else if (parameter == "AC_HTAILARM") {
      *AC_cfg >> HTailArm;
      if (debug_lvl > 0) cout << "    H. Tail Arm: " << HTailArm << endl;
    } else if (parameter == "AC_VTAILAREA") {
      *AC_cfg >> VTailArea;
      if (debug_lvl > 0) cout << "    V. Tail Area: " << VTailArea << endl;
    } else if (parameter == "AC_VTAILARM") {
      *AC_cfg >> VTailArm;
      if (debug_lvl > 0) cout << "    V. Tail Arm: " << VTailArm << endl;
    } else if (parameter == "AC_IXX") {
      *AC_cfg >> bixx;
      if (debug_lvl > 0) cout << "    baseIxx: " << bixx << endl;
      MassBalance->SetBaseIxx(bixx);
    } else if (parameter == "AC_IYY") {
      *AC_cfg >> biyy;
      if (debug_lvl > 0) cout << "    baseIyy: " << biyy << endl;
      MassBalance->SetBaseIyy(biyy);
    } else if (parameter == "AC_IZZ") {
      *AC_cfg >> bizz;
      if (debug_lvl > 0) cout << "    baseIzz: " << bizz << endl;
      MassBalance->SetBaseIzz(bizz);
    } else if (parameter == "AC_IXY") {
      *AC_cfg >> bixy;
      if (debug_lvl > 0) cout << "    baseIxy: " << bixy  << endl;
      MassBalance->SetBaseIxy(bixy);
    } else if (parameter == "AC_IXZ") {
      *AC_cfg >> bixz;
      if (debug_lvl > 0) cout << "    baseIxz: " << bixz  << endl;
      MassBalance->SetBaseIxz(bixz);
    } else if (parameter == "AC_EMPTYWT") {
      *AC_cfg >> EW;
      MassBalance->SetEmptyWeight(EW);
      if (debug_lvl > 0) cout << "    EmptyWeight: " << EW  << endl;
    } else if (parameter == "AC_CGLOC") {
      *AC_cfg >> vbaseXYZcg(eX) >> vbaseXYZcg(eY) >> vbaseXYZcg(eZ);
      MassBalance->SetBaseCG(vbaseXYZcg);
      if (debug_lvl > 0) cout << "    CG (x, y, z): " << vbaseXYZcg << endl;
    } else if (parameter == "AC_EYEPTLOC") {
      *AC_cfg >> vXYZep(eX) >> vXYZep(eY) >> vXYZep(eZ);
      if (debug_lvl > 0) cout << "    Eyepoint (x, y, z): " << vXYZep << endl;
    } else if (parameter == "AC_AERORP") {
      *AC_cfg >> vXYZrp(eX) >> vXYZrp(eY) >> vXYZrp(eZ);
      if (debug_lvl > 0) cout << "    Ref Pt (x, y, z): " << vXYZrp << endl;
    } else if (parameter == "AC_ALPHALIMITS") {
      *AC_cfg >> alphaclmin >> alphaclmax;
      if (debug_lvl > 0) cout << "    Maximum Alpha: " << alphaclmax
             << "    Minimum Alpha: " << alphaclmin
             << endl;
    } else if (parameter == "AC_POINTMASS") {
      *AC_cfg >> pmWt >> pmX >> pmY >> pmZ;
      if (debug_lvl > 0) cout << "    Point Mass Object: " << pmWt << " lbs. at "
                         << "X, Y, Z (in.): " << pmX << "  " << pmY << "  " << pmZ
                         << endl;
    }
  }
  
  // calculate some derived parameters
  if (cbar != 0.0) {
    lbarh = HTailArm/cbar;
    lbarv = VTailArm/cbar;
    if (WingArea != 0.0) {
      vbarh = HTailArm*HTailArea / (cbar*WingArea);
      vbarv = VTailArm*VTailArea / (cbar*WingArea);
    }
  }     
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadPropulsion(FGConfigFile* AC_cfg)
{
  if (!Propulsion->Load(AC_cfg)) {
    cerr << "  Propulsion not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadFlightControls(FGConfigFile* AC_cfg)
{
  if (!FCS->Load(AC_cfg)) {
    cerr << "  Flight Controls not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadAerodynamics(FGConfigFile* AC_cfg)
{
  if (!Aerodynamics->Load(AC_cfg)) {
    cerr << "  Aerodynamics not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadUndercarriage(FGConfigFile* AC_cfg)
{
  if (!GroundReactions->Load(AC_cfg)) {
    cerr << "  Ground Reactions not successfully loaded" << endl;
    return false;
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::ReadOutput(FGConfigFile* AC_cfg)
{
  if (!Output->Load(AC_cfg)) {
    cerr << "  Output not successfully loaded" << endl;
    return false;
  }
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

void FGAircraft::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAircraft" << endl;
    if (from == 1) cout << "Destroyed:    FGAircraft" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 32) { // Turbulence
  }
}

