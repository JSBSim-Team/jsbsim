/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAerodynamics.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the aerodynamic forces (gear and collision)

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created
04/22/01   JSB   Moved code into here from FGAircraft

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAerodynamics.h"

static const char *IdSrc = "$Id: FGAerodynamics.cpp,v 1.10 2001/04/23 19:35:48 jberndt Exp $";
static const char *IdHdr = ID_AERODYNAMICS;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAerodynamics::FGAerodynamics(FGFDMExec* FDMExec) : FGModel(FDMExec),
    vMoments(3),
    vForces(3),
    vFs(3),
    vLastFs(3),
    vDXYZcg(3),
    vAeroBodyForces(3)
{
  Name = "FGAerodynamics";

  AxisIdx["DRAG"]  = 0;
  AxisIdx["SIDE"]  = 1;
  AxisIdx["LIFT"]  = 2;
  AxisIdx["ROLL"]  = 3;
  AxisIdx["PITCH"] = 4;
  AxisIdx["YAW"]   = 5;

  Coeff = new CoeffArray[6];

  if (debug_lvl & 2) cout << "Instantiated: FGAerodynamics" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAerodynamics::~FGAerodynamics()
{
  unsigned int i,j;
  for (i=0; i<6; i++) {
    for (j=0; j<Coeff[i].size(); j++) {
      delete Coeff[i][j];
    }
  }
  delete[] Coeff;

  if (debug_lvl & 2) cout << "Destroyed:    FGAerodynamics" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Run(void)
{
  float alpha, beta;

  if (!FGModel::Run()) {

    unsigned int axis_ctr,ctr;

    alpha = Translation->Getalpha();
    beta = Translation->Getbeta();

    vLastFs = vFs;
    vFs.InitMatrix();

    for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
      for (ctr=0; ctr < Coeff[axis_ctr].size(); ctr++) {
        vFs(axis_ctr+1) += Coeff[axis_ctr][ctr]->TotalValue();
      }
    }

    vAeroBodyForces = State->GetTs2b(alpha, beta)*vFs;
    vForces += vAeroBodyForces;

    // see http://home.earthlink.net/~apeden/jsbsim_moments_due_to_forces.txt
    // for details on this

    vDXYZcg(eX) = -(Aircraft->GetXYZrp(eX) - MassBalance->GetXYZcg(eX))/12.0;
    vDXYZcg(eY) =  (Aircraft->GetXYZrp(eY) - MassBalance->GetXYZcg(eY))/12.0;
    vDXYZcg(eZ) = -(Aircraft->GetXYZrp(eZ) - MassBalance->GetXYZcg(eZ))/12.0;

    vMoments(eL) += vAeroBodyForces(eZ)*vDXYZcg(eY) - vAeroBodyForces(eY)*vDXYZcg(eZ);
    vMoments(eM) += vAeroBodyForces(eX)*vDXYZcg(eZ) - vAeroBodyForces(eZ)*vDXYZcg(eX);
    vMoments(eN) += vAeroBodyForces(eY)*vDXYZcg(eX) - vAeroBodyForces(eX)*vDXYZcg(eY);

    for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
      for (ctr = 0; ctr < Coeff[axis_ctr+3].size(); ctr++) {
        vMoments(axis_ctr+1) += Coeff[axis_ctr+3][ctr]->TotalValue();
      }
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::LoadAerodynamics(FGConfigFile* AC_cfg)
{
  string token, axis;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/AERODYNAMICS")) {
    if (token == "AXIS") {
      CoeffArray ca;
      axis = AC_cfg->GetValue("NAME");
      AC_cfg->GetNextConfigLine();
      while ((token = AC_cfg->GetValue()) != string("/AXIS")) {
        ca.push_back(new FGCoefficient(FDMExec, AC_cfg));
        if (debug_lvl > 0) DisplayCoeffFactors(ca.back()->Getmultipliers());
      }
      Coeff[AxisIdx[axis]] = ca;
      AC_cfg->GetNextConfigLine();
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAerodynamics::DisplayCoeffFactors(vector <eParam> multipliers)
{
  unsigned int i;

  cout << "   Non-Dimensionalized by: ";

  for (i=0; i<multipliers.size();i++) cout << State->paramdef[multipliers[i]];

  cout << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetCoefficientStrings(void)
{
  string CoeffStrings = "";
  bool firstime = true;

  for (unsigned int axis = 0; axis < 6; axis++) {
    for (unsigned int sd = 0; sd < Coeff[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        CoeffStrings += ", ";
      }
      CoeffStrings += Coeff[axis][sd]->Getname();
    }
  }

  return CoeffStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetCoefficientValues(void)
{
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
      sprintf(buffer, "%9.6f", Coeff[axis][sd]->GetSD());
      SDValues += string(buffer);
    }
  }

  return SDValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAerodynamics::Debug(void)
{
    //TODO: Add your source code here
}

