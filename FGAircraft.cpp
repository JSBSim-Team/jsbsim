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
FGFDMExec class and scheduled as an FDM entry. LoadAircraft() is supplied with a
name of a valid, registered aircraft, and the data file is parsed.
 
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

[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
      Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
      School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
      JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
      NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
      Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
      1982 ISBN 0-471-08936-2
 
The aerodynamic coefficients used in this model are:
 
Longitudinal
  CL0 - Reference lift at zero alpha
  CD0 - Reference drag at zero alpha
  CDM - Drag due to Mach
  CLa - Lift curve slope (w.r.t. alpha)
  CDa - Drag curve slope (w.r.t. alpha)
  CLq - Lift due to pitch rate
  CLM - Lift due to Mach
  CLadt - Lift due to alpha rate
 
  Cmadt - Pitching Moment due to alpha rate
  Cm0 - Reference Pitching moment at zero alpha
  Cma - Pitching moment slope (w.r.t. alpha)
  Cmq - Pitch damping (pitch moment due to pitch rate)
  CmM - Pitch Moment due to Mach
 
Lateral
  Cyb - Side force due to sideslip
  Cyr - Side force due to yaw rate
 
  Clb - Dihedral effect (roll moment due to sideslip)
  Clp - Roll damping (roll moment due to roll rate)
  Clr - Roll moment due to yaw rate
  Cnb - Weathercocking stability (yaw moment due to sideslip)
  Cnp - Rudder adverse yaw (yaw moment due to roll rate)
  Cnr - Yaw damping (yaw moment due to yaw rate)
 
Control
  CLDe - Lift due to elevator
  CDDe - Drag due to elevator
  CyDr - Side force due to rudder
  CyDa - Side force due to aileron
 
  CmDe - Pitch moment due to elevator
  ClDa - Roll moment due to aileron
  ClDr - Roll moment due to rudder
  CnDr - Yaw moment due to rudder
  CnDa - Yaw moment due to aileron
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sys/stat.h>
#include <sys/types.h>

#ifdef FGFS
#  ifndef __BORLANDC__
#    include <simgear/compiler.h>
#  endif
#  ifdef FG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  include <cmath>
#endif

#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGInertial.h"
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

static const char *IdSrc = "$Id: FGAircraft.cpp,v 1.78 2001/04/28 00:04:47 jberndt Exp $";
static const char *IdHdr = ID_AIRCRAFT;

extern char highint[5];
extern char halfint[5];
extern char normint[6];
extern char reset[5];
extern char underon[5];
extern char underoff[6];
extern char fgblue[6];
extern char fgcyan[6];
extern char fgred[6];
extern char fggreen[6];
extern char fgdef[6];

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex),
    vMoments(3),
    vForces(3),
    vXYZrp(3),
    vXYZep(3),
    vEuler(3),
    vDXYZcg(3),
    vAeroBodyForces(3)
{
  Name = "FGAircraft";

  GearUp = false;

  alphaclmin = alphaclmax = 0;

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


FGAircraft::~FGAircraft()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGAircraft" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::LoadAircraft(string aircraft_path, string engine_path, string fname) {
  string path;
  string filename;
  string aircraftCfgFileName;
  string token;

  AircraftPath = aircraft_path;
  EnginePath = engine_path;

# ifndef macintosh
  aircraftCfgFileName = AircraftPath + "/" + fname + "/" + fname + ".xml";
# else
  aircraftCfgFileName = AircraftPath + ";" + fname + ";" + fname + ".xml";
# endif

  FGConfigFile AC_cfg(aircraftCfgFileName);
  if (!AC_cfg.IsOpen()) return false;

  ReadPrologue(&AC_cfg);

  while ((AC_cfg.GetNextConfigLine() != string("EOF")) &&
         (token = AC_cfg.GetValue()) != string("/FDM_CONFIG")) {
    if (token == "METRICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Metrics" << fgdef << endl;
      ReadMetrics(&AC_cfg);
    } else if (token == "AERODYNAMICS") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Aerodynamics" << fgdef << endl;
      ReadAerodynamics(&AC_cfg);
    } else if (token == "UNDERCARRIAGE") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Landing Gear" << fgdef << endl;
      ReadUndercarriage(&AC_cfg);
    } else if (token == "PROPULSION") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Propulsion" << fgdef << endl;
      ReadPropulsion(&AC_cfg);
    } else if (token == "FLIGHT_CONTROL") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Flight Control" << fgdef << endl;
      ReadFlightControls(&AC_cfg);
    } else if (token == "OUTPUT") {
      if (debug_lvl > 0) cout << fgcyan << "\n  Reading Output directives" << fgdef << endl;
      ReadOutput(&AC_cfg);
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    GetState();

    vForces.InitMatrix();
    vMoments.InitMatrix();

    FMProp();
    FMAero();
    FMMass();
    FMGear();

    return false;
  } else {                               // skip Run() execution this time
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::FMAero(void)
{
    vForces += Aerodynamics->GetForces();
    vMoments += Aerodynamics->GetMoments();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::FMGear(void)
{
  if ( !GearUp ) {
    vector <FGLGear>::iterator iGear = lGear.begin();
    while (iGear != lGear.end()) {
      vForces  += iGear->Force();
      vMoments += iGear->Moment();
      iGear++;
    }
  } else {
    // Crash Routine
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::FMMass(void)
{
  vForces += Inertial->GetForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::FMProp(void)
{
  vForces += Propulsion->GetForces();
  vMoments += Propulsion->GetMoments();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::GetState(void)
{
  dt = State->Getdt();

  alpha = Translation->Getalpha();
  beta = Translation->Getbeta();
  vEuler = Rotation->GetEuler();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadMetrics(FGConfigFile* AC_cfg)
{
  string token = "";
  string parameter;
  float EW, bixx, biyy, bizz, bixz, biyz;
  FGColumnVector vbaseXYZcg(3);

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/METRICS")) {
    *AC_cfg >> parameter;
    if (parameter == string("AC_WINGAREA")) {
      *AC_cfg >> WingArea;
      if (debug_lvl > 0) cout << "    WingArea: " << WingArea  << endl;
    } else if (parameter == "AC_WINGSPAN") {
      *AC_cfg >> WingSpan;
      if (debug_lvl > 0) cout << "    WingSpan: " << WingSpan  << endl;
    } else if (parameter == "AC_CHORD") {
      *AC_cfg >> cbar;
      if (debug_lvl > 0) cout << "    Chord: " << cbar << endl;
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
    } else if (parameter == "AC_IXZ") {
      *AC_cfg >> bixz;
      if (debug_lvl > 0) cout << "    baseIxz: " << bixz  << endl;
      MassBalance->SetBaseIxz(bixz);
    } else if (parameter == "AC_IYZ") {
      *AC_cfg >> biyz;
      if (debug_lvl > 0) cout << "    baseIyz: " << biyz  << endl;
      MassBalance->SetBaseIyz(biyz);
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
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadPropulsion(FGConfigFile* AC_cfg) {
  if (!Propulsion->LoadPropulsion(AC_cfg)) {
    cerr << "Propulsion not successfully loaded" << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadFlightControls(FGConfigFile* AC_cfg) {
  if (!FCS->LoadFCS(AC_cfg)) {
    cerr << "Flight Controls not successfully loaded" << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadAerodynamics(FGConfigFile* AC_cfg)
{
  if (!Aerodynamics->LoadAerodynamics(AC_cfg)) {
    cerr << "Aerodynamics not successfully loaded" << endl;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadUndercarriage(FGConfigFile* AC_cfg) {
  string token;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/UNDERCARRIAGE")) {
    lGear.push_back(FGLGear(AC_cfg, FDMExec));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadOutput(FGConfigFile* AC_cfg) {
  string token, parameter;
  int OutRate = 0;
  int subsystems = 0;

  token = AC_cfg->GetValue("NAME");
  Output->SetFilename(token);
  token = AC_cfg->GetValue("TYPE");
  Output->SetType(token);
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/OUTPUT")) {
    *AC_cfg >> parameter;
    if (parameter == "RATE_IN_HZ") *AC_cfg >> OutRate;
    if (parameter == "SIMULATION") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssSimulation;
    }
    if (parameter == "AEROSURFACES") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssAerosurfaces;
    }
    if (parameter == "RATES") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssRates;
    }
    if (parameter == "VELOCITIES") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssVelocities;
    }
    if (parameter == "FORCES") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssForces;
    }
    if (parameter == "MOMENTS") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssMoments;
    }
    if (parameter == "ATMOSPHERE") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssAtmosphere;
    }
    if (parameter == "MASSPROPS") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssMassProps;
    }
    if (parameter == "POSITION") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssPosition;
    }
    if (parameter == "COEFFICIENTS") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssCoefficients;
    }
    if (parameter == "GROUND_REACTIONS") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssGroundReactions;
    }
    if (parameter == "FCS") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssFCS;
    }
    if (parameter == "PROPULSION") {
      *AC_cfg >> parameter;
      if (parameter == "ON") subsystems += ssPropulsion;
    }
  }

  Output->SetSubsystems(subsystems);

  OutRate = OutRate>120?120:(OutRate<0?0:OutRate);
  Output->SetRate( (int)(0.5 + 1.0/(State->Getdt()*OutRate)) );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::ReadPrologue(FGConfigFile* AC_cfg) {
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
  if (CFGVersion != string(NEEDED_CFG_VERSION)) {
    cerr << endl << fgred << "YOU HAVE AN INCOMPATIBLE CFG FILE FOR THIS AIRCRAFT."
            " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cerr << "Current version needed is: " << NEEDED_CFG_VERSION << endl;
    cerr << "         You have version: " << CFGVersion << endl << fgdef << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAircraft::GetGroundReactionStrings(void) {
  string GroundReactionStrings = "";
  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionStrings += ", ";
    GroundReactionStrings += (lGear[i].GetName() + "_WOW, ");
    GroundReactionStrings += (lGear[i].GetName() + "_compressLength, ");
    GroundReactionStrings += (lGear[i].GetName() + "_compressSpeed, ");
    GroundReactionStrings += (lGear[i].GetName() + "_Force");

    firstime = false;
  }

  return GroundReactionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAircraft::GetGroundReactionValues(void) {
  char buff[20];
  string GroundReactionValues = "";

  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionValues += ", ";
    GroundReactionValues += string( lGear[i].GetWOW()?"1":"0" ) + ", ";
    GroundReactionValues += (string(gcvt(lGear[i].GetCompLen(),    5, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetCompVel(),    6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetCompForce(), 10, buff)));

    firstime = false;
  }

  return GroundReactionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::Debug(void)
{
    //TODO: Add your source code here
}

