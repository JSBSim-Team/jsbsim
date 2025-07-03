/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAccelerations.h
 Author:       Jon S. Berndt
 Date started: 07/12/11

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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
07/12/11   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGACCELERATIONS_H
#define FGACCELERATIONS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"
#include "math/FGColumnVector3.h"
#include "math/LagrangeMultiplier.h"
#include "math/FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Handles the calculation of accelerations.

    - Calculate the angular accelerations
    - Calculate the translational accelerations

    This class is collecting all the forces and the moments acting on the body
    to calculate the corresponding accelerations according to Newton's second
    law. This is also where the friction forces related to the ground reactions
    are evaluated.

    JSBSim provides several ways to calculate the influence of the gravity on
    the vehicle. The different options can be selected via the following
    properties :
    @property simulation/gravity-model (read/write) Selects the gravity model.
              Two options are available : 0 (Standard gravity assuming the Earth
              is spherical) or 1 (WGS84 gravity taking the Earth oblateness into
              account). WGS84 gravity is the default.
    @property simulation/gravitational-torque (read/write) Enables/disables the
              calculations of the gravitational torque on the vehicle. This is
              mainly relevant for spacecrafts that are orbiting at low altitudes.
              Gravitational torque calculations are disabled by default.

    Special care is taken in the calculations to obtain maximum fidelity in
    JSBSim results. In FGAccelerations, this is obtained by avoiding as much as
    possible the transformations from one frame to another. As a consequence,
    the frames in which the accelerations are primarily evaluated are dictated
    by the frames in which FGPropagate resolves the equations of movement (the
    ECI frame for the translations and the body frame for the rotations).

    @see Mark Harris and Robert Lyle, "Spacecraft Gravitational Torques",
         NASA SP-8024, May 1969

    @author Jon S. Berndt, Mathias Froehlich, Bertrand Coconnier
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAccelerations : public FGModel {
public:
  /** Constructor.
      @param Executive a pointer to the parent executive object */
  explicit FGAccelerations(FGFDMExec* Executive);

  /// Destructor
  ~FGAccelerations();

  /** Initializes the FGAccelerations class after instantiation and prior to first execution.
      The base class FGModel::InitModel is called first, initializing pointers to the
      other FGModel objects (and others).  */
  bool InitModel(void) override;

  /** Runs the state propagation model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  /** Retrieves the body axis acceleration.
      Retrieves the computed body axis accelerations based on the
      applied forces and accounting for a rotating body frame.
      The vector returned is represented by an FGColumnVector3 reference. The vector
      for the acceleration in Body frame is organized (Ax, Ay, Az). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vUVWdot(1) is Ax. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eX=1, eY=2, eZ=3.
      units ft/sec^2
      @return Body axis translational acceleration in ft/sec^2.
  */
  const FGColumnVector3& GetUVWdot(void) const { return vUVWdot; }

  /** Retrieves the body axis acceleration in the ECI frame.
      Retrieves the computed body axis accelerations based on the applied forces.
      The ECI frame being an inertial frame this vector does not contain the
      Coriolis and centripetal accelerations. The vector is expressed in the
      Body frame.
      The vector returned is represented by an FGColumnVector3 reference. The
      vector for the acceleration in Body frame is organized (Aix, Aiy, Aiz). The
      vector is 1-based, so that the first element can be retrieved using the
      "()" operator. In other words, vUVWidot(1) is Aix. Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      vector returned by this call are, eX=1, eY=2, eZ=3.
      units ft/sec^2
      @return Body axis translational acceleration in ft/sec^2.
  */
  const FGColumnVector3& GetUVWidot(void) const { return vUVWidot; }
  double GetUVWidot(int idx) const { return vUVWidot(idx); }

  /** Retrieves the body axis angular acceleration vector.
      Retrieves the body axis angular acceleration vector in rad/sec^2. The
      angular acceleration vector is determined from the applied moments and
      accounts for a rotating frame.
      The vector returned is represented by an FGColumnVector3 reference. The vector
      for the angular acceleration in Body frame is organized (Pdot, Qdot, Rdot). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vPQRdot(1) is Pdot. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eP=1, eQ=2, eR=3.
      units rad/sec^2
      @return The angular acceleration vector.
  */
  const FGColumnVector3& GetPQRdot(void) const {return vPQRdot;}

  /** Retrieves the axis angular acceleration vector in the ECI frame.
      Retrieves the body axis angular acceleration vector measured in the ECI
      frame and expressed in the body frame. The angular acceleration vector is
      determined from the applied moments.
      The vector returned is represented by an FGColumnVector3 reference. The
      vector for the angular acceleration in Body frame is organized (Pidot,
      Qidot, Ridot). The vector is 1-based, so that the first element can be
      retrieved using the "()" operator. In other words, vPQRidot(1) is Pidot.
      Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the vector returned by this call are, eP=1, eQ=2, eR=3.
      units rad/sec^2
      @return The angular acceleration vector.
  */
  const FGColumnVector3& GetPQRidot(void) const {return vPQRidot;}
  double GetPQRidot(int idx) const { return vPQRidot(idx); }

  /** Retrieves a body frame acceleration component.
      Retrieves a body frame acceleration component. The acceleration returned
      is extracted from the vUVWdot vector (an FGColumnVector3). The vector for
      the acceleration in Body frame is organized (Ax, Ay, Az). The vector is
      1-based. In other words, GetUVWdot(1) returns Ax. Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      acceleration returned by this call are, eX=1, eY=2, eZ=3.
      units ft/sec^2
      @param idx the index of the acceleration component desired (1-based).
      @return The body frame acceleration component.
  */
  double GetUVWdot(int idx) const { return vUVWdot(idx); }

  /** Retrieves the acceleration resulting from the applied forces.
      Retrieves the ratio of the sum of all forces applied on the craft to its
      mass. This does include the friction forces but not the gravity.
      The vector returned is represented by an FGColumnVector3 reference. The
      vector for the acceleration in Body frame is organized (Ax, Ay, Az). The
      vector is 1-based, so that the first element can be retrieved using the
      "()" operator. In other words, vBodyAccel(1) is Ax. Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      vector returned by this call are, eX=1, eY=2, eZ=3.
      units ft/sec^2
      @return The acceleration resulting from the applied forces.
  */
  const FGColumnVector3& GetBodyAccel(void) const { return vBodyAccel; }

  double GetGravAccelMagnitude(void) const { return in.vGravAccel.Magnitude(); }

  /** Retrieves a component of the acceleration resulting from the applied forces.
      Retrieves a component of the ratio between the sum of all forces applied
      on the craft to its mass. The value returned is extracted from the vBodyAccel
      vector (an FGColumnVector3). The vector for the acceleration in Body frame
      is organized (Ax, Ay, Az). The vector is 1-based. In other words,
      GetBodyAccel(1) returns Ax. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this
      call are, eX=1, eY=2, eZ=3.
      units ft/sec^2
      @param idx the index of the acceleration component desired (1-based).
      @return The component of the acceleration resulting from the applied forces.
  */
  double GetBodyAccel(int idx) const { return vBodyAccel(idx); }

  /** Retrieves a body frame angular acceleration component.
      Retrieves a body frame angular acceleration component. The angular
      acceleration returned is extracted from the vPQRdot vector (an
      FGColumnVector3). The vector for the angular acceleration in Body frame
      is organized (Pdot, Qdot, Rdot). The vector is 1-based. In other words,
      GetPQRdot(1) returns Pdot (roll acceleration). Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      angular acceleration returned by this call are, eP=1, eQ=2, eR=3.
      units rad/sec^2
      @param axis the index of the angular acceleration component desired (1-based).
      @return The body frame angular acceleration component.
  */
  double GetPQRdot(int axis) const {return vPQRdot(axis);}

  /** Retrieves a component of the total moments applied on the body.
      Retrieves a component of the total moments applied on the body. This does
      include the moments generated by friction forces and the gravitational
      torque (if the property \e simulation/gravitational-torque is set to true).
      The vector for the total moments in the body frame is organized (Mx, My
      , Mz). The vector is 1-based. In other words, GetMoments(1) returns Mx.
      Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the moments returned by this call are, eX=1, eY=2, eZ=3.
      units lbs*ft
      @param idx the index of the moments component desired (1-based).
      @return The total moments applied on the body.
   */
  double GetMoments(int idx) const { return in.Moment(idx) + vFrictionMoments(idx); }
  FGColumnVector3 GetMoments(void) const { return in.Moment + vFrictionMoments; }

  /** Retrieves the total forces applied on the body.
      Retrieves the total forces applied on the body. This does include the
      friction forces but not the gravity.
      The vector for the total forces in the body frame is organized (Fx, Fy
      , Fz). The vector is 1-based. In other words, GetForces(1) returns Fx.
      Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the forces returned by this call are, eX=1, eY=2, eZ=3.
      units lbs
      @param idx the index of the forces component desired (1-based).
      @return The total forces applied on the body.
   */
  double GetForces(int idx) const { return in.Force(idx) + vFrictionForces(idx); }
  FGColumnVector3 GetForces(void) const { return in.Force + vFrictionForces; }

  /** Retrieves the ground moments applied on the body.
      Retrieves the ground moments applied on the body. This does include the
      ground normal reaction and friction moments.
      The vector for the ground moments in the body frame is organized (Mx, My
      , Mz). The vector is 1-based. In other words, GetGroundMoments(1) returns
      Mx. Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the moments returned by this call are, eX=1, eY=2, eZ=3.
      units lbs*ft
      @param idx the index of the moments component desired (1-based).
      @return The ground moments applied on the body.
   */
  double GetGroundMoments(int idx) const { return in.GroundMoment(idx) + vFrictionMoments(idx); }
  FGColumnVector3 GetGroundMoments(void) const { return in.GroundMoment + vFrictionMoments; }

  /** Retrieves the ground forces applied on the body.
      Retrieves the ground forces applied on the body. This does include the
      ground normal reaction and friction forces.
      The vector for the ground forces in the body frame is organized (Fx, Fy
      , Fz). The vector is 1-based. In other words, GetGroundForces(1) returns
      Fx. Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the forces returned by this call are, eX=1, eY=2, eZ=3.
      units lbs.
      @param idx the index of the forces component desired (1-based).
      @return The ground forces applied on the body.
   */
  double GetGroundForces(int idx) const { return in.GroundForce(idx) + vFrictionForces(idx); }
  FGColumnVector3 GetGroundForces(void) const { return in.GroundForce + vFrictionForces; }

  /** Retrieves the weight applied on the body.
      Retrieves the weight applied on the body i.e. the force that results from
      the gravity applied to the body mass.
      The vector for the weight forces in the body frame is organized (Fx, Fy ,
      Fz). The vector is 1-based. In other words, GetWeight(1) returns
      Fx. Various convenience enumerators are defined in FGJSBBase. The relevant
      enumerators for the forces returned by this call are, eX=1, eY=2, eZ=3.
      units lbs.
      @param idx the index of the forces component desired (1-based).
      @return The ground forces applied on the body.
  */
  double GetWeight(int idx) const { return in.Mass * (in.Tec2b * in.vGravAccel)(idx); }
  FGColumnVector3 GetWeight(void) const { return in.Mass * in.Tec2b * in.vGravAccel; }

  /** Initializes the FGAccelerations class prior to a new execution.
      Initializes the class prior to a new execution when the input data stored
      in the Inputs structure have been set to their initial values.
   */
  void InitializeDerivatives(void);

  /** Sets the property forces/hold-down. This allows to do hard 'hold-down'
      such as for rockets on a launch pad with engines ignited.
      @param hd enables the 'hold-down' function if non-zero
  */
  void SetHoldDown(bool hd);

  struct Inputs {
    /// The body inertia matrix expressed in the body frame
    FGMatrix33 J;
    /// The inverse of the inertia matrix J
    FGMatrix33 Jinv;
    /// Transformation matrix from the ECI to the Body frame
    FGMatrix33 Ti2b;
    /// Transformation matrix from the Body to the ECI frame
    FGMatrix33 Tb2i;
    /// Transformation matrix from the ECEF to the Body frame
    FGMatrix33 Tec2b;
    /// Transformation matrix from the ECEF to the ECI frame
    FGMatrix33 Tec2i;
    /// Total moments applied to the body except friction and gravity (expressed in the body frame)
    FGColumnVector3 Moment;
    /// Moments generated by the ground normal reactions expressed in the body frame. Does not account for friction.
    FGColumnVector3 GroundMoment;
    /// Total forces applied to the body except friction and gravity (expressed in the body frame)
    FGColumnVector3 Force;
    /// Forces generated by the ground normal reactions expressed in the body frame. Does not account for friction.
    FGColumnVector3 GroundForce;
    /// Gravity intensity vector (expressed in the ECEF frame).
    FGColumnVector3 vGravAccel;
    /// Angular velocities of the body with respect to the ECI frame (expressed in the body frame).
    FGColumnVector3 vPQRi;
    /// Angular velocities of the body with respect to the local frame (expressed in the body frame).
    FGColumnVector3 vPQR;
    /// Velocities of the body with respect to the local frame (expressed in the body frame).
    FGColumnVector3 vUVW;
    /// Body position (X,Y,Z) measured in the ECI frame.
    FGColumnVector3 vInertialPosition;
    /// Earth rotating vector (expressed in the ECI frame).
    FGColumnVector3 vOmegaPlanet;
    /// Terrain velocities with respect to the local frame (expressed in the ECEF frame).
    FGColumnVector3 TerrainVelocity;
    /// Terrain angular velocities with respect to the local frame (expressed in the ECEF frame).
    FGColumnVector3 TerrainAngularVel;
    /// Time step
    double DeltaT;
    /// Body mass
    double Mass;
    /// List of Lagrange multipliers set by FGLGear for friction forces calculations.
    std::vector<LagrangeMultiplier*> *MultipliersList;
  } in;

private:

  FGColumnVector3 vPQRdot, vPQRidot;
  FGColumnVector3 vUVWdot, vUVWidot;
  FGColumnVector3 vBodyAccel;
  FGColumnVector3 vFrictionForces;
  FGColumnVector3 vFrictionMoments;

  bool gravTorque;

  void CalculatePQRdot(void);
  void CalculateUVWdot(void);

  void CalculateFrictionForces(double dt);

  void bind(void);
  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
