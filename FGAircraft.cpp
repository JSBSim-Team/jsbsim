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
# ifndef __BORLANDC__
#  include <Include/compiler.h>
# endif
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

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAircraft";

  AxisIdx["LIFT"]  = 0;
  AxisIdx["SIDE"]  = 1;
  AxisIdx["DRAG"]  = 2;
  AxisIdx["ROLL"]  = 3;
  AxisIdx["PITCH"] = 4;
  AxisIdx["YAW"]   = 5;
}


/******************************************************************************/


FGAircraft::~FGAircraft(void)
{
}

/******************************************************************************/

bool FGAircraft::LoadAircraft(string aircraft_path, string engine_path, string fname)
{
  string path;
  string filename;
  string aircraftCfgFileName;
  string token;

  AircraftPath = aircraft_path;
  EnginePath = engine_path;

  aircraftCfgFileName = AircraftPath + "/" + fname + "/" + fname + ".cfg";

  FGConfigFile AC_cfg(aircraftCfgFileName);

  ReadPrologue(&AC_cfg);

  while ((AC_cfg.GetNextConfigLine() != "EOF") &&
                                   (token = AC_cfg.GetValue()) != "/FDM_CONFIG")
  {
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
    }
  }

  return true;
}

/******************************************************************************/

bool FGAircraft::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    GetState();

    for (int i = 0; i < 3; i++)  Forces[i] = Moments[i] = 0.0;

    MassChange();

    FMProp(); FMAero(); FMGear(); FMMass();

    PutState();
  } else {                               // skip Run() execution this time
  }
  return false;
}

/******************************************************************************/

void FGAircraft::MassChange()
{
  float Xt, Yt, Zt, Tw;
  float IXXt, IYYt, IZZt, IXZt;
  int t;

  // UPDATE TANK CONTENTS
  //
  // For each engine, cycle through the tanks and draw an equal amount of
  // fuel (or oxidizer) from each active tank. The needed amount of fuel is
  // determined by the engine in the FGEngine class. If more fuel is needed
  // than is available in the tank, then that amount is considered a shortage,
  // and will be drawn from the next tank. If the engine cannot be fed what it
  // needs, it will be considered to be starved, and will shut down.

  float Oshortage, Fshortage;

  for (int e=0; e<numEngines; e++) {
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

  Xt = Yt = Zt = Tw = 0;
  for (t=0; t<numTanks; t++) {
    Xt += Tank[t]->GetX()*Tank[t]->GetContents();
    Yt += Tank[t]->GetY()*Tank[t]->GetContents();
    Zt += Tank[t]->GetZ()*Tank[t]->GetContents();

    Tw += Tank[t]->GetContents();
  }

  Xcg = (Xt + EmptyWeight*baseXcg) / (Tw + EmptyWeight);
  Ycg = (Yt + EmptyWeight*baseYcg) / (Tw + EmptyWeight);
  Zcg = (Zt + EmptyWeight*baseZcg) / (Tw + EmptyWeight);

  // Calculate new moments of inertia here

  IXXt = IYYt = IZZt = IXZt = 0.0;
  for (t=0; t<numTanks; t++) {
    IXXt += ((Tank[t]->GetX()-Xcg)/12.0)*((Tank[t]->GetX() - Xcg)/12.0)*Tank[t]->GetContents()/GRAVITY;
    IYYt += ((Tank[t]->GetY()-Ycg)/12.0)*((Tank[t]->GetY() - Ycg)/12.0)*Tank[t]->GetContents()/GRAVITY;
    IZZt += ((Tank[t]->GetZ()-Zcg)/12.0)*((Tank[t]->GetZ() - Zcg)/12.0)*Tank[t]->GetContents()/GRAVITY;
    IXZt += ((Tank[t]->GetX()-Xcg)/12.0)*((Tank[t]->GetZ() - Zcg)/12.0)*Tank[t]->GetContents()/GRAVITY;
  }

  Ixx = baseIxx + IXXt;
  Iyy = baseIyy + IYYt;
  Izz = baseIzz + IZZt;
  Ixz = baseIxz + IXZt;

}

/******************************************************************************/

void FGAircraft::FMAero(void)
{
  float F[3];
  float dxcg,dycg,dzcg;
  float ca, cb, sa, sb;
  unsigned int axis_ctr,ctr;
  F[0] = F[1] = F[2] = 0.0;

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr=0; ctr < Coeff[axis_ctr].size(); ctr++) {
      F[axis_ctr] += Coeff[axis_ctr][ctr].TotalValue();
    }
  }

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  Forces[0] += - F[DragCoeff]*ca*cb
               - F[SideCoeff]*ca*sb
               + F[LiftCoeff]*sa;
  Forces[1] +=   F[DragCoeff]*sb
                 + F[SideCoeff]*cb;
  Forces[2] += - F[DragCoeff]*sa*cb
               - F[SideCoeff]*sa*sb
               - F[LiftCoeff]*ca;

  // The d*cg distances below, given in inches, are the distances FROM the c.g.
  // TO the reference point. Since the c.g. and ref point are given in inches in
  // the structural system (X positive rearwards) and the body coordinate system
  // is given with X positive out the nose, the dxcg and dzcg values are
  // *rotated* 180 degrees about the Y axis.

  dxcg = -(Xrp - Xcg)/12; //cg and rp values are in inches
  dycg =  (Yrp - Ycg)/12;
  dzcg = -(Zrp - Zcg)/12;

  Moments[0] +=  Forces[2]*dycg - Forces[1]*dzcg; //rolling moment
  Moments[1] +=  Forces[0]*dzcg - Forces[2]*dxcg; //pitching moment
  Moments[2] += -Forces[0]*dycg + Forces[1]*dxcg; //yawing moment

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr = 0; ctr < Coeff[axis_ctr+3].size(); ctr++) {
      Moments[axis_ctr] += Coeff[axis_ctr+3][ctr].TotalValue();
    }
  }
}

