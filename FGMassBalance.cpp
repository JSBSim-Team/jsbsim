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

namespace JSBSim {

static const char *IdSrc = "$Id: FGMassBalance.cpp,v 1.43 2004/10/05 14:08:54 jberndt Exp $";
static const char *IdHdr = ID_MASSBALANCE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGMassBalance";
  Weight = EmptyWeight = Mass = 0.0;

  vbaseXYZcg.InitMatrix(0.0);
  baseJ.InitMatrix();
  mJ.InitMatrix();
  mJinv.InitMatrix();
  pmJ.InitMatrix();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMassBalance::~FGMassBalance()
{
  unbind();
  PointMasses.clear();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Load(Element* el)
{
  Element *element;
  string element_name = "";
  double bixx, biyy, bizz, bixy, bixz, biyz;

  bixx = biyy = bizz = bixy = bixz = biyz = 0.0;
  bixx = el->FindElementValueAsNumberConvertTo("IXX", "SLUG*FT2");
  biyy = el->FindElementValueAsNumberConvertTo("IYY", "SLUG*FT2");
  bizz = el->FindElementValueAsNumberConvertTo("IZZ", "SLUG*FT2");
  bixy = el->FindElementValueAsNumberConvertTo("IXY", "SLUG*FT2");
  bixz = el->FindElementValueAsNumberConvertTo("IXZ", "SLUG*FT2");
  biyz = el->FindElementValueAsNumberConvertTo("IYZ", "SLUG*FT2");
  SetAircraftBaseInertias(FGMatrix33(  bixx,  -bixy,  -bixz,
                                      -bixy,  biyy,  -biyz,
                                      -bixz,  -biyz,  bizz ));
  EmptyWeight = el->FindElementValueAsNumberConvertTo("EMPTYWT", "LBS");

  element = el->FindElement("LOCATION");
  while (element) {
    element_name = element->GetAttributeValue("NAME");
    if (element_name == "CG") vbaseXYZcg.InitMatrix(element, "IN");
    element = el->FindNextElement("LOCATION");
  }

// Find all POINTMASS elements that descend from this METRICS branch of the
// config file.

  element = el->FindElement("POINTMASS");
  while (element) {
    AddPointMass(element);
    element = el->FindNextElement("POINTMASS");
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Run(void)
{
  double denom, k1, k2, k3, k4, k5, k6;
  double Ixx, Iyy, Izz, Ixy, Ixz, Iyz;

  if (!FGModel::Run()) {

    Weight = EmptyWeight + Propulsion->GetTanksWeight() + GetPointMassWeight();

    Mass = lbtoslug*Weight;

// Calculate new CG

    vXYZcg = (Propulsion->GetTanksMoment() + EmptyWeight*vbaseXYZcg
                                       + GetPointMassMoment() ) / Weight;

// Calculate new total moments of inertia

    // At first it is the base configuration inertia matrix ...
    mJ = baseJ;
    // ... with the additional term originating from the parallel axis theorem.
    mJ += GetPointmassInertia( lbtoslug * EmptyWeight, vbaseXYZcg );
    // Then add the contributions from the additional pointmasses.
    mJ += CalculatePMInertias();
    mJ += Propulsion->CalculateTankInertias();

    Ixx = mJ(1,1);
    Iyy = mJ(2,2);
    Izz = mJ(3,3);
    Ixy = -mJ(1,2);
    Ixz = -mJ(1,3);
    Iyz = -mJ(2,3);

// Calculate inertia matrix inverse (ref. Stevens and Lewis, "Flight Control & Simulation")

    k1 = (Iyy*Izz - Iyz*Iyz);
    k2 = (Iyz*Ixz + Ixy*Izz);
    k3 = (Ixy*Iyz + Iyy*Ixz);

    denom = 1.0/(Ixx*k1 - Ixy*k2 - Ixz*k3 );
    k1 = k1*denom;
    k2 = k2*denom;
    k3 = k3*denom;
    k4 = (Izz*Ixx - Ixz*Ixz)*denom;
    k5 = (Ixy*Ixz + Iyz*Ixx)*denom;
    k6 = (Ixx*Iyy - Ixy*Ixy)*denom;

    mJinv.InitMatrix( k1, k2, k3,
                      k2, k4, k5,
                      k3, k5, k6 );

    Debug(2);

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::AddPointMass(Element* el)
{
  fix this

//  PointMassLoc.push_back(FGColumnVector3(X, Y, Z));
//  PointMassWeight.push_back(weight);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetPointMassWeight(void)
{
  double PM_total_weight = 0.0;

  for (unsigned int i=0; i<PointMasses.size(); i++) {
    PM_total_weight += PointMasses[i].Weight;
  }
  return PM_total_weight;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGMassBalance::GetPointMassMoment(void)
{
  PointMassCG.InitMatrix();

  for (unsigned int i=0; i<PointMasses.size(); i++) {
    PointMassCG += PointMasses[i].Weight*PointMasses[i].Location;
  }
  return PointMassCG;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMassBalance::CalculatePMInertias(void)
{
  unsigned int size;

  size = PointMasses.size();
  if (size == 0) return pmJ;

  pmJ = FGMatrix33();

  for (unsigned int i=0; i<size; i++)
    pmJ += GetPointmassInertia( lbtoslug * PointMasses[i].Weight, PointMasses[i].Location );

  return pmJ;
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

void FGMassBalance::bind(void)
{
  typedef double (FGMassBalance::*PMF)(int) const;
  PropertyManager->Tie("inertia/mass-slugs", this,
                       &FGMassBalance::GetMass);
  PropertyManager->Tie("inertia/weight-lbs", this,
                       &FGMassBalance::GetWeight);
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
    if (from == 0) cout << "Instantiated: FGMassBalance" << endl;
    if (from == 1) cout << "Destroyed:    FGMassBalance" << endl;
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
