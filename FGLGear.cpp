/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLGear.cpp
 Author:       Jon S. Berndt
 Date started: 11/18/99
 Purpose:      Encapsulates the landing gear elements
 Called by:    FGAircraft

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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created
01/30/01   NHP   Extended gear model to properly simulate steering and braking

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGLGear.h"
#include <algorithm>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


static const char *IdSrc = "$Id: FGLGear.cpp,v 1.44 2001/03/22 14:10:24 jberndt Exp $";
static const char *IdHdr = ID_LGEAR;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLGear::FGLGear(FGConfigFile* AC_cfg, FGFDMExec* fdmex) : vXYZ(3),
                                                           vMoment(3),
                                                           vWhlBodyVec(3),
                                                           Exec(fdmex)
{
  string tmp;
  *AC_cfg >> tmp >> name >> vXYZ(1) >> vXYZ(2) >> vXYZ(3)  
            >> kSpring >> bDamp>> dynamicFCoeff >> staticFCoeff
                  >> rollingFCoeff >> sSteerType >> sBrakeGroup >> maxSteerAngle;
    
  cout << "    Name: " << name << endl;
  cout << "      Location: " << vXYZ << endl;
  cout << "      Spring Constant:  " << kSpring << endl;
  cout << "      Damping Constant: " << bDamp << endl;
  cout << "      Dynamic Friction: " << dynamicFCoeff << endl;
  cout << "      Static Friction:  " << staticFCoeff << endl;
  cout << "      Rolling Friction: " << rollingFCoeff << endl;
  cout << "      Steering Type:    " << sSteerType << endl;
  cout << "      Grouping:         " << sBrakeGroup << endl;
  cout << "      Max Steer Angle:  " << maxSteerAngle << endl;

  if      (sBrakeGroup == "LEFT"  ) eBrakeGrp = bgLeft;
  else if (sBrakeGroup == "RIGHT" ) eBrakeGrp = bgRight;
  else if (sBrakeGroup == "CENTER") eBrakeGrp = bgCenter;
  else if (sBrakeGroup == "NOSE"  ) eBrakeGrp = bgNose;
  else if (sBrakeGroup == "TAIL"  ) eBrakeGrp = bgTail;
  else if (sBrakeGroup == "NONE"  ) eBrakeGrp = bgNone;
  else {
    cerr << "Improper braking group specification in config file: "
         << sBrakeGroup << " is undefined." << endl;
  }

  if      (sSteerType == "STEERABLE") eSteerType = stSteer;
  else if (sSteerType == "FIXED"    ) eSteerType = stFixed;
  else if (sSteerType == "CASTERED" ) eSteerType = stCaster;
  else {
    cerr << "Improper steering type specification in config file: "
         << sSteerType << " is undefined." << endl;
  }

// Add some AI here to determine if gear is located properly according to its
// brake group type ??

  State       = Exec->GetState();
  Aircraft    = Exec->GetAircraft();
  Position    = Exec->GetPosition();
  Rotation    = Exec->GetRotation();
  FCS         = Exec->GetFCS();
  
  WOW = false;
  ReportEnable = true;
  FirstContact = false;
  Reported = false;
  DistanceTraveled = 0.0;
  MaximumStrutForce = MaximumStrutTravel = 0.0;
  
  vWhlBodyVec     = (vXYZ - Aircraft->GetXYZcg()) / 12.0;
  vWhlBodyVec(eX) = -vWhlBodyVec(eX);
  vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

  vLocalGear = State->GetTb2l() * vWhlBodyVec;

  if (debug_lvl & 2) cout << "Instantiated: FGLGear" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::FGLGear(const FGLGear& lgear)
{
  State    = lgear.State;
  Aircraft = lgear.Aircraft;
  Position = lgear.Position;
  Rotation = lgear.Rotation;
  Exec     = lgear.Exec;
  FCS      = lgear.FCS;

  vXYZ = lgear.vXYZ;
  vMoment = lgear.vMoment;
  vWhlBodyVec = lgear.vWhlBodyVec;
  vLocalGear = lgear.vLocalGear;

  WOW                = lgear.WOW;
  ReportEnable       = lgear.ReportEnable;
  FirstContact       = lgear.FirstContact;
  DistanceTraveled   = lgear.DistanceTraveled;
  MaximumStrutForce  = lgear.MaximumStrutForce;
  MaximumStrutTravel = lgear.MaximumStrutTravel;

  kSpring         = lgear.kSpring;
  bDamp           = lgear.bDamp;
  compressLength  = lgear.compressLength;
  compressSpeed   = lgear.compressSpeed;
  staticFCoeff    = lgear.staticFCoeff;
  dynamicFCoeff   = lgear.dynamicFCoeff;
  rollingFCoeff   = lgear.rollingFCoeff;
  brakePct        = lgear.brakePct;
  maxCompLen      = lgear.maxCompLen;
  SinkRate        = lgear.SinkRate;
  GroundSpeed     = lgear.GroundSpeed;
  Reported        = lgear.Reported;
  name            = lgear.name;
  sSteerType      = lgear.sSteerType;
  eSteerType      = lgear.eSteerType;
  sBrakeGroup     = lgear.sBrakeGroup;
  eBrakeGrp       = lgear.eBrakeGrp;
  maxSteerAngle   = lgear.maxSteerAngle;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::~FGLGear()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGLGear" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector FGLGear::Force(void)
{
  float SteerGain, SteerAngle, BrakeFCoeff;
  float SinWheel, CosWheel, SideWhlVel, RollingWhlVel;
  float RudderPedal, RollingForce, SideForce, FCoeff;
  float WheelSlip;

  FGColumnVector vForce(3);
  FGColumnVector vLocalForce(3);
  //FGColumnVector vLocalGear(3);     // Vector: CG to this wheel (Local)
  FGColumnVector vWhlVelVec(3);     // Velocity of this wheel (Local)
  
  vWhlBodyVec     = (vXYZ - Aircraft->GetXYZcg()) / 12.0;
  vWhlBodyVec(eX) = -vWhlBodyVec(eX);
  vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

  vLocalGear = State->GetTb2l() * vWhlBodyVec;
  
// For now, gear compression is assumed to happen in the Local Z axis,
// not the strut axis as it should be.  Will fix this later.

  compressLength = vLocalGear(eZ) - Position->GetDistanceAGL();

  if (compressLength > 0.00) {
     
    WOW = true;

// The next equation should really use the vector to the contact patch of the tire
// including the strut compression and not vWhlBodyVec.  Will fix this later.

    vWhlVelVec      =  State->GetTb2l() * (Rotation->GetPQR() * vWhlBodyVec);
    vWhlVelVec     +=  Position->GetVel();

    compressSpeed   =  vWhlVelVec(eZ);

    if (!FirstContact) {
      FirstContact  = true;
      SinkRate      =  compressSpeed;
      GroundSpeed   =  Position->GetVel().Magnitude();
    }

// The following needs work regarding friction coefficients and braking and
// steering The BrakeFCoeff formula assumes that an anti-skid system is used.
// It also assumes that we won't be turning and braking at the same time.
// Will fix this later.

    switch (eBrakeGrp) {
    case bgLeft:
        SteerGain = -maxSteerAngle;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgLeft)) +
                                            staticFCoeff*FCS->GetBrake(bgLeft);
      break;
    case bgRight:
        SteerGain = -maxSteerAngle;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgRight)) +
                                            staticFCoeff*FCS->GetBrake(bgRight);
      break;
    case bgCenter:
        SteerGain = -maxSteerAngle;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgCenter)) +
                                            staticFCoeff*FCS->GetBrake(bgCenter);
      break;
    case bgNose:
        SteerGain = maxSteerAngle;
        BrakeFCoeff = rollingFCoeff;
      break;
    case bgTail:
        SteerGain = -maxSteerAngle;
        BrakeFCoeff = rollingFCoeff;
      break;
    case bgNone:
        SteerGain = -maxSteerAngle;
        BrakeFCoeff = rollingFCoeff;
      break;
    default:
      cerr << "Improper brake group membership detected for this gear." << endl;
      break;
    }

