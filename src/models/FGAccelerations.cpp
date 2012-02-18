/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAccelerations.cpp
 Author:       Jon S. Berndt
 Date started: 07/12/11
 Purpose:      Calculates derivatives of rotational and translational rates, and
               of the attitude quaternion.
 Called by:    FGFDMExec

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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
This class encapsulates the calculation of the derivatives of the state vectors
UVW and PQR - the translational and rotational rates relative to the planet
fixed frame. The derivatives relative to the inertial frame are also calculated
as a side effect. Also, the derivative of the attitude quaterion is also calculated.

HISTORY
--------------------------------------------------------------------------------
07/12/11   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1] Stevens and Lewis, "Aircraft Control and Simulation", Second edition (2004)
    Wiley
[2] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
[3] Erin Catto, "Iterative Dynamics with Temporal Coherence", February 22, 2005
[4] Mark Harris and Robert Lyle, "Spacecraft Gravitational Torques",
    NASA SP-8024, May 1969

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAccelerations.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGAccelerations.cpp,v 1.13 2012/02/18 19:11:37 bcoconni Exp $";
static const char *IdHdr = ID_ACCELERATIONS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAccelerations::FGAccelerations(FGFDMExec* fdmex)
  : FGModel(fdmex)
{
  Debug(0);
  Name = "FGAccelerations";
  gravType = gtWGS84;
  gravTorque = false;

  vPQRidot.InitMatrix();
  vUVWidot.InitMatrix();
  vGravAccel.InitMatrix();
  vBodyAccel.InitMatrix();
  vQtrndot = FGQuaternion(0,0,0);

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAccelerations::~FGAccelerations(void)
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAccelerations::InitModel(void)
{
  vPQRidot.InitMatrix();
  vUVWidot.InitMatrix();
  vGravAccel.InitMatrix();
  vBodyAccel.InitMatrix();
  vQtrndot = FGQuaternion(0,0,0);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to calculate derivatives.
*/

bool FGAccelerations::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;  // Fast return if we have nothing to do ...
  if (Holding) return false;

  CalculatePQRdot();   // Angular rate derivative
  CalculateUVWdot();   // Translational rate derivative
  CalculateQuatdot();  // Angular orientation derivative

  ResolveFrictionForces(in.DeltaT * rate);  // Update rate derivatives with friction forces

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute body frame rotational accelerations based on the current body moments
//
// vPQRdot is the derivative of the absolute angular velocity of the vehicle
// (body rate with respect to the ECEF frame), expressed in the body frame,
// where the derivative is taken in the body frame.
// J is the inertia matrix
// Jinv is the inverse inertia matrix
// vMoments is the moment vector in the body frame
// in.vPQRi is the total inertial angular velocity of the vehicle
// expressed in the body frame.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation",
//            Second edition (2004), eqn 1.5-16e (page 50)

void FGAccelerations::CalculatePQRdot(void)
{
  if (gravTorque) {
    // Compute the gravitational torque
    // Reference: See Harris and Lyle "Spacecraft Gravitational Torques",
    //            NASA SP-8024 (1969) eqn (2) (page 7)
    FGColumnVector3 R = in.Ti2b * in.vInertialPosition;
    double invRadius = 1.0 / R.Magnitude();
    R *= invRadius;
    in.Moment += (3.0 * in.GAccel * invRadius) * (R * (in.J * R));
  }

  // Compute body frame rotational accelerations based on the current body
  // moments and the total inertial angular velocity expressed in the body
  // frame.

  vPQRidot = in.Jinv * (in.Moment - in.vPQRi * (in.J * in.vPQRi));
  vPQRdot = vPQRidot - in.vPQRi * (in.Ti2b * in.vOmegaPlanet);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the quaternion orientation derivative
//
// vQtrndot is the quaternion derivative.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation",
//            Second edition (2004), eqn 1.5-16b (page 50)

void FGAccelerations::CalculateQuatdot(void)
{
  // Compute quaternion orientation derivative on current body rates
  vQtrndot = in.qAttitudeECI.GetQDot(in.vPQRi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This set of calculations results in the body and inertial frame accelerations
// being computed.
// Compute body and inertial frames accelerations based on the current body
// forces including centripetal and Coriolis accelerations for the former.
// in.vOmegaPlanet is the Earth angular rate - expressed in the inertial frame -
//   so it has to be transformed to the body frame. More completely,
//   in.vOmegaPlanet is the rate of the ECEF frame relative to the Inertial
//   frame (ECI), expressed in the Inertial frame.
// in.Force is the total force on the vehicle in the body frame.
// in.vPQR is the vehicle body rate relative to the ECEF frame, expressed
//   in the body frame.
// in.vUVW is the vehicle velocity relative to the ECEF frame, expressed
//   in the body frame.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation",
//            Second edition (2004), eqns 1.5-13 (pg 48) and 1.5-16d (page 50)

void FGAccelerations::CalculateUVWdot(void)
{
  vBodyAccel = in.Force / in.Mass;

  vUVWdot = vBodyAccel - (in.vPQR + 2.0 * (in.Ti2b * in.vOmegaPlanet)) * in.vUVW;

  // Include Centripetal acceleration.
  vUVWdot -= in.Ti2b * (in.vOmegaPlanet * (in.vOmegaPlanet * in.vInertialPosition));

  // Include Gravitation accel
  switch (gravType) {
    case gtStandard:
      {
        double radius = in.vInertialPosition.Magnitude();
        vGravAccel = -(in.GAccel / radius) * in.vInertialPosition;
      }
      break;
    case gtWGS84:
      vGravAccel = in.Tec2i * in.J2Grav;
      break;
  }

  vUVWdot += in.Ti2b * vGravAccel;
  vUVWidot = in.Tb2i * vBodyAccel + vGravAccel;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Resolves the contact forces just before integrating the EOM.
// This routine is using Lagrange multipliers and the projected Gauss-Seidel
// (PGS) method.
// Reference: See Erin Catto, "Iterative Dynamics with Temporal Coherence",
//            February 22, 2005
// In JSBSim there is only one rigid body (the aircraft) and there can be
// multiple points of contact between the aircraft and the ground. As a
// consequence our matrix Jac*M^-1*Jac^T is not sparse and the algorithm
// described in Catto's paper has been adapted accordingly.
// The friction forces are resolved in the body frame relative to the origin
// (Earth center).

void FGAccelerations::ResolveFrictionForces(double dt)
{
  const double invMass = 1.0 / in.Mass;
  const FGMatrix33& Jinv = in.Jinv;
  FGColumnVector3 vdot, wdot;
  vector<LagrangeMultiplier*>& multipliers = *in.MultipliersList;
  int n = multipliers.size();

  vFrictionForces.InitMatrix();
  vFrictionMoments.InitMatrix();

  // If no gears are in contact with the ground then return
  if (!n) return;

  vector<double> a(n*n); // Will contain Jac*M^-1*Jac^T
  vector<double> rhs(n);

  // Assemble the linear system of equations
  for (int i=0; i < n; i++) {
    FGColumnVector3 v1 = invMass * multipliers[i]->ForceJacobian;
    FGColumnVector3 v2 = Jinv * multipliers[i]->MomentJacobian; // Should be J^-T but J is symmetric and so is J^-1

    for (int j=0; j < i; j++)
      a[i*n+j] = a[j*n+i]; // Takes advantage of the symmetry of Jac^T*M^-1*Jac
    for (int j=i; j < n; j++)
      a[i*n+j] = DotProduct(v1, multipliers[j]->ForceJacobian)
               + DotProduct(v2, multipliers[j]->MomentJacobian);
  }

  // Assemble the RHS member

  // Translation
  vdot = vUVWdot;
  if (dt > 0.) // Zeroes out the relative movement between the aircraft and the ground
    vdot += (in.vUVW - in.Tec2b * in.TerrainVelocity) / dt;

  // Rotation
  wdot = vPQRdot;
  if (dt > 0.) // Zeroes out the relative movement between the aircraft and the ground
    wdot += (in.vPQR - in.Tec2b * in.TerrainAngularVel) / dt;

  // Prepare the linear system for the Gauss-Seidel algorithm :
  // 1. Compute the right hand side member 'rhs'
  // 2. Divide every line of 'a' and 'rhs' by a[i,i]. This is in order to save
  //    a division computation at each iteration of Gauss-Seidel.
  for (int i=0; i < n; i++) {
    double d = 1.0 / a[i*n+i];

    rhs[i] = -(DotProduct(multipliers[i]->ForceJacobian, vdot)
              +DotProduct(multipliers[i]->MomentJacobian, wdot))*d;
    for (int j=0; j < n; j++)
      a[i*n+j] *= d;
  }

  // Resolve the Lagrange multipliers with the projected Gauss-Seidel method
  for (int iter=0; iter < 50; iter++) {
    double norm = 0.;

    for (int i=0; i < n; i++) {
      double lambda0 = multipliers[i]->value;
      double dlambda = rhs[i];

      for (int j=0; j < n; j++)
        dlambda -= a[i*n+j]*multipliers[j]->value;

      multipliers[i]->value = Constrain(multipliers[i]->Min, lambda0+dlambda, multipliers[i]->Max);
      dlambda = multipliers[i]->value - lambda0;

      norm += fabs(dlambda);
    }

    if (norm < 1E-5) break;
  }

  // Calculate the total friction forces and moments

  for (int i=0; i< n; i++) {
    double lambda = multipliers[i]->value;
    vFrictionForces += lambda * multipliers[i]->ForceJacobian;
    vFrictionMoments += lambda * multipliers[i]->MomentJacobian;
  }

  FGColumnVector3 accel = invMass * vFrictionForces;
  FGColumnVector3 omegadot = Jinv * vFrictionMoments;

  vBodyAccel += accel;
  vUVWdot += accel;
  vUVWidot += in.Tb2i * accel;
  vPQRdot += omegadot;
  vPQRidot += omegadot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAccelerations::InitializeDerivatives(void)
{
  // Make an initial run and set past values
  CalculatePQRdot();           // Angular rate derivative
  CalculateUVWdot();           // Translational rate derivative
  CalculateQuatdot();          // Angular orientation derivative
  ResolveFrictionForces(0.);   // Update rate derivatives with friction forces
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAccelerations::bind(void)
{
  typedef double (FGAccelerations::*PMF)(int) const;

  PropertyManager->Tie("accelerations/pdot-rad_sec2", this, eP, (PMF)&FGAccelerations::GetPQRdot);
  PropertyManager->Tie("accelerations/qdot-rad_sec2", this, eQ, (PMF)&FGAccelerations::GetPQRdot);
  PropertyManager->Tie("accelerations/rdot-rad_sec2", this, eR, (PMF)&FGAccelerations::GetPQRdot);

  PropertyManager->Tie("accelerations/udot-ft_sec2", this, eU, (PMF)&FGAccelerations::GetUVWdot);
  PropertyManager->Tie("accelerations/vdot-ft_sec2", this, eV, (PMF)&FGAccelerations::GetUVWdot);
  PropertyManager->Tie("accelerations/wdot-ft_sec2", this, eW, (PMF)&FGAccelerations::GetUVWdot);

  PropertyManager->Tie("simulation/gravity-model", &gravType);
  PropertyManager->Tie("simulation/gravitational-torque", &gravTorque);

  PropertyManager->Tie("forces/fbx-total-lbs", this, eX, (PMF)&FGAccelerations::GetForces);
  PropertyManager->Tie("forces/fby-total-lbs", this, eY, (PMF)&FGAccelerations::GetForces);
  PropertyManager->Tie("forces/fbz-total-lbs", this, eZ, (PMF)&FGAccelerations::GetForces);
  PropertyManager->Tie("moments/l-total-lbsft", this, eL, (PMF)&FGAccelerations::GetMoments);
  PropertyManager->Tie("moments/m-total-lbsft", this, eM, (PMF)&FGAccelerations::GetMoments);
  PropertyManager->Tie("moments/n-total-lbsft", this, eN, (PMF)&FGAccelerations::GetMoments);

  PropertyManager->Tie("moments/l-gear-lbsft", this, eL, (PMF)&FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("moments/m-gear-lbsft", this, eM, (PMF)&FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("moments/n-gear-lbsft", this, eN, (PMF)&FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("forces/fbx-gear-lbs", this, eX, (PMF)&FGAccelerations::GetGroundForces);
  PropertyManager->Tie("forces/fby-gear-lbs", this, eY, (PMF)&FGAccelerations::GetGroundForces);
  PropertyManager->Tie("forces/fbz-gear-lbs", this, eZ, (PMF)&FGAccelerations::GetGroundForces);
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

void FGAccelerations::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAccelerations" << endl;
    if (from == 1) cout << "Destroyed:    FGAccelerations" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 && from == 2) { // Runtime state variables
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
}
