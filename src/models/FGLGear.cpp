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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created
01/30/01   NHP   Extended gear model to properly simulate steering and braking
07/08/09   BC    Modified gear model to support large angles between aircraft
                 and ground

/%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGLGear.h"
#include "models/FGGroundReactions.h"
#include "math/FGTable.h"
#include "input_output/FGXMLElement.h"
#include "models/FGInertial.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Body To Structural (body frame is rotated 180 deg about Y and lengths are
// given in ft instead of inches)
const FGMatrix33 FGLGear::Tb2s(-1./inchtoft, 0., 0., 0., 1./inchtoft, 0., 0., 0., -1./inchtoft);
const FGMatrix33 FGLGear::Ts2b(-inchtoft, 0., 0., 0., inchtoft, 0., 0., 0., -inchtoft);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLGear::FGLGear(Element* el, FGFDMExec* fdmex, int number, const struct Inputs& inputs) :
  FGSurface(fdmex, number),
  FGForce(fdmex),
  in(inputs),
  GearNumber(number),
  SteerAngle(0.0),
  Castered(false),
  StaticFriction(false),
  eSteerType(stSteer)
{
  kSpring = bDamp = bDampRebound = dynamicFCoeff = staticFCoeff = rollingFCoeff = maxSteerAngle = 0;
  isRetractable = false;
  eDampType = dtLinear;
  eDampTypeRebound = dtLinear;

  name = el->GetAttributeValue("name");
  string sContactType = el->GetAttributeValue("type");
  if (sContactType == "BOGEY") {
    eContactType = ctBOGEY;
  } else if (sContactType == "STRUCTURE") {
    eContactType = ctSTRUCTURE;
  } else {
    // Unknown contact point types will be treated as STRUCTURE.
    eContactType = ctSTRUCTURE;
  }

  // Default values for structural contact points
  if (eContactType == ctSTRUCTURE) {
    kSpring = in.EmptyWeight;
    bDamp = kSpring;
    bDampRebound = kSpring * 10;
    staticFCoeff = 1.0;
    dynamicFCoeff = 1.0;
  }

  PropertyManager = fdmex->GetPropertyManager();

  fStrutForce = 0;
  Element* strutForce = el->FindElement("strut_force");
  if (strutForce) {
    Element* springFunc = strutForce->FindElement("function");
    fStrutForce = new FGFunction(fdmex, springFunc);
  }
  else {
    if (el->FindElement("spring_coeff"))
      kSpring = el->FindElementValueAsNumberConvertTo("spring_coeff", "LBS/FT");
    if (el->FindElement("damping_coeff")) {
      Element* dampCoeff = el->FindElement("damping_coeff");
      if (dampCoeff->GetAttributeValue("type") == "SQUARE") {
        eDampType = dtSquare;
        bDamp   = el->FindElementValueAsNumberConvertTo("damping_coeff", "LBS/FT2/SEC2");
      } else {
        bDamp   = el->FindElementValueAsNumberConvertTo("damping_coeff", "LBS/FT/SEC");
      }
    }

    if (el->FindElement("damping_coeff_rebound")) {
      Element* dampCoeffRebound = el->FindElement("damping_coeff_rebound");
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
  }

  if (el->FindElement("dynamic_friction"))
    dynamicFCoeff = el->FindElementValueAsNumber("dynamic_friction");
  if (el->FindElement("static_friction"))
    staticFCoeff = el->FindElementValueAsNumber("static_friction");
  if (el->FindElement("rolling_friction"))
    rollingFCoeff = el->FindElementValueAsNumber("rolling_friction");
  if (el->FindElement("retractable"))
    isRetractable = ((unsigned int)el->FindElementValueAsNumber("retractable"))>0.0?true:false;

  if (el->FindElement("max_steer"))
    maxSteerAngle = el->FindElementValueAsNumberConvertTo("max_steer", "DEG");

  Element* castered_el = el->FindElement("castered");

  if ((maxSteerAngle == 360 && !castered_el)
      || (castered_el && castered_el->GetDataAsNumber() != 0.0)) {
    eSteerType = stCaster;
    Castered = true;
  }
  else if (maxSteerAngle == 0.0) {
    eSteerType = stFixed;
  }
  else
    eSteerType = stSteer;

  GroundReactions = fdmex->GetGroundReactions();

  ForceY_Table = 0;
  Element* force_table = el->FindElement("table");
  while (force_table) {
    string force_type = force_table->GetAttributeValue("name");
    if (force_type == "CORNERING_COEFF") {
      ForceY_Table = new FGTable(PropertyManager, force_table);
      break;
    } else {
      cerr << "Undefined force table for " << name << " contact point" << endl;
    }
    force_table = el->FindNextElement("table");
  }

  Element* element = el->FindElement("location");
  if (element) vXYZn = element->FindElementTripletConvertTo("IN");
  else {cerr << "No location given for contact " << name << endl; exit(-1);}
  SetTransformType(FGForce::tCustom);

  element = el->FindElement("orientation");
  if (element && (eContactType == ctBOGEY)) {
    FGQuaternion quatFromEuler(element->FindElementTripletConvertTo("RAD"));

    mTGear = quatFromEuler.GetT();
  }
  else {
    mTGear(1,1) = 1.;
    mTGear(2,2) = 1.;
    mTGear(3,3) = 1.;
  }

  string sBrakeGroup = el->FindElementValue("brake_group");

  if      (sBrakeGroup == "LEFT"  ) eBrakeGrp = bgLeft;
  else if (sBrakeGroup == "RIGHT" ) eBrakeGrp = bgRight;
  else if (sBrakeGroup == "CENTER") eBrakeGrp = bgCenter;
  else if (sBrakeGroup == "NOSE"  ) eBrakeGrp = bgCenter; // Nose brake is not supported by FGFCS
  else if (sBrakeGroup == "TAIL"  ) eBrakeGrp = bgCenter; // Tail brake is not supported by FGFCS
  else if (sBrakeGroup == "NONE"  ) eBrakeGrp = bgNone;
  else if (sBrakeGroup.empty()    ) eBrakeGrp = bgNone;
  else {
    cerr << "Improper braking group specification in config file: "
         << sBrakeGroup << " is undefined." << endl;
  }

// Add some AI here to determine if gear is located properly according to its
// brake group type ??

  useFCSGearPos = false;
  ReportEnable = true;
  TakeoffReported = LandingReported = false;

  // Set Pacejka terms

  Stiffness = 0.06;
  Shape = 2.8;
  Peak = staticFCoeff;
  Curvature = 1.03;

  ResetToIC();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::~FGLGear()
{
  delete ForceY_Table;
  delete fStrutForce;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::ResetToIC(void)
{
  GearPos  = 1.0;

  WOW = lastWOW = false;
  FirstContact = false;
  StartedGroundRun = false;
  LandingDistanceTraveled = TakeoffDistanceTraveled = TakeoffDistanceTraveled50ft = 0.0;
  MaximumStrutForce = MaximumStrutTravel = 0.0;
  SinkRate = GroundSpeed = 0.0;
  SteerAngle = 0.0;

  vWhlVelVec.InitMatrix();

  compressLength  = 0.0;
  compressSpeed   = 0.0;
  maxCompLen      = 0.0;

  WheelSlip = 0.0;

  // Initialize Lagrange multipliers
  for (int i=0; i < 3; i++) {
    LMultiplier[i].ForceJacobian.InitMatrix();
    LMultiplier[i].LeverArm.InitMatrix();
    LMultiplier[i].Min = 0.0;
    LMultiplier[i].Max = 0.0;
    LMultiplier[i].value = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGLGear::GetBodyForces(FGSurface *surface)
{
  double gearPos = 1.0;

  vFn.InitMatrix();

  if (isRetractable) gearPos = GetGearUnitPos();

  if (gearPos > 0.99) { // Gear DOWN
    FGColumnVector3 normal, terrainVel, dummy;
    FGLocation gearLoc, contact;
    FGColumnVector3 vWhlBodyVec = Ts2b * (vXYZn - in.vXYZcg);

    vLocalGear = in.Tb2l * vWhlBodyVec; // Get local frame wheel location
    gearLoc = in.Location.LocalToLocation(vLocalGear);

    // Compute the height of the theoretical location of the wheel (if strut is
    // not compressed) with respect to the ground level
    double height = fdmex->GetInertial()->GetContactPoint(gearLoc, contact,
                                                          normal, terrainVel,
                                                          dummy);

    // Does this surface contact point interact with another surface?
    if (surface) {
      if (!fdmex->GetTrimStatus())
        height -= (*surface).GetBumpHeight();
      staticFFactor = (*surface).GetStaticFFactor();
      rollingFFactor = (*surface).GetRollingFFactor();
      maximumForce = (*surface).GetMaximumForce();
      isSolid =  (*surface).GetSolid();
    }

    FGColumnVector3 vWhlDisplVec;
    double LGearProj = 1.0;

    if (height < 0.0) {
      WOW = true;
      vGroundNormal = in.Tec2b * normal;

      // The height returned by GetGroundCallback() is the AGL and is expressed
      // in the Z direction of the local coordinate frame. We now need to
      // transform this height in actual compression of the strut (BOGEY) or in
      // the normal direction to the ground (STRUCTURE)
      double normalZ = (in.Tec2l*normal)(eZ);
      LGearProj = -(mTGear.Transposed() * vGroundNormal)(eZ);

      // The following equations use the vector to the tire contact patch
      // including the strut compression.
      switch(eContactType) {
      case ctBOGEY:
        if (isSolid) {
          compressLength = LGearProj > 0.0 ? height * normalZ / LGearProj : 0.0;
          vWhlDisplVec = mTGear * FGColumnVector3(0., 0., -compressLength);
        } else {
          // Gears don't (or hardly) compress in liquids
          WOW = false;
        }
        break;
      case ctSTRUCTURE:
        compressLength = height * normalZ / DotProduct(normal, normal);
        vWhlDisplVec = compressLength * vGroundNormal;
        break;
      }
    }
    else
      WOW = false;

    if (WOW) {
      FGColumnVector3 vWhlContactVec = vWhlBodyVec + vWhlDisplVec;
      vActingXYZn = vXYZn + Tb2s * vWhlDisplVec;
      FGColumnVector3 vBodyWhlVel = in.PQR * vWhlContactVec;
      vBodyWhlVel += in.UVW - in.Tec2b * terrainVel;
      vWhlVelVec = mTGear.Transposed() * vBodyWhlVel;

      InitializeReporting();
      ComputeSteeringAngle();
      ComputeGroundFrame();

      vGroundWhlVel = mT.Transposed() * vBodyWhlVel;

      if (fdmex->GetTrimStatus() || in.TotalDeltaT == 0.0)
        compressSpeed = 0.0; // Steady state is sought during trimming
      else {
        compressSpeed = -vGroundWhlVel(eZ);
        if (eContactType == ctBOGEY)
          compressSpeed /= LGearProj;

        // If the gear is entering in contact with the ground during the current
        // time step, the compression speed might actually be lower than the
        // aircraft velocity projected along the gear leg (compressSpeed).
        double maxCompressSpeed = compressLength/in.TotalDeltaT;
        if (fabs(compressSpeed) > maxCompressSpeed)
          compressSpeed = sign(compressSpeed)*maxCompressSpeed;
      }

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
      compressLength = 0.0;
      compressSpeed = 0.0;
      WheelSlip = 0.0;
      StrutForce = 0.0;
      vWhlDisplVec.InitMatrix();

      LMultiplier[ftRoll].value = 0.0;
      LMultiplier[ftSide].value = 0.0;
      LMultiplier[ftDynamic].value = 0.0;

      // Return to neutral position between 1.0 and 0.8 gear pos.
      SteerAngle *= max(gearPos-0.8, 0.0)/0.2;

      ResetReporting();
    }
  }

  if (!WOW) {
    // Let wheel spin down slowly
    vWhlVelVec(eX) -= 13.0 * in.TotalDeltaT;
    if (vWhlVelVec(eX) < 0.0) vWhlVelVec(eX) = 0.0;
  }

  if (!fdmex->GetTrimStatus()) {
    ReportTakeoffOrLanding();

    // Require both WOW and LastWOW to be true before checking crash conditions
    // to allow the WOW flag to be used in terminating a scripted run.
    if (WOW && lastWOW) CrashDetect();

    lastWOW = WOW;
  }

  return FGForce::GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Build a local "ground" coordinate system defined by
//  eX : projection of the rolling direction on the ground
//  eY : projection of the sliping direction on the ground
//  eZ : normal to the ground

void FGLGear::ComputeGroundFrame(void)
{
  FGColumnVector3 roll = mTGear * FGColumnVector3(cos(SteerAngle), sin(SteerAngle), 0.);
  FGColumnVector3 side = vGroundNormal * roll;

  roll -= DotProduct(roll, vGroundNormal) * vGroundNormal;
  roll.Normalize();
  side.Normalize();

  mT(eX,eX) = roll(eX);
  mT(eY,eX) = roll(eY);
  mT(eZ,eX) = roll(eZ);
  mT(eX,eY) = side(eX);
  mT(eY,eY) = side(eY);
  mT(eZ,eY) = side(eZ);
  mT(eX,eZ) = vGroundNormal(eX);
  mT(eY,eZ) = vGroundNormal(eY);
  mT(eZ,eZ) = vGroundNormal(eZ);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Calculate tire slip angle.

void FGLGear::ComputeSlipAngle(void)
{
// Check that the speed is non-null otherwise keep the current angle
  if (vGroundWhlVel.Magnitude(eX,eY) > 1E-3)
    WheelSlip = -atan2(vGroundWhlVel(eY), fabs(vGroundWhlVel(eX)))*radtodeg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the steering angle in any case.
// This will also make sure that animations will look right.

void FGLGear::ComputeSteeringAngle(void)
{
  if (Castered) {
      // Check that the speed is non-null otherwise keep the current angle
      if (vWhlVelVec.Magnitude(eX,eY) > 0.1)
        SteerAngle = atan2(vWhlVelVec(eY), fabs(vWhlVelVec(eX)));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Reset reporting functionality after takeoff

void FGLGear::ResetReporting(void)
{
  if (in.DistanceAGL > 200.0) {
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
    GroundSpeed   =  in.Vground;
    TakeoffReported = false;
  }

  // If the takeoff run is starting, initialize.

  if ((in.Vground > 0.1) &&
      (in.BrakePos[bgLeft] == 0) &&
      (in.BrakePos[bgRight] == 0) &&
      (in.TakeoffThrottle && !StartedGroundRun))
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
    LandingDistanceTraveled += in.Vground * in.TotalDeltaT;

  if (StartedGroundRun) {
    TakeoffDistanceTraveled50ft += in.Vground * in.TotalDeltaT;
    if (WOW) TakeoffDistanceTraveled += in.Vground * in.TotalDeltaT;
  }

  if ( ReportEnable
       && in.Vground <= 0.05
       && !LandingReported
       && in.WOW)
  {
    if (debug_lvl > 0) Report(erLand);
  }

  if ( ReportEnable
       && !TakeoffReported
       && (in.DistanceAGL - vLocalGear(eZ)) > 50.0
       && !in.WOW)
  {
    if (debug_lvl > 0) Report(erTakeoff);
  }

  if (lastWOW != WOW)
  {
    ostringstream buf;
    buf << "GEAR_CONTACT: " << fdmex->GetSimTime() << " seconds: " << name;
    PutMessage(buf.str(), WOW);
  }
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
    ostringstream buf;
    buf << "*CRASH DETECTED* " << fdmex->GetSimTime() << " seconds: " << name;
    PutMessage(buf.str());
    // fdmex->SuspendIntegration();
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
  BrakeFCoeff = rollingFFactor * rollingFCoeff;

  if (eBrakeGrp != bgNone)
    BrakeFCoeff += in.BrakePos[eBrakeGrp] * staticFFactor * (staticFCoeff - rollingFCoeff);
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
  FCoeff *= staticFFactor;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the vertical force on the wheel using square-law damping (per comment
// in paper AIAA-2000-4303 - see header prologue comments). We might consider
// allowing for both square and linear damping force calculation. Also need to
// possibly give a "rebound damping factor" that differs from the compression
// case.

void FGLGear::ComputeVerticalStrutForce()
{
  if (fStrutForce)
    StrutForce = min(fStrutForce->GetValue(), (double)0.0);
  else {
    double springForce = -compressLength * kSpring;
    double dampForce = 0;

    if (compressSpeed >= 0.0) {

      if (eDampType == dtLinear)
        dampForce = -compressSpeed * bDamp;
      else
        dampForce = -compressSpeed * compressSpeed * bDamp;

    } else {

      if (eDampTypeRebound == dtLinear)
        dampForce   = -compressSpeed * bDampRebound;
      else
        dampForce   =  compressSpeed * compressSpeed * bDampRebound;

    }

    StrutForce = min(springForce + dampForce, (double)0.0);
    if (StrutForce > maximumForce) {
      StrutForce = maximumForce;
      compressLength = -StrutForce / kSpring;
    }
  }

  // The reaction force of the wheel is always normal to the ground
  switch (eContactType) {
  case ctBOGEY:
    // Project back the strut force in the local coordinate frame of the ground
    vFn(eZ) = StrutForce / (mTGear.Transposed()*vGroundNormal)(eZ);
    break;
  case ctSTRUCTURE:
    vFn(eZ) = -StrutForce;
    break;
  }

  // Remember these values for reporting
  MaximumStrutForce = max(MaximumStrutForce, fabs(StrutForce));
  MaximumStrutTravel = max(MaximumStrutTravel, fabs(compressLength));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGLGear::GetGearUnitPos(void) const
{
  // hack to provide backward compatibility to gear/gear-pos-norm property
  if( useFCSGearPos || in.FCSGearPos != 1.0 ) {
    useFCSGearPos = true;
    return in.FCSGearPos;
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
  if ((eContactType == ctSTRUCTURE) && (vGroundWhlVel.Magnitude(eX,eY) > 1E-3)) {

    FGColumnVector3 velocityDirection = vGroundWhlVel;

    StaticFriction = false;

    velocityDirection(eZ) = 0.;
    velocityDirection.Normalize();

    LMultiplier[ftDynamic].ForceJacobian = mT * velocityDirection;
    LMultiplier[ftDynamic].Max = 0.;
    LMultiplier[ftDynamic].Min = -fabs(staticFFactor * dynamicFCoeff * vFn(eZ));
    LMultiplier[ftDynamic].LeverArm = vWhlContactVec;

    // The Lagrange multiplier value obtained from the previous iteration is
    // kept. This is supposed to accelerate the convergence of the projected
    // Gauss-Seidel algorithm. The code just below is to make sure that the
    // initial value is consistent with the current friction coefficient and
    // normal reaction.
    LMultiplier[ftDynamic].value = Constrain(LMultiplier[ftDynamic].Min, LMultiplier[ftDynamic].value, LMultiplier[ftDynamic].Max);

    GroundReactions->RegisterLagrangeMultiplier(&LMultiplier[ftDynamic]);
  }
  else {
    // Static friction is used for ctSTRUCTURE when the contact point is not
    // moving. It is always used for ctBOGEY elements because the friction
    // coefficients of a tyre depend on the direction of the movement (roll &
    // side directions). This cannot be handled properly by the so-called
    // "dynamic friction".
    StaticFriction = true;

    LMultiplier[ftRoll].ForceJacobian = mT * FGColumnVector3(1.,0.,0.);
    LMultiplier[ftSide].ForceJacobian = mT * FGColumnVector3(0.,1.,0.);
    LMultiplier[ftRoll].LeverArm = vWhlContactVec;
    LMultiplier[ftSide].LeverArm = vWhlContactVec;

    switch(eContactType) {
    case ctBOGEY:
      LMultiplier[ftRoll].Max = fabs(BrakeFCoeff * vFn(eZ));
      LMultiplier[ftSide].Max = fabs(FCoeff * vFn(eZ));
      break;
    case ctSTRUCTURE:
      LMultiplier[ftRoll].Max = fabs(staticFFactor * staticFCoeff * vFn(eZ));
      LMultiplier[ftSide].Max = LMultiplier[ftRoll].Max;
      break;
    }

    LMultiplier[ftRoll].Min = -LMultiplier[ftRoll].Max;
    LMultiplier[ftSide].Min = -LMultiplier[ftSide].Max;

    // The Lagrange multiplier value obtained from the previous iteration is
    // kept. This is supposed to accelerate the convergence of the projected
    // Gauss-Seidel algorithm. The code just below is to make sure that the
    // initial value is consistent with the current friction coefficient and
    // normal reaction.
    LMultiplier[ftRoll].value = Constrain(LMultiplier[ftRoll].Min, LMultiplier[ftRoll].value, LMultiplier[ftRoll].Max);
    LMultiplier[ftSide].value = Constrain(LMultiplier[ftSide].Min, LMultiplier[ftSide].value, LMultiplier[ftSide].Max);

    GroundReactions->RegisterLagrangeMultiplier(&LMultiplier[ftRoll]);
    GroundReactions->RegisterLagrangeMultiplier(&LMultiplier[ftSide]);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This routine is called after the Lagrange multiplier has been computed in
// the FGAccelerations class. The friction forces of the landing gear are then
// updated accordingly.
void FGLGear::UpdateForces(void)
{
  if (StaticFriction) {
    vFn(eX) = LMultiplier[ftRoll].value;
    vFn(eY) = LMultiplier[ftSide].value;
  }
  else {
    FGColumnVector3 forceDir = mT.Transposed() * LMultiplier[ftDynamic].ForceJacobian;
    vFn(eX) = LMultiplier[ftDynamic].value * forceDir(eX);
    vFn(eY) = LMultiplier[ftDynamic].value * forceDir(eY);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::SetstaticFCoeff(double coeff)
{
  staticFCoeff = coeff;
  Peak = coeff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::bind(void)
{
  string property_name;
  string base_property_name;

  switch(eContactType) {
  case ctBOGEY:
    eSurfaceType = FGSurface::ctBOGEY;
    base_property_name = CreateIndexedPropertyName("gear/unit", GearNumber);
    break;
  case ctSTRUCTURE:
    eSurfaceType = FGSurface::ctSTRUCTURE;
    base_property_name = CreateIndexedPropertyName("contact/unit", GearNumber);
    break;
  default:
    return;
  }
  FGSurface::bind();

  property_name = base_property_name + "/WOW";
  PropertyManager->Tie( property_name.c_str(), &WOW );
  property_name = base_property_name + "/x-position";
  PropertyManager->Tie( property_name.c_str(), (FGForce*)this,
                        &FGForce::GetLocationX, &FGForce::SetLocationX);
  property_name = base_property_name + "/y-position";
  PropertyManager->Tie( property_name.c_str(), (FGForce*)this,
                        &FGForce::GetLocationY, &FGForce::SetLocationY);
  property_name = base_property_name + "/z-position";
  PropertyManager->Tie( property_name.c_str(), (FGForce*)this,
                        &FGForce::GetLocationZ, &FGForce::SetLocationZ);
  property_name = base_property_name + "/compression-ft";
  PropertyManager->Tie( property_name.c_str(), &compressLength );
  property_name = base_property_name + "/compression-velocity-fps";
  PropertyManager->Tie( property_name.c_str(), &compressSpeed );
  property_name = base_property_name + "/static_friction_coeff";
  PropertyManager->Tie( property_name.c_str(), (FGLGear*)this,
                        &FGLGear::GetstaticFCoeff, &FGLGear::SetstaticFCoeff);
  property_name = base_property_name + "/dynamic_friction_coeff";
  PropertyManager->Tie( property_name.c_str(), &dynamicFCoeff );

  if (eContactType == ctBOGEY) {
    property_name = base_property_name + "/slip-angle-deg";
    PropertyManager->Tie( property_name.c_str(), &WheelSlip );
    property_name = base_property_name + "/wheel-speed-fps";
    PropertyManager->Tie( property_name.c_str(), (FGLGear*)this,
                          &FGLGear::GetWheelRollVel);
    property_name = base_property_name + "/side_friction_coeff";
    PropertyManager->Tie( property_name.c_str(), &FCoeff );
    property_name = base_property_name + "/rolling_friction_coeff";
    PropertyManager->Tie( property_name.c_str(), &rollingFCoeff );

    if (eSteerType == stCaster) {
      property_name = base_property_name + "/steering-angle-deg";
      PropertyManager->Tie( property_name.c_str(), this, &FGLGear::GetSteerAngleDeg );
      property_name = base_property_name + "/castered";
      PropertyManager->Tie( property_name.c_str(), &Castered);
    }
  }

  if( isRetractable ) {
    property_name = base_property_name + "/pos-norm";
    PropertyManager->Tie( property_name.c_str(), &GearPos );
  }

  if (eSteerType != stFixed) {
    // This property allows the FCS to override the steering position angle that
    // is set by the property fcs/steer-cmd-norm. The prefix fcs/ has been kept
    // for backward compatibility.
    string tmp = CreateIndexedPropertyName("fcs/steer-pos-deg", GearNumber);
    PropertyManager->Tie(tmp.c_str(), this, &FGLGear::GetSteerAngleDeg, &FGLGear::SetSteerAngleDeg);
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
    cout << "  [Altitude (ASL): " << in.DistanceASL << " ft. / "
         << in.DistanceASL*FGJSBBase::fttom << " m  | Temperature: "
         << in.Temperature - 459.67 << " F / "
         << RankineToCelsius(in.Temperature) << " C]" << endl;
    cout << "  [Velocity (KCAS): " << in.VcalibratedKts << "]" << endl;
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
  static const char* sSteerType[] = {"STEERABLE", "FIXED", "CASTERED" };
  static const char* sBrakeGroup[] = {"NONE", "LEFT", "RIGHT", "CENTER", "NOSE", "TAIL"};
  static const char* sContactType[] = {"BOGEY", "STRUCTURE" };

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor - loading and initialization
      cout << "    " << sContactType[eContactType] << " " << name          << endl;
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
        cout << "      Steering Type:    " << sSteerType[eSteerType] << endl;
        cout << "      Grouping:         " << sBrakeGroup[eBrakeGrp] << endl;
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
    }
  }
}

} // namespace JSBSim
