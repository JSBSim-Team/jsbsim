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
#include "FGPropulsion.h"
#include "FGPropertyManager.h"
#include "FGInertial.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGMassBalance.cpp,v 1.34 2004/03/03 11:56:52 jberndt Exp $";
static const char *IdHdr = ID_MASSBALANCE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGMassBalance";
  Weight = EmptyWeight = Mass = 0.0;
  Ixx = Iyy = Izz = Ixy = Ixz = Iyz = 0.0;
  baseIxx = baseIyy = baseIzz = baseIxy = baseIxz = baseIyz = 0.0;
  pmIxx = pmIyy = pmIzz = pmIxy = pmIxz = pmIyz = 0.0;

  vbaseXYZcg(eX) = 0.0;
  vbaseXYZcg(eY) = 0.0;
  vbaseXYZcg(eZ) = 0.0;
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

    Mass = Weight / Inertial->SLgravity();

// Calculate new CG here.

    vXYZcg = (Propulsion->GetTanksMoment() + EmptyWeight*vbaseXYZcg
                                       + GetPointMassMoment() ) / Weight;

// Calculate new moments of inertia here

    CalculatePMInertia();

    Ixx = baseIxx + Propulsion->GetTanksIxx() + pmIxx;
    Iyy = baseIyy + Propulsion->GetTanksIyy() + pmIyy;
    Izz = baseIzz + Propulsion->GetTanksIzz() + pmIzz;
    Ixy = baseIxy + Propulsion->GetTanksIxy() + pmIxy;
    Ixz = baseIxz + Propulsion->GetTanksIxz() + pmIxz;
    Iyz = baseIyz + Propulsion->GetTanksIyz() + pmIyz;

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

void FGMassBalance::CalculatePMInertia(void)
{
  int size;

  size = PointMassLoc.size();
  if (size == 0) return;

  pmIxx = pmIyy = pmIzz = pmIxy = pmIxz = pmIyz = 0.0;

  for (unsigned int i=0; i<size; i++) {

    vPMxyz = StructuralToBody(PointMassLoc[i]); // get vector, CG to PM

    pmIxx += vPMxyz(eX)*vPMxyz(eX)*PointMassWeight[i];
    pmIyy += vPMxyz(eY)*vPMxyz(eY)*PointMassWeight[i];
    pmIzz += vPMxyz(eZ)*vPMxyz(eZ)*PointMassWeight[i];
    pmIxy += vPMxyz(eX)*vPMxyz(eY)*PointMassWeight[i];
    pmIxz += vPMxyz(eX)*vPMxyz(eZ)*PointMassWeight[i];
    pmIyz += vPMxyz(eY)*vPMxyz(eZ)*PointMassWeight[i];
  }

  pmIxx /= Inertial->SLgravity();
  pmIyy /= Inertial->SLgravity();
  pmIzz /= Inertial->SLgravity();
  pmIxy /= Inertial->SLgravity();
  pmIxz /= Inertial->SLgravity();
  pmIyz /= Inertial->SLgravity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGMassBalance::StructuralToBody(const FGColumnVector3& r) const
{
  // Under the assumption that in the structural frame the:
  //
  // - X-axis is directed afterwards,
  // - Y-axis is directed towards the right,
  // - Z-axis is directed upwards,
  //
  // (as documented in http://jsbsim.sourceforge.net/JSBSimCoordinates.pdf)
  // we have to subtract first the center of gravity of the plane which
  // is also defined in the structural frame:
  //
  //   FGColumnVector3 cgOff = r - vXYZcg;
  //
  // Next, we do a change of units:
  //
  //   cgOff *= inchtoft;
  //
  // And then a 180 degree rotation is done about the Y axis so that the:
  //
  // - X-axis is directed forward,
  // - Y-axis is directed towards the right,
  // - Z-axis is directed downward.
  //
  // This is needed because the structural and body frames are 180 degrees apart.

  return FGColumnVector3(inchtoft*(vXYZcg(1)-r(1)),
                         inchtoft*(r(2)-vXYZcg(2)),
                         inchtoft*(vXYZcg(3)-r(3)));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::bind(void) // UNITS SHOULD BE CHANGED FOR Ixx, etc. to SLUG-FT^2 ??
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
  PropertyManager->Tie("inertia/iyz-lbsft2", this,
                       &FGMassBalance::GetIyz);
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
  PropertyManager->Untie("inertia/iyz-lbsft2");
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
}
