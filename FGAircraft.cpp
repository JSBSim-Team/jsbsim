/*******************************************************************************
 
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
 
********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
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
 
********************************************************************************
INCLUDES
*******************************************************************************/

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
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex),
    vMoments(3),
    vForces(3),
    vXYZrp(3),
    vbaseXYZcg(3),
    vXYZcg(3),
    vXYZep(3),
    vEuler(3),
    vFs(3)
{
  Name = "FGAircraft";

  AxisIdx["DRAG"]  = 0;
  AxisIdx["SIDE"]  = 1;
  AxisIdx["LIFT"]  = 2;
  AxisIdx["ROLL"]  = 3;
  AxisIdx["PITCH"] = 4;
  AxisIdx["YAW"]   = 5;

  GearUp = false;

  numTanks = numEngines = numSelectedFuelTanks = numSelectedOxiTanks = 0;
}


/******************************************************************************/


FGAircraft::~FGAircraft(void) {}

/******************************************************************************/

bool FGAircraft::LoadAircraft(string aircraft_path, string engine_path, string fname) {
  string path;
  string filename;
  string aircraftCfgFileName;
  string token;

  AircraftPath = aircraft_path;
  EnginePath = engine_path;

  aircraftCfgFileName = AircraftPath + "/" + fname + "/" + fname + ".cfg";

  FGConfigFile AC_cfg(aircraftCfgFileName);
  if (!AC_cfg.IsOpen()) return false;

  ReadPrologue(&AC_cfg);

  while ((AC_cfg.GetNextConfigLine() != "EOF") &&
         (token = AC_cfg.GetValue()) != "/FDM_CONFIG") {
    if (token == "METRICS") {
      cout << "  Reading Metrics" << endl;
      ReadMetrics(&AC_cfg);
    } else if (token == "AERODYNAMICS") {
      cout << "  Reading Aerodynamics" << endl;
      ReadAerodynamics(&AC_cfg);
    } else if (token == "UNDERCARRIAGE") {
      cout << "  Reading Landing Gear" << endl;
      ReadUndercarriage(&AC_cfg);
    } else if (token == "PROPULSION") {
      cout << "  Reading Propulsion" << endl;
      ReadPropulsion(&AC_cfg);
    } else if (token == "FLIGHT_CONTROL") {
      cout << "  Reading Flight Control" << endl;
      ReadFlightControls(&AC_cfg);
    } else if (token == "OUTPUT") {
      ReadOutput(&AC_cfg);
    }
  }

  return true;
}

/******************************************************************************/

bool FGAircraft::Run(void) {
  if (!FGModel::Run()) {                 // if false then execute this Run()
    GetState();

    for (int i = 1; i <= 3; i++)  vForces(i) = vMoments(i) = 0.0;

    MassChange();

    FMProp();
    FMAero();
    FMGear();
    FMMass();

    nlf=vFs(eZ)/Weight;
  } else {                               // skip Run() execution this time
  }


  return false;
}

/******************************************************************************/