// Note to Jon: Need to substitute the correct variable for RudderPedal.
// It is assumed that rudder pedal has a range of -1.0 to 1.0.

    switch (eSteerType) {
    case stSteer:
      SteerAngle = SteerGain*RudderPedal;
      break;
    case stFixed:
      SteerAngle = 0.0;
      break;
    case stCaster:

    // Note to Jon: This is not correct for castering gear.  I'll fix it later.
      SteerAngle = 0.0;
      break;
    default:
      cerr << "Improper steering type membership detected for this gear." << endl;
      break;
    }

// Transform the wheel velocities from the local axis system to the wheel axis system.
// For now, steering angle is assumed to happen in the Local Z axis,
// not the strut axis as it should be.  Will fix this later.
// Note to Jon: Please substitute the correct variable for Deg2Rad conversion.

    SinWheel      = sin(Rotation->Getpsi() + SteerAngle*DEGTORAD);
    CosWheel      = cos(Rotation->Getpsi() + SteerAngle*DEGTORAD);
    RollingWhlVel = vWhlVelVec(eX)*CosWheel + vWhlVelVec(eY)*SinWheel;
    SideWhlVel    = vWhlVelVec(eY)*CosWheel - vWhlVelVec(eX)*SinWheel;

// Calculate tire slip angle.

    if (RollingWhlVel == 0.0 && SideWhlVel == 0.0) {
      WheelSlip = 0.0;
    } else {
      WheelSlip = RADTODEG*atan2(SideWhlVel, RollingWhlVel);
    }

    // The following code normalizes the wheel velocity vector, reverses it, and zeroes out
    // the z component of the velocity. The question is, should the Z axis velocity be zeroed
    // out first before the normalization takes place or not? Subsequent to that, the Wheel
    // Velocity vector now points as a unit vector backwards and parallel to the wheel
    // velocity vector. It acts AT the wheel.