/******************************************************************************/

void FGAircraft::FMGear(void)
{
  if (GearUp) {
    // crash routine
  } else {
    for (unsigned int i=0;i<lGear.size();i++) {
      //      lGear[i].
    }
  }
}

/******************************************************************************/

void FGAircraft::FMMass(void)
{
  Forces[0] += -GRAVITY*sin(tht) * Mass;
  Forces[1] +=  GRAVITY*sin(phi)*cos(tht) * Mass;
  Forces[2] +=  GRAVITY*cos(phi)*cos(tht) * Mass;
}

/******************************************************************************/

void FGAircraft::FMProp(void)
{
  for (int i=0;i<numEngines;i++) {
    Forces[0] += Engine[i]->CalcThrust();
  }
}

/******************************************************************************/

void FGAircraft::GetState(void)
{
  dt = State->Getdt();

  alpha = Translation->Getalpha();
  beta = Translation->Getbeta();
  phi = Rotation->Getphi();
  tht = Rotation->Gettht();
  psi = Rotation->Getpsi();
}

/******************************************************************************/

void FGAircraft::PutState(void)
{
}

/******************************************************************************/

void FGAircraft::ReadMetrics(FGConfigFile* AC_cfg)
{
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
    else if (parameter == "AC_CGLOC") *AC_cfg >> baseXcg >> baseYcg >> baseZcg;
    else if (parameter == "AC_EYEPTLOC") *AC_cfg >> Xep >> Yep >> Zep;
    else if (parameter == "AC_AERORP") *AC_cfg >> Xrp >> Yrp >> Zrp;
  }
}

/******************************************************************************/

void FGAircraft::ReadPropulsion(FGConfigFile* AC_cfg)
{
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

void FGAircraft::ReadFlightControls(FGConfigFile* AC_cfg)
{
  string token;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/FLIGHT_CONTROL") {

    AC_cfg->GetNextConfigLine();
  }
}

/******************************************************************************/

void FGAircraft::ReadAerodynamics(FGConfigFile* AC_cfg)
{
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
      }
      AC_cfg->GetNextConfigLine();
    }
  }
}

/******************************************************************************/

void FGAircraft::ReadUndercarriage(FGConfigFile* AC_cfg)
{
  string token;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/UNDERCARRIAGE") {
    lGear.push_back(new FGLGear(AC_cfg));
  }
}

/******************************************************************************/

void FGAircraft::ReadPrologue(FGConfigFile* AC_cfg)
{
  string token = AC_cfg->GetValue();

  AircraftName = AC_cfg->GetValue("NAME");
  cout << "Reading Aircraft Configuration File: " << AircraftName << endl;
  CFGVersion = strtod(AC_cfg->GetValue("VERSION").c_str(),NULL);
  cout << "                            Version: " << CFGVersion << endl;

  if (CFGVersion < NEEDED_CFG_VERSION) {
    cout << endl << "YOU HAVE AN OLD CFG FILE FOR THIS AIRCRAFT."
                    " RESULTS WILL BE UNPREDICTABLE !!" << endl;
    cout << "Current version needed is: " << NEEDED_CFG_VERSION << endl;
    cout << "         You have version: " << CFGVersion << endl << endl;
    exit(-1);
  }
}

