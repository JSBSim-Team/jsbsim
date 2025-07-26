/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAccelerations.cpp
 Author:       Jon S. Berndt
 Date started: 07/12/11
 Purpose:      Calculates derivatives of rotational and translational rates, and
               of the attitude quaternion.
 Called by:    FGFDMExec

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This class encapsulates the calculation of the derivatives of the state vectors
UVW and PQR - the translational and rotational rates relative to the planet
fixed frame. The derivatives relative to the inertial frame are also calculated
as a side effect. Also, the derivative of the attitude quaterion is also
calculated.

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
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAccelerations::FGAccelerations(FGFDMExec* fdmex)
  : FGModel(fdmex)
{
  Debug(0);
  Name = "FGAccelerations";
  gravTorque = false;

  vPQRidot.InitMatrix();
  vUVWidot.InitMatrix();
  vUVWdot.InitMatrix();
  vBodyAccel.InitMatrix();

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
  if (!FGModel::InitModel()) return false;

  vPQRidot.InitMatrix();
  vUVWidot.InitMatrix();
  vUVWdot.InitMatrix();
  vBodyAccel.InitMatrix();

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

  if (!FDMExec->GetHoldDown())
    CalculateFrictionForces(in.DeltaT * rate);  // Update rate derivatives with friction forces

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
    in.Moment += (3.0 * in.vGravAccel.Magnitude() * invRadius) * (R * (in.J * R));
  }

  // Compute body frame rotational accelerations based on the current body
  // moments and the total inertial angular velocity expressed in the body
  // frame.
