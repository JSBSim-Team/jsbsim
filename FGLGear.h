/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGLGear.h
 Author:       Jon S. Berndt
 Date started: 11/18/99

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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGLGEAR_H
#define FGLGEAR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#endif

#include <string>
#include "FGConfigFile.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"
#include "FGFDMExec.h"
#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_LGEAR "$Id: FGLGear.h,v 1.55 2003/12/02 05:42:12 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGAircraft;
class FGPosition;
class FGRotation;
class FGFCS;
class FGState;
class FGMassBalance;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Landing gear model.
    Calculates forces and moments due to landing gear reactions. This is done in
    several steps, and is dependent on what kind of gear is being modeled. Here
    are the parameters that can be specified in the config file for modeling
    landing gear:
    <p>
    <b><u>Physical Characteristics</u></b><br>
    <ol>
    <li>X, Y, Z location, in inches in structural coordinate frame</li>
    <li>Spring constant, in lbs/ft</li>
    <li>Damping coefficient, in lbs/ft/sec</li>
    <li>Dynamic Friction Coefficient</li>
    <li>Static Friction Coefficient</li>
    </ol></p><p>
    <b><u>Operational Properties</b></u><br>
    <ol>
    <li>Name</li>
    <li>Steerability attribute {one of STEERABLE | FIXED | CASTERED}</li>
    <li>Brake Group Membership {one of LEFT | CENTER | RIGHT | NOSE | TAIL | NONE}</li>
    <li>Max Steer Angle, in degrees</li>
    </ol></p>
    <p>
    <b><u>Algorithm and Approach to Modeling</u></b><br>
    <ol>
    <li>Find the location of the uncompressed landing gear relative to the CG of
    the aircraft. Remember, the structural coordinate frame that the aircraft is
    defined in is: X positive towards the tail, Y positive out the right side, Z
    positive upwards. The locations of the various parts are given in inches in
    the config file.</li>
    <li>The vector giving the location of the gear (relative to the cg) is
    rotated 180 degrees about the Y axis to put the coordinates in body frame (X
    positive forwards, Y positive out the right side, Z positive downwards, with
    the origin at the cg). The lengths are also now given in feet.</li>
    <li>The new gear location is now transformed to the local coordinate frame
    using the body-to-local matrix. (Mb2l).</li>
    <li>Knowing the location of the center of gravity relative to the ground
    (height above ground level or AGL) now enables gear deflection to be
    calculated. The gear compression value is the local frame gear Z location
    value minus the height AGL. [Currently, we make the assumption that the gear
    is oriented - and the deflection occurs in - the Z axis only. Additionally,
    the vector to the landing gear is currently not modified - which would
    (correctly) move the point of contact to the actual compressed-gear point of
    contact. Eventually, articulated gear may be modeled, but initially an
    effort must be made to model a generic system.] As an example, say the
    aircraft left main gear location (in local coordinates) is Z = 3 feet
    (positive) and the height AGL is 2 feet. This tells us that the gear is
    compressed 1 foot.</li>
    <li>If the gear is compressed, a Weight-On-Wheels (WOW) flag is set.</li>
    <li>With the compression length calculated, the compression velocity may now
    be calculated. This will be used to determine the damping force in the
    strut. The aircraft rotational rate is multiplied by the vector to the wheel
    to get a wheel velocity in body frame. That velocity vector is then
    transformed into the local coordinate frame.</li>
    <li>The aircraft cg velocity in the local frame is added to the
    just-calculated wheel velocity (due to rotation) to get a total wheel
    velocity in the local frame.</li>
    <li>The compression speed is the Z-component of the vector.</li>
    <li>With the wheel velocity vector no longer needed, it is normalized and
    multiplied by a -1 to reverse it. This will be used in the friction force
    calculation.</li>
    <li>Since the friction force takes place solely in the runway plane, the Z
    coordinate of the normalized wheel velocity vector is set to zero.</li>
    <li>The gear deflection force (the force on the aircraft acting along the
    local frame Z axis) is now calculated given the spring and damper
    coefficients, and the gear deflection speed and stroke length. Keep in mind
    that gear forces always act in the negative direction (in both local and
    body frames), and are not capable of generating a force in the positive
    sense (one that would attract the aircraft to the ground). So, the gear
    forces are always negative - they are limited to values of zero or less. The
    gear force is simply the negative of the sum of the spring compression
    length times the spring coefficient and the gear velocity times the damping
    coefficient.</li>
    <li>The lateral/directional force acting on the aircraft through the landing

    gear (along the local frame X and Y axes) is calculated next. First, the
    friction coefficient is multiplied by the recently calculated Z-force. This
    is the friction force. It must be given direction in addition to magnitude.
    We want the components in the local frame X and Y axes. From step 9, above,
    the conditioned wheel velocity vector is taken and the X and Y parts are
    multiplied by the friction force to get the X and Y components of friction.
    </li>
    <li>The wheel force in local frame is next converted to body frame.</li>
    <li>The moment due to the gear force is calculated by multiplying r x F
    (radius to wheel crossed into the wheel force). Both of these operands are
    in body frame.</li>
    </ol>
    @author Jon S. Berndt
    @version $Id: FGLGear.h,v 1.55 2003/12/02 05:42:12 jberndt Exp $
    @see Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
	   NASA-Ames", NASA CR-2497, January 1975
    @see Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
	   Wiley & Sons, 1979 ISBN 0-471-03032-5
    @see W. A. Ragsdale, "A Generic Landing Gear Dynamics Model for LASRS++",
     AIAA-2000-4303
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGLGear : public FGJSBBase
{
public:
  /// Brake grouping enumerators
  enum BrakeGroup {bgNone=0, bgLeft, bgRight, bgCenter, bgNose, bgTail };
  /// Steering group membership enumerators
  enum SteerType {stSteer, stFixed, stCaster};
  /// Report type enumerators
  enum ReportType {erNone=0, erTakeoff, erLand};
  /** Constructor
      @param Executive a pointer to the parent executive object
      @param File a pointer to the config file instance */
  FGLGear(FGConfigFile* File, FGFDMExec* Executive);
  /** Constructor
      @param lgear a reference to an existing FGLGear object     */
  FGLGear(const FGLGear& lgear);
  /// Destructor
  ~FGLGear();


  /// The Force vector for this gear
  FGColumnVector3& Force(void);
  /// The Moment vector for this gear
  FGColumnVector3& Moment(void) {return vMoment;}

  /// Gets the location of the gear in Body axes
  FGColumnVector3& GetBodyLocation(void) { return vWhlBodyVec; }
  double GetBodyLocation(int idx) { return vWhlBodyVec(idx); }

  FGColumnVector3& GetLocalGear(void) { return vLocalGear; }
  double GetLocalGear(int idx) { return vLocalGear(idx); }

  /// Gets the name of the gear
  inline string GetName(void)      {return name;          }
  /// Gets the Weight On Wheels flag value
  inline bool   GetWOW(void)       {return WOW;           }
  /// Gets the current compressed length of the gear in feet
  inline double  GetCompLen(void)   {return compressLength;}
  /// Gets the current gear compression velocity in ft/sec
  inline double  GetCompVel(void)   {return compressSpeed; }
  /// Gets the gear compression force in pounds
  inline double  GetCompForce(void) {return Force()(3);    }
  inline double  GetBrakeFCoeff(void) {return BrakeFCoeff;}

  /// Gets the current normalized tire pressure
  inline double  GetTirePressure(void) { return TirePressureNorm; }
  /// Sets the new normalized tire pressure
  inline void    SetTirePressure(double p) { TirePressureNorm = p; }
  
  /// Sets the brake value in percent (0 - 100)
  inline void SetBrake(double bp) {brakePct = bp;}

  /** Set the console touchdown reporting feature
      @param flag true turns on touchdown reporting, false turns it off */
  inline void SetReport(bool flag) { ReportEnable = flag; }
  /** Get the console touchdown reporting feature
      @return true if reporting is turned on */
  inline bool GetReport(void)    { return ReportEnable; }
  inline double GetSteerAngle(void) { return SteerAngle;}
  inline double GetstaticFCoeff(void) { return staticFCoeff;}
  
  inline int GetBrakeGroup(void) { return (int)eBrakeGrp; }
  inline int GetSteerType(void)  { return (int)eSteerType; }
  
  inline bool GetRetractable(void)         { return isRetractable;   }
  inline bool GetGearUnitUp(void)          { return GearUp;          }
  inline bool GetGearUnitDown(void)        { return GearDown;        }
  inline double GetWheelSideForce(void)    { return SideForce;       }
  inline double GetWheelRollForce(void)    { return RollingForce;    }
  inline double GetBodyXForce(void)        { return vLocalForce(eX); }
  inline double GetBodyYForce(void)        { return vLocalForce(eY); }
  inline double GetWheelSlipAngle(void)    { return WheelSlip;       }
  double GetWheelVel(int axis)             { return vWhlVelVec(axis);}
  
private:
  FGColumnVector3 vXYZ;
  FGColumnVector3 vMoment;
  FGColumnVector3 vWhlBodyVec;
  FGColumnVector3 vLocalGear;
  FGColumnVector3 vForce;
  FGColumnVector3 vLocalForce;
  FGColumnVector3 vWhlVelVec;     // Velocity of this wheel (Local)
  double SteerAngle;
  double kSpring;
  double bDamp;
  double compressLength;
  double compressSpeed;
  double staticFCoeff, dynamicFCoeff, rollingFCoeff;
  double brakePct;
  double BrakeFCoeff;
  double maxCompLen;
  double SinkRate;
  double GroundSpeed;
  double TakeoffDistanceTraveled;
  double TakeoffDistanceTraveled50ft;
  double LandingDistanceTraveled;
  double MaximumStrutForce;
  double MaximumStrutTravel;
  double SideWhlVel, RollingWhlVel;
  double RollingForce, SideForce, FCoeff;
  double WheelSlip;
  double lastWheelSlip;
  double TirePressureNorm;
  bool WOW;
  bool lastWOW;
  bool FirstContact;
  bool StartedGroundRun;
  bool LandingReported;
  bool TakeoffReported;
  bool ReportEnable;
  bool isRetractable;
  bool GearUp, GearDown;
  bool Servicable;
  string name;
  string sSteerType;
  string sBrakeGroup;
  string sRetractable;
  
  BrakeGroup eBrakeGrp;
  SteerType  eSteerType;
  double  maxSteerAngle;

  FGFDMExec*  Exec;
  FGState*    State;
  FGAircraft* Aircraft;
  FGPosition* Position;
  FGRotation* Rotation;
  FGFCS*      FCS;
  FGMassBalance* MassBalance;

  void Report(ReportType rt);
  void Debug(int from);
};
}
#include "FGAircraft.h"
#include "FGPosition.h"
#include "FGRotation.h"
#include "FGFCS.h"
#include "FGMassBalance.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#include "FGState.h"

#endif
