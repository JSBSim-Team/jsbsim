/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLGear.cpp
 Author:       Jon S. Berndt
               Norman H. Princen
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


static const char *IdSrc = "$Id: FGLGear.cpp,v 1.79 2002/04/05 14:55:10 dmegginson Exp $";
static const char *IdHdr = ID_LGEAR;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLGear::FGLGear(FGConfigFile* AC_cfg, FGFDMExec* fdmex) : Exec(fdmex)
{
  string tmp;
  
  *AC_cfg >> tmp >> name >> vXYZ(1) >> vXYZ(2) >> vXYZ(3)
            >> kSpring >> bDamp>> dynamicFCoeff >> staticFCoeff
                  >> rollingFCoeff >> sSteerType >> sBrakeGroup 
                     >> maxSteerAngle >> sRetractable;

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
  
  if ( sRetractable == "RETRACT" ) {
    isRetractable = true;
  } else  {
    isRetractable = false;
  }  
  
  GearUp = false;
  GearDown = true;

// Add some AI here to determine if gear is located properly according to its
// brake group type ??

  State       = Exec->GetState();
  Aircraft    = Exec->GetAircraft();
  Position    = Exec->GetPosition();
  Rotation    = Exec->GetRotation();
  FCS         = Exec->GetFCS();
  MassBalance = Exec->GetMassBalance();

  WOW = lastWOW = false;
  ReportEnable = true;
  FirstContact = false;
  Reported = false;
  DistanceTraveled = 0.0;
  MaximumStrutForce = MaximumStrutTravel = 0.0;
  SinkRate = GroundSpeed = 0.0;

  vWhlBodyVec     = (vXYZ - MassBalance->GetXYZcg()) / 12.0;
  vWhlBodyVec(eX) = -vWhlBodyVec(eX);
  vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

  vLocalGear = State->GetTb2l() * vWhlBodyVec;

  compressLength  = 0.0;
  compressSpeed   = 0.0;
  brakePct        = 0.0;
  maxCompLen      = 0.0;

  Debug(0);
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
  MassBalance = lgear.MassBalance;

  vXYZ = lgear.vXYZ;
  vMoment = lgear.vMoment;
  vWhlBodyVec = lgear.vWhlBodyVec;
  vLocalGear = lgear.vLocalGear;

  WOW                = lgear.WOW;
  lastWOW            = lgear.lastWOW;
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
  sRetractable    = lgear.sRetractable;
  eSteerType      = lgear.eSteerType;
  sBrakeGroup     = lgear.sBrakeGroup;
  eBrakeGrp       = lgear.eBrakeGrp;
  maxSteerAngle   = lgear.maxSteerAngle;
  isRetractable   = lgear.isRetractable;
  GearUp          = lgear.GearUp;
  GearDown        = lgear.GearDown;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::~FGLGear()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGLGear::Force(void)
{
  double SteerGain = 0;
  double SinWheel, CosWheel;

  vForce.InitMatrix();
  vMoment.InitMatrix();

  if (isRetractable) {
    if (FCS->GetGearPos() < 0.01) {
      GearUp   = true;
      GearDown = false;
     } else if (FCS->GetGearPos() > 0.99) {
      GearDown = true;
      GearUp   = false;
     } else {
      GearUp   = false;
      GearDown = false;
     }
  } else {
      GearUp   = false;
      GearDown = true;
  }         
      
  if (GearDown) {

    vWhlBodyVec     = (vXYZ - MassBalance->GetXYZcg()) / 12.0;
    vWhlBodyVec(eX) = -vWhlBodyVec(eX);
    vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

// vWhlBodyVec now stores the vector from the cg to this wheel

    vLocalGear = State->GetTb2l() * vWhlBodyVec;

// vLocalGear now stores the vector from the cg to the wheel in local coords.

    compressLength = vLocalGear(eZ) - Position->GetDistanceAGL();

// The compression length is currently measured in the Z-axis, only, at this time.
// It should be measured along the strut axis. If the local-frame gear position
// "hangs down" below the CG greater than the altitude, then the compressLength
// will be positive - i.e. the gear will have made contact.

    if (compressLength > 0.00) {

      WOW = true;// Weight-On-Wheels is true

// The next equation should really use the vector to the contact patch of the tire
// including the strut compression and not vWhlBodyVec.  Will fix this later.
// As it stands, now, the following equation takes the aircraft body-frame
// rotational rate and calculates the cross-product with the vector from the CG
// to the wheel, thus producing the instantaneous velocity vector of the tire
// in Body coords. The frame is also converted to local coordinates. When the
// aircraft local-frame velocity is added to this quantity, the total velocity of
// the wheel in local frame is then known. Subsequently, the compression speed
// (used for calculating damping force) is found by taking the Z-component of the
// wheel velocity.

      vWhlVelVec      =  State->GetTb2l() * (Rotation->GetPQR() * vWhlBodyVec);

      vWhlVelVec     +=  Position->GetVel();

      compressSpeed   =  vWhlVelVec(eZ);

// If this is the first time the wheel has made contact, remember some values
// for later printout.

      if (!FirstContact) {
        FirstContact  = true;
        SinkRate      =  compressSpeed;
        GroundSpeed   =  Position->GetVel().Magnitude();
      }

// The following needs work regarding friction coefficients and braking and
// steering The BrakeFCoeff formula assumes that an anti-skid system is used.
// It also assumes that we won't be turning and braking at the same time.
// Will fix this later.
// [JSB] The braking force coefficients include normal rolling coefficient +
// a percentage of the static friction coefficient based on braking applied.

      switch (eBrakeGrp) {
      case bgLeft:
        SteerGain = -1.0;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgLeft)) +
                                              staticFCoeff*FCS->GetBrake(bgLeft);
        break;
      case bgRight:
        SteerGain = -1.0;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgRight)) +
                                             staticFCoeff*FCS->GetBrake(bgRight);
        break;
      case bgCenter:
        SteerGain = -0.10;
        BrakeFCoeff = rollingFCoeff*(1.0 - FCS->GetBrake(bgCenter)) +
                                             staticFCoeff*FCS->GetBrake(bgCenter);
        break;
      case bgNose:
        SteerGain = 1.0;
        BrakeFCoeff = rollingFCoeff;
        break;
      case bgTail:
        SteerGain = -1.0;
        BrakeFCoeff = rollingFCoeff;
        break;
      case bgNone:
        SteerGain = -1.0;
        BrakeFCoeff = rollingFCoeff;
        break;
      default:
        cerr << "Improper brake group membership detected for this gear." << endl;
        break;
      }

      switch (eSteerType) {
      case stSteer:
        SteerAngle = SteerGain*FCS->GetDrPos();
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

      SinWheel      = sin(Rotation->Getpsi() + SteerAngle);
      CosWheel      = cos(Rotation->Getpsi() + SteerAngle);
      RollingWhlVel = vWhlVelVec(eX)*CosWheel + vWhlVelVec(eY)*SinWheel;
      SideWhlVel    = vWhlVelVec(eY)*CosWheel - vWhlVelVec(eX)*SinWheel;

// Calculate tire slip angle.

      if (RollingWhlVel == 0.0 && SideWhlVel == 0.0) {
        WheelSlip = 0.0;
      } else {
        WheelSlip = radtodeg*atan2(SideWhlVel, RollingWhlVel);
      }

// Compute the sideforce coefficients using similar assumptions to LaRCSim for now.
// Allow a maximum of 10 degrees tire slip angle before wheel slides.  At that point,
// transition from static to dynamic friction.  There are more complicated formulations
// of this that avoid the discrete jump.  Will fix this later.

      if (fabs(WheelSlip) <= 10.0) {
        FCoeff = staticFCoeff*WheelSlip/10.0;
      } else {
        FCoeff = dynamicFCoeff*fabs(WheelSlip)/WheelSlip;
      }

// Compute the vertical force on the wheel using square-law damping (per comment
// in paper AIAA-2000-4303 - see header prologue comments). We might consider
// allowing for both square and linear damping force calculation. Also need to
// possibly give a "rebound damping factor" that differs from the compression
// case.

      vLocalForce(eZ) =  min(-compressLength * kSpring
                             - compressSpeed * bDamp, (double)0.0);

      MaximumStrutForce = max(MaximumStrutForce, fabs(vLocalForce(eZ)));
      MaximumStrutTravel = max(MaximumStrutTravel, fabs(compressLength));

// Compute the forces in the wheel ground plane.

      RollingForce = 0;
      if (fabs(RollingWhlVel) > 1E-3) {
        RollingForce = vLocalForce(eZ) * BrakeFCoeff * fabs(RollingWhlVel)/RollingWhlVel;
      }
      SideForce    = vLocalForce(eZ) * FCoeff;

// Transform these forces back to the local reference frame.

      vLocalForce(eX) = RollingForce*CosWheel - SideForce*SinWheel;
      vLocalForce(eY) = SideForce*CosWheel    + RollingForce*SinWheel;

// Note to Jon: At this point the forces will be too big when the airplane is
// stopped or rolling to a stop.  We need to make sure that the gear forces just
// balance out the non-gear forces when the airplane is stopped.  That way the
// airplane won't start to accelerate until the non-gear/ forces are larger than
// the gear forces.  I think that the proper fix should go into FGAircraft::FMGear.
// This routine would only compute the local strut forces and return them to
// FMGear. All of the gear forces would get adjusted in FMGear using the total
// non-gear forces. Then the gear moments would be calculated. If strange things
// start happening to the airplane during testing as it rolls to a stop, then we
// need to implement this change.  I ran out of time to do it now but have the
// equations.

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

      compressLength = 0.0; // reset compressLength to zero for data output validity
    }

    if (FirstContact) {
      DistanceTraveled += Position->GetVel().Magnitude()*State->Getdt()*Aircraft->GetRate();
    }
  
    if (ReportEnable && Position->GetVel().Magnitude() <= 0.05 && !Reported) {
      if (debug_lvl > 0) Report();
    }

    if (lastWOW != WOW) {
      PutMessage("GEAR_CONTACT", WOW);
    }

    lastWOW = WOW;

// Crash detection logic (really out-of-bounds detection)

    if (compressLength > 500.0 ||
        vForce.Magnitude() > 100000000.0 ||
        vMoment.Magnitude() > 5000000000.0 ||
        SinkRate > 1.4666*30)
    {
      PutMessage("Crash Detected: Simulation FREEZE.");
      Exec->Freeze();
    }
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
    if (from == 0) { // Constructor
      cout << "    Name: "               << name          << endl;
      cout << "      Location: "         << vXYZ          << endl;
      cout << "      Spring Constant:  " << kSpring       << endl;
      cout << "      Damping Constant: " << bDamp         << endl;
      cout << "      Dynamic Friction: " << dynamicFCoeff << endl;
      cout << "      Static Friction:  " << staticFCoeff  << endl;
      cout << "      Rolling Friction: " << rollingFCoeff << endl;
      cout << "      Steering Type:    " << sSteerType    << endl;
      cout << "      Grouping:         " << sBrakeGroup   << endl;
      cout << "      Max Steer Angle:  " << maxSteerAngle << endl;
      cout << "      Retractable:      " << sRetractable  << endl;
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

