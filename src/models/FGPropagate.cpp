/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropagate.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
 Called by:    FGFDMExec

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This class encapsulates the integration of rates and accelerations to get the
current position of the aircraft.

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
    Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
    School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
    JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
    Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
    1982 ISBN 0-471-08936-2

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cmath>
#include <iomanip>

#include "FGPropagate.h"
#include <FGFDMExec.h>
#include <FGState.h>
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGInertial.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropagate.cpp,v 1.39 2009/05/28 23:12:56 jberndt Exp $";
static const char *IdHdr = ID_PROPAGATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropagate::FGPropagate(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Debug(0);
  Name = "FGPropagate";

  last2_vPQRdot.InitMatrix();
  last_vPQRdot.InitMatrix();
  vPQRdot.InitMatrix();
  
  last2_vQtrndot = FGQuaternion(0,0,0);
  last_vQtrndot = FGQuaternion(0,0,0);
  vQtrndot = FGQuaternion(0,0,0);

  last2_vUVWdot.InitMatrix();
  last_vUVWdot.InitMatrix();
  vUVWdot.InitMatrix();
  
  last2_vLocationDot.InitMatrix();
  last_vLocationDot.InitMatrix();
  vLocationDot.InitMatrix();

  vOmegaLocal.InitMatrix();

  integrator_rotational_rate = eAdamsBashforth2;
  integrator_translational_rate = eTrapezoidal;
  integrator_rotational_position = eAdamsBashforth2;
  integrator_translational_position = eTrapezoidal;

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropagate::~FGPropagate(void)
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropagate::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  // For initialization ONLY:
  SeaLevelRadius = LocalTerrainRadius = Inertial->GetRefRadius();

  VState.vLocation.SetRadius( LocalTerrainRadius + 4.0 );
  VState.vLocation.SetEllipse(Inertial->GetSemimajor(), Inertial->GetSemiminor());
  vOmega = FGColumnVector3( 0.0, 0.0, Inertial->omega() ); // Earth rotation vector

  last2_vPQRdot.InitMatrix();
  last_vPQRdot.InitMatrix();
  vPQRdot.InitMatrix();
  
  last2_vQtrndot = FGQuaternion(0,0,0);
  last_vQtrndot = FGQuaternion(0,0,0);
  vQtrndot = FGQuaternion(0,0,0);

  last2_vUVWdot.InitMatrix();
  last_vUVWdot.InitMatrix();
  vUVWdot.InitMatrix();
  
  last2_vLocationDot.InitMatrix();
  last_vLocationDot.InitMatrix();
  vLocationDot.InitMatrix();

  vOmegaLocal.InitMatrix();

  integrator_rotational_rate = eAdamsBashforth2;
  integrator_translational_rate = eTrapezoidal;
  integrator_rotational_position = eAdamsBashforth2;
  integrator_translational_position = eTrapezoidal;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInitialState(const FGInitialCondition *FGIC)
{
  SetSeaLevelRadius(FGIC->GetSeaLevelRadiusFtIC());
  SetTerrainElevation(FGIC->GetTerrainElevationFtIC());

  // Set the position lat/lon/radius
  VState.vLocation.SetPosition( FGIC->GetLongitudeRadIC(),
                          FGIC->GetLatitudeRadIC(),
                          FGIC->GetAltitudeASLFtIC() + FGIC->GetSeaLevelRadiusFtIC() );

  VehicleRadius = GetRadius();
  radInv = 1.0/VehicleRadius;

  // Set the Orientation from the euler angles
  VState.vQtrn = FGQuaternion( FGIC->GetPhiRadIC(),
                        FGIC->GetThetaRadIC(),
                        FGIC->GetPsiRadIC() );

  // Set the velocities in the instantaneus body frame
  VState.vUVW = FGColumnVector3( FGIC->GetUBodyFpsIC(),
                          FGIC->GetVBodyFpsIC(),
                          FGIC->GetWBodyFpsIC() );

  // Set the angular velocities in the instantaneus body frame.
  VState.vPQR = FGColumnVector3( FGIC->GetPRadpsIC(),
                          FGIC->GetQRadpsIC(),
                          FGIC->GetRRadpsIC() );

  // Compute the local frame ECEF velocity
  vVel = GetTb2l()*VState.vUVW;

  // Finally, make sure that the quaternion stays normalized.
  VState.vQtrn.Normalize();

  // Recompute the LocalTerrainRadius.
  RecomputeLocalTerrainRadius();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to perform EOM integration
Notes:   [JB] Run in standalone mode, SeaLevelRadius will be reference radius.
         In FGFS, SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.

At the top of this Run() function, see several "shortcuts" (or, aliases) being
set up for use later, rather than using the longer class->function() notation.

This propagation is done using the current state values
and current derivatives. Based on these values we compute an approximation to the
state values for (now + dt).

In the code below, variables named beginning with a small "v" refer to a 
a column vector, variables named beginning with a "T" refer to a transformation
matrix. ECEF refers to Earth Centered Earth Fixed. ECI refers to Earth Centered
Inertial.

*/

bool FGPropagate::Run(void)
{
  if (FGModel::Run()) return true;  // Fast return if we have nothing to do ...
  if (FDMExec->Holding()) return false;

  RecomputeLocalTerrainRadius();

  // Calculate current aircraft radius from center of planet

  VehicleRadius = GetRadius();
  radInv = 1.0/VehicleRadius;

  // These local copies of the transformation matrices are for use this
  // pass through Run() only.

  Tl2b = GetTl2b();           // local to body frame transform
  Tb2l = Tl2b.Transposed();   // body to local frame transform
  Tl2ec = GetTl2ec();         // local to ECEF transform
  Tec2l = Tl2ec.Transposed(); // ECEF to local frame transform
  Tec2b = Tl2b * Tec2l;       // ECEF to body frame transform
  Tb2ec = Tec2b.Transposed(); // body to ECEF frame tranform
  Ti2ec = GetTi2ec();         // ECI to ECEF transform
  Tec2i = Ti2ec.Transposed(); // ECEF to ECI frame transform
  Ti2b  = Tec2b*Ti2ec;        // ECI to body frame transform
  Tb2i  = Ti2b.Transposed();  // body to ECI frame transform

  // Compute vehicle velocity wrt ECEF frame, expressed in Local horizontal frame.
  vVel = Tb2l * VState.vUVW;

  // Inertial angular velocity measured in the body frame.
  vPQRi = VState.vPQR + Tec2b*vOmega;

  // Calculate state derivatives
  CalculatePQRdot();      // Angular rate derivative
  CalculateUVWdot();      // Translational rate derivative
  CalculateQuatdot();     // Angular orientation derivative
  CalculateLocationdot(); // Translational position derivative

  // Integrate to propagate the state

  double dt = State->Getdt()*rate;  // The 'stepsize'

  // Propagate rotational velocity

  switch(integrator_rotational_rate) {
  case eRectEuler:       VState.vPQR += dt*vPQRdot;
    break;
  case eTrapezoidal:     VState.vPQR += 0.5*dt*(vPQRdot + last_vPQRdot);
    break;
  case eAdamsBashforth2: VState.vPQR += dt*(1.5*vPQRdot - 0.5*last_vPQRdot);
    break;
  case eAdamsBashforth3: VState.vPQR += (1/12.0)*dt*(23.0*vPQRdot - 16.0*last_vPQRdot + 5.0*last2_vPQRdot);
    break;
  case eNone: // do nothing, freeze angular rate
    break;
  }
  
  // Propagate translational velocity

  switch(integrator_translational_rate) {
  case eRectEuler:       VState.vUVW += dt*vUVWdot;
    break;
  case eTrapezoidal:     VState.vUVW += 0.5*dt*(vUVWdot + last_vUVWdot);
    break;
  case eAdamsBashforth2: VState.vUVW += dt*(1.5*vUVWdot - 0.5*last_vUVWdot);
    break;
  case eAdamsBashforth3: VState.vUVW += (1/12.0)*dt*(23.0*vUVWdot - 16.0*last_vUVWdot + 5.0*last2_vUVWdot);
    break;
  case eNone: // do nothing, freeze translational rate
    break;
  }

  // Propagate angular position

  switch(integrator_rotational_position) {
  case eRectEuler:       VState.vQtrn += dt*vQtrndot;
    break;
  case eTrapezoidal:     VState.vQtrn += 0.5*dt*(vQtrndot + last_vQtrndot);
    break;
  case eAdamsBashforth2: VState.vQtrn += dt*(1.5*vQtrndot - 0.5*last_vQtrndot);
    break;
  case eAdamsBashforth3: VState.vQtrn += (1/12.0)*dt*(23.0*vQtrndot - 16.0*last_vQtrndot + 5.0*last2_vQtrndot);
    break;
  case eNone: // do nothing, freeze angular position
    break;
  }

  // Propagate translational position

  switch(integrator_translational_position) {
  case eRectEuler:       VState.vLocation += dt*vLocationDot;
    break;
  case eTrapezoidal:     VState.vLocation += 0.5*dt*(vLocationDot + last_vLocationDot);
    break;
  case eAdamsBashforth2: VState.vLocation += dt*(1.5*vLocationDot - 0.5*last_vLocationDot);
    break;
  case eAdamsBashforth3: VState.vLocation += (1/12.0)*dt*(23.0*vLocationDot - 16.0*last_vLocationDot + 5.0*last2_vLocationDot);
    break;
  case eNone: // do nothing, freeze translational position
    break;
  }
  
  // Set past values
  
  last2_vPQRdot = last_vPQRdot;
  last_vPQRdot = vPQRdot;
  
  last2_vUVWdot = last_vUVWdot;
  last_vUVWdot = vUVWdot;
  
  last2_vQtrndot = last_vQtrndot;
  last_vQtrndot = vQtrndot;

  last2_vLocationDot = last_vLocationDot;
  last_vLocationDot = vLocationDot;

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute body frame rotational accelerations based on the current body moments
//
// vPQRdot is the derivative of the absolute angular velocity of the vehicle 
// (body rate with respect to the inertial frame), expressed in the body frame,
// where the derivative is taken in the body frame.
// J is the inertia matrix
// Jinv is the inverse inertia matrix
// vMoments is the moment vector in the body frame
// vPQRi is the total inertial angular velocity of the vehicle
// expressed in the body frame.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation", 
//            Second edition (2004), eqn 1.5-16e (page 50)

void FGPropagate::CalculatePQRdot(void)
{
  const FGColumnVector3& vMoments = Aircraft->GetMoments(); // current moments
  const FGMatrix33& J = MassBalance->GetJ();                // inertia matrix
  const FGMatrix33& Jinv = MassBalance->GetJinv();          // inertia matrix inverse

  // Compute body frame rotational accelerations based on the current body
  // moments and the total inertial angular velocity expressed in the body
  // frame.

  vPQRdot = Jinv*(vMoments - vPQRi*(J*vPQRi));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the quaternion orientation derivative
//
// vQtrndot is the quaternion derivative.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation", 
//            Second edition (2004), eqn 1.5-16b (page 50)

void FGPropagate::CalculateQuatdot(void)
{
  vOmegaLocal.InitMatrix( radInv*vVel(eEast),
                         -radInv*vVel(eNorth),
                         -radInv*vVel(eEast)*VState.vLocation.GetTanLatitude() );

  // Compute quaternion orientation derivative on current body rates
  vQtrndot = VState.vQtrn.GetQDot( VState.vPQR - Tl2b*vOmegaLocal);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This set of calculations results in the body frame accelerations being
// computed.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation", 
//            Second edition (2004), eqn 1.5-16d (page 50)

void FGPropagate::CalculateUVWdot(void)
{
  double mass = MassBalance->GetMass();                      // mass
  const FGColumnVector3& vForces = Aircraft->GetForces();    // current forces

  const FGColumnVector3 vGravAccel( 0.0, 0.0, Inertial->GetGAccel(VehicleRadius) );

  // Begin to compute body frame accelerations based on the current body forces
  vUVWdot = vForces/mass - VState.vPQR * VState.vUVW;

  // Include Coriolis acceleration.
  vUVWdot -= 2.0 * (Ti2b *vOmega) * VState.vUVW;

  // Include Centrifugal acceleration.
  if (!GroundReactions->GetWOW()) {
    vUVWdot -= Ti2b*(vOmega*(vOmega*(Tec2i*VState.vLocation)));
  }

  // Include Gravitation accel
  FGColumnVector3 gravAccel = Tl2b*vGravAccel;
  vUVWdot += gravAccel;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::CalculateLocationdot(void)
{
  // Transform the vehicle velocity relative to the ECEF frame, expressed
  // in the body frame, to be expressed in the ECEF frame.
  vLocationDot = Tb2ec * VState.vUVW;

  // Now, transform the velocity vector of the body relative to the origin (Earth
  // center) to be expressed in the inertial frame, and add the vehicle velocity
  // contribution due to the rotation of the planet. The above velocity is only
  // relative to the rotating ECEF frame.
  // Reference: See Stevens and Lewis, "Aircraft Control and Simulation", 
  //            Second edition (2004), eqn 1.5-16c (page 50)

  vInertialVelocity = Tec2i * vLocationDot + (vOmega * (Tec2i * VState.vLocation));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::RecomputeLocalTerrainRadius(void)
{
  double t = State->Getsim_time();

  // Get the LocalTerrain radius.
  FGLocation contactloc;
  FGColumnVector3 dv;
  FDMExec->GetGroundCallback()->GetAGLevel(t, VState.vLocation, contactloc, dv, dv);
  LocalTerrainRadius = contactloc.GetRadius();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetTerrainElevation(double terrainElev)
{
  LocalTerrainRadius = terrainElev + SeaLevelRadius;
  FDMExec->GetGroundCallback()->SetTerrainGeoCentRadius(LocalTerrainRadius);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetTerrainElevation(void) const
{
  return FDMExec->GetGroundCallback()->GetTerrainGeoCentRadius()-SeaLevelRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGPropagate::GetTi2ec(void)
{
  return VState.vLocation.GetTi2ec(Inertial->GetEarthPositionAngle());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGPropagate::GetTec2i(void)
{
  return VState.vLocation.GetTec2i(Inertial->GetEarthPositionAngle());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetAltitudeASL(double altASL)
{
  VState.vLocation.SetRadius( altASL + SeaLevelRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetLocalTerrainRadius(void) const
{
  return LocalTerrainRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetDistanceAGL(void) const
{
  return VState.vLocation.GetRadius() - LocalTerrainRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGL(double tt)
{
  VState.vLocation.SetRadius( tt + LocalTerrainRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::bind(void)
{
  typedef double (FGPropagate::*PMF)(int) const;
//  typedef double (FGPropagate::*dPMF)() const;
  PropertyManager->Tie("velocities/h-dot-fps", this, &FGPropagate::Gethdot);

  PropertyManager->Tie("velocities/v-north-fps", this, eNorth, (PMF)&FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-east-fps", this, eEast, (PMF)&FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-down-fps", this, eDown, (PMF)&FGPropagate::GetVel);

  PropertyManager->Tie("velocities/u-fps", this, eU, (PMF)&FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/v-fps", this, eV, (PMF)&FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/w-fps", this, eW, (PMF)&FGPropagate::GetUVW);

  PropertyManager->Tie("velocities/p-rad_sec", this, eP, (PMF)&FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/q-rad_sec", this, eQ, (PMF)&FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/r-rad_sec", this, eR, (PMF)&FGPropagate::GetPQR);

  PropertyManager->Tie("velocities/pi-rad_sec", this, eP, (PMF)&FGPropagate::GetPQRi);
  PropertyManager->Tie("velocities/qi-rad_sec", this, eQ, (PMF)&FGPropagate::GetPQRi);
  PropertyManager->Tie("velocities/ri-rad_sec", this, eR, (PMF)&FGPropagate::GetPQRi);

  PropertyManager->Tie("velocities/eci-velocity-mag-fps", this, &FGPropagate::GetInertialVelocityMagnitude);

  PropertyManager->Tie("accelerations/pdot-rad_sec2", this, eP, (PMF)&FGPropagate::GetPQRdot);
  PropertyManager->Tie("accelerations/qdot-rad_sec2", this, eQ, (PMF)&FGPropagate::GetPQRdot);
  PropertyManager->Tie("accelerations/rdot-rad_sec2", this, eR, (PMF)&FGPropagate::GetPQRdot);

  PropertyManager->Tie("accelerations/udot-ft_sec2", this, eU, (PMF)&FGPropagate::GetUVWdot);
  PropertyManager->Tie("accelerations/vdot-ft_sec2", this, eV, (PMF)&FGPropagate::GetUVWdot);
  PropertyManager->Tie("accelerations/wdot-ft_sec2", this, eW, (PMF)&FGPropagate::GetUVWdot);

  PropertyManager->Tie("position/h-sl-ft", this, &FGPropagate::GetAltitudeASL, &FGPropagate::SetAltitudeASL, true);
  PropertyManager->Tie("position/h-sl-meters", this, &FGPropagate::GetAltitudeASLmeters, &FGPropagate::SetAltitudeASLmeters, true);
  PropertyManager->Tie("position/lat-gc-rad", this, &FGPropagate::GetLatitude, &FGPropagate::SetLatitude);
  PropertyManager->Tie("position/long-gc-rad", this, &FGPropagate::GetLongitude, &FGPropagate::SetLongitude);
  PropertyManager->Tie("position/lat-gc-deg", this, &FGPropagate::GetLatitudeDeg, &FGPropagate::SetLatitudeDeg);
  PropertyManager->Tie("position/long-gc-deg", this, &FGPropagate::GetLongitudeDeg, &FGPropagate::SetLongitudeDeg);
  PropertyManager->Tie("position/lat-geod-rad", this, &FGPropagate::GetGeodLatitudeRad);
  PropertyManager->Tie("position/lat-geod-deg", this, &FGPropagate::GetGeodLatitudeDeg);
  PropertyManager->Tie("position/geod-alt-ft", this, &FGPropagate::GetGeodeticAltitude);
  PropertyManager->Tie("position/h-agl-ft", this,  &FGPropagate::GetDistanceAGL, &FGPropagate::SetDistanceAGL);
  PropertyManager->Tie("position/radius-to-vehicle-ft", this, &FGPropagate::GetRadius);
  PropertyManager->Tie("position/terrain-elevation-asl-ft", this,
                          &FGPropagate::GetTerrainElevation,
                          &FGPropagate::SetTerrainElevation, false);

  PropertyManager->Tie("metrics/terrain-radius", this, &FGPropagate::GetLocalTerrainRadius);

  PropertyManager->Tie("attitude/phi-rad", this, (int)ePhi, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/theta-rad", this, (int)eTht, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/psi-rad", this, (int)ePsi, (PMF)&FGPropagate::GetEuler);

  PropertyManager->Tie("attitude/roll-rad", this, (int)ePhi, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this, (int)eTht, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this, (int)ePsi, (PMF)&FGPropagate::GetEuler);
  
  PropertyManager->Tie("simulation/integrator/rate/rotational", &integrator_rotational_rate);
  PropertyManager->Tie("simulation/integrator/rate/translational", &integrator_translational_rate);
  PropertyManager->Tie("simulation/integrator/position/rotational", &integrator_rotational_position);
  PropertyManager->Tie("simulation/integrator/position/translational", &integrator_translational_position);
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

void FGPropagate::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPropagate" << endl;
    if (from == 1) cout << "Destroyed:    FGPropagate" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
    if (from == 2) { // State sanity checking
      if (fabs(VState.vPQR.Magnitude()) > 1000.0) {
        cerr << endl << "Vehicle rotation rate is excessive (>1000 rad/sec): " << VState.vPQR.Magnitude() << endl;
        exit(-1);
      }
      if (fabs(VState.vUVW.Magnitude()) > 1.0e10) {
        cerr << endl << "Vehicle velocity is excessive (>1e10 ft/sec): " << VState.vUVW.Magnitude() << endl;
        exit(-1);
      }
      if (fabs(GetDistanceAGL()) > 1e10) {
        cerr << endl << "Vehicle altitude is excessive (>1e10 ft): " << GetDistanceAGL() << endl;
        exit(-1);
      }
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