void FGAircraft::MassChange() {
  static FGColumnVector vXYZtank(3);
  float Tw;
  float IXXt, IYYt, IZZt, IXZt;
  unsigned int t;
  unsigned int axis_ctr;

  for (axis_ctr=1; axis_ctr<=3; axis_ctr++) vXYZtank(axis_ctr) = 0.0;

  // UPDATE TANK CONTENTS
  //
  // For each engine, cycle through the tanks and draw an equal amount of
  // fuel (or oxidizer) from each active tank. The needed amount of fuel is
  // determined by the engine in the FGEngine class. If more fuel is needed
  // than is available in the tank, then that amount is considered a shortage,
  // and will be drawn from the next tank. If the engine cannot be fed what it
  // needs, it will be considered to be starved, and will shut down.

  float Oshortage, Fshortage;

  for (unsigned int e=0; e<numEngines; e++) {
    Fshortage = Oshortage = 0.0;
    for (t=0; t<numTanks; t++) {
      switch(Engine[e]->GetType()) {
      case FGEngine::etRocket:

        switch(Tank[t]->GetType()) {
        case FGTank::ttFUEL:
          if (Tank[t]->GetSelected()) {
            Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                         numSelectedFuelTanks)*(dt*rate) + Fshortage);
          }
          break;
        case FGTank::ttOXIDIZER:
          if (Tank[t]->GetSelected()) {
            Oshortage = Tank[t]->Reduce((Engine[e]->CalcOxidizerNeed()/
                                         numSelectedOxiTanks)*(dt*rate) + Oshortage);
          }
          break;
        }
        break;

      case FGEngine::etPiston:
      case FGEngine::etTurboJet:
      case FGEngine::etTurboProp:

        if (Tank[t]->GetSelected()) {
          Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                       numSelectedFuelTanks)*(dt*rate) + Fshortage);
        }
        break;
      }
    }
    if ((Fshortage <= 0.0) || (Oshortage <= 0.0)) Engine[e]->SetStarved();
    else Engine[e]->SetStarved(false);
  }

  Weight = EmptyWeight;
  for (t=0; t<numTanks; t++)
    Weight += Tank[t]->GetContents();

  Mass = Weight / GRAVITY;
  // Calculate new CG here.

  Tw = 0;
  for (t=0; t<numTanks; t++) {
    vXYZtank(eX) += Tank[t]->GetX()*Tank[t]->GetContents();
    vXYZtank(eY) += Tank[t]->GetY()*Tank[t]->GetContents();
    vXYZtank(eZ) += Tank[t]->GetZ()*Tank[t]->GetContents();

    Tw += Tank[t]->GetContents();
  }

  vXYZcg = (vXYZtank + EmptyWeight*vbaseXYZcg) / (Tw + EmptyWeight);

  // Calculate new moments of inertia here

  IXXt = IYYt = IZZt = IXZt = 0.0;
  for (t=0; t<numTanks; t++) {
    IXXt += ((Tank[t]->GetX()-vXYZcg(eX))/12.0)*((Tank[t]->GetX() - vXYZcg(eX))/12.0)*Tank[t]->GetContents()/GRAVITY;
    IYYt += ((Tank[t]->GetY()-vXYZcg(eY))/12.0)*((Tank[t]->GetY() - vXYZcg(eY))/12.0)*Tank[t]->GetContents()/GRAVITY;
    IZZt += ((Tank[t]->GetZ()-vXYZcg(eZ))/12.0)*((Tank[t]->GetZ() - vXYZcg(eZ))/12.0)*Tank[t]->GetContents()/GRAVITY;
    IXZt += ((Tank[t]->GetX()-vXYZcg(eX))/12.0)*((Tank[t]->GetZ() - vXYZcg(eZ))/12.0)*Tank[t]->GetContents()/GRAVITY;
  }

  Ixx = baseIxx + IXXt;
  Iyy = baseIyy + IYYt;
  Izz = baseIzz + IZZt;
  Ixz = baseIxz + IXZt;

}

/******************************************************************************/

