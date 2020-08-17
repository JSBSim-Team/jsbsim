/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMassBalance.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models weight and balance

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) --------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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

#include <iomanip>

#include "FGMassBalance.h"
#include "FGFDMExec.h"
#include "FGGroundReactions.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex)
  : FGModel(fdmex), GroundReactions(nullptr)
{
  Name = "FGMassBalance";
  Weight = EmptyWeight = Mass = 0.0;

  vbaseXYZcg.InitMatrix();
  vXYZcg.InitMatrix();
  vLastXYZcg.InitMatrix();
  vDeltaXYZcg.InitMatrix();
  baseJ.InitMatrix();
  mJ.InitMatrix();
  mJinv.InitMatrix();
  pmJ.InitMatrix();
  Propagate = fdmex->GetPropagate();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMassBalance::~FGMassBalance()
{
  for(auto pm: PointMasses) delete pm;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  GroundReactions = FDMExec->GetGroundReactions();
  vLastXYZcg.InitMatrix();
  vDeltaXYZcg.InitMatrix();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

static FGMatrix33 ReadInertiaMatrix(Element* document)
{
  double bixx, biyy, bizz, bixy, bixz, biyz;

  bixx = biyy = bizz = bixy = bixz = biyz = 0.0;
  if (document->FindElement("ixx"))
    bixx = document->FindElementValueAsNumberConvertTo("ixx", "SLUG*FT2");
  if (document->FindElement("iyy"))
    biyy = document->FindElementValueAsNumberConvertTo("iyy", "SLUG*FT2");
  if (document->FindElement("izz"))
    bizz = document->FindElementValueAsNumberConvertTo("izz", "SLUG*FT2");
  if (document->FindElement("ixy"))
    bixy = document->FindElementValueAsNumberConvertTo("ixy", "SLUG*FT2");
  if (document->FindElement("ixz"))
    bixz = document->FindElementValueAsNumberConvertTo("ixz", "SLUG*FT2");
  if (document->FindElement("iyz"))
    biyz = document->FindElementValueAsNumberConvertTo("iyz", "SLUG*FT2");

  // Transform the inertia products from the structural frame to the body frame
  // and create the inertia matrix.
  return FGMatrix33( bixx, -bixy,  bixz,
                    -bixy,  biyy, -biyz,
                     bixz, -biyz,  bizz );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Load(Element* document)
{
  string element_name = "";

  Name = "Mass Properties Model: " + document->GetAttributeValue("name");

  // Perform base class Pre-Load
  if (!FGModel::Upload(document, true))
    return false;

  SetAircraftBaseInertias(ReadInertiaMatrix(document));
  if (document->FindElement("emptywt")) {
    EmptyWeight = document->FindElementValueAsNumberConvertTo("emptywt", "LBS");
  }

  Element *element = document->FindElement("location");
  while (element) {
    element_name = element->GetAttributeValue("name");
    if (element_name == "CG") vbaseXYZcg = element->FindElementTripletConvertTo("IN");
    element = document->FindNextElement("location");
  }

// Find all POINTMASS elements that descend from this METRICS branch of the
// config file.

  element = document->FindElement("pointmass");
  while (element) {
    AddPointMass(element);
    element = document->FindNextElement("pointmass");
  }

  double ChildFDMWeight = 0.0;
  for (int fdm=0; fdm<FDMExec->GetFDMCount(); fdm++) {
    if (FDMExec->GetChildFDM(fdm)->mated) ChildFDMWeight += FDMExec->GetChildFDM(fdm)->exec->GetMassBalance()->GetWeight();
  }

  Weight = EmptyWeight + in.TanksWeight + GetTotalPointMassWeight()
    + in.GasMass*slugtolb + ChildFDMWeight;

  Mass = lbtoslug*Weight;

  PostLoad(document, FDMExec);

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

  // Compensate displacements of the structural frame when the mass distribution
  // is modified while the aircraft is in contact with the ground.
  if (FDMExec->GetHoldDown() || GroundReactions->GetWOW())
    Propagate->NudgeBodyLocation(vDeltaXYZcgBody);

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

// Calculate inertia matrix inverse (ref. Stevens and Lewis, "Flight Control &
// Simulation")

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

  mJinv = { k1, k2, k3,
            k2, k4, k5,
            k3, k5, k6 };

  RunPostFunctions();

  Debug(0);

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::AddPointMass(Element* el)
{
  Element* loc_element = el->FindElement("location");
  string pointmass_name = el->GetAttributeValue("name");
  if (!loc_element) {
    cerr << el->ReadFrom() << "Pointmass " << pointmass_name
         << " has no location." << endl;
    exit(-1);
  }

  double w = el->FindElementValueAsNumberConvertTo("weight", "LBS");
  FGColumnVector3 vXYZ = loc_element->FindElementTripletConvertTo("IN");

  PointMass *pm = new PointMass(w, vXYZ);
  pm->SetName(pointmass_name);

  Element* form_element = el->FindElement("form");
  if (form_element) {
    double radius=0, length=0;
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
  else {
    pm->SetPointMassShapeType(PointMass::esUnspecified);
    pm->SetPointMassMoI(ReadInertiaMatrix(el));
  }

  pm->bind(PropertyManager, PointMasses.size());
  PointMasses.push_back(pm);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMassBalance::GetTotalPointMassWeight(void) const
{
  double PM_total_weight = 0.0;

  for(auto pm: PointMasses)
    PM_total_weight += pm->Weight;

  return PM_total_weight;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGMassBalance::GetPointMassMoment(void)
{
  PointMassCG.InitMatrix();

  for (auto pm: PointMasses)
    PointMassCG += pm->Weight * pm->Location;

  return PointMassCG;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGMassBalance::CalculatePMInertias(void)
{
  if (PointMasses.empty()) return pmJ;

  pmJ.InitMatrix();

  for (auto pm: PointMasses) {
    pmJ += GetPointmassInertia( lbtoslug * pm->Weight, pm->Location );
    pmJ += pm->GetPointMassInertia();
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
  PropertyManager->Tie("inertia/ixx-slugs_ft2", this,
                       &FGMassBalance::GetIxx);
  PropertyManager->Tie("inertia/iyy-slugs_ft2", this,
                       &FGMassBalance::GetIyy);
  PropertyManager->Tie("inertia/izz-slugs_ft2", this,
                       &FGMassBalance::GetIzz);
  PropertyManager->Tie("inertia/ixy-slugs_ft2", this,
                       &FGMassBalance::GetIxy);
  PropertyManager->Tie("inertia/ixz-slugs_ft2", this,
                       &FGMassBalance::GetIxz);
  PropertyManager->Tie("inertia/iyz-slugs_ft2", this,
                       &FGMassBalance::GetIyz);
  typedef int (FGMassBalance::*iOPV)() const;
  PropertyManager->Tie("inertia/print-mass-properties", this, (iOPV)0,
                       &FGMassBalance::GetMassPropertiesReport, false);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// This function binds properties for each pointmass object created.
//
void FGMassBalance::PointMass::bind(FGPropertyManager* PropertyManager,
                                    unsigned int num) {
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

void FGMassBalance::GetMassPropertiesReport(int i)
{
  cout << endl << fgblue << highint 
       << "  Mass Properties Report (English units: lbf, in, slug-ft^2)"
       << reset << endl;
  cout << "                                  " << underon << "    Weight    CG-X    CG-Y"
       << "    CG-Z         Ixx         Iyy         Izz" 
       << "         Ixy         Ixz         Iyz" << underoff << endl;
  cout.precision(1);
  cout << highint << setw(34) << left << "    Base Vehicle " << normint
       << right << setw(10) << EmptyWeight
       << setw(8) << vbaseXYZcg(eX) << setw(8) << vbaseXYZcg(eY) << setw(8) << vbaseXYZcg(eZ)
       << setw(12) << baseJ(1,1) << setw(12) << baseJ(2,2) << setw(12) << baseJ(3,3)
       << setw(12) << baseJ(1,2) << setw(12) << baseJ(1,3) << setw(12) << baseJ(2,3) << endl;

  for (unsigned int i=0;i<PointMasses.size();i++) {
    PointMass* pm = PointMasses[i];
    double pmweight = pm->GetPointMassWeight();
    cout << highint << left << setw(4) << i << setw(30) << pm->GetName() << normint
         << right << setw(10) << pmweight << setw(8) << pm->GetLocation()(eX)
         << setw(8) << pm->GetLocation()(eY) << setw(8) << pm->GetLocation()(eZ)
         << setw(12) << pm->GetPointMassMoI(1,1) << setw(12) << pm->GetPointMassMoI(2,2) << setw(12) << pm->GetPointMassMoI(3,3)
         << setw(12) << pm->GetPointMassMoI(1,2) << setw(12) << pm->GetPointMassMoI(1,3) << setw(12) << pm->GetPointMassMoI(2,3) << endl;         
  }

  cout << FDMExec->GetPropulsionTankReport();

  cout << "    " << underon << setw(136) << " " << underoff << endl;
  cout << highint << left << setw(30) << "    Total: " << right << setw(14) << Weight 
       << setw(8) << vXYZcg(eX)
       << setw(8) << vXYZcg(eY)
       << setw(8) << vXYZcg(eZ)
       << setw(12) << mJ(1,1)
       << setw(12) << mJ(2,2)
       << setw(12) << mJ(3,3)
       << setw(12) << mJ(1,2)
       << setw(12) << mJ(1,3)
       << setw(12) << mJ(2,3)
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
    }
  }
}
}
