/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGLGear.h
 Author:       Jon S. Berndt
 Date started: 11/18/99

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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

#include <FGJSBBase.h>
#include <FGFDMExec.h>
#include <input_output/FGXMLElement.h>
#include <math/FGColumnVector3.h>
#include <math/FGTable.h>
#include <string>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_LGEAR "$Id: FGLGear.h,v 1.27 2009/02/17 08:04:15 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGAircraft;
class FGPropagate;
class FGFCS;
class FGState;
class FGMassBalance;
class FGAuxiliary;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Landing gear model.
    Calculates forces and moments due to landing gear reactions. This is done in
    several steps, and is dependent on what kind of gear is being modeled. Here
    are the parameters that can be specified in the config file for modeling
    landing gear:
    <p>
    <h3>Physical Characteristics</h3>
    <ol>
    <li>X, Y, Z location, in inches in structural coordinate frame</li>
    <li>Spring constant, in lbs/ft</li>
    <li>Damping coefficient, in lbs/ft/sec</li>
    <li>Dynamic Friction Coefficient</li>
    <li>Static Friction Coefficient</li>
    </ol></p><p>
    <h3>Operational Properties</h3>
    <ol>
    <li>Name</li>
    <li>Steerability attribute {one of STEERABLE | FIXED | CASTERED}</li>
    <li>Brake Group Membership {one of LEFT | CENTER | RIGHT | NOSE | TAIL | NONE}</li>
    <li>Max Steer Angle, in degrees</li>
    </ol></p>
    <p>
    <h3>Algorithm and Approach to Modeling</h3>
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

    <h3>Configuration File Format:</h3>
@code
        <contact type="{BOGEY | STRUCTURE}" name="{string}">
            <location unit="{IN | M}">
                <x> {number} </x>
                <y> {number} </y>
                <z> {number} </z>
            </location>
            <static_friction> {number} </static_friction>
            <dynamic_friction> {number} </dynamic_friction>
            <rolling_friction> {number} </rolling_friction>
            <spring_coeff unit="{LBS/FT | N/M}"> {number} </spring_coeff>
            <damping_coeff unit="{LBS/FT/SEC | N/M/SEC}"> {number} </damping_coeff>
            <damping_coeff_rebound unit="{LBS/FT/SEC | N/M/SEC}"> {number} </damping_coeff_rebound>
            <max_steer unit="DEG"> {number | 0 | 360} </max_steer>
            <brake_group> {NONE | LEFT | RIGHT | CENTER | NOSE | TAIL} </brake_group>
            <retractable>{0 | 1}</retractable>
            <table type="{CORNERING_COEFF}">
            </table>
            <relaxation_velocity>
               <rolling unit="{FT/SEC | KTS | M/S}"> {number} </rolling>
               <side unit="{FT/SEC | KTS | M/S}"> {number} </side>
            </relaxation_velocity>
            <force_lag_filter>
               <rolling> {number} </rolling>
               <side> {number} </side>
            </force_lag_filter>
            <wheel_slip_filter> {number} </wheel_slip_filter>  
        </contact>
