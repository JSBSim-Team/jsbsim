/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropagate.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
 Called by:    FGFDMExec

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
[6] S. Buss, "Accurate and Efficient Simulation of Rigid Body Rotations",
    Technical Report, Department of Mathematics, University of California,
    San Diego, 1999
[7] Barker L.E., Bowles R.L. and Williams L.H., "Development and Application of
    a Local Linearization Algorithm for the Integration of Quaternion Rate
    Equations in Real-Time Flight Simulation Problems", NASA TN D-7347,
    December 1973
[8] Phillips W.F, Hailey C.E and Gebert G.A, "Review of Attitude Representations
    Used for Aircraft Kinematics", Journal Of Aircraft Vol. 38, No. 4,
    July-August 2001

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>

#include "FGPropagate.h"
#include "initialization/FGInitialCondition.h"
#include "FGFDMExec.h"
#include "simgear/io/iostreams/sgstream.hxx"
#include "FGInertial.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropagate::FGPropagate(FGFDMExec* fdmex)
  : FGModel(fdmex)
{
  Name = "FGPropagate";

  Inertial = FDMExec->GetInertial();

  /// These define the indices use to select the various integrators.
  // eNone = 0, eRectEuler, eTrapezoidal, eAdamsBashforth2, eAdamsBashforth3, eAdamsBashforth4};

  integrator_rotational_rate = eRectEuler;
  integrator_translational_rate = eAdamsBashforth2;
  integrator_rotational_position = eRectEuler;
  integrator_translational_position = eAdamsBashforth3;

  VState.dqPQRidot.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqUVWidot.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqInertialVelocity.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqQtrndot.resize(5, FGQuaternion(0.0,0.0,0.0));

  epa = 0.0;

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
  VState.vLocation.SetEllipse(in.SemiMajor, in.SemiMinor);
  Inertial->SetAltitudeAGL(VState.vLocation, 4.0);

  VState.dqPQRidot.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqUVWidot.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqInertialVelocity.resize(5, FGColumnVector3(0.0,0.0,0.0));
  VState.dqQtrndot.resize(5, FGColumnVector3(0.0,0.0,0.0));

  integrator_rotational_rate = eRectEuler;
  integrator_translational_rate = eAdamsBashforth2;
  integrator_rotational_position = eRectEuler;
  integrator_translational_position = eAdamsBashforth3;

  epa = 0.0;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInitialState(const FGInitialCondition* FGIC)
{
  // Initialize the State Vector elements and the transformation matrices

  // Set the position lat/lon/radius
  VState.vLocation = FGIC->GetPosition();

  epa = FGIC->GetEarthPositionAngleIC();
  Ti2ec = { cos(epa), sin(epa), 0.0,
            -sin(epa), cos(epa), 0.0,
            0.0, 0.0, 1.0 };
  Tec2i = Ti2ec.Transposed();          // ECEF to ECI frame transform

  VState.vInertialPosition = Tec2i * VState.vLocation;

  UpdateLocationMatrices();

  // Set the orientation from the euler angles (is normalized within the
  // constructor). The Euler angles represent the orientation of the body
  // frame relative to the local frame.
  VState.qAttitudeLocal = FGIC->GetOrientation();

  VState.qAttitudeECI = Ti2l.GetQuaternion()*VState.qAttitudeLocal;
  UpdateBodyMatrices();

  // Set the velocities in the instantaneus body frame
  VState.vUVW = FGIC->GetUVWFpsIC();

  // Compute the local frame ECEF velocity
  vVel = Tb2l * VState.vUVW;

  // Compute local terrain velocity
  RecomputeLocalTerrainVelocity();

  // Set the angular velocities of the body frame relative to the ECEF frame,
  // expressed in the body frame.
  VState.vPQR = FGIC->GetPQRRadpsIC();

  VState.vPQRi = VState.vPQR + Ti2b * in.vOmegaPlanet;

  CalculateInertialVelocity(); // Translational position derivative
  CalculateQuatdot();  // Angular orientation derivative
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Initialize the past value deques

void FGPropagate::InitializeDerivatives()
{
  VState.dqPQRidot.assign(5, in.vPQRidot);
  VState.dqUVWidot.assign(5, in.vUVWidot);
  VState.dqInertialVelocity.assign(5, VState.vInertialVelocity);
  VState.dqQtrndot.assign(5, VState.vQtrndot);
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

bool FGPropagate::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;  // Fast return if we have nothing to do ...
  if (Holding) return false;

  double dt = in.DeltaT * rate;  // The 'stepsize'

  // Propagate rotational / translational velocity, angular /translational position, respectively.

  if (!FDMExec->IntegrationSuspended()) {
    Integrate(VState.qAttitudeECI,      VState.vQtrndot,      VState.dqQtrndot,          dt, integrator_rotational_position);
    Integrate(VState.vPQRi,             in.vPQRidot,          VState.dqPQRidot,          dt, integrator_rotational_rate);
    Integrate(VState.vInertialPosition, VState.vInertialVelocity, VState.dqInertialVelocity, dt, integrator_translational_position);
    Integrate(VState.vInertialVelocity, in.vUVWidot,          VState.dqUVWidot,          dt, integrator_translational_rate);
  }

  // CAUTION : the order of the operations below is very important to get
  // transformation matrices that are consistent with the new state of the
  // vehicle

  // 1. Update the Earth position angle (EPA)
  epa += in.vOmegaPlanet(eZ)*dt;

  // 2. Update the Ti2ec and Tec2i transforms from the updated EPA
  double cos_epa = cos(epa);
  double sin_epa = sin(epa);
  Ti2ec = { cos_epa, sin_epa, 0.0,
            -sin_epa, cos_epa, 0.0,
            0.0, 0.0, 1.0 };
  Tec2i = Ti2ec.Transposed();          // ECEF to ECI frame transform

  // 3. Update the location from the updated Ti2ec and inertial position
  VState.vLocation = Ti2ec*VState.vInertialPosition;

  // 4. Update the other "Location-based" transformation matrices from the
  //    updated vLocation vector.
  UpdateLocationMatrices();

  // 5. Update the "Orientation-based" transformation matrices from the updated
  //    orientation quaternion and vLocation vector.
  UpdateBodyMatrices();

  // Translational position derivative (velocities are integrated in the
  // inertial frame)
  CalculateUVW();

  // Set auxilliary state variables
  RecomputeLocalTerrainVelocity();

  VState.vPQR = VState.vPQRi - Ti2b * in.vOmegaPlanet;

  // Angular orientation derivative
  CalculateQuatdot();

  VState.qAttitudeLocal = Tl2b.GetQuaternion();

  // Compute vehicle velocity wrt ECEF frame, expressed in Local horizontal
  // frame.
  vVel = Tb2l * VState.vUVW;

  // Compute orbital parameters in the inertial frame
  ComputeOrbitalParameters();

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetHoldDown(bool hd)
{
  if (hd) {
    VState.vUVW.InitMatrix();
    CalculateInertialVelocity();
    VState.vPQR.InitMatrix();
    VState.vPQRi = Ti2b * in.vOmegaPlanet;
    CalculateQuatdot();
    InitializeDerivatives();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the quaternion orientation derivative
//
// vQtrndot is the quaternion derivative.
// Reference: See Stevens and Lewis, "Aircraft Control and Simulation",
//            Second edition (2004), eqn 1.5-16b (page 50)

void FGPropagate::CalculateQuatdot(void)
{
  // Compute quaternion orientation derivative on current body rates
  VState.vQtrndot = VState.qAttitudeECI.GetQDot(VState.vPQRi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Transform the velocity vector of the body relative to the origin (Earth
  // center) to be expressed in the inertial frame, and add the vehicle velocity
  // contribution due to the rotation of the planet.
  // Reference: See Stevens and Lewis, "Aircraft Control and Simulation",
  //            Second edition (2004), eqn 1.5-16c (page 50)

void FGPropagate::CalculateInertialVelocity(void)
{
  VState.vInertialVelocity = Tb2i * VState.vUVW + (in.vOmegaPlanet * VState.vInertialPosition);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Transform the velocity vector of the inertial frame to be expressed in the
  // body frame relative to the origin (Earth center), and substract the vehicle
  // velocity contribution due to the rotation of the planet.

void FGPropagate::CalculateUVW(void)
{
  VState.vUVW = Ti2b * (VState.vInertialVelocity - (in.vOmegaPlanet * VState.vInertialPosition));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::Integrate( FGColumnVector3& Integrand,
                             FGColumnVector3& Val,
                             deque <FGColumnVector3>& ValDot,
                             double dt,
                             eIntegrateType integration_type)
{
  ValDot.push_front(Val);
  ValDot.pop_back();

  switch(integration_type) {
  case eRectEuler:       Integrand += dt*ValDot[0];
    break;
  case eTrapezoidal:     Integrand += 0.5*dt*(ValDot[0] + ValDot[1]);
    break;
  case eAdamsBashforth2: Integrand += dt*(1.5*ValDot[0] - 0.5*ValDot[1]);
    break;
  case eAdamsBashforth3: Integrand += (1/12.0)*dt*(23.0*ValDot[0] - 16.0*ValDot[1] + 5.0*ValDot[2]);
    break;
  case eAdamsBashforth4: Integrand += (1/24.0)*dt*(55.0*ValDot[0] - 59.0*ValDot[1] + 37.0*ValDot[2] - 9.0*ValDot[3]);
    break;
  case eAdamsBashforth5: Integrand += dt*((1901./720.)*ValDot[0] - (1387./360.)*ValDot[1] + (109./30.)*ValDot[2] - (637./360.)*ValDot[3] + (251./720.)*ValDot[4]);
    break;
  case eNone: // do nothing, freeze translational rate
    break;
  case eBuss1:
  case eBuss2:
  case eLocalLinearization:
    {
      LogException err(FDMExec->GetLogger());
      err << "Can only use Buss (1 & 2) or local linearization integration methods in for rotational position!";
      throw err;
    }
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::Integrate( FGQuaternion& Integrand,
                             FGQuaternion& Val,
                             deque <FGQuaternion>& ValDot,
                             double dt,
                             eIntegrateType integration_type)
{
  ValDot.push_front(Val);
  ValDot.pop_back();

  switch(integration_type) {
  case eRectEuler:       Integrand += dt*ValDot[0];
    break;
  case eTrapezoidal:     Integrand += 0.5*dt*(ValDot[0] + ValDot[1]);
    break;
  case eAdamsBashforth2: Integrand += dt*(1.5*ValDot[0] - 0.5*ValDot[1]);
    break;
  case eAdamsBashforth3: Integrand += (1/12.0)*dt*(23.0*ValDot[0] - 16.0*ValDot[1] + 5.0*ValDot[2]);
    break;
  case eAdamsBashforth4: Integrand += (1/24.0)*dt*(55.0*ValDot[0] - 59.0*ValDot[1] + 37.0*ValDot[2] - 9.0*ValDot[3]);
    break;
  case eAdamsBashforth5: Integrand += dt*((1901./720.)*ValDot[0] - (1387./360.)*ValDot[1] + (109./30.)*ValDot[2] - (637./360.)*ValDot[3] + (251./720.)*ValDot[4]);
    break;
  case eBuss1:
    {
      // This is the first order method as described in Samuel R. Buss paper[6].
      // The formula from Buss' paper is transposed below to quaternions and is
      // actually the exact solution of the quaternion differential equation
      // qdot = 1/2*w*q when w is constant.
      Integrand = Integrand * QExp(0.5 * dt * VState.vPQRi);
    }
    return; // No need to normalize since the quaternion exponential is always normal
  case eBuss2:
    {
      // This is the 'augmented second-order method' from S.R. Buss paper [6].
      // Unlike Runge-Kutta or Adams-Bashforth, it is a one-pass second-order
      // method (see reference [6]).
      FGColumnVector3 wi = VState.vPQRi;
      FGColumnVector3 wdoti = in.vPQRidot;
      FGColumnVector3 omega = wi + 0.5*dt*wdoti + dt*dt/12.*wdoti*wi;
      Integrand = Integrand * QExp(0.5 * dt * omega);
    }
    return; // No need to normalize since the quaternion exponential is always normal
  case eLocalLinearization:
    {
      // This is the local linearization algorithm of Barker et al. (see ref. [7])
      // It is also a one-pass second-order method. The code below is based on the
      // more compact formulation issued from equation (107) of ref. [8]. The
      // constants C1, C2, C3 and C4 have the same value than those in ref. [7] pp. 11
      FGColumnVector3 wi = 0.5 * VState.vPQRi;
      FGColumnVector3 wdoti = 0.5 * in.vPQRidot;
      double omegak2 = DotProduct(VState.vPQRi, VState.vPQRi);
      double omegak = omegak2 > 1E-6 ? sqrt(omegak2) : 1E-6;
      double rhok = 0.5 * dt * omegak;
      double C1 = cos(rhok);
      double C2 = 2.0 * sin(rhok) / omegak;
      double C3 = 4.0 * (1.0 - C1) / (omegak*omegak);
      double C4 = 4.0 * (dt - C2) / (omegak*omegak);
      FGColumnVector3 Omega = C2*wi + C3*wdoti + C4*wi*wdoti;
      FGQuaternion q;

      q(1) = C1 - C4*DotProduct(wi, wdoti);
      q(2) = Omega(eP);
      q(3) = Omega(eQ);
      q(4) = Omega(eR);

      Integrand = Integrand * q;

      /* Cross check with ref. [7] pp.11-12 formulas and code pp. 20
      double pk = VState.vPQRi(eP);
      double qk = VState.vPQRi(eQ);
      double rk = VState.vPQRi(eR);
      double pdotk = in.vPQRidot(eP);
      double qdotk = in.vPQRidot(eQ);
      double rdotk = in.vPQRidot(eR);
      double Ap = -0.25 * (pk*pdotk + qk*qdotk + rk*rdotk);
      double Bp = 0.25 * (pk*qdotk - qk*pdotk);
      double Cp = 0.25 * (pdotk*rk - pk*rdotk);
      double Dp = 0.25 * (qk*rdotk - qdotk*rk);
      double C2p = sin(rhok) / omegak;
      double C3p = 2.0 * (1.0 - cos(rhok)) / (omegak*omegak);
      double H = C1 + C4 * Ap;
      double G = -C2p*rk - C3p*rdotk + C4*Bp;
      double J = C2p*qk + C3p*qdotk - C4*Cp;
      double K = C2p*pk + C3p*pdotk - C4*Dp;

      FGLogging log(FDMExec->GetLogger(), LogLevel::INFO);
      log << "q:       " << q << "\n";

      // Warning! In the paper of Barker et al. the quaternion components are not
      // ordered the same way as in JSBSim (see equations (2) and (3) of ref. [7]
      // as well as the comment just below equation (3))
      log << "FORTRAN: " << H << " , " << K << " , " << J << " , " << -G << "\n";*/
    }
    break; // The quaternion q is not normal so the normalization needs to be done.
  case eNone: // do nothing, freeze rotational rate
    break;
  default:
    break;
  }

  Integrand.Normalize();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::UpdateLocationMatrices(void)
{
  Tl2ec = VState.vLocation.GetTl2ec(); // local to ECEF transform
  Tec2l = Tl2ec.Transposed();          // ECEF to local frame transform
  Ti2l  = Tec2l * Ti2ec;               // ECI to local frame transform
  Tl2i  = Ti2l.Transposed();           // local to ECI transform
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::UpdateBodyMatrices(void)
{
  Ti2b  = VState.qAttitudeECI.GetT(); // ECI to body frame transform
  Tb2i  = Ti2b.Transposed();          // body to ECI frame transform
  Tl2b  = Ti2b * Tl2i;                // local to body frame transform
  Tb2l  = Tl2b.Transposed();          // body to local frame transform
  Tec2b = Ti2b * Tec2i;               // ECEF to body frame transform
  Tb2ec = Tec2b.Transposed();         // body to ECEF frame tranform

  Qec2b = Tec2b.GetQuaternion();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::ComputeOrbitalParameters(void)
{
  const FGColumnVector3 Z{0., 0., 1.};
  FGColumnVector3 R = VState.vInertialPosition;
  FGColumnVector3 angularMomentum = R * VState.vInertialVelocity;
  h = angularMomentum.Magnitude();
  Inclination = acos(angularMomentum(eZ)/h)*radtodeg;
  FGColumnVector3 N;
  if (abs(Inclination) > 1E-8) {
    N = Z * angularMomentum;
    RightAscension = atan2(N(eY), N(eX))*radtodeg;
    N.Normalize();
  }
  else {
    RightAscension = 0.0;
    N = {1., 0., 0.};
    PerigeeArgument = 0.0;
  }
  R.Normalize();
  double vr = DotProduct(R, VState.vInertialVelocity);
  FGColumnVector3 eVector = (VState.vInertialVelocity*angularMomentum/in.GM - R);
  Eccentricity = eVector.Magnitude();
  if (Eccentricity > 1E-8) {
    eVector /= Eccentricity;
    if (abs(Inclination) > 1E-8) {
      PerigeeArgument = acos(DotProduct(N, eVector)) * radtodeg;
      if (eVector(eZ) < 0) PerigeeArgument = 360. - PerigeeArgument;
    }
  }
  else
  {
    eVector = {1., 0., 0.};
    PerigeeArgument = 0.0;
  }

  TrueAnomaly = acos(Constrain(-1.0, DotProduct(eVector, R), 1.0)) * radtodeg;
  if (vr < 0.0) TrueAnomaly = 360. - TrueAnomaly;
  ApoapsisRadius = h*h / (in.GM * (1-Eccentricity));
  PeriapsisRadius = h*h / (in.GM * (1+Eccentricity));

  if (Eccentricity < 1.0) {
    double semimajor = 0.5*(ApoapsisRadius + PeriapsisRadius);
    OrbitalPeriod = 2.*M_PI*pow(semimajor, 1.5)/sqrt(in.GM);
  }
  else
    OrbitalPeriod = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInertialOrientation(const FGQuaternion& Qi)
{
  VState.qAttitudeECI = Qi;
  VState.qAttitudeECI.Normalize();
  UpdateBodyMatrices();
  VState.qAttitudeLocal = Tl2b.GetQuaternion();
  CalculateQuatdot();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInertialVelocity(const FGColumnVector3& Vi) {
  VState.vInertialVelocity = Vi;
  CalculateUVW();
  vVel = Tb2l * VState.vUVW;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInertialRates(const FGColumnVector3& vRates) {
  VState.vPQRi = Ti2b * vRates;
  VState.vPQR = VState.vPQRi - Ti2b * in.vOmegaPlanet;
  CalculateQuatdot();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetAltitudeASL() const
{
  return VState.vLocation.GetRadius() - VState.vLocation.GetSeaLevelRadius();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetAltitudeASL(double altASL)
{
  double slr = VState.vLocation.GetSeaLevelRadius();
  VState.vLocation.SetRadius(slr + altASL);
  UpdateVehicleState();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::RecomputeLocalTerrainVelocity()
{
  FGLocation contact;
  FGColumnVector3 normal;
  Inertial->GetContactPoint(VState.vLocation, contact, normal,
                            LocalTerrainVelocity, LocalTerrainAngularVelocity);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetTerrainElevation(void) const
{
  FGColumnVector3 vDummy;
  FGLocation contact;
  contact.SetEllipse(in.SemiMajor, in.SemiMinor);
  Inertial->GetContactPoint(VState.vLocation, contact, vDummy, vDummy, vDummy);
  return contact.GetGeodAltitude();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetTerrainElevation(double terrainElev)
{
  Inertial->SetTerrainElevation(terrainElev);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetLocalTerrainRadius(void) const
{
  FGLocation contact;
  FGColumnVector3 vDummy;
  Inertial->GetContactPoint(VState.vLocation, contact, vDummy, vDummy, vDummy);
  return contact.GetRadius();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetDistanceAGL(void) const
{
  return Inertial->GetAltitudeAGL(VState.vLocation);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropagate::GetDistanceAGLKm(void) const
{
  return GetDistanceAGL()*0.0003048;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGL(double tt)
{
  Inertial->SetAltitudeAGL(VState.vLocation, tt);
  UpdateVehicleState();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGLKm(double tt)
{
  SetDistanceAGL(tt*3280.8399);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetVState(const VehicleState& vstate)
{
  //ToDo: Shouldn't all of these be set from the vstate vector passed in?
  VState.vLocation = vstate.vLocation;
  UpdateLocationMatrices();
  SetInertialOrientation(vstate.qAttitudeECI);
  RecomputeLocalTerrainVelocity();
  VState.vUVW = vstate.vUVW;
  vVel = Tb2l * VState.vUVW;
  VState.vPQR = vstate.vPQR;
  VState.vPQRi = VState.vPQR + Ti2b * in.vOmegaPlanet;
  VState.vInertialPosition = vstate.vInertialPosition;
  CalculateQuatdot();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::UpdateVehicleState(void)
{
  RecomputeLocalTerrainVelocity();
  VState.vInertialPosition = Tec2i * VState.vLocation;
  UpdateLocationMatrices();
  UpdateBodyMatrices();
  vVel = Tb2l * VState.vUVW;
  VState.qAttitudeLocal = Tl2b.GetQuaternion();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetLocation(const FGLocation& l)
{
  VState.vLocation = l;
  UpdateVehicleState();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGPropagate::GetEulerDeg(void) const
{
  return VState.qAttitudeLocal.GetEuler() * radtodeg;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::DumpState(void)
{
  FGLogging log(FDMExec->GetLogger(), LogLevel::INFO);
  log << "\n";
  log << LogFormat::BLUE
      << "------------------------------------------------------------------" << LogFormat::RESET << "\n";
  log << LogFormat::BOLD << fixed
      << "State Report at sim time: " << FDMExec->GetSimTime() << " seconds" << LogFormat::RESET << "\n";
  log << "  " << LogFormat::UNDERLINE_ON
      << "Position" << LogFormat::UNDERLINE_OFF << "\n";
  log << "    ECI:   " << VState.vInertialPosition.Dump(", ") << " (x,y,z, in ft)\n";
  log << "    ECEF:  " << VState.vLocation << " (x,y,z, in ft)\n";
  log << "    Local: " << VState.vLocation.GetGeodLatitudeDeg()
                       << ", " << VState.vLocation.GetLongitudeDeg()
                       << ", " << GetAltitudeASL() << " (geodetic lat, lon, alt ASL in deg and ft)\n";

  log << "\n  " << LogFormat::UNDERLINE_ON
      << "Orientation" << LogFormat::UNDERLINE_OFF << "\n";
  log << "    ECI:   " << VState.qAttitudeECI.GetEulerDeg().Dump(", ") << " (phi, theta, psi in deg)\n";
  log << "    Local: " << VState.qAttitudeLocal.GetEulerDeg().Dump(", ") << " (phi, theta, psi in deg)\n";

  log << "\n  " << LogFormat::UNDERLINE_ON
      << "Velocity" << LogFormat::UNDERLINE_OFF << "\n";
  log << "    ECI:   " << VState.vInertialVelocity.Dump(", ") << " (x,y,z in ft/s)\n";
  log << "    ECEF:  " << (Tb2ec * VState.vUVW).Dump(", ") << " (x,y,z in ft/s)\n";
  log << "    Local: " << GetVel() << " (n,e,d in ft/sec)\n";
  log << "    Body:  " << GetUVW() << " (u,v,w in ft/sec)\n";

  log << "\n" << "  " << LogFormat::UNDERLINE_ON
      << "Body Rates (relative to given frame, expressed in body frame)" << LogFormat::UNDERLINE_OFF << "\n";
  log << "    ECI:   " << (VState.vPQRi * radtodeg).Dump(", ") << " (p,q,r in deg/s)" << "\n";
  log << "    ECEF:  " << (VState.vPQR * radtodeg).Dump(", ") << " (p,q,r in deg/s)" << "\n";
}

//******************************************************************************

void FGPropagate::WriteStateFile(int num)
{
  sg_ofstream outfile;

  if (num == 0) return;

  SGPath path = FDMExec->GetOutputPath();

  if (path.isNull()) path = SGPath("initfile.");
  else               path.append("initfile.");

  // Append sim time to the filename since there may be more than one created during a simulation run
  path.concat(to_string((double)FDMExec->GetSimTime())+".xml");

  switch(num) {
  case 1:
    outfile.open(path);
    if (outfile.is_open()) {
      outfile << "<?xml version=\"1.0\"?>\n";
      outfile << "<initialize name=\"reset00\">\n";
      outfile << "  <ubody unit=\"FT/SEC\"> " << VState.vUVW(eU) << " </ubody> \n";
      outfile << "  <vbody unit=\"FT/SEC\"> " << VState.vUVW(eV) << " </vbody> \n";
      outfile << "  <wbody unit=\"FT/SEC\"> " << VState.vUVW(eW) << " </wbody> \n";
      outfile << "  <phi unit=\"DEG\"> " << VState.qAttitudeLocal.GetEuler(ePhi)*radtodeg << " </phi>\n";
      outfile << "  <theta unit=\"DEG\"> " << VState.qAttitudeLocal.GetEuler(eTht)*radtodeg << " </theta>\n";
      outfile << "  <psi unit=\"DEG\"> " << VState.qAttitudeLocal.GetEuler(ePsi)*radtodeg << " </psi>\n";
      outfile << "  <longitude unit=\"DEG\"> " << VState.vLocation.GetLongitudeDeg() << " </longitude>\n";
      outfile << "  <latitude unit=\"DEG\"> " << VState.vLocation.GetLatitudeDeg() << " </latitude>\n";
      outfile << "  <altitude unit=\"FT\"> " << GetDistanceAGL() << " </altitude>\n";
      outfile << "</initialize>\n";
      outfile.close();
    } else {
      FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
      log << "Could not open and/or write the state to the initial conditions file: "
          << path << "\n";
    }
    break;
  case 2:
    outfile.open(path);
    if (outfile.is_open()) {
      outfile << "<?xml version=\"1.0\"?>\n";
      outfile << "<initialize name=\"IC File\" version=\"2.0\">\n";
      outfile << "\n";
      outfile << "  <position frame=\"ECEF\">\n";
      outfile << "    <latitude unit=\"DEG\" type=\"geodetic\"> " << VState.vLocation.GetGeodLatitudeDeg() << " </latitude>\n";
      outfile << "    <longitude unit=\"DEG\"> " << VState.vLocation.GetLongitudeDeg() << " </longitude>\n";
      outfile << "    <altitudeMSL unit=\"FT\"> " << GetAltitudeASL() << " </altitudeMSL>\n";
      outfile << "  </position>\n";
      outfile << "\n";
      outfile << "  <orientation unit=\"DEG\" frame=\"LOCAL\">\n";
      outfile << "    <yaw> " << VState.qAttitudeLocal.GetEulerDeg(eYaw) << " </yaw>\n";
      outfile << "    <pitch> " << VState.qAttitudeLocal.GetEulerDeg(ePitch) << " </pitch>\n";
      outfile << "    <roll> " << VState.qAttitudeLocal.GetEulerDeg(eRoll) << " </roll>\n";
      outfile << "  </orientation>\n";
      outfile << "\n";
      outfile << "  <velocity unit=\"FT/SEC\" frame=\"LOCAL\">\n";
      outfile << "    <x> " << GetVel(eNorth) << " </x>\n";
      outfile << "    <y> " << GetVel(eEast) << " </y>\n";
      outfile << "    <z> " << GetVel(eDown) << " </z>\n";
      outfile << "  </velocity>\n";
      outfile << "\n";
      outfile << "  <attitude_rate unit=\"DEG/SEC\" frame=\"BODY\">\n";
      outfile << "    <roll> " << (VState.vPQR*radtodeg)(eRoll) << " </roll>\n";
      outfile << "    <pitch> " << (VState.vPQR*radtodeg)(ePitch) << " </pitch>\n";
      outfile << "    <yaw> " << (VState.vPQR*radtodeg)(eYaw) << " </yaw>\n";
      outfile << "  </attitude_rate>\n";
      outfile << "\n";
      outfile << "</initialize>\n";
      outfile.close();
    } else {
      FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
      log << "Could not open and/or write the state to the initial conditions file: "
          << path << "\n";
    }
    break;
  default:
    FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
    log << "When writing a state file, the supplied value must be 1 or 2 for the version number of the resulting IC file\n";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::bind(void)
{
  PropertyManager->Tie("velocities/h-dot-fps", this, &FGPropagate::Gethdot);

  PropertyManager->Tie("velocities/v-north-fps", this, eNorth, &FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-east-fps", this, eEast, &FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-down-fps", this, eDown, &FGPropagate::GetVel);

  PropertyManager->Tie("velocities/u-fps", this, eU, &FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/v-fps", this, eV, &FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/w-fps", this, eW, &FGPropagate::GetUVW);

  PropertyManager->Tie("velocities/p-rad_sec", this, eP, &FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/q-rad_sec", this, eQ, &FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/r-rad_sec", this, eR, &FGPropagate::GetPQR);

  PropertyManager->Tie("velocities/pi-rad_sec", this, eP, &FGPropagate::GetPQRi);
  PropertyManager->Tie("velocities/qi-rad_sec", this, eQ, &FGPropagate::GetPQRi);
  PropertyManager->Tie("velocities/ri-rad_sec", this, eR, &FGPropagate::GetPQRi);

  PropertyManager->Tie("velocities/eci-x-fps", this, eX, &FGPropagate::GetInertialVelocity);
  PropertyManager->Tie("velocities/eci-y-fps", this, eY, &FGPropagate::GetInertialVelocity);
  PropertyManager->Tie("velocities/eci-z-fps", this, eZ, &FGPropagate::GetInertialVelocity);

  PropertyManager->Tie("velocities/eci-velocity-mag-fps", this, &FGPropagate::GetInertialVelocityMagnitude);
  PropertyManager->Tie("velocities/ned-velocity-mag-fps", this, &FGPropagate::GetNEDVelocityMagnitude);

  PropertyManager->Tie("position/h-sl-ft", this, &FGPropagate::GetAltitudeASL,
                       &FGPropagate::SetAltitudeASL);
  PropertyManager->Tie("position/h-sl-meters", this, &FGPropagate::GetAltitudeASLmeters,
                       &FGPropagate::SetAltitudeASLmeters);
  PropertyManager->Tie("position/lat-gc-rad", this, &FGPropagate::GetLatitude,
                       &FGPropagate::SetLatitude);
  PropertyManager->Tie("position/long-gc-rad", this, &FGPropagate::GetLongitude,
                       &FGPropagate::SetLongitude);
  PropertyManager->Tie("position/lat-gc-deg", this, &FGPropagate::GetLatitudeDeg,
                       &FGPropagate::SetLatitudeDeg);
  PropertyManager->Tie("position/long-gc-deg", this, &FGPropagate::GetLongitudeDeg,
                       &FGPropagate::SetLongitudeDeg);
  PropertyManager->Tie("position/lat-geod-rad", this, &FGPropagate::GetGeodLatitudeRad);
  PropertyManager->Tie("position/lat-geod-deg", this, &FGPropagate::GetGeodLatitudeDeg);
  PropertyManager->Tie("position/geod-alt-ft", this, &FGPropagate::GetGeodeticAltitude);
  PropertyManager->Tie("position/h-agl-ft", this,  &FGPropagate::GetDistanceAGL,
                       &FGPropagate::SetDistanceAGL);
  PropertyManager->Tie("position/geod-alt-km", this, &FGPropagate::GetGeodeticAltitudeKm);
  PropertyManager->Tie("position/h-agl-km", this,  &FGPropagate::GetDistanceAGLKm,
                       &FGPropagate::SetDistanceAGLKm);
  PropertyManager->Tie("position/radius-to-vehicle-ft", this, &FGPropagate::GetRadius);
  PropertyManager->Tie("position/terrain-elevation-asl-ft", this, &FGPropagate::GetTerrainElevation,
                       &FGPropagate::SetTerrainElevation);

  PropertyManager->Tie("position/eci-x-ft", this, eX, &FGPropagate::GetInertialPosition);
  PropertyManager->Tie("position/eci-y-ft", this, eY, &FGPropagate::GetInertialPosition);
  PropertyManager->Tie("position/eci-z-ft", this, eZ, &FGPropagate::GetInertialPosition);

  PropertyManager->Tie("position/ecef-x-ft", this, eX, &FGPropagate::GetLocation);
  PropertyManager->Tie("position/ecef-y-ft", this, eY, &FGPropagate::GetLocation);
  PropertyManager->Tie("position/ecef-z-ft", this, eZ, &FGPropagate::GetLocation);

  PropertyManager->Tie("position/epa-rad", this, &FGPropagate::GetEarthPositionAngle);
  PropertyManager->Tie("metrics/terrain-radius", this, &FGPropagate::GetLocalTerrainRadius);

  PropertyManager->Tie("attitude/phi-rad", this, ePhi, &FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/theta-rad", this, eTht, &FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/psi-rad", this, ePsi, &FGPropagate::GetEuler);

  PropertyManager->Tie("attitude/phi-deg", this, ePhi, &FGPropagate::GetEulerDeg);
  PropertyManager->Tie("attitude/theta-deg", this, eTht, &FGPropagate::GetEulerDeg);
  PropertyManager->Tie("attitude/psi-deg", this, ePsi, &FGPropagate::GetEulerDeg);

  PropertyManager->Tie("attitude/roll-rad", this, ePhi, &FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this, eTht, &FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this, ePsi, &FGPropagate::GetEuler);

  PropertyManager->Tie("orbital/specific-angular-momentum-ft2_sec", &h);
  PropertyManager->Tie("orbital/inclination-deg", &Inclination);
  PropertyManager->Tie("orbital/right-ascension-deg", &RightAscension);
  PropertyManager->Tie("orbital/eccentricity", &Eccentricity);
  PropertyManager->Tie("orbital/argument-of-perigee-deg", &PerigeeArgument);
  PropertyManager->Tie("orbital/true-anomaly-deg", &TrueAnomaly);
  PropertyManager->Tie("orbital/apoapsis-radius-ft", &ApoapsisRadius);
  PropertyManager->Tie("orbital/periapsis-radius-ft", &PeriapsisRadius);
  PropertyManager->Tie("orbital/period-sec", &OrbitalPeriod);

  PropertyManager->Tie("simulation/integrator/rate/rotational", (int*)&integrator_rotational_rate);
  PropertyManager->Tie("simulation/integrator/rate/translational", (int*)&integrator_translational_rate);
  PropertyManager->Tie("simulation/integrator/position/rotational", (int*)&integrator_rotational_position);
  PropertyManager->Tie("simulation/integrator/position/translational", (int*)&integrator_translational_position);

  PropertyManager->Tie<FGPropagate, int>("simulation/write-state-file", this,
                                         nullptr, &FGPropagate::WriteStateFile);
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
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGPropagate\n";
    if (from == 1) log << "Destroyed:    FGPropagate\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 && from == 2) { // Runtime state variables
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    log << "\n" << LogFormat::BLUE << LogFormat::BOLD << left << fixed
        << "  Propagation Report (English units: ft, degrees) at simulation time " << FDMExec->GetSimTime() << " seconds"
        << LogFormat::RESET << "\n\n";
    log << LogFormat::BOLD << "  Earth Position Angle (deg): " << setw(8) << setprecision(3) << LogFormat::RESET
        << GetEarthPositionAngleDeg() << "\n\n";
    log << LogFormat::BOLD << "  Body velocity (ft/sec): " << setw(8) << setprecision(3) << LogFormat::RESET << VState.vUVW << "\n";
    log << LogFormat::BOLD << "  Local velocity (ft/sec): " << setw(8) << setprecision(3) << LogFormat::RESET << vVel << "\n";
    log << LogFormat::BOLD << "  Inertial velocity (ft/sec): " << setw(8) << setprecision(3) << LogFormat::RESET << VState.vInertialVelocity << "\n";
    log << LogFormat::BOLD << "  Inertial Position (ft): " << setw(10) << setprecision(3) << LogFormat::RESET << VState.vInertialPosition << "\n";
    log << LogFormat::BOLD << "  Latitude (deg): " << setw(8) << setprecision(3) << LogFormat::RESET << VState.vLocation.GetLatitudeDeg() << "\n";
    log << LogFormat::BOLD << "  Longitude (deg): " << setw(8) << setprecision(3) << LogFormat::RESET << VState.vLocation.GetLongitudeDeg() << "\n";
    log << LogFormat::BOLD << "  Altitude ASL (ft): " << setw(8) << setprecision(3) << LogFormat::RESET << GetAltitudeASL() << "\n";
    //    log << LogFormat::BOLD << "  Acceleration (NED, ft/sec^2): " << setw(8) << setprecision(3) << LogFormat::RESET << Tb2l*GetUVWdot() << "\n";
    log << "\n";
    log << LogFormat::BOLD << "  Matrix ECEF to Body (Orientation of Body with respect to ECEF): \n"
        << LogFormat::RESET << Tec2b.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tec2b.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Body to ECEF (Orientation of ECEF with respect to Body):\n"
        << LogFormat::RESET << Tb2ec.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tb2ec.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Local to Body (Orientation of Body with respect to Local):\n"
        << LogFormat::RESET << Tl2b.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tl2b.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Body to Local (Orientation of Local with respect to Body):\n"
        << LogFormat::RESET << Tb2l.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tb2l.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Local to ECEF (Orientation of ECEF with respect to Local):\n"
        << LogFormat::RESET << Tl2ec.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tl2ec.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix ECEF to Local (Orientation of Local with respect to ECEF):\n"
        << LogFormat::RESET << Tec2l.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tec2l.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix ECEF to Inertial (Orientation of Inertial with respect to ECEF):\n"
        << LogFormat::RESET << Tec2i.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tec2i.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Inertial to ECEF (Orientation of ECEF with respect to Inertial):\n"
        << LogFormat::RESET << Ti2ec.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Ti2ec.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Inertial to Body (Orientation of Body with respect to Inertial):\n"
        << LogFormat::RESET << Ti2b.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Ti2b.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Body to Inertial (Orientation of Inertial with respect to Body):\n"
        << LogFormat::RESET << Tb2i.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tb2i.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Inertial to Local (Orientation of Local with respect to Inertial):\n"
        << LogFormat::RESET << Ti2l.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Ti2l.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";

    log << LogFormat::BOLD << "  Matrix Local to Inertial (Orientation of Inertial with respect to Local):\n"
        << LogFormat::RESET << Tl2i.Dump("\t", "    ");
    log << LogFormat::BOLD << "\n    Associated Euler angles (deg): " << setw(8)
        << setprecision(3) << LogFormat::RESET << (Tl2i.GetQuaternion().GetEuler()*radtodeg)
                  << "\n\n";
  }
  if (debug_lvl & 16) { // Sanity checking
    if (from == 2) { // State sanity checking
      if (fabs(VState.vPQR.Magnitude()) > 1000.0) {
        LogException err(FDMExec->GetLogger());
        err << "Vehicle rotation rate is excessive (>1000 rad/sec): " << VState.vPQR.Magnitude() << "\n";
        throw err;
      }
      if (fabs(VState.vUVW.Magnitude()) > 1.0e10) {
        LogException err(FDMExec->GetLogger());
        err << "Vehicle velocity is excessive (>1e10 ft/sec): " << VState.vUVW.Magnitude() << "\n";
        throw err;
      }
      if (fabs(GetDistanceAGL()) > 1e10) {
        LogException err(FDMExec->GetLogger());
        err << "Vehicle altitude is excessive (>1e10 ft): " << GetDistanceAGL() << "\n";
        throw err;
      }
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