void FGAircraft::FMAero(void) {
  static FGColumnVector vDXYZcg(3);
  unsigned int axis_ctr,ctr;

  for (axis_ctr=1; axis_ctr<=3; axis_ctr++) vFs(axis_ctr) = 0.0;

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr=0; ctr < Coeff[axis_ctr].size(); ctr++) {
      vFs(axis_ctr+1) += Coeff[axis_ctr][ctr].TotalValue();
    }
  }

  vForces += State->GetTs2b(alpha, beta)*vFs;

  // The d*cg distances below, given in inches, are the distances FROM the c.g.
  // TO the reference point. Since the c.g. and ref point are given in inches in
  // the structural system (X positive rearwards) and the body coordinate system
  // is given with X positive out the nose, the dxcg and dzcg values are
  // *rotated* 180 degrees about the Y axis.

  vDXYZcg(eX) = -(vXYZrp(eX) - vXYZcg(eX))/12.0;  //cg and rp values are in inches
  vDXYZcg(eY) =  (vXYZrp(eY) - vXYZcg(eY))/12.0;
  vDXYZcg(eZ) = -(vXYZrp(eZ) - vXYZcg(eZ))/12.0;

  vMoments(eL) += vForces(eZ)*vDXYZcg(eY) - vForces(eY)*vDXYZcg(eZ); // rolling moment
  vMoments(eM) += vForces(eX)*vDXYZcg(eZ) - vForces(eZ)*vDXYZcg(eX); // pitching moment
  vMoments(eN) += vForces(eY)*vDXYZcg(eX) - vForces(eX)*vDXYZcg(eY); // yawing moment

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr = 0; ctr < Coeff[axis_ctr+3].size(); ctr++) {
      vMoments(axis_ctr+1) += Coeff[axis_ctr+3][ctr].TotalValue();
    }
  }
}

/******************************************************************************/

void FGAircraft::FMGear(void) {
  if (GearUp) {
    // crash routine
  }
  else {
    for (unsigned int i=0;i<lGear.size();i++) {
      vForces  += lGear[i]->Force();
      vMoments += lGear[i]->Moment();
    }
  }
}

/******************************************************************************/

void FGAircraft::FMMass(void) {
  vForces(eX) += -GRAVITY*sin(vEuler(eTht)) * Mass;
  vForces(eY) +=  GRAVITY*sin(vEuler(ePhi))*cos(vEuler(eTht)) * Mass;
  vForces(eZ) +=  GRAVITY*cos(vEuler(ePhi))*cos(vEuler(eTht)) * Mass;
}

/******************************************************************************/

void FGAircraft::FMProp(void) {
  for (unsigned int i=0;i<numEngines;i++) {
    vForces(eX) += Engine[i]->CalcThrust();
  }
}

/******************************************************************************/

void FGAircraft::GetState(void) {
  dt = State->Getdt();

  alpha = Translation->Getalpha();
  beta = Translation->Getbeta();
  vEuler = Rotation->GetEuler();
}

/******************************************************************************/

void FGAircraft::ReadMetrics(FGConfigFile* AC_cfg) {
  string token = "";
  string parameter;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/METRICS") {
    *AC_cfg >> parameter;
    if (parameter == "AC_WINGAREA") *AC_cfg >> WingArea;
    else if (parameter == "AC_WINGSPAN") *AC_cfg >> WingSpan;
    else if (parameter == "AC_CHORD") *AC_cfg >> cbar;
    else if (parameter == "AC_IXX") *AC_cfg >> baseIxx;
    else if (parameter == "AC_IYY") *AC_cfg >> baseIyy;
    else if (parameter == "AC_IZZ") *AC_cfg >> baseIzz;
    else if (parameter == "AC_IXZ") *AC_cfg >> baseIxz;
    else if (parameter == "AC_EMPTYWT") *AC_cfg >> EmptyWeight;
    else if (parameter == "AC_CGLOC") *AC_cfg >> vbaseXYZcg(eX) >> vbaseXYZcg(eY) >> vbaseXYZcg(eZ);
    else if (parameter == "AC_EYEPTLOC") *AC_cfg >> vXYZep(eX) >> vXYZep(eY) >> vXYZep(eZ);
    else if (parameter == "AC_AERORP") *AC_cfg >> vXYZrp(eX) >> vXYZrp(eY) >> vXYZrp(eZ);
    else if (parameter == "AC_ALPHALIMITS") *AC_cfg >> alphaclmin >> alphaclmax;
  }
}

/******************************************************************************/