//  if (HoldDown && !FDMExec->GetTrimStatus()) {
  if (FDMExec->GetHoldDown()) {
    // The rotational acceleration in ECI is calculated so that the rotational
    // acceleration is zero in the body frame.
    vPQRdot.InitMatrix();
    vPQRidot = vPQRdot - in.vPQRi * (in.Ti2b * in.vOmegaPlanet);
  }
  else {
    vPQRidot = in.Jinv * (in.Moment - in.vPQRi * (in.J * in.vPQRi));
    vPQRdot = vPQRidot + in.vPQRi * (in.Ti2b * in.vOmegaPlanet);
  }
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
  if (FDMExec->GetHoldDown() && !FDMExec->GetTrimStatus())
    vBodyAccel.InitMatrix();
  else
    vBodyAccel = in.Force / in.Mass;

  vUVWdot = vBodyAccel - (in.vPQR + 2.0 * (in.Ti2b * in.vOmegaPlanet)) * in.vUVW;

  // Include Centripetal acceleration.
  vUVWdot -= in.Ti2b * (in.vOmegaPlanet * (in.vOmegaPlanet * in.vInertialPosition));

  if (FDMExec->GetHoldDown()) {
    // The acceleration in ECI is calculated so that the acceleration is zero
    // in the body frame.
    vUVWidot = in.vOmegaPlanet * (in.vOmegaPlanet * in.vInertialPosition);
    vUVWdot.InitMatrix();
  }
  else {
    vUVWdot += in.Tec2b * in.vGravAccel;
    vUVWidot = in.Tb2i * vBodyAccel + in.Tec2i * in.vGravAccel;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAccelerations::SetHoldDown(bool hd)
{
  if (hd) {
    vUVWidot = in.vOmegaPlanet * (in.vOmegaPlanet * in.vInertialPosition);
    vUVWdot.InitMatrix();
    vPQRdot.InitMatrix();
    vPQRidot = vPQRdot - in.vPQRi * (in.Ti2b * in.vOmegaPlanet);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Computes the contact forces just before integrating the EOM.
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

void FGAccelerations::CalculateFrictionForces(double dt)
{
  vector<LagrangeMultiplier*>& multipliers = *in.MultipliersList;
  size_t n = multipliers.size();

  vFrictionForces.InitMatrix();
  vFrictionMoments.InitMatrix();

  // If no gears are in contact with the ground then return
  if (!n) return;

  vector<double> a(n*n); // Will contain Jac*M^-1*Jac^T
  vector<double> rhs(n);

  // Assemble the linear system of equations
  for (unsigned int i=0; i < n; i++) {
    FGColumnVector3 U = multipliers[i]->ForceJacobian;
    FGColumnVector3 r = multipliers[i]->LeverArm;
    FGColumnVector3 v1 = U / in.Mass;
    FGColumnVector3 v2 = in.Jinv * (r*U); // Should be J^-T but J is symmetric and so is J^-1

    for (unsigned int j=0; j < i; j++)
      a[i*n+j] = a[j*n+i]; // Takes advantage of the symmetry of Jac^T*M^-1*Jac

    for (unsigned int j=i; j < n; j++) {
      U = multipliers[j]->ForceJacobian;
      r = multipliers[j]->LeverArm;
      a[i*n+j] = DotProduct(U, v1 + v2*r);
    }
  }

  // Assemble the RHS member

  // Translation
  FGColumnVector3 vdot = vUVWdot;
  if (dt > 0.) // Zeroes out the relative movement between the aircraft and the ground
    vdot += (in.vUVW - in.Tec2b * in.TerrainVelocity) / dt;

  // Rotation
  FGColumnVector3 wdot = vPQRdot;
  if (dt > 0.) // Zeroes out the relative movement between the aircraft and the ground
    wdot += (in.vPQR - in.Tec2b * in.TerrainAngularVel) / dt;

  // Prepare the linear system for the Gauss-Seidel algorithm :
  // 1. Compute the right hand side member 'rhs'
  // 2. Divide every line of 'a' and 'rhs' by a[i,i]. This is in order to save
  //    a division computation at each iteration of Gauss-Seidel.
  for (unsigned int i=0; i < n; i++) {
    double d = a[i*n+i];
    FGColumnVector3 U = multipliers[i]->ForceJacobian;
    FGColumnVector3 r = multipliers[i]->LeverArm;

    rhs[i] = -DotProduct(U, vdot + wdot*r)/d;

    for (unsigned int j=0; j < n; j++)
      a[i*n+j] /= d;
  }

  // Resolve the Lagrange multipliers with the projected Gauss-Seidel method
  for (int iter=0; iter < 50; iter++) {
    double norm = 0.;

    for (unsigned int i=0; i < n; i++) {
      double lambda0 = multipliers[i]->value;
      double dlambda = rhs[i];

      for (unsigned int j=0; j < n; j++)
        dlambda -= a[i*n+j]*multipliers[j]->value;

      multipliers[i]->value = Constrain(multipliers[i]->Min, lambda0+dlambda, multipliers[i]->Max);
      dlambda = multipliers[i]->value - lambda0;

      norm += fabs(dlambda);
    }

    if (norm < 1E-5) break;
  }

  // Calculate the total friction forces and moments

  for (unsigned int i=0; i< n; i++) {
    double lambda = multipliers[i]->value;
    FGColumnVector3 U = multipliers[i]->ForceJacobian;
    FGColumnVector3 r = multipliers[i]->LeverArm;

    FGColumnVector3 F = lambda * U;
    vFrictionForces += F;
    vFrictionMoments += r * F;
  }

  FGColumnVector3 accel = vFrictionForces / in.Mass;
  FGColumnVector3 omegadot = in.Jinv * vFrictionMoments;

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
  CalculateFrictionForces(0.);   // Update rate derivatives with friction forces
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAccelerations::bind(void)
{
  PropertyManager->Tie("accelerations/pdot-rad_sec2", this, eP, &FGAccelerations::GetPQRdot);
  PropertyManager->Tie("accelerations/qdot-rad_sec2", this, eQ, &FGAccelerations::GetPQRdot);
  PropertyManager->Tie("accelerations/rdot-rad_sec2", this, eR, &FGAccelerations::GetPQRdot);

  PropertyManager->Tie("accelerations/pidot-rad_sec2", this, eP, &FGAccelerations::GetPQRidot);
  PropertyManager->Tie("accelerations/qidot-rad_sec2", this, eQ, &FGAccelerations::GetPQRidot);
  PropertyManager->Tie("accelerations/ridot-rad_sec2", this, eR, &FGAccelerations::GetPQRidot);

  PropertyManager->Tie("accelerations/udot-ft_sec2", this, eU, &FGAccelerations::GetUVWdot);
  PropertyManager->Tie("accelerations/vdot-ft_sec2", this, eV, &FGAccelerations::GetUVWdot);
  PropertyManager->Tie("accelerations/wdot-ft_sec2", this, eW, &FGAccelerations::GetUVWdot);

  PropertyManager->Tie("accelerations/uidot-ft_sec2", this, eU, &FGAccelerations::GetUVWidot);
  PropertyManager->Tie("accelerations/vidot-ft_sec2", this, eV, &FGAccelerations::GetUVWidot);
  PropertyManager->Tie("accelerations/widot-ft_sec2", this, eW, &FGAccelerations::GetUVWidot);

  PropertyManager->Tie("accelerations/gravity-ft_sec2", this, &FGAccelerations::GetGravAccelMagnitude);
  PropertyManager->Tie("simulation/gravitational-torque", &gravTorque);
  PropertyManager->Tie("forces/fbx-weight-lbs", this, eX, &FGAccelerations::GetWeight);
  PropertyManager->Tie("forces/fby-weight-lbs", this, eY, &FGAccelerations::GetWeight);
  PropertyManager->Tie("forces/fbz-weight-lbs", this, eZ, &FGAccelerations::GetWeight);

  PropertyManager->Tie("forces/fbx-total-lbs", this, eX, &FGAccelerations::GetForces);
  PropertyManager->Tie("forces/fby-total-lbs", this, eY, &FGAccelerations::GetForces);
  PropertyManager->Tie("forces/fbz-total-lbs", this, eZ, &FGAccelerations::GetForces);
  PropertyManager->Tie("moments/l-total-lbsft", this, eL, &FGAccelerations::GetMoments);
  PropertyManager->Tie("moments/m-total-lbsft", this, eM, &FGAccelerations::GetMoments);
  PropertyManager->Tie("moments/n-total-lbsft", this, eN, &FGAccelerations::GetMoments);

  PropertyManager->Tie("moments/l-gear-lbsft", this, eL, &FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("moments/m-gear-lbsft", this, eM, &FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("moments/n-gear-lbsft", this, eN, &FGAccelerations::GetGroundMoments);
  PropertyManager->Tie("forces/fbx-gear-lbs", this, eX, &FGAccelerations::GetGroundForces);
  PropertyManager->Tie("forces/fby-gear-lbs", this, eY, &FGAccelerations::GetGroundForces);
  PropertyManager->Tie("forces/fbz-gear-lbs", this, eZ, &FGAccelerations::GetGroundForces);
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
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGAccelerations\n";
    if (from == 1) log << "Destroyed:    FGAccelerations\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 && from == 2) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
