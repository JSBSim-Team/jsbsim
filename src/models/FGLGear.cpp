/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLGear.cpp
 Author:       Jon S. Berndt
               Norman H. Princen
               Bertrand Coconnier
 Date started: 11/18/99
 Purpose:      Encapsulates the landing gear elements
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created
01/30/01   NHP   Extended gear model to properly simulate steering and braking
07/08/09   BC    Modified gear model to support large angles between aircraft and ground

/%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGLGear.h"
#include "FGGroundReactions.h"
#include "FGFCS.h"
#include "FGAuxiliary.h"
#include "FGAtmosphere.h"
#include "FGMassBalance.h"
#include "math/FGTable.h"
#include <cstdlib>
#include <cstring>

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: FGLGear.cpp,v 1.77 2010/09/24 19:55:33 andgi Exp $";
static const char *IdHdr = ID_LGEAR;

// Body To Structural (body frame is rotated 180 deg about Y and lengths are given in
// ft instead of inches)
const FGMatrix33 FGLGear::Tb2s(-1./inchtoft, 0., 0., 0., 1./inchtoft, 0., 0., 0., -1./inchtoft);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLGear::FGLGear(Element* el, FGFDMExec* fdmex, int number) :
  FGForce(fdmex),
  GearNumber(number),
  SteerAngle(0.0),
  Castered(false),
  StaticFriction(false)
{
  Element *force_table=0;
  Element *dampCoeff=0;
  Element *dampCoeffRebound=0;
  string force_type="";

  kSpring = bDamp = bDampRebound = dynamicFCoeff = staticFCoeff = rollingFCoeff = maxSteerAngle = 0;
  sSteerType = sBrakeGroup = sSteerType = "";
  isRetractable = 0;
  eDampType = dtLinear;
  eDampTypeRebound = dtLinear;

  name = el->GetAttributeValue("name");
  sContactType = el->GetAttributeValue("type");
  if (sContactType == "BOGEY") {
    eContactType = ctBOGEY;
  } else if (sContactType == "STRUCTURE") {
    eContactType = ctSTRUCTURE;
  } else {
    // Unknown contact point types will be treated as STRUCTURE.
    eContactType = ctSTRUCTURE;
  }

  if (el->FindElement("spring_coeff"))
    kSpring = el->FindElementValueAsNumberConvertTo("spring_coeff", "LBS/FT");
  if (el->FindElement("damping_coeff")) {
    dampCoeff = el->FindElement("damping_coeff");
    if (dampCoeff->GetAttributeValue("type") == "SQUARE") {
      eDampType = dtSquare;
      bDamp   = el->FindElementValueAsNumberConvertTo("damping_coeff", "LBS/FT2/SEC2");
    } else {
      bDamp   = el->FindElementValueAsNumberConvertTo("damping_coeff", "LBS/FT/SEC");
    }
  }

  if (el->FindElement("damping_coeff_rebound")) {
    dampCoeffRebound = el->FindElement("damping_coeff_rebound");
    if (dampCoeffRebound->GetAttributeValue("type") == "SQUARE") {
      eDampTypeRebound = dtSquare;
      bDampRebound   = el->FindElementValueAsNumberConvertTo("damping_coeff_rebound", "LBS/FT2/SEC2");
    } else {
      bDampRebound   = el->FindElementValueAsNumberConvertTo("damping_coeff_rebound", "LBS/FT/SEC");
    }
  } else {
    bDampRebound   = bDamp;
    eDampTypeRebound = eDampType;
  }

  if (el->FindElement("dynamic_friction"))
    dynamicFCoeff = el->FindElementValueAsNumber("dynamic_friction");
  if (el->FindElement("static_friction"))
    staticFCoeff = el->FindElementValueAsNumber("static_friction");
  if (el->FindElement("rolling_friction"))
    rollingFCoeff = el->FindElementValueAsNumber("rolling_friction");
  if (el->FindElement("max_steer"))
    maxSteerAngle = el->FindElementValueAsNumberConvertTo("max_steer", "DEG");
  if (el->FindElement("retractable"))
    isRetractable = ((unsigned int)el->FindElementValueAsNumber("retractable"))>0.0?true:false;

  ForceY_Table = 0;
  force_table = el->FindElement("table");
  while (force_table) {
    force_type = force_table->GetAttributeValue("type");
    if (force_type == "CORNERING_COEFF") {
      ForceY_Table = new FGTable(fdmex->GetPropertyManager(), force_table);
    } else {
      cerr << "Undefined force table for " << name << " contact point" << endl;
    }
    force_table = el->FindNextElement("table");
  }

  sBrakeGroup = el->FindElementValue("brake_group");

  if (maxSteerAngle == 360) sSteerType = "CASTERED";
  else if (maxSteerAngle == 0.0) sSteerType = "FIXED";
  else sSteerType = "STEERABLE";

  Element* element = el->FindElement("location");
  if (element) vXYZn = element->FindElementTripletConvertTo("IN");
  else {cerr << "No location given for contact " << name << endl; exit(-1);}
  SetTransformType(FGForce::tCustom);

  element = el->FindElement("orientation");
  if (element && (eContactType == ctBOGEY)) {
    vGearOrient = element->FindElementTripletConvertTo("RAD");

    double cp,sp,cr,sr,cy,sy;

    cp=cos(vGearOrient(ePitch)); sp=sin(vGearOrient(ePitch));
    cr=cos(vGearOrient(eRoll));  sr=sin(vGearOrient(eRoll));
    cy=cos(vGearOrient(eYaw));   sy=sin(vGearOrient(eYaw));

    mTGear(1,1) =  cp*cy;
    mTGear(2,1) =  cp*sy;
    mTGear(3,1) = -sp;

    mTGear(1,2) = sr*sp*cy - cr*sy;
    mTGear(2,2) = sr*sp*sy + cr*cy;
    mTGear(3,2) = sr*cp;

    mTGear(1,3) = cr*sp*cy + sr*sy;
    mTGear(2,3) = cr*sp*sy - sr*cy;
    mTGear(3,3) = cr*cp;
  }
  else {
    mTGear(1,1) = 1.;
    mTGear(2,2) = 1.;
    mTGear(3,3) = 1.;
  }

  if      (sBrakeGroup == "LEFT"  ) eBrakeGrp = bgLeft;
  else if (sBrakeGroup == "RIGHT" ) eBrakeGrp = bgRight;
  else if (sBrakeGroup == "CENTER") eBrakeGrp = bgCenter;
  else if (sBrakeGroup == "NOSE"  ) eBrakeGrp = bgNose;
  else if (sBrakeGroup == "TAIL"  ) eBrakeGrp = bgTail;
  else if (sBrakeGroup == "NONE"  ) eBrakeGrp = bgNone;
  else if (sBrakeGroup.empty()    ) {eBrakeGrp = bgNone;
                                     sBrakeGroup = "NONE (defaulted)";}
  else {
    cerr << "Improper braking group specification in config file: "
         << sBrakeGroup << " is undefined." << endl;
  }

  if      (sSteerType == "STEERABLE") eSteerType = stSteer;
  else if (sSteerType == "FIXED"    ) eSteerType = stFixed;
  else if (sSteerType == "CASTERED" ) {eSteerType = stCaster; Castered = true;}
  else if (sSteerType.empty()       ) {eSteerType = stFixed;
                                       sSteerType = "FIXED (defaulted)";}
  else {
    cerr << "Improper steering type specification in config file: "
         << sSteerType << " is undefined." << endl;
  }

  Auxiliary       = fdmex->GetAuxiliary();
  Propagate       = fdmex->GetPropagate();
  FCS             = fdmex->GetFCS();
  MassBalance     = fdmex->GetMassBalance();
  GroundReactions = fdmex->GetGroundReactions();

  GearUp = false;
  GearDown = true;
  GearPos  = 1.0;
  useFCSGearPos = false;
  Servicable = true;

// Add some AI here to determine if gear is located properly according to its
// brake group type ??

  WOW = lastWOW = false;
  ReportEnable = true;
  FirstContact = false;
  StartedGroundRun = false;
  TakeoffReported = LandingReported = false;
  LandingDistanceTraveled = TakeoffDistanceTraveled = TakeoffDistanceTraveled50ft = 0.0;
  MaximumStrutForce = MaximumStrutTravel = 0.0;
  SinkRate = GroundSpeed = 0.0;

  vWhlBodyVec = MassBalance->StructuralToBody(vXYZn);
  vLocalGear = Propagate->GetTb2l() * vWhlBodyVec;
  vWhlVelVec.InitMatrix();

  compressLength  = 0.0;
  compressSpeed   = 0.0;
  brakePct        = 0.0;
  maxCompLen      = 0.0;

  WheelSlip = 0.0;
  TirePressureNorm = 1.0;

  // Set Pacejka terms

  Stiffness = 0.06;
  Shape = 2.8;
  Peak = staticFCoeff;
  Curvature = 1.03;

  // Initialize Lagrange multipliers
  memset(LMultiplier, 0, sizeof(LMultiplier));

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::~FGLGear()
{
  delete ForceY_Table;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGLGear::GetBodyForces(void)
{
  double t = fdmex->GetSimTime();
  dT = fdmex->GetDeltaT()*GroundReactions->GetRate();

  vFn.InitMatrix();

  if (isRetractable) ComputeRetractionState();

  if (GearDown) {
    vWhlBodyVec = MassBalance->StructuralToBody(vXYZn); // Get wheel in body frame
    vLocalGear = Propagate->GetTb2l() * vWhlBodyVec; // Get local frame wheel location

    gearLoc = Propagate->GetLocation().LocalToLocation(vLocalGear);
    // Compute the height of the theoretical location of the wheel (if strut is
    // not compressed) with respect to the ground level
    double height = fdmex->GetGroundCallback()->GetAGLevel(t, gearLoc, contact, normal, cvel);
    vGroundNormal = Propagate->GetTec2b() * normal;

    // The height returned above is the AGL and is expressed in the Z direction
    // of the ECEF coordinate frame. We now need to transform this height in
    // actual compression of the strut (BOGEY) of in the normal direction to the
    // ground (STRUCTURE)
    double normalZ = (Propagate->GetTec2l()*normal)(eZ);
    double LGearProj = -(mTGear.Transposed() * vGroundNormal)(eZ);

    switch (eContactType) {
    case ctBOGEY:
      compressLength = LGearProj > 0.0 ? height * normalZ / LGearProj : 0.0;
      break;
    case ctSTRUCTURE:
      compressLength = height * normalZ / DotProduct(normal, normal);
      break;
    }

    if (compressLength > 0.00) {

      WOW = true;

      // The following equations use the vector to the tire contact patch
      // including the strut compression.
      FGColumnVector3 vWhlDisplVec;

      switch(eContactType) {
      case ctBOGEY:
        vWhlDisplVec = mTGear * FGColumnVector3(0., 0., -compressLength);
        break;
      case ctSTRUCTURE:
        vWhlDisplVec = compressLength * vGroundNormal;
        break;
      }

      FGColumnVector3 vWhlContactVec = vWhlBodyVec + vWhlDisplVec;
      vActingXYZn = vXYZn + Tb2s * vWhlDisplVec;
      FGColumnVector3 vBodyWhlVel = Propagate->GetPQR() * vWhlContactVec;
      vBodyWhlVel += Propagate->GetUVW() - Propagate->GetTec2b() * cvel;

      vWhlVelVec = mTGear.Transposed() * vBodyWhlVel;

      InitializeReporting();
      ComputeSteeringAngle();
      ComputeGroundCoordSys();

      vLocalWhlVel = Transform().Transposed() * vBodyWhlVel;

      compressSpeed = -vLocalWhlVel(eX);
      if (eContactType == ctBOGEY)
        compressSpeed /= LGearProj;

      ComputeVerticalStrutForce();

      // Compute the friction coefficients in the wheel ground plane.
      if (eContactType == ctBOGEY) {
        ComputeSlipAngle();
        ComputeBrakeForceCoefficient();
        ComputeSideForceCoefficient();
      }

      // Prepare the Jacobians and the Lagrange multipliers for later friction
      // forces calculations.
      ComputeJacobian(vWhlContactVec);

    } else { // Gear is NOT compressed

      WOW = false;
      compressLength = 0.0;
      compressSpeed = 0.0;
      WheelSlip = 0.0;
      StrutForce = 0.0;

      // Let wheel spin down slowly
      vWhlVelVec(eX) -= 13.0*dT;
      if (vWhlVelVec(eX) < 0.0) vWhlVelVec(eX) = 0.0;

      // Return to neutral position between 1.0 and 0.8 gear pos.
      SteerAngle *= max(GetGearUnitPos()-0.8, 0.0)/0.2;

      ResetReporting();
    }
  }

  ReportTakeoffOrLanding();

  // Require both WOW and LastWOW to be true before checking crash conditions
  // to allow the WOW flag to be used in terminating a scripted run.
  if (WOW && lastWOW) CrashDetect();

  lastWOW = WOW;

  return FGForce::GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Build a local "ground" coordinate system defined by
//  eX : normal to the ground
//  eY : projection of the rolling direction on the ground
//  eZ : projection of the sliping direction on the ground

void FGLGear::ComputeGroundCoordSys(void)
{
  // Euler angles are built up to create a local frame to describe the forces
  // applied to the gear by the ground. Here pitch, yaw and roll do not have
  // any physical meaning. It is just a convenient notation.
  // First, "pitch" and "yaw" are determined in order to align eX with the
  // ground normal.
  if (vGroundNormal(eZ) < -1.0)
    vOrient(ePitch) = 0.5*M_PI;
  else if (1.0 < vGroundNormal(eZ))
    vOrient(ePitch) = -0.5*M_PI;
  else
    vOrient(ePitch) = asin(-vGroundNormal(eZ));

  if (fabs(vOrient(ePitch)) == 0.5*M_PI)
    vOrient(eYaw) = 0.;
  else
    vOrient(eYaw) = atan2(vGroundNormal(eY), vGroundNormal(eX));
  
  vOrient(eRoll) = 0.;
  UpdateCustomTransformMatrix();

  if (eContactType == ctBOGEY) {
    // In the case of a bogey, the third angle "roll" is used to align the axis eY and eZ
    // to the rolling and sliping direction respectively.
    FGColumnVector3 updatedRollingAxis = Transform().Transposed() * mTGear
                                       * FGColumnVector3(-sin(SteerAngle), cos(SteerAngle), 0.);

    vOrient(eRoll) = atan2(updatedRollingAxis(eY), -updatedRollingAxis(eZ));
    UpdateCustomTransformMatrix();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::ComputeRetractionState(void)
{
  double gearPos = GetGearUnitPos();
  if (gearPos < 0.01) {
    GearUp   = true;
    WOW      = false;
    GearDown = false;
    vWhlVelVec.InitMatrix();
  } else if (gearPos > 0.99) {
    GearDown = true;
    GearUp   = false;
  } else {
    GearUp   = false;
    GearDown = false;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Calculate tire slip angle.

void FGLGear::ComputeSlipAngle(void)
{
// Check that the speed is non-null otherwise use the current angle
  if (vLocalWhlVel.Magnitude(eY,eZ) > 1E-3)
    WheelSlip = -atan2(vLocalWhlVel(eZ), fabs(vLocalWhlVel(eY)))*radtodeg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the steering angle in any case.
// This will also make sure that animations will look right.

void FGLGear::ComputeSteeringAngle(void)
{
  switch (eSteerType) {
  case stSteer:
    SteerAngle = degtorad * FCS->GetSteerPosDeg(GearNumber);
    break;
  case stFixed:
    SteerAngle = 0.0;
    break;
  case stCaster:
    if (!Castered)
      SteerAngle = degtorad * FCS->GetSteerPosDeg(GearNumber);
    else {
      // Check that the speed is non-null otherwise use the current angle
      if (vWhlVelVec.Magnitude(eX,eY) > 0.1)
        SteerAngle = atan2(vWhlVelVec(eY), fabs(vWhlVelVec(eX)));
    }
    break;
  default:
    cerr << "Improper steering type membership detected for this gear." << endl;
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Reset reporting functionality after takeoff

void FGLGear::ResetReporting(void)
{
  if (Propagate->GetDistanceAGL() > 200.0) {
    FirstContact = false;
    StartedGroundRun = false;
    LandingReported = false;
    TakeoffReported = true;
    LandingDistanceTraveled = 0.0;
    MaximumStrutForce = MaximumStrutTravel = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::InitializeReporting(void)
{
  // If this is the first time the wheel has made contact, remember some values
  // for later printout.

  if (!FirstContact) {
    FirstContact  = true;
    SinkRate      =  compressSpeed;
    GroundSpeed   =  Propagate->GetVel().Magnitude();
    TakeoffReported = false;
  }

  // If the takeoff run is starting, initialize.

  if ((Propagate->GetVel().Magnitude() > 0.1) &&
      (FCS->GetBrake(bgLeft) == 0) &&
      (FCS->GetBrake(bgRight) == 0) &&
      (FCS->GetThrottlePos(0) > 0.90) && !StartedGroundRun)
  {
    TakeoffDistanceTraveled = 0;
    TakeoffDistanceTraveled50ft = 0;
    StartedGroundRun = true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Takeoff and landing reporting functionality

void FGLGear::ReportTakeoffOrLanding(void)
{
  if (FirstContact)
    LandingDistanceTraveled += Auxiliary->GetVground()*dT;

  if (StartedGroundRun) {
    TakeoffDistanceTraveled50ft += Auxiliary->GetVground()*dT;
    if (WOW) TakeoffDistanceTraveled += Auxiliary->GetVground()*dT;
  }

  if ( ReportEnable
       && Auxiliary->GetVground() <= 0.05
       && !LandingReported
       && GroundReactions->GetWOW())
  {
    if (debug_lvl > 0) Report(erLand);
  }

  if ( ReportEnable
       && !TakeoffReported
       && (Propagate->GetDistanceAGL() - vLocalGear(eZ)) > 50.0
       && !GroundReactions->GetWOW())
  {
    if (debug_lvl > 0) Report(erTakeoff);
  }

  if (lastWOW != WOW) PutMessage("GEAR_CONTACT: " + name, WOW);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Crash detection logic (really out-of-bounds detection)

void FGLGear::CrashDetect(void)
{
  if ( (compressLength > 500.0 ||
      vFn.Magnitude() > 100000000.0 ||
      GetMoments().Magnitude() > 5000000000.0 ||
      SinkRate > 1.4666*30 ) && !fdmex->IntegrationSuspended())
  {
    PutMessage("Crash Detected: Simulation FREEZE.");
    fdmex->SuspendIntegration();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The following needs work regarding friction coefficients and braking and
// steering The BrakeFCoeff formula assumes that an anti-skid system is used.
// It also assumes that we won't be turning and braking at the same time.
// Will fix this later.
// [JSB] The braking force coefficients include normal rolling coefficient +
// a percentage of the static friction coefficient based on braking applied.

void FGLGear::ComputeBrakeForceCoefficient(void)
{
  switch (eBrakeGrp) {
  case bgLeft:
    BrakeFCoeff =  ( rollingFCoeff*(1.0 - FCS->GetBrake(bgLeft)) +
                     staticFCoeff*FCS->GetBrake(bgLeft) );
    break;
  case bgRight:
    BrakeFCoeff =  ( rollingFCoeff*(1.0 - FCS->GetBrake(bgRight)) +
                     staticFCoeff*FCS->GetBrake(bgRight) );
    break;
  case bgCenter:
    BrakeFCoeff =  ( rollingFCoeff*(1.0 - FCS->GetBrake(bgCenter)) +
                     staticFCoeff*FCS->GetBrake(bgCenter) );
    break;
  case bgNose:
    BrakeFCoeff =  ( rollingFCoeff*(1.0 - FCS->GetBrake(bgCenter)) +
                     staticFCoeff*FCS->GetBrake(bgCenter) );
    break;
  case bgTail:
    BrakeFCoeff =  ( rollingFCoeff*(1.0 - FCS->GetBrake(bgCenter)) +
                     staticFCoeff*FCS->GetBrake(bgCenter) );
    break;
  case bgNone:
    BrakeFCoeff =  rollingFCoeff;
    break;
  default:
    cerr << "Improper brake group membership detected for this gear." << endl;
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the sideforce coefficients using Pacejka's Magic Formula.
//
//   y(x) = D sin {C arctan [Bx - E(Bx - arctan Bx)]}
//
// Where: B = Stiffness Factor (0.06, here)
//        C = Shape Factor (2.8, here)
//        D = Peak Factor (0.8, here)
//        E = Curvature Factor (1.03, here)

void FGLGear::ComputeSideForceCoefficient(void)
{
  if (ForceY_Table) {
    FCoeff = ForceY_Table->GetValue(WheelSlip);
  } else {
    double StiffSlip = Stiffness*WheelSlip;
    FCoeff = Peak * sin(Shape*atan(StiffSlip - Curvature*(StiffSlip - atan(StiffSlip))));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the vertical force on the wheel using square-law damping (per comment
// in paper AIAA-2000-4303 - see header prologue comments). We might consider
// allowing for both square and linear damping force calculation. Also need to
// possibly give a "rebound damping factor" that differs from the compression
// case.

void FGLGear::ComputeVerticalStrutForce(void)
{
  double springForce = 0;
  double dampForce = 0;

  springForce = -compressLength * kSpring;

  if (compressSpeed >= 0.0) {

    if (eDampType == dtLinear)   dampForce = -compressSpeed * bDamp;
    else         dampForce = -compressSpeed * compressSpeed * bDamp;

  } else {

    if (eDampTypeRebound == dtLinear)
      dampForce   = -compressSpeed * bDampRebound;
    else
      dampForce   =  compressSpeed * compressSpeed * bDampRebound;

  }

  StrutForce = min(springForce + dampForce, (double)0.0);

  // The reaction force of the wheel is always normal to the ground
  switch (eContactType) {
  case ctBOGEY:
    // Project back the strut force in the local coordinate frame of the ground
    vFn(eX) = StrutForce / (mTGear.Transposed()*vGroundNormal)(eZ);
    break;
  case ctSTRUCTURE:
    vFn(eX) = -StrutForce;
    break;
  }

  // Remember these values for reporting
  MaximumStrutForce = max(MaximumStrutForce, fabs(StrutForce));
  MaximumStrutTravel = max(MaximumStrutTravel, fabs(compressLength));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGLGear::GetGearUnitPos(void)
{
  // hack to provide backward compatibility to gear/gear-pos-norm property
  if( useFCSGearPos || FCS->GetGearPos() != 1.0 ) {
    useFCSGearPos = true;
    return FCS->GetGearPos();
  }
  return GearPos;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the jacobian entries for the friction forces resolution later
// in FGPropagate

void FGLGear::ComputeJacobian(const FGColumnVector3& vWhlContactVec)
{
  // When the point of contact is moving, dynamic friction is used
  // This type of friction is limited to ctSTRUCTURE elements because their
  // friction coefficient is the same in every directions
  if ((eContactType == ctSTRUCTURE) && (vLocalWhlVel.Magnitude(eY,eZ) > 1E-3)) {
    FGColumnVector3 velocityDirection = vLocalWhlVel;

    StaticFriction = false;

    velocityDirection(eX) = 0.;
    velocityDirection.Normalize();

    LMultiplier[ftDynamic].ForceJacobian = Transform()*velocityDirection;
    LMultiplier[ftDynamic].MomentJacobian = vWhlContactVec * LMultiplier[ftDynamic].ForceJacobian;
    LMultiplier[ftDynamic].Max = 0.;
    LMultiplier[ftDynamic].Min = -fabs(dynamicFCoeff * vFn(eX));
    LMultiplier[ftDynamic].value = Constrain(LMultiplier[ftDynamic].Min, LMultiplier[ftDynamic].value, LMultiplier[ftDynamic].Max);
  }
  else {
    // Static friction is used for ctSTRUCTURE when the contact point is not moving.
    // It is always used for ctBOGEY elements because the friction coefficients
    // of a tyre depend on the direction of the movement (roll & side directions).
    // This cannot be handled properly by the so-called "dynamic friction".
    StaticFriction = true;

    LMultiplier[ftRoll].ForceJacobian = Transform()*FGColumnVector3(0.,1.,0.);
    LMultiplier[ftSide].ForceJacobian = Transform()*FGColumnVector3(0.,0.,1.);
    LMultiplier[ftRoll].MomentJacobian = vWhlContactVec * LMultiplier[ftRoll].ForceJacobian;
    LMultiplier[ftSide].MomentJacobian = vWhlContactVec * LMultiplier[ftSide].ForceJacobian;

    switch(eContactType) {
    case ctBOGEY:
      LMultiplier[ftRoll].Max = fabs(BrakeFCoeff * vFn(eX));
      LMultiplier[ftSide].Max = fabs(FCoeff * vFn(eX));
      break;
    case ctSTRUCTURE:
      LMultiplier[ftRoll].Max = fabs(staticFCoeff * vFn(eX));
      LMultiplier[ftSide].Max = fabs(staticFCoeff * vFn(eX));
      break;
    }

    LMultiplier[ftRoll].Min = -LMultiplier[ftRoll].Max;
    LMultiplier[ftSide].Min = -LMultiplier[ftSide].Max;
    LMultiplier[ftRoll].value = Constrain(LMultiplier[ftRoll].Min, LMultiplier[ftRoll].value, LMultiplier[ftRoll].Max);
    LMultiplier[ftSide].value = Constrain(LMultiplier[ftSide].Min, LMultiplier[ftSide].value, LMultiplier[ftSide].Max);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This function is used by the MultiplierIterator class to enumerate the
// Lagrange multipliers of a landing gear. This allows to encapsulate the storage
// of the multipliers in FGLGear without exposing it. From an outside point of
// view, each FGLGear instance has a number of Lagrange multipliers which can be
// accessed through this routine without knowing the exact constraint which they
// model.

FGPropagate::LagrangeMultiplier* FGLGear::GetMultiplierEntry(int entry)
{
  switch(entry) {
  case 0:
    if (StaticFriction)
      return &LMultiplier[ftRoll];
    else
      return &LMultiplier[ftDynamic];
  case 1:
    if (StaticFriction)
      return &LMultiplier[ftSide];
  default:
    return NULL;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This routine is called after the Lagrange multiplier has been computed. The
// friction forces of the landing gear are then updated accordingly.
FGColumnVector3& FGLGear::UpdateForces(void)
{
  if (StaticFriction) {
    vFn(eY) = LMultiplier[ftRoll].value;
    vFn(eZ) = LMultiplier[ftSide].value;
  }
  else
    vFn += LMultiplier[ftDynamic].value * (Transform ().Transposed() * LMultiplier[ftDynamic].ForceJacobian);

  // Return the updated force in the body frame
  return FGForce::GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::bind(void)
{
  string property_name;
  string base_property_name;
  base_property_name = CreateIndexedPropertyName("gear/unit", GearNumber);
  if (eContactType == ctBOGEY) {
    property_name = base_property_name + "/slip-angle-deg";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &WheelSlip );
    property_name = base_property_name + "/WOW";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &WOW );
    property_name = base_property_name + "/wheel-speed-fps";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), (FGLGear*)this,
                          &FGLGear::GetWheelRollVel);
    property_name = base_property_name + "/z-position";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), (FGForce*)this,
                          &FGForce::GetLocationZ, &FGForce::SetLocationZ);
    property_name = base_property_name + "/compression-ft";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &compressLength );
    property_name = base_property_name + "/side_friction_coeff";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &FCoeff );

    property_name = base_property_name + "/static_friction_coeff";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &staticFCoeff );

    if (eSteerType == stCaster) {
      property_name = base_property_name + "/steering-angle-deg";
      fdmex->GetPropertyManager()->Tie( property_name.c_str(), this, &FGLGear::GetSteerAngleDeg );
      property_name = base_property_name + "/castered";
      fdmex->GetPropertyManager()->Tie( property_name.c_str(), &Castered);
    }
  }

  if( isRetractable ) {
    property_name = base_property_name + "/pos-norm";
    fdmex->GetPropertyManager()->Tie( property_name.c_str(), &GearPos );
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::Report(ReportType repType)
{
  if (fabs(TakeoffDistanceTraveled) < 0.001) return; // Don't print superfluous reports

  switch(repType) {
  case erLand:
    cout << endl << "Touchdown report for " << name << " (WOW at time: "
         << fdmex->GetSimTime() << " seconds)" << endl;
    cout << "  Sink rate at contact:  " << SinkRate                << " fps,    "
                                << SinkRate*0.3048          << " mps"     << endl;
    cout << "  Contact ground speed:  " << GroundSpeed*.5925       << " knots,  "
                                << GroundSpeed*0.3048       << " mps"     << endl;
    cout << "  Maximum contact force: " << MaximumStrutForce       << " lbs,    "
                                << MaximumStrutForce*4.448  << " Newtons" << endl;
    cout << "  Maximum strut travel:  " << MaximumStrutTravel*12.0 << " inches, "
                                << MaximumStrutTravel*30.48 << " cm"      << endl;
    cout << "  Distance traveled:     " << LandingDistanceTraveled        << " ft,     "
                                << LandingDistanceTraveled*0.3048  << " meters"  << endl;
    LandingReported = true;
    break;
  case erTakeoff:
    cout << endl << "Takeoff report for " << name << " (Liftoff at time: "
        << fdmex->GetSimTime() << " seconds)" << endl;
    cout << "  Distance traveled:                " << TakeoffDistanceTraveled
         << " ft,     " << TakeoffDistanceTraveled*0.3048  << " meters"  << endl;
    cout << "  Distance traveled (over 50'):     " << TakeoffDistanceTraveled50ft
         << " ft,     " << TakeoffDistanceTraveled50ft*0.3048 << " meters" << endl;
    cout << "  [Altitude (ASL): " << Propagate->GetAltitudeASL() << " ft. / "
         << Propagate->GetAltitudeASLmeters() << " m  | Temperature: "
         << fdmex->GetAtmosphere()->GetTemperature() - 459.67 << " F / "
         << RankineToCelsius(fdmex->GetAtmosphere()->GetTemperature()) << " C]" << endl;
    cout << "  [Velocity (KCAS): " << Auxiliary->GetVcalibratedKTS() << "]" << endl;
    TakeoffReported = true;
    break;
  case erNone:
    break;
  }
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

void FGLGear::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor - loading and initialization
      cout << "    " << sContactType << " " << name          << endl;
      cout << "      Location: "         << vXYZn          << endl;
      cout << "      Spring Constant:  " << kSpring       << endl;

      if (eDampType == dtLinear)
        cout << "      Damping Constant: " << bDamp << " (linear)" << endl;
      else
        cout << "      Damping Constant: " << bDamp << " (square law)" << endl;

      if (eDampTypeRebound == dtLinear)
        cout << "      Rebound Damping Constant: " << bDampRebound << " (linear)" << endl;
      else 
        cout << "      Rebound Damping Constant: " << bDampRebound << " (square law)" << endl;

      cout << "      Dynamic Friction: " << dynamicFCoeff << endl;
      cout << "      Static Friction:  " << staticFCoeff  << endl;
      if (eContactType == ctBOGEY) {
        cout << "      Rolling Friction: " << rollingFCoeff << endl;
        cout << "      Steering Type:    " << sSteerType    << endl;
        cout << "      Grouping:         " << sBrakeGroup   << endl;
        cout << "      Max Steer Angle:  " << maxSteerAngle << endl;
        cout << "      Retractable:      " << isRetractable  << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGLGear" << endl;
    if (from == 1) cout << "Destroyed:    FGLGear" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim

