/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMassBalance.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models weight and balance

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) --------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "FGMassBalance.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGMassBalance.cpp,v 1.41 2012/03/25 11:05:37 bcoconni Exp $";
static const char *IdHdr = ID_MASSBALANCE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGMassBalance";
  Weight = EmptyWeight = Mass = 0.0;

  vbaseXYZcg.InitMatrix(0.0);
  vXYZcg.InitMatrix(0.0);
  vLastXYZcg.InitMatrix(0.0);
  vDeltaXYZcg.InitMatrix(0.0);
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
  for (unsigned int i=0; i<PointMasses.size(); i++) delete PointMasses[i];
  PointMasses.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::InitModel(void)
{
  vLastXYZcg.InitMatrix(0.0);
  vDeltaXYZcg.InitMatrix(0.0);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Load(Element* el)
{
  Element *element;
  string element_name = "";
  double bixx, biyy, bizz, bixy, bixz, biyz;

  FGModel::Load(el); // Perform base class Load.

  bixx = biyy = bizz = bixy = bixz = biyz = 0.0;
  if (el->FindElement("ixx"))
    bixx = el->FindElementValueAsNumberConvertTo("ixx", "SLUG*FT2");
  if (el->FindElement("iyy"))
    biyy = el->FindElementValueAsNumberConvertTo("iyy", "SLUG*FT2");
  if (el->FindElement("izz"))
    bizz = el->FindElementValueAsNumberConvertTo("izz", "SLUG*FT2");
  if (el->FindElement("ixy"))
    bixy = el->FindElementValueAsNumberConvertTo("ixy", "SLUG*FT2");
  if (el->FindElement("ixz"))
    bixz = el->FindElementValueAsNumberConvertTo("ixz", "SLUG*FT2");
  if (el->FindElement("iyz"))
    biyz = el->FindElementValueAsNumberConvertTo("iyz", "SLUG*FT2");
  SetAircraftBaseInertias(FGMatrix33(  bixx,  -bixy,  bixz,
                                      -bixy,  biyy,  -biyz,
                                       bixz,  -biyz,  bizz ));
  if (el->FindElement("emptywt")) {
    EmptyWeight = el->FindElementValueAsNumberConvertTo("emptywt", "LBS");
  }

  element = el->FindElement("location");
  while (element) {
    element_name = element->GetAttributeValue("name");
    if (element_name == "CG") vbaseXYZcg = element->FindElementTripletConvertTo("IN");
    element = el->FindNextElement("location");
  }

// Find all POINTMASS elements that descend from this METRICS branch of the
// config file.

  element = el->FindElement("pointmass");
  while (element) {
    AddPointMass(element);
    element = el->FindNextElement("pointmass");
  }

  double ChildFDMWeight = 0.0;
  for (int fdm=0; fdm<FDMExec->GetFDMCount(); fdm++) {
    if (FDMExec->GetChildFDM(fdm)->mated) ChildFDMWeight += FDMExec->GetChildFDM(fdm)->exec->GetMassBalance()->GetWeight();
  }

  Weight = EmptyWeight + in.TanksWeight + GetTotalPointMassWeight()
    + in.GasMass*slugtolb + ChildFDMWeight;

  Mass = lbtoslug*Weight;

  PostLoad(el, PropertyManager);

  Debug(2);
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Run(bool Holding)
{
  double denom, k1, k2, k3, k4, k5, k6;
  double Ixx, Iyy, Izz, Ixy, Ixz, Iyz;

  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  RunPreFunctions();

  double ChildFDMWeight = 0.0;
  for (int fdm=0; fdm<FDMExec->GetFDMCount(); fdm++) {
    if (FDMExec->GetChildFDM(fdm)->mated) ChildFDMWeight += FDMExec->GetChildFDM(fdm)->exec->GetMassBalance()->GetWeight();
  }

  Weight = EmptyWeight + in.TanksWeight + GetTotalPointMassWeight()
    + in.GasMass*slugtolb + ChildFDMWeight;

  Mass = lbtoslug*Weight;

// Calculate new CG

  vXYZcg = (EmptyWeight*vbaseXYZcg
            + GetPointMassMoment()
            + in.TanksMoment
            + in.GasMoment) / Weight;

  // Track frame-by-frame delta CG, and move the EOM-tracked location
  // by this amount.
  if (vLastXYZcg.Magnitude() == 0.0) vLastXYZcg = vXYZcg;
  vDeltaXYZcg = vXYZcg - vLastXYZcg;
  vDeltaXYZcgBody = StructuralToBody(vLastXYZcg) - StructuralToBody(vXYZcg);
  vLastXYZcg = vXYZcg;
  FDMExec->GetPropagate()->NudgeBodyLocation(vDeltaXYZcgBody);

// Calculate new total moments of inertia

  // At first it is the base configuration inertia matrix ...
  mJ = baseJ;
  // ... with the additional term originating from the parallel axis theorem.
  mJ += GetPointmassInertia( lbtoslug * EmptyWeight, vbaseXYZcg );
  // Then add the contributions from the additional pointmasses.
  mJ += CalculatePMInertias();
  mJ += in.TankInertia;
  mJ += in.GasInertia;

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

  RunPostFunctions();

  Debug(0);

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::AddPointMass(Element* el)
{
  double radius=0, length=0;
  Element* loc_element = el->FindElement("location");
  string pointmass_name = el->GetAttributeValue("name");
  if (!loc_element) {
    cerr << "Pointmass " << pointmass_name << " has no location." << endl;
    exit(-1);
  }

  double w = el->FindElementValueAsNumberConvertTo("weight", "LBS");
  FGColumnVector3 vXYZ = loc_element->FindElementTripletConvertTo("IN");

  PointMass *pm = new PointMass(w, vXYZ);
  pm->SetName(pointmass_name);

  Element* form_element = el->FindElement("form");
  if (form_element) {
    string shape = form_element->GetAttributeValue("shape");
    Element* radius_element = form_element->FindElement("radius");
    Element* length_element = form_element->FindElement("length");
    if (radius_element) radius = form_element->FindElementValueAsNumberConvertTo("radius", "FT");
    if (length_element) length = form_element->FindElementValueAsNumberConvertTo("length", "FT");
    if (shape == "tube") {
      pm->SetPointMassShapeType(PointMass::esTube);
      pm->SetRadius(radius);
      pm->SetLength(length);
      pm->CalculateShapeInertia();
    } else if (shape == "cylinder") {
      pm->SetPointMassShapeType(PointMass::esCylinder);
      pm->SetRadius(radius);
      pm->SetLength(length);
      pm->CalculateShapeInertia();
    } else if (shape == "sphere") {
      pm->SetPointMassShapeType(PointMass::esSphere);
      pm->SetRadius(radius);
      pm->CalculateShapeInertia();
    } else if (shape == "ball") {
      pm->SetPointMassShapeType(PointMass::esBall);
      pm->SetRadius(radius);
      pm->CalculateShapeInertia();
    } else {
    }
  }

  pm->bind(PropertyManager, PointMasses.size());
  PointMasses.push_back(pm);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetTotalPointMassWeight(void) const
{
  double PM_total_weight = 0.0;

  for (unsigned int i=0; i<PointMasses.size(); i++) {
    PM_total_weight += PointMasses[i]->Weight;
  }
  return PM_total_weight;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGMassBalance::GetPointMassMoment(void)
{
  PointMassCG.InitMatrix();

  for (unsigned int i=0; i<PointMasses.size(); i++) {
    PointMassCG += PointMasses[i]->Weight*PointMasses[i]->Location;
  }
  return PointMassCG;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGMassBalance::CalculatePMInertias(void)
{
  unsigned int size;

  size = PointMasses.size();
  if (size == 0) return pmJ;

  pmJ = FGMatrix33();

  for (unsigned int i=0; i<size; i++) {
    pmJ += GetPointmassInertia( lbtoslug * PointMasses[i]->Weight, PointMasses[i]->Location );
    pmJ += PointMasses[i]->GetPointMassInertia();
  }

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
  PropertyManager->Tie("inertia/empty-weight-lbs", this,
                       &FGMassBalance::GetEmptyWeight);
  PropertyManager->Tie("inertia/cg-x-in", this,1,
                       (PMF)&FGMassBalance::GetXYZcg);
  PropertyManager->Tie("inertia/cg-y-in", this,2,
                       (PMF)&FGMassBalance::GetXYZcg);
  PropertyManager->Tie("inertia/cg-z-in", this,3,
                       (PMF)&FGMassBalance::GetXYZcg);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// This function binds properties for each pointmass object created.
//
void FGMassBalance::PointMass::bind(FGPropertyManager* PropertyManager, int num) {
  string tmp = CreateIndexedPropertyName("inertia/pointmass-weight-lbs", num);
  PropertyManager->Tie( tmp.c_str(), this, &PointMass::GetPointMassWeight,
                                       &PointMass::SetPointMassWeight);

  tmp = CreateIndexedPropertyName("inertia/pointmass-location-X-inches", num);
  PropertyManager->Tie( tmp.c_str(), this, eX, &PointMass::GetPointMassLocation,
                                           &PointMass::SetPointMassLocation);
  tmp = CreateIndexedPropertyName("inertia/pointmass-location-Y-inches", num);
  PropertyManager->Tie( tmp.c_str(), this, eY, &PointMass::GetPointMassLocation,
                                           &PointMass::SetPointMassLocation);
  tmp = CreateIndexedPropertyName("inertia/pointmass-location-Z-inches", num);
  PropertyManager->Tie( tmp.c_str(), this, eZ, &PointMass::GetPointMassLocation,
                                           &PointMass::SetPointMassLocation);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::GetMassPropertiesReport(void) const
{
  cout << endl << fgblue << highint 
       << "  Mass Properties Report (English units: lbf, in, slug-ft^2)"
       << reset << endl;
  cout << "                                  " << underon << "    Weight    CG-X    CG-Y"
       << "    CG-Z         Ixx         Iyy         Izz" << underoff << endl;
  cout.precision(1);
  cout << highint << setw(34) << left << "    Base Vehicle " << normint
       << right << setw(10) << EmptyWeight << setw(8) << vbaseXYZcg(eX) << setw(8)
       << vbaseXYZcg(eY) << setw(8) << vbaseXYZcg(eZ) << setw(12) << baseJ(1,1)
       << setw(12) << baseJ(2,2) << setw(12) << baseJ(3,3) << endl;

  for (unsigned int i=0;i<PointMasses.size();i++) {
    PointMass* pm = PointMasses[i];
    double pmweight = pm->GetPointMassWeight();
    cout << highint << left << setw(4) << i << setw(30) << pm->GetName() << normint
         << right << setw(10) << pmweight << setw(8) << pm->GetLocation()(eX)
         << setw(8) << pm->GetLocation()(eY) << setw(8) << pm->GetLocation()(eZ)
         << setw(12) << pm->GetPointMassMoI(1,1) << setw(12) << pm->GetPointMassMoI(2,2)
         << setw(12) << pm->GetPointMassMoI(3,3) << endl;
  }

  cout << FDMExec->GetPropulsionTankReport();

  cout << underon << setw(104) << " " << underoff << endl;
  cout << highint << left << setw(30) << "    Total: " << right << setw(14) << Weight 
       << setw(8) << vXYZcg(eX)
       << setw(8) << vXYZcg(eY)
       << setw(8) << vXYZcg(eZ)
       << setw(12) << mJ(1,1)
       << setw(12) << mJ(2,2)
       << setw(12) << mJ(3,3)
       << normint << endl;

  cout.setf(ios_base::fixed);
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
    if (from == 2) { // Loading
      cout << endl << "  Mass and Balance:" << endl;
      cout << "    baseIxx: " << baseJ(1,1) << " slug-ft2" << endl;
      cout << "    baseIyy: " << baseJ(2,2) << " slug-ft2" << endl;
      cout << "    baseIzz: " << baseJ(3,3) << " slug-ft2" << endl;
      cout << "    baseIxy: " << baseJ(1,2) << " slug-ft2" << endl;
      cout << "    baseIxz: " << baseJ(1,3) << " slug-ft2" << endl;
      cout << "    baseIyz: " << baseJ(2,3) << " slug-ft2" << endl;
      cout << "    Empty Weight: " << EmptyWeight << " lbm" << endl;
      cout << "    CG (x, y, z): " << vbaseXYZcg << endl;
      // ToDo: Need to add point mass outputs here
      for (unsigned int i=0; i<PointMasses.size(); i++) {
        cout << "    Point Mass Object: " << PointMasses[i]->Weight << " lbs. at "
                   << "X, Y, Z (in.): " << PointMasses[i]->Location(eX) << "  "
                   << PointMasses[i]->Location(eY) << "  "
                   << PointMasses[i]->Location(eZ) << endl;
      }
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
