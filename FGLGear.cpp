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

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGLGear.cpp,v 1.30 2000/10/29 17:05:24 jsb Exp $";
static const char *IdHdr = ID_LGEAR;

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
	          >> SteerType >> BrakeGroup >> maxSteerAngle;
    
  cout << "    Name: " << name << endl;
  cout << "      Location: " << vXYZ << endl;
  cout << "      Spring Constant:  " << kSpring << endl;
  cout << "      Damping Constant: " << bDamp << endl;
  cout << "      Dynamic Friction: " << dynamicFCoeff << endl;
  cout << "      Static Friction:  " << staticFCoeff << endl;
  cout << "      Grouping:         " << BrakeGroup << endl;
  cout << "      Steering Type:    " << SteerType << endl;
  cout << "      Max Steer Angle:  " << maxSteerAngle << endl;

  if      (BrakeGroup == "LEFT"  ) eBrakeGrp = bgLeft;
  else if (BrakeGroup == "RIGHT" ) eBrakeGrp = bgRight;
  else if (BrakeGroup == "CENTER") eBrakeGrp = bgCenter;
  else if (BrakeGroup == "NOSE"  ) eBrakeGrp = bgNose;
  else if (BrakeGroup == "TAIL"  ) eBrakeGrp = bgTail;
  else if (BrakeGroup == "NONE"  ) eBrakeGrp = bgNone;
  else {
    cerr << "Improper braking group specification in config file: "
         << BrakeGroup << " is undefined." << endl;
  }

// add some AI here to determine if gear is located properly according to its
// brake group type

  State       = Exec->GetState();
  Aircraft    = Exec->GetAircraft();
  Position    = Exec->GetPosition();
  Rotation    = Exec->GetRotation();
  
  WOW = false;
  ReportEnable=true;
  FirstContact = false;
  Reported = false;
  DistanceTraveled = 0.0;
  MaximumStrutForce = MaximumStrutTravel = 0.0;
  
  vWhlBodyVec     = (vXYZ - Aircraft->GetXYZcg()) / 12.0;
  vWhlBodyVec(eX) = -vWhlBodyVec(eX);
  vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

  vLocalGear = State->GetTb2l() * vWhlBodyVec;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::FGLGear(const FGLGear& lgear)
{
  State    = lgear.State;
  Aircraft = lgear.Aircraft;
  Position = lgear.Position;
  Rotation = lgear.Rotation;
  Exec     = lgear.Exec;

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
  brakePct        = lgear.brakePct;
  maxCompLen      = lgear.maxCompLen;
  SinkRate        = lgear.SinkRate;
  GroundSpeed     = lgear.GroundSpeed;
  Reported        = lgear.Reported;
  name            = lgear.name;
  SteerType       = lgear.SteerType;
  BrakeGroup      = lgear.BrakeGroup;
  eBrakeGrp       = lgear.eBrakeGrp;
  maxSteerAngle   = lgear.maxSteerAngle;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLGear::~FGLGear(void) {}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector FGLGear::Force(void)
{
  FGColumnVector vForce(3);
  FGColumnVector vLocalForce(3);
  //FGColumnVector vLocalGear(3);     // Vector: CG to this wheel (Local)
  FGColumnVector vWhlVelVec(3);     // Velocity of this wheel (Local)
  
  vWhlBodyVec     = (vXYZ - Aircraft->GetXYZcg()) / 12.0;
  vWhlBodyVec(eX) = -vWhlBodyVec(eX);
  vWhlBodyVec(eZ) = -vWhlBodyVec(eZ);

  vLocalGear = State->GetTb2l() * vWhlBodyVec;
  
  compressLength = vLocalGear(eZ) - Position->GetDistanceAGL();

  if (compressLength > 0.00) {
     
    WOW = true;
    vWhlVelVec      =  State->GetTb2l() * (Rotation->GetPQR() * vWhlBodyVec);
    vWhlVelVec     +=  Position->GetVel();

    compressSpeed   =  vWhlVelVec(eZ);

    if (!FirstContact) {
      FirstContact  = true;
      SinkRate      =  compressSpeed;
      GroundSpeed   =  Position->GetVel().Magnitude();
    }

    // The following code normalizes the wheel velocity vector, reverses it, and zeroes out
    // the z component of the velocity. The question is, should the Z axis velocity be zeroed
    // out first before the normalization takes place or not? Subsequent to that, the Wheel
    // Velocity vector now points as a unit vector backwards and parallel to the wheel
    // velocity vector. It acts AT the wheel.

    vWhlVelVec      = -1.0 * vWhlVelVec.Normalize();
    vWhlVelVec(eZ)  =  0.00;

// the following needs work regarding friction coefficients and braking and steering

    switch (eBrakeGrp) {
    case bgLeft:
      break;
    case bgRight:
      break;
    case bgCenter:
      break;
    case bgNose:
      break;
    case bgTail:
      break;
    case bgNone:
      break;
    default:
      cerr << "Improper brake group membership detected for this gear." << endl;
      break;
    }
//

    vLocalForce(eZ) =  min(-compressLength * kSpring - compressSpeed * bDamp, (float)0.0);
    vLocalForce(eX) =  fabs(vLocalForce(eZ) * staticFCoeff) * vWhlVelVec(eX);
    vLocalForce(eY) =  fabs(vLocalForce(eZ) * staticFCoeff) * vWhlVelVec(eY);

    MaximumStrutForce = max(MaximumStrutForce, fabs(vLocalForce(eZ)));
    MaximumStrutTravel = max(MaximumStrutTravel, fabs(compressLength));

    vForce  = State->GetTl2b() * vLocalForce ;
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