// Note to Jon: I commented out this line because I wasn't sure we want to do this.
//    vWhlVelVec      = -1.0 * vWhlVelVec.Normalize();
//    vWhlVelVec(eZ)  =  0.00;

// Compute the sideforce coefficients using similar assumptions to LaRCSim for now.
// Allow a maximum of 10 degrees tire slip angle before wheel slides.  At that point,
// transition from static to dynamic friction.  There are more complicated formulations
// of this that avoid the discrete jump.  Will fix this later.

    if (fabs(WheelSlip) <= 10.0) {
      FCoeff = staticFCoeff*WheelSlip/10.0;
    } else {
      FCoeff = dynamicFCoeff*fabs(WheelSlip)/WheelSlip;
    }

// Compute the vertical force on the wheel.

    vLocalForce(eZ) =  min(-compressLength * kSpring - compressSpeed * bDamp, (float)0.0);

    MaximumStrutForce = max(MaximumStrutForce, fabs(vLocalForce(eZ)));
    MaximumStrutTravel = max(MaximumStrutTravel, fabs(compressLength));

// Compute the forces in the wheel ground plane.

    RollingForce = vLocalForce(eZ) * BrakeFCoeff * fabs(RollingWhlVel)/RollingWhlVel;
    SideForce    = vLocalForce(eZ) * FCoeff;

// Transform these forces back to the local reference frame.

    vLocalForce(eX) = RollingForce*CosWheel - SideForce*SinWheel;
    vLocalForce(eY) = SideForce*CosWheel    + RollingForce*SinWheel;

// Note to Jon: At this point the forces will be too big when the airplane is stopped or
// rolling to a stop.  We need to make sure that the gear forces just balance out the non-gear forces
// when the airplane is stopped.  That way the airplane won't start to accelerate until the non-gear
// forces are larger than the gear forces.  I think that the proper fix should go into FGAircraft::FMGear.
// This routine would only compute the local strut forces and return them to FMGear.  All of the gear
// forces would get adjusted in FMGear using the total non-gear forces.  Then the gear moments would be
// calculated.  If strange things start happening to the airplane during testing as it rolls to a stop,
// then we need to implement this change.  I ran out of time to do it now but have the equations.

// Transform the forces back to the body frame and compute the moment.

    vForce  = State->GetTl2b() * vLocalForce;
    vMoment = vWhlBodyVec * vForce;

  } else {

    WOW = false;

    if (Position->GetDistanceAGL() > 200.0) {
      FirstContact = false;
      Reported = false;
      DistanceTraveled = 0.0;
      MaximumStrutForce = MaximumStrutTravel = 0.0;
    }

    vForce.InitMatrix();
    vMoment.InitMatrix();
  }

  if (FirstContact) {
    DistanceTraveled += Position->GetVel().Magnitude()*State->Getdt()*Aircraft->GetRate();
  }

  if (ReportEnable && Position->GetVel().Magnitude() <= 0.05 && !Reported) {
    Report();
  }
  
  return vForce;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::Report(void)
{
  cout << endl << "Touchdown report for " << name << endl;
  cout << "  Sink rate at contact:  " << SinkRate                << " fps,    "
                              << SinkRate*0.3408          << " mps"     << endl;
  cout << "  Contact ground speed:  " << GroundSpeed*.5925       << " knots,  "
                              << GroundSpeed*0.3408       << " mps"     << endl;
  cout << "  Maximum contact force: " << MaximumStrutForce       << " lbs,    "
                              << MaximumStrutForce*4.448  << " Newtons" << endl;
  cout << "  Maximum strut travel:  " << MaximumStrutTravel*12.0 << " inches, "
                              << MaximumStrutTravel*30.48 << " cm"      << endl;
  cout << "  Distance traveled:     " << DistanceTraveled        << " ft,     "
                              << DistanceTraveled*0.3408  << " meters"  << endl;
  Reported = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLGear::Debug(void)
{
  // TODO: Add user code here
}