@endcode
    @author Jon S. Berndt
    @version $Id: FGLGear.h,v 1.27 2009/02/17 08:04:15 jberndt Exp $
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
  /// Contact point type
  enum ContactType {ctBOGEY, ctSTRUCTURE, ctUNKNOWN};
  /// Report type enumerators
  enum ReportType {erNone=0, erTakeoff, erLand};
  /// Damping types
  enum DampType {dtLinear=0, dtSquare};
  /** Constructor
      @param el a pointer to the XML element that contains the CONTACT info.
      @param Executive a pointer to the parent executive object
      @param number integer identifier for this instance of FGLGear
  */
  FGLGear(Element* el, FGFDMExec* Executive, int number);
  /// Destructor
  ~FGLGear();

  /// The Force vector for this gear
  FGColumnVector3& Force(void);
  /// The Moment vector for this gear
  FGColumnVector3& Moment(void) {return vMoment;}

  /// Gets the location of the gear in Body axes
  FGColumnVector3& GetBodyLocation(void) { return vWhlBodyVec; }
  double GetBodyLocation(int idx) const { return vWhlBodyVec(idx); }

  FGColumnVector3& GetLocalGear(void) { return vLocalGear; }
  double GetLocalGear(int idx) const { return vLocalGear(idx); }

  /// Gets the name of the gear
  inline string GetName(void) const {return name;          }
  /// Gets the Weight On Wheels flag value
  inline bool   GetWOW(void) const {return WOW;           }
  /// Gets the current compressed length of the gear in feet
  inline double  GetCompLen(void) const {return compressLength;}
  /// Gets the current gear compression velocity in ft/sec
  inline double  GetCompVel(void) const {return compressSpeed; }
  /// Gets the gear compression force in pounds
  inline double  GetCompForce(void) const {return vForce(eZ);    }
  inline double  GetBrakeFCoeff(void) const {return BrakeFCoeff;}

  /// Gets the current normalized tire pressure
  inline double  GetTirePressure(void) const { return TirePressureNorm; }
  /// Sets the new normalized tire pressure
  inline void    SetTirePressure(double p) { TirePressureNorm = p; }

  /// Sets the brake value in percent (0 - 100)
  inline void SetBrake(double bp) {brakePct = bp;}

  /// Sets the weight-on-wheels flag.
  void SetWOW(bool wow) {WOW = wow;}

  /** Set the console touchdown reporting feature
      @param flag true turns on touchdown reporting, false turns it off */
  inline void SetReport(bool flag) { ReportEnable = flag; }
  /** Get the console touchdown reporting feature
      @return true if reporting is turned on */
  inline bool GetReport(void) const  { return ReportEnable; }
  double GetSteerNorm(void) const    { return radtodeg/maxSteerAngle*SteerAngle; }
  double GetDefaultSteerAngle(double cmd) const { return cmd*maxSteerAngle; }
  double GetstaticFCoeff(void) const { return staticFCoeff; }

  inline int GetBrakeGroup(void) const { return (int)eBrakeGrp; }
  inline int GetSteerType(void) const  { return (int)eSteerType; }

  inline double GetZPosition(void) const { return vXYZ(3); }
  inline void SetZPosition(double z) { vXYZ(3) = z; }

  bool GetSteerable(void) const { return eSteerType != stFixed; }
  inline bool GetRetractable(void) const      { return isRetractable;   }
  inline bool GetGearUnitUp(void) const       { return GearUp;          }
  inline bool GetGearUnitDown(void) const     { return GearDown;        }
  inline double GetWheelSideForce(void) const { return SideForce;       }
  inline double GetWheelRollForce(void) const { return RollingForce;    }
  inline double GetWheelSideVel(void) const   { return SideWhlVel;      }
  inline double GetWheelRollVel(void) const   { return RollingWhlVel;   }
  inline double GetBodyXForce(void) const     { return vLocalForce(eX); }
  inline double GetBodyYForce(void) const     { return vLocalForce(eY); }
  inline double GetWheelSlipAngle(void) const { return WheelSlip;       }
  double GetWheelVel(int axis) const          { return vWhlVelVec(axis);}
  bool IsBogey(void) const                    { return (eContactType == ctBOGEY);}
  double GetGearUnitPos(void);

  void bind(void);

private:
  int GearNumber;
  FGColumnVector3 vXYZ;
  FGColumnVector3 vMoment;
  FGColumnVector3 vWhlBodyVec;
  FGColumnVector3 vLocalGear;
  FGColumnVector3 vForce;
  FGColumnVector3 last_vForce; // remove this
  FGColumnVector3 vLocalForce;
  FGColumnVector3 vWhlVelVec;     // Velocity of this wheel (Local)
  FGColumnVector3 normal, cvel;
  FGLocation contact, gearLoc;
  FGTable *ForceY_Table;
  double dT;
  double SteerAngle;
  double kSpring;
  double bDamp;
  double bDampRebound;
  double compressLength;
  double compressSpeed;
  double staticFCoeff, dynamicFCoeff, rollingFCoeff;
  double Stiffness, Shape, Peak, Curvature; // Pacejka factors
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
  double TirePressureNorm;
  double SinWheel, CosWheel;
  double GearPos;
  bool   useFCSGearPos;
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
  string sContactType;

  BrakeGroup  eBrakeGrp;
  ContactType eContactType;
  SteerType   eSteerType;
  DampType    eDampType;
  DampType    eDampTypeRebound;
  double  maxSteerAngle;
  double RFRV;  // Rolling force relaxation velocity
  double SFRV;  // Side force relaxation velocity
  double LongForceLagFilterCoeff; // Longitudinal Force Lag Filter Coefficient
  double LatForceLagFilterCoeff; // Lateral Force Lag Filter Coefficient
  double WheelSlipLagFilterCoeff; // Wheel slip angle lag filter coefficient

  Filter LongForceFilter;
  Filter LatForceFilter;
  Filter WheelSlipFilter;

  FGFDMExec*     Exec;
  FGState*       State;
  FGAircraft*    Aircraft;
  FGPropagate*   Propagate;
  FGAuxiliary*   Auxiliary;
  FGFCS*         FCS;
  FGMassBalance* MassBalance;

  void ComputeRetractionState(void);
  void ComputeBrakeForceCoefficient(void);
  void ComputeSteeringAngle(void);
  void ComputeSlipAngle(void);
  void ComputeSideForceCoefficient(void);
  void ComputeVerticalStrutForce(void);
  void CrashDetect(void);
  void InitializeReporting(void);
  void ResetReporting(void);
  void ReportTakeoffOrLanding(void);
  void Report(ReportType rt);
  void Debug(int from);
};
}
#include "FGAircraft.h"
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGFCS.h"
#include "FGMassBalance.h"
#include "FGState.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