void FGAircraft::ReadPropulsion(FGConfigFile* AC_cfg) {
  string token;
  string engine_name;
  string parameter;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/PROPULSION") {
    *AC_cfg >> parameter;

    if (parameter == "AC_ENGINE") {

      *AC_cfg >> engine_name;
      Engine[numEngines] = new FGEngine(FDMExec, EnginePath, engine_name, numEngines);
      numEngines++;

    } else if (parameter == "AC_TANK") {

      Tank[numTanks] = new FGTank(AC_cfg);
      switch(Tank[numTanks]->GetType()) {
      case FGTank::ttFUEL:
        numSelectedFuelTanks++;
        break;
      case FGTank::ttOXIDIZER:
        numSelectedOxiTanks++;
        break;
      }
      numTanks++;
    }
  }
}

/******************************************************************************/

void FGAircraft::ReadFlightControls(FGConfigFile* AC_cfg) {
  string token;

  FCS->LoadFCS(AC_cfg);
}

/******************************************************************************/

void FGAircraft::ReadAerodynamics(FGConfigFile* AC_cfg) {
  string token, axis;

  AC_cfg->GetNextConfigLine();

  Coeff.push_back(*(new CoeffArray()));
  Coeff.push_back(*(new CoeffArray()));
  Coeff.push_back(*(new CoeffArray()));
  Coeff.push_back(*(new CoeffArray()));
  Coeff.push_back(*(new CoeffArray()));
  Coeff.push_back(*(new CoeffArray()));

  while ((token = AC_cfg->GetValue()) != "/AERODYNAMICS") {
    if (token == "AXIS") {
      axis = AC_cfg->GetValue("NAME");
      AC_cfg->GetNextConfigLine();
      while ((token = AC_cfg->GetValue()) != "/AXIS") {
        Coeff[AxisIdx[axis]].push_back(*(new FGCoefficient(FDMExec, AC_cfg)));
        DisplayCoeffFactors(Coeff[AxisIdx[axis]].back().Getmultipliers());
      }
      AC_cfg->GetNextConfigLine();
    }
  }
}

/******************************************************************************/

void FGAircraft::ReadUndercarriage(FGConfigFile* AC_cfg) {
  string token;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/UNDERCARRIAGE") {
    lGear.push_back(new FGLGear(AC_cfg, FDMExec));
  }
}

/******************************************************************************/

void FGAircraft::ReadOutput(FGConfigFile* AC_cfg) {
  string token, parameter;
  int OutRate = 0;
  int subsystems = 0;

  token = AC_cfg->GetValue("NAME");
  Output->SetFilename(token);
  token = AC_cfg->GetValue("TYPE");
  Output->SetType(token);
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/OUTPUT") {
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
  }

  Output->SetSubsystems(subsystems);

  OutRate = OutRate>120?120:(OutRate<0?0:OutRate);
  Output->SetRate( (int)(0.5 + 1.0/(State->Getdt()*OutRate)) );
}

/******************************************************************************/

void FGAircraft::ReadPrologue(FGConfigFile* AC_cfg) {
  string token = AC_cfg->GetValue();
  string scratch;
  AircraftName = AC_cfg->GetValue("NAME");
  cout << "Reading Aircraft Configuration File: " << AircraftName << endl;
  scratch=AC_cfg->GetValue("VERSION").c_str();

  CFGVersion = AC_cfg->GetValue("VERSION");
  cout << "                            Version: " << CFGVersion << endl;
  if (CFGVersion != NEEDED_CFG_VERSION) {
    cout << endl << "YOU HAVE AN INCOMPATIBLE CFG FILE FOR THIS AIRCRAFT."
    " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cout << "Current version needed is: " << NEEDED_CFG_VERSION << endl;
    cout << "         You have version: " << CFGVersion << endl << endl;
    //exit(-1);
  }


}

/******************************************************************************/

