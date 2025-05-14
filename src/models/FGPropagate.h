/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropagate.h
 Author:       Jon S. Berndt
 Date started: 1/5/99

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

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPAGATE_H
#define FGPROPAGATE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "models/FGModel.h"
#include "math/FGLocation.h"
#include "math/FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGInitialCondition;
class FGInertial;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the EOM and integration/propagation of state.
    The Equations of Motion (EOM) for JSBSim are integrated to propagate the
    state of the vehicle given the forces and moments that act on it. The
    integration accounts for a rotating Earth.

    Integration of rotational and translation position and rate can be
    customized as needed or frozen by the selection of no integrator. The
    selection of which integrator to use is done through the setting of
    the associated property. There are four properties which can be set:

    @code
    simulation/integrator/rate/rotational
    simulation/integrator/rate/translational
    simulation/integrator/position/rotational
    simulation/integrator/position/translational
    @endcode

    Each of the integrators listed above can be set to one of the following values:

    @code
    0: No integrator (Freeze)
    1: Rectangular Euler
    2: Trapezoidal
    3: Adams Bashforth 2
    4: Adams Bashforth 3
    5: Adams Bashforth 4
    @endcode

    @author Jon S. Berndt, Mathias Froehlich, Bertrand Coconnier
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGPropagate : public FGModel {
public:

  /** The current vehicle state vector structure contains the translational and
    angular position, and the translational and angular velocity. */
  struct VehicleState {
    /** Represents the current location of the vehicle in Earth centered Earth
        fixed (ECEF) frame.
        units ft */
    FGLocation vLocation;

    /** The velocity vector of the vehicle with respect to the ECEF frame,
        expressed in the body system.
        units ft/sec */
    FGColumnVector3 vUVW;

    /** The angular velocity vector for the vehicle relative to the ECEF frame,
        expressed in the body frame.
        units rad/sec */
    FGColumnVector3 vPQR;

    /** The angular velocity vector for the vehicle body frame relative to the
        ECI frame, expressed in the body frame.
        units rad/sec */
    FGColumnVector3 vPQRi;

    /** The current orientation of the vehicle, that is, the orientation of the
        body frame relative to the local, NED frame. */
    FGQuaternion qAttitudeLocal;

    /** The current orientation of the vehicle, that is, the orientation of the
        body frame relative to the inertial (ECI) frame. */
    FGQuaternion qAttitudeECI;

    FGQuaternion vQtrndot;

    FGColumnVector3 vInertialVelocity;

    FGColumnVector3 vInertialPosition;

    std::deque <FGColumnVector3> dqPQRidot;
    std::deque <FGColumnVector3> dqUVWidot;
    std::deque <FGColumnVector3> dqInertialVelocity;
    std::deque <FGQuaternion>    dqQtrndot;
  };

  /** Constructor.
      The constructor initializes several variables, and sets the initial set
      of integrators to use as follows:
      - integrator, rotational rate = Adams Bashforth 2
      - integrator, translational rate = Adams Bashforth 2
      - integrator, rotational position = Trapezoidal
      - integrator, translational position = Trapezoidal
      @param Executive a pointer to the parent executive object */
  explicit FGPropagate(FGFDMExec* Executive);

  /// Destructor
  ~FGPropagate();

  /// These define the indices use to select the various integrators.
  enum eIntegrateType {eNone = 0, eRectEuler, eTrapezoidal, eAdamsBashforth2,
                       eAdamsBashforth3, eAdamsBashforth4, eBuss1, eBuss2, eLocalLinearization, eAdamsBashforth5};

  /** Initializes the FGPropagate class after instantiation and prior to first execution.
      The base class FGModel::InitModel is called first, initializing pointers to the
      other FGModel objects (and others).  */
  bool InitModel(void);

  void InitializeDerivatives();

  /** Runs the state propagation model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);

  /** Retrieves the velocity vector.
      The vector returned is represented by an FGColumnVector reference. The vector
      for the velocity in Local frame is organized (Vnorth, Veast, Vdown). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vVel(1) is Vnorth. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eNorth=1, eEast=2, eDown=3.
      units ft/sec
      @return The vehicle velocity vector with respect to the Earth centered frame,
              expressed in Local horizontal frame.
  */
  const FGColumnVector3& GetVel(void) const { return vVel; }

  /** Retrieves the body frame vehicle velocity vector.
      The vector returned is represented by an FGColumnVector3 reference. The vector
      for the velocity in Body frame is organized (Vx, Vy, Vz). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vUVW(1) is Vx. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eX=1, eY=2, eZ=3.
      units ft/sec
      @return The body frame vehicle velocity vector in ft/sec.
  */
  const FGColumnVector3& GetUVW(void) const { return VState.vUVW; }

  /** Retrieves the body angular rates vector, relative to the ECEF frame.
      Retrieves the body angular rates (p, q, r), which are calculated by integration
      of the angular acceleration.
      The vector returned is represented by an FGColumnVector3 reference. The vector
      for the angular velocity in Body frame is organized (P, Q, R). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vPQR(1) is P. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eP=1, eQ=2, eR=3.
      units rad/sec
      @return The body frame angular rates in rad/sec.
  */
  const FGColumnVector3& GetPQR(void) const {return VState.vPQR;}

  /** Retrieves the body angular rates vector, relative to the ECI (inertial) frame.
      Retrieves the body angular rates (p, q, r), which are calculated by integration
      of the angular acceleration.
      The vector returned is represented by an FGColumnVector reference. The vector
      for the angular velocity in Body frame is organized (P, Q, R). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vPQR(1) is P. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eP=1, eQ=2, eR=3.
      units rad/sec
      @return The body frame inertial angular rates in rad/sec.
  */
  const FGColumnVector3& GetPQRi(void) const {return VState.vPQRi;}

  /** Retrieves the time derivative of the body orientation quaternion.
      Retrieves the time derivative of the body orientation quaternion based on
      the rate of change of the orientation between the body and the ECI frame.
      The quaternion returned is represented by an FGQuaternion reference. The
      quaternion is 1-based, so that the first element can be retrieved using
      the "()" operator.
      units rad/sec^2
      @return The time derivative of the body orientation quaternion.
  */
  const FGQuaternion& GetQuaterniondot(void) const {return VState.vQtrndot;}

  /** Retrieves the Euler angles that define the vehicle orientation.
      Extracts the Euler angles from the quaternion that stores the orientation
      in the Local frame. The order of rotation used is Yaw-Pitch-Roll. The
      vector returned is represented by an FGColumnVector reference. The vector
      for the Euler angles is organized (Phi, Theta, Psi). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, the returned vector item with subscript (1) is Phi.
      Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the vector returned by this call are, ePhi=1, eTht=2, ePsi=3.
      units radians
      @return The Euler angle vector, where the first item in the
              vector is the angle about the X axis, the second is the
              angle about the Y axis, and the third item is the angle
              about the Z axis (Phi, Theta, Psi).
  */
  const FGColumnVector3& GetEuler(void) const { return VState.qAttitudeLocal.GetEuler(); }

  /** Retrieves the Euler angles (in degrees) that define the vehicle orientation.
      Extracts the Euler angles from the quaternion that stores the orientation
      in the Local frame. The order of rotation used is Yaw-Pitch-Roll. The
      vector returned is represented by an FGColumnVector reference. The vector
      for the Euler angles is organized (Phi, Theta, Psi). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, the returned vector item with subscript (1) is Phi.
      Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the vector returned by this call are, ePhi=1, eTht=2, ePsi=3.
      units degrees
      @return The Euler angle vector, where the first item in the
              vector is the angle about the X axis, the second is the
              angle about the Y axis, and the third item is the angle
              about the Z axis (Phi, Theta, Psi).
  */
  FGColumnVector3 GetEulerDeg(void) const;

  /** Retrieves a body frame velocity component.
      Retrieves a body frame velocity component. The velocity returned is
      extracted from the vUVW vector (an FGColumnVector). The vector for the
      velocity in Body frame is organized (Vx, Vy, Vz). The vector is 1-based.
      In other words, GetUVW(1) returns Vx. Various convenience enumerators
      are defined in FGJSBBase. The relevant enumerators for the velocity
      returned by this call are, eX=1, eY=2, eZ=3.
      units ft/sec
      @param idx the index of the velocity component desired (1-based).
      @return The body frame velocity component.
  */
  double GetUVW(int idx) const { return VState.vUVW(idx); }

  /** Retrieves a Local frame velocity component.
      Retrieves a Local frame velocity component. The velocity returned is
      extracted from the vVel vector (an FGColumnVector). The vector for the
      velocity in Local frame is organized (Vnorth, Veast, Vdown). The vector
      is 1-based. In other words, GetVel(1) returns Vnorth. Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      velocity returned by this call are, eNorth=1, eEast=2, eDown=3.
      units ft/sec
      @param idx the index of the velocity component desired (1-based).
      @return The body frame velocity component.
  */
  double GetVel(int idx) const { return vVel(idx); }

  /** Retrieves the total inertial velocity in ft/sec.
  */
  double GetInertialVelocityMagnitude(void) const { return VState.vInertialVelocity.Magnitude(); }

  /** Retrieves the total local NED velocity in ft/sec.
  */
  double GetNEDVelocityMagnitude(void) const { return VState.vUVW.Magnitude(); }

  /** Retrieves the inertial velocity vector in ft/sec.
  */
  const FGColumnVector3& GetInertialVelocity(void) const { return VState.vInertialVelocity; }
  double GetInertialVelocity(int i) const { return VState.vInertialVelocity(i); }

  /** Retrieves the inertial position vector.
  */
  const FGColumnVector3& GetInertialPosition(void) const { return VState.vInertialPosition; }
  double GetInertialPosition(int i) const { return VState.vInertialPosition(i); }

  /** Calculates and retrieves the velocity vector relative to the earth centered earth fixed (ECEF) frame.
  */
  FGColumnVector3 GetECEFVelocity(void) const {return Tb2ec * VState.vUVW; }

  /** Calculates and retrieves the velocity vector relative to the earth centered earth fixed (ECEF) frame
      for a particular axis.
  */
  double GetECEFVelocity(int idx) const {return (Tb2ec * VState.vUVW)(idx); }

  /** Returns the current altitude above sea level.
      This function returns the altitude above sea level.
      units ft
      @return The current altitude above sea level in feet.
  */
  double GetAltitudeASL(void) const;

  /** Returns the current altitude above sea level.
      This function returns the altitude above sea level.
      units meters
      @return The current altitude above sea level in meters.
  */
  double GetAltitudeASLmeters(void) const { return GetAltitudeASL()*fttom;}

  /** Retrieves a body frame angular velocity component relative to the ECEF frame.
      Retrieves a body frame angular velocity component. The angular velocity
      returned is extracted from the vPQR vector (an FGColumnVector). The vector
      for the angular velocity in Body frame is organized (P, Q, R). The vector
      is 1-based. In other words, GetPQR(1) returns P (roll rate). Various
      convenience enumerators are defined in FGJSBBase. The relevant enumerators
      for the angular velocity returned by this call are, eP=1, eQ=2, eR=3.
      units rad/sec
      @param axis the index of the angular velocity component desired (1-based).
      @return The body frame angular velocity component.
  */
  double GetPQR(int axis) const {return VState.vPQR(axis);}

  /** Retrieves a body frame angular velocity component relative to the ECI (inertial) frame.
      Retrieves a body frame angular velocity component. The angular velocity
      returned is extracted from the vPQR vector (an FGColumnVector). The vector
      for the angular velocity in Body frame is organized (P, Q, R). The vector
      is 1-based. In other words, GetPQR(1) returns P (roll rate). Various
      convenience enumerators are defined in FGJSBBase. The relevant enumerators
      for the angular velocity returned by this call are, eP=1, eQ=2, eR=3.
      units rad/sec
      @param axis the index of the angular velocity component desired (1-based).
      @return The body frame angular velocity component.
  */
  double GetPQRi(int axis) const {return VState.vPQRi(axis);}

  /** Retrieves a vehicle Euler angle component.
      Retrieves an Euler angle (Phi, Theta, or Psi) from the quaternion that
      stores the vehicle orientation relative to the Local frame. The order of
      rotations used is Yaw-Pitch-Roll. The Euler angle with subscript (1) is
      Phi. Various convenience enumerators are defined in FGJSBBase. The
      relevant enumerators for the Euler angle returned by this call are,
      ePhi=1, eTht=2, ePsi=3 (e.g. GetEuler(eTht) returns Theta).
      units radians
      @return An Euler angle.
  */
  double GetEuler(int axis) const { return VState.qAttitudeLocal.GetEuler(axis); }

  /** Retrieves a vehicle Euler angle component in degrees.
      Retrieves an Euler angle (Phi, Theta, or Psi) from the quaternion that
      stores the vehicle orientation relative to the Local frame. The order of
      rotations used is Yaw-Pitch-Roll. The Euler angle with subscript (1) is
      Phi. Various convenience enumerators are defined in FGJSBBase. The
      relevant enumerators for the Euler angle returned by this call are,
      ePhi=1, eTht=2, ePsi=3 (e.g. GetEuler(eTht) returns Theta).
      units degrees
      @return An Euler angle in degrees.
  */
  double GetEulerDeg(int axis) const { return VState.qAttitudeLocal.GetEuler(axis) * radtodeg; }

  /** Retrieves the cosine of a vehicle Euler angle component.
      Retrieves the cosine of an Euler angle (Phi, Theta, or Psi) from the
      quaternion that stores the vehicle orientation relative to the Local frame.
      The order of rotations used is Yaw-Pitch-Roll. The Euler angle
      with subscript (1) is Phi. Various convenience enumerators are defined in
      FGJSBBase. The relevant enumerators for the Euler angle referred to in this
      call are, ePhi=1, eTht=2, ePsi=3 (e.g. GetCosEuler(eTht) returns cos(theta)).
      units none
      @return The cosine of an Euler angle.
  */
  double GetCosEuler(int idx) const { return VState.qAttitudeLocal.GetCosEuler(idx); }

  /** Retrieves the sine of a vehicle Euler angle component.
      Retrieves the sine of an Euler angle (Phi, Theta, or Psi) from the
      quaternion that stores the vehicle orientation relative to the Local frame.
      The order of rotations used is Yaw-Pitch-Roll. The Euler angle
      with subscript (1) is Phi. Various convenience enumerators are defined in
      FGJSBBase. The relevant enumerators for the Euler angle referred to in this
      call are, ePhi=1, eTht=2, ePsi=3 (e.g. GetSinEuler(eTht) returns sin(theta)).
      units none
      @return The sine of an Euler angle.
  */
  double GetSinEuler(int idx) const { return VState.qAttitudeLocal.GetSinEuler(idx); }

  /** Returns the current altitude rate.
      Returns the current altitude rate (rate of climb).
      units ft/sec
      @return The current rate of change in altitude.
  */
  double Gethdot(void) const { return -vVel(eDown); }

  /** Returns the "constant" LocalTerrainRadius.
      The LocalTerrainRadius parameter is set by the calling application or set to
      sea level + terrain elevation if JSBSim is running in standalone mode.
      units feet
      @return distance of the local terrain from the center of the earth.
      */
  double GetLocalTerrainRadius(void) const;

  /** Returns the Earth position angle.
      @return Earth position angle in radians.
   */
  double GetEarthPositionAngle(void) const { return epa; }

  /** Returns the Earth position angle in degrees.
      @return Earth position angle in degrees.
  */
  double GetEarthPositionAngleDeg(void) const { return epa*radtodeg;}

  const FGColumnVector3& GetTerrainVelocity(void) const { return LocalTerrainVelocity; }
  const FGColumnVector3& GetTerrainAngularVelocity(void) const { return LocalTerrainAngularVelocity; }
  void RecomputeLocalTerrainVelocity();

  double GetTerrainElevation(void) const;
  double GetDistanceAGL(void)  const;
  double GetDistanceAGLKm(void)  const;
  double GetRadius(void) const {
      if (VState.vLocation.GetRadius() == 0) return 1.0;
      else return VState.vLocation.GetRadius();
  }
  double GetLongitude(void) const { return VState.vLocation.GetLongitude(); }
  double GetLatitude(void) const { return VState.vLocation.GetLatitude(); }

  double GetGeodLatitudeRad(void) const { return VState.vLocation.GetGeodLatitudeRad(); }
  double GetGeodLatitudeDeg(void) const { return VState.vLocation.GetGeodLatitudeDeg(); }

  double GetGeodeticAltitude(void) const { return VState.vLocation.GetGeodAltitude(); }
  double GetGeodeticAltitudeKm(void) const { return VState.vLocation.GetGeodAltitude()*0.0003048; }

  double GetLongitudeDeg(void) const { return VState.vLocation.GetLongitudeDeg(); }
  double GetLatitudeDeg(void) const { return VState.vLocation.GetLatitudeDeg(); }
  const FGLocation& GetLocation(void) const { return VState.vLocation; }
  double GetLocation(int i) const { return VState.vLocation(i); }

  /** Retrieves the local-to-body transformation matrix.
      The quaternion class, being the means by which the orientation of the
      vehicle is stored, manages the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.  */
  const FGMatrix33& GetTl2b(void) const { return Tl2b; }

  /** Retrieves the body-to-local transformation matrix.
      The quaternion class, being the means by which the orientation of the
      vehicle is stored, manages the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.  */
  const FGMatrix33& GetTb2l(void) const { return Tb2l; }

  /** Retrieves the ECEF-to-body transformation matrix.
      @return a reference to the ECEF-to-body transformation matrix.  */
  const FGMatrix33& GetTec2b(void) const { return Tec2b; }

  /** Retrieves the body-to-ECEF transformation matrix.
      @return a reference to the body-to-ECEF matrix.  */
  const FGMatrix33& GetTb2ec(void) const { return Tb2ec; }

  /** Retrieves the ECI-to-body transformation matrix.
      @return a reference to the ECI-to-body transformation matrix.  */
  const FGMatrix33& GetTi2b(void) const { return Ti2b; }

  /** Retrieves the body-to-ECI transformation matrix.
      @return a reference to the body-to-ECI matrix.  */
  const FGMatrix33& GetTb2i(void) const { return Tb2i; }

  /** Retrieves the ECEF-to-ECI transformation matrix.
      @return a reference to the ECEF-to-ECI transformation matrix.
      @see SetEarthPositionAngle */
  const FGMatrix33& GetTec2i(void) const { return Tec2i; }

  /** Retrieves the ECI-to-ECEF transformation matrix.
      @return a reference to the ECI-to-ECEF matrix.
      @see SetEarthPositionAngle */
  const FGMatrix33& GetTi2ec(void) const { return Ti2ec; }

  /** Retrieves the ECEF-to-local transformation matrix.
      Retrieves the ECEF-to-local transformation matrix. Note that the so-called
      local from is also know as the NED frame (for North, East, Down).
      @return a reference to the ECEF-to-local matrix.  */
  const FGMatrix33& GetTec2l(void) const { return Tec2l; }

  /** Retrieves the local-to-ECEF transformation matrix.
      Retrieves the local-to-ECEF transformation matrix. Note that the so-called
      local from is also know as the NED frame (for North, East, Down).
      @return a reference to the local-to-ECEF matrix.  */
  const FGMatrix33& GetTl2ec(void) const { return Tl2ec; }

  /** Retrieves the local-to-inertial transformation matrix.
      @return a reference to the local-to-inertial transformation matrix.
      @see SetEarthPositionAngle */
  const FGMatrix33& GetTl2i(void) const { return Tl2i; }

  /** Retrieves the inertial-to-local transformation matrix.
      @return a reference to the inertial-to-local matrix.
      @see SetEarthPositionAngle */
  const FGMatrix33& GetTi2l(void) const { return Ti2l; }

  const VehicleState& GetVState(void) const { return VState; }

  void SetVState(const VehicleState& vstate);

  /** Sets the Earth position angle.
      This is the relative angle around the Z axis of the ECEF frame with
      respect to the inertial frame.
      @param EPA Earth position angle in radians.
   */
  void SetEarthPositionAngle(double EPA) {epa = EPA;}

  void SetInertialOrientation(const FGQuaternion& Qi);
  void SetInertialVelocity(const FGColumnVector3& Vi);
  void SetInertialRates(const FGColumnVector3& vRates);

  /** Returns the quaternion that goes from Local to Body. */
  const FGQuaternion GetQuaternion(void) const { return VState.qAttitudeLocal; }

  /** Returns the quaternion that goes from ECI to Body. */
  const FGQuaternion GetQuaternionECI(void) const { return VState.qAttitudeECI; }

  /** Returns the quaternion that goes from ECEF to Body. */
  const FGQuaternion GetQuaternionECEF(void) const { return Qec2b; }

  void SetPQR(unsigned int i, double val) {
    VState.vPQR(i) = val;
    VState.vPQRi = VState.vPQR + Ti2b * in.vOmegaPlanet;
  }

  void SetUVW(unsigned int i, double val) {
    VState.vUVW(i) = val;
    CalculateInertialVelocity();
  }

// SET functions

  void SetLongitude(double lon)
  {
    VState.vLocation.SetLongitude(lon);
    UpdateVehicleState();
  }
  void SetLongitudeDeg(double lon) { SetLongitude(lon*degtorad); }
  void SetLatitude(double lat)
  {
    VState.vLocation.SetLatitude(lat);
    UpdateVehicleState();
  }
  void SetLatitudeDeg(double lat) { SetLatitude(lat*degtorad); }
  void SetRadius(double r)
  {
    VState.vLocation.SetRadius(r);
    VState.vInertialPosition = Tec2i * VState.vLocation;
  }

  void SetAltitudeASL(double altASL);
  void SetAltitudeASLmeters(double altASL) { SetAltitudeASL(altASL/fttom); }

  void SetTerrainElevation(double tt);
  void SetDistanceAGL(double tt);
  void SetDistanceAGLKm(double tt);

  void SetInitialState(const FGInitialCondition*);
  void SetLocation(const FGLocation& l);
  void SetLocation(const FGColumnVector3& lv)
  {
      FGLocation l = FGLocation(lv);
      SetLocation(l);
  }
  void SetPosition(const double Lon, const double Lat, const double Radius)
  {
      FGLocation l = FGLocation(Lon, Lat, Radius);
      SetLocation(l);
  }

  void NudgeBodyLocation(const FGColumnVector3& deltaLoc) {
    VState.vInertialPosition -= Tb2i*deltaLoc;
    VState.vLocation -= Tb2ec*deltaLoc;
  }

  /** Sets the property forces/hold-down. This allows to do hard 'hold-down'
      such as for rockets on a launch pad with engines ignited.
      @param hd enables the 'hold-down' function if non-zero
  */
  void SetHoldDown(bool hd);

  void DumpState(void);

  struct Inputs {
    FGColumnVector3 vPQRidot;
    FGColumnVector3 vUVWidot;
    FGColumnVector3 vOmegaPlanet;
    double SemiMajor;
    double SemiMinor;
    double GM; // Gravitational parameter
    double DeltaT;
  } in;

private:

// state vector

  struct VehicleState VState;

  std::shared_ptr<FGInertial> Inertial;
  FGColumnVector3 vVel;
  FGMatrix33 Tec2b;
  FGMatrix33 Tb2ec;
  FGMatrix33 Tl2b;   // local to body frame matrix copy for immediate local use
  FGMatrix33 Tb2l;   // body to local frame matrix copy for immediate local use
  FGMatrix33 Tl2ec;  // local to ECEF matrix copy for immediate local use
  FGMatrix33 Tec2l;  // ECEF to local frame matrix copy for immediate local use
  FGMatrix33 Tec2i;  // ECEF to ECI frame matrix copy for immediate local use
  FGMatrix33 Ti2ec;  // ECI to ECEF frame matrix copy for immediate local use
  FGMatrix33 Ti2b;   // ECI to body frame rotation matrix
  FGMatrix33 Tb2i;   // body to ECI frame rotation matrix
  FGMatrix33 Ti2l;
  FGMatrix33 Tl2i;
  double epa;        // Earth Position Angle

  // Orbital parameters
  double h;               // Specific angular momentum
  double Inclination;     // Inclination (angle between the orbital plane and the equatorial plane)
  double RightAscension;  // Right ascension of the ascending node
  double Eccentricity;    // Eccentricity
  double PerigeeArgument; // Argument of perigee (angle between the apsis line and the node line)
  double TrueAnomaly;     // True anomaly (angle of the vehicule from the apsis line)
  double ApoapsisRadius;  // Apoapsis radius (farthest point from the planet)
  double PeriapsisRadius; // Periapsis radius (closest point to the planet)
  double OrbitalPeriod;   // Period of elliptic orbits

  FGQuaternion Qec2b;

  FGColumnVector3 LocalTerrainVelocity, LocalTerrainAngularVelocity;

  eIntegrateType integrator_rotational_rate;
  eIntegrateType integrator_translational_rate;
  eIntegrateType integrator_rotational_position;
  eIntegrateType integrator_translational_position;

  void CalculateInertialVelocity(void);
  void CalculateUVW(void);
  void CalculateQuatdot(void);

  void Integrate( FGColumnVector3& Integrand,
                  FGColumnVector3& Val,
                  std::deque <FGColumnVector3>& ValDot,
                  double dt,
                  eIntegrateType integration_type);

  void Integrate( FGQuaternion& Integrand,
                  FGQuaternion& Val,
                  std::deque <FGQuaternion>& ValDot,
                  double dt,
                  eIntegrateType integration_type);

  void UpdateLocationMatrices(void);
  void UpdateBodyMatrices(void);
  void UpdateVehicleState(void);
  void ComputeOrbitalParameters(void);

  void WriteStateFile(int num);
  void bind(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
