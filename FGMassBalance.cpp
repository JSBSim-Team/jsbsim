/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMassBalance.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models weight and balance

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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

This class models the change in weight and balance of the aircraft due to fuel
burnoff, etc.

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGMassBalance.h"
#include "FGPropertyManager.h"

static const char *IdSrc = "$Id: FGMassBalance.cpp,v 1.27 2002/06/05 03:47:48 jberndt Exp $";
static const char *IdHdr = ID_MASSBALANCE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGMassBalance";
  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMassBalance::~FGMassBalance()
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Run(void)
{
  if (!FGModel::Run()) {

    Weight = EmptyWeight + Propulsion->GetTanksWeight() + GetPointMassWeight();

    Mass = Weight / Inertial->gravity();

// Calculate new CG here.

    vXYZcg = (Propulsion->GetTanksMoment() + EmptyWeight*vbaseXYZcg
                                       + GetPointMassMoment() ) / Weight;

// Calculate new moments of inertia here

    Ixx = baseIxx + Propulsion->GetTanksIxx(vXYZcg) + GetPMIxx();
    Iyy = baseIyy + Propulsion->GetTanksIyy(vXYZcg) + GetPMIyy();
    Izz = baseIzz + Propulsion->GetTanksIzz(vXYZcg) + GetPMIzz();
    Ixy = baseIxy + Propulsion->GetTanksIxy(vXYZcg) + GetPMIxy();
    Ixz = baseIxz + Propulsion->GetTanksIxz(vXYZcg) + GetPMIxz();

    Debug(2);

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::AddPointMass(double weight, double X, double Y, double Z)
{
  PointMassLoc.push_back(*(new FGColumnVector3(X, Y, Z)));
  PointMassWeight.push_back(weight);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPointMassWeight(void)
{
  double PM_total_weight = 0.0;

  for (unsigned int i=0; i<PointMassWeight.size(); i++) {
    PM_total_weight += PointMassWeight[i];
  }
  return PM_total_weight;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGMassBalance::GetPointMassMoment(void)
{
  PointMassCG.InitMatrix();

  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    PointMassCG += PointMassWeight[i]*PointMassLoc[i];
  }
  return PointMassCG;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPMIxx(void)
{
  double I = 0.0;
  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    I += (PointMassLoc[i](eX)-vXYZcg(eX))*(PointMassLoc[i](eX)-vXYZcg(eX))*PointMassWeight[i];
  }
  I /= (144.0*Inertial->gravity());
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPMIyy(void)
{
  double I = 0.0;
  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    I += (PointMassLoc[i](eY)-vXYZcg(eY))*(PointMassLoc[i](eY)-vXYZcg(eY))*PointMassWeight[i];
  }
  I /= (144.0*Inertial->gravity());
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPMIzz(void)
{
  double I = 0.0;
  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    I += (PointMassLoc[i](eZ)-vXYZcg(eZ))*(PointMassLoc[i](eZ)-vXYZcg(eZ))*PointMassWeight[i];
  }
  I /= (144.0*Inertial->gravity());
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPMIxy(void)
{
  double I = 0.0;
  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    I += (PointMassLoc[i](eX)-vXYZcg(eX))*(PointMassLoc[i](eY)-vXYZcg(eY))*PointMassWeight[i];
  }
  I /= (144.0*Inertial->gravity());
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPMIxz(void)
{
  double I = 0.0;
  for (unsigned int i=0; i<PointMassLoc.size(); i++) {
    I += (PointMassLoc[i](eX)-vXYZcg(eX))*(PointMassLoc[i](eZ)-vXYZcg(eZ))*PointMassWeight[i];
  }
  I /= (144.0*Inertial->gravity());
  return I;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::bind(void)
{ 
  typedef double (FGMassBalance::*PMF)(int) const;
  PropertyManager->Tie("inertia/mass-slugs", this,
                       &FGMassBalance::GetMass);
  PropertyManager->Tie("inertia/weight-lbs", this,
                       &FGMassBalance::GetWeight);
  PropertyManager->Tie("inertia/ixx-lbsft2", this,
                       &FGMassBalance::GetIxx);
  PropertyManager->Tie("inertia/iyy-lbsft2", this,
                       &FGMassBalance::GetIyy);
  PropertyManager->Tie("inertia/izz-lbsft2", this,
                       &FGMassBalance::GetIzz);
  PropertyManager->Tie("inertia/ixy-lbsft2", this,
                       &FGMassBalance::GetIxy);
  PropertyManager->Tie("inertia/ixz-lbsft2", this,
                       &FGMassBalance::GetIxz);
  PropertyManager->Tie("inertia/cg-x-ft", this,1,
                       (PMF)&FGMassBalance::GetXYZcg);
  PropertyManager->Tie("inertia/cg-y-ft", this,2,
                       (PMF)&FGMassBalance::GetXYZcg);
  PropertyManager->Tie("inertia/cg-z-ft", this,3,
                       (PMF)&FGMassBalance::GetXYZcg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::unbind(void)
{
  PropertyManager->Untie("inertia/mass-slugs");
  PropertyManager->Untie("inertia/weight-lbs");
  PropertyManager->Untie("inertia/ixx-lbsft2");
  PropertyManager->Untie("inertia/iyy-lbsft2");
  PropertyManager->Untie("inertia/izz-lbsft2");
  PropertyManager->Untie("inertia/ixy-lbsft2");
  PropertyManager->Untie("inertia/ixz-lbsft2");
  PropertyManager->Untie("inertia/cg-x-ft");
  PropertyManager->Untie("inertia/cg-y-ft");
  PropertyManager->Untie("inertia/cg-z-ft");
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

void FGMassBalance::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPiston" << endl;
    if (from == 1) cout << "Destroyed:    FGPiston" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
    if (from == 2) {
      if (EmptyWeight <= 0.0 || EmptyWeight > 1e9)
        cout << "MassBalance::EmptyWeight out of bounds: " << EmptyWeight << endl;
      if (Weight <= 0.0 || Weight > 1e9)
        cout << "MassBalance::Weight out of bounds: " << Weight << endl;
      if (Mass <= 0.0 || Mass > 1e9)
        cout << "MassBalance::Mass out of bounds: " << Mass << endl;
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