void FGAircraft::DisplayCoeffFactors(int multipliers) {
  cout << "   Non-Dimensionalized by: ";

  if (multipliers & FG_QBAR)      cout << "qbar ";
  if (multipliers & FG_WINGAREA)  cout << "S ";
  if (multipliers & FG_WINGSPAN)  cout << "b ";
  if (multipliers & FG_CBAR)      cout << "c ";
  if (multipliers & FG_ALPHA)     cout << "alpha ";
  if (multipliers & FG_ALPHADOT)  cout << "alphadot ";
  if (multipliers & FG_BETA)      cout << "beta ";
  if (multipliers & FG_BETADOT)   cout << "betadot ";
  if (multipliers & FG_PITCHRATE) cout << "q ";
  if (multipliers & FG_ROLLRATE)  cout << "p ";
  if (multipliers & FG_YAWRATE)   cout << "r ";

  if (multipliers & FG_ELEVATOR_CMD)  cout << "De cmd ";
  if (multipliers & FG_AILERON_CMD)   cout << "Da cmd ";
  if (multipliers & FG_RUDDER_CMD)    cout << "Dr cmd ";
  if (multipliers & FG_FLAPS_CMD)     cout << "Df cmd ";
  if (multipliers & FG_SPOILERS_CMD)  cout << "Dsp cmd ";
  if (multipliers & FG_SPDBRAKE_CMD)   cout << "Dsb cmd ";

  if (multipliers & FG_ELEVATOR_POS)  cout << "De ";
  if (multipliers & FG_AILERON_POS)   cout << "Da ";
  if (multipliers & FG_RUDDER_POS)    cout << "Dr ";
  if (multipliers & FG_FLAPS_POS)     cout << "Df ";
  if (multipliers & FG_SPOILERS_POS)  cout << "Dsp ";
  if (multipliers & FG_SPDBRAKE_POS)  cout << "Dsb ";

  if (multipliers & FG_MACH)      cout << "Mach ";
  if (multipliers & FG_ALTITUDE)  cout << "h ";
  if (multipliers & FG_BI2VEL)    cout << "b /(2*Vt) ";
  if (multipliers & FG_CI2VEL)    cout << "c /(2*Vt) ";

  cout << endl;
}

/******************************************************************************/

string FGAircraft::GetCoefficientStrings(void) {
  string CoeffStrings = "";
  bool firstime = true;

  for (unsigned int axis = 0; axis < 6; axis++) {
    for (unsigned int sd = 0; sd < Coeff[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        CoeffStrings += ", ";
      }
      CoeffStrings += Coeff[axis][sd].Getname();
    }
  }

  return CoeffStrings;
}

/******************************************************************************/

string FGAircraft::GetCoefficientValues(void) {
  string SDValues = "";
  char buffer[10];
  bool firstime = true;

  for (unsigned int axis = 0; axis < 6; axis++) {
    for (unsigned int sd = 0; sd < Coeff[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        SDValues += ", ";
      }
      sprintf(buffer, "%9.6f", Coeff[axis][sd].GetSD());
      SDValues += string(buffer);
    }
  }

  return SDValues;
  ;
}

/******************************************************************************/

string FGAircraft::GetGroundReactionStrings(void) {
  string GroundReactionStrings = "";
  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionStrings += ", ";
    GroundReactionStrings += (lGear[i]->GetName() + "_WOW, ");
    GroundReactionStrings += (lGear[i]->GetName() + "_compressLength, ");
    GroundReactionStrings += (lGear[i]->GetName() + "_compressSpeed, ");
    GroundReactionStrings += (lGear[i]->GetName() + "_Force");

    firstime = false;
  }

  return GroundReactionStrings;
}

/******************************************************************************/

string FGAircraft::GetGroundReactionValues(void) {
  char buff[20];
  string GroundReactionValues = "";

  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionValues += ", ";
    GroundReactionValues += string( lGear[i]->GetWOW()?"1":"0" ) + ", ";
    GroundReactionValues += (string(gcvt(lGear[i]->GetCompLen(),    5, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i]->GetCompVel(),    6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i]->GetCompForce(), 10, buff)));

    firstime = false;
  }

  return GroundReactionValues;
}

