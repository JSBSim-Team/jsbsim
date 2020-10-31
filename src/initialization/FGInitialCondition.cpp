/*******************************************************************************

 Header:       FGInitialCondition.cpp
 Author:       Tony Peden, Bertrand Coconnier
 Date started: 7/1/99

 --------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) ---------

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
7/1/99   TP   Created
11/25/10 BC   Complete revision - Use minimal set of variables to prevent
              inconsistent states. Wind is correctly handled.


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to take a set of initial conditions and provide a
kinematically consistent set of body axis velocity components, euler angles, and
altitude.  This class does not attempt to trim the model i.e.  the sim will most
likely start in a very dynamic state (unless, of course, you have chosen your
IC's wisely) even after setting it up with this class.

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGInitialCondition.h"
#include "models/FGInertial.h"
#include "models/FGAtmosphere.h"
#include "models/FGAccelerations.h"
#include "input_output/FGXMLFileRead.h"
#include "FGTrim.h"

using namespace std;

namespace JSBSim {

//******************************************************************************

FGInitialCondition::FGInitialCondition(FGFDMExec *FDMExec) : fdmex(FDMExec)
{
  InitializeIC();

  if(FDMExec) {
    Atmosphere=fdmex->GetAtmosphere();
    Aircraft=fdmex->GetAircraft();
  } else {
    cout << "FGInitialCondition: This class requires a pointer to a valid FGFDMExec object" << endl;
  }

  Debug(0);
}

//******************************************************************************

FGInitialCondition::~FGInitialCondition()
{
  Debug(1);
}

//******************************************************************************

void FGInitialCondition::ResetIC(double u0, double v0, double w0,
                                 double p0, double q0, double r0,
                                 double alpha0, double beta0,
                                 double phi0, double theta0, double psi0,
                                 double latRad0, double lonRad0, double altAGLFt0,
                                 double gamma0)
{
  double calpha = cos(alpha0), cbeta = cos(beta0);
  double salpha = sin(alpha0), sbeta = sin(beta0);

  InitializeIC();

  vPQR_body = {p0, q0, r0};
  alpha = alpha0;  beta = beta0;

  position.SetLongitude(lonRad0);
  position.SetLatitude(latRad0);
  fdmex->GetInertial()->SetAltitudeAGL(position, altAGLFt0);
  lastLatitudeSet = setgeoc;
  lastAltitudeSet = setagl;

  orientation = FGQuaternion(phi0, theta0, psi0);
  const FGMatrix33& Tb2l = orientation.GetTInv();

  vUVW_NED = Tb2l * FGColumnVector3(u0, v0, w0);
  vt = vUVW_NED.Magnitude();
  lastSpeedSet = setuvw;

  Tw2b = { calpha*cbeta, -calpha*sbeta,  -salpha,
                  sbeta,         cbeta,      0.0,
           salpha*cbeta, -salpha*sbeta,   calpha };
  Tb2w = Tw2b.Transposed();

  SetFlightPathAngleRadIC(gamma0);
}

//******************************************************************************

void FGInitialCondition::InitializeIC(void)
{
  alpha = beta = 0.0;
  epa = 0.0;

  double a = fdmex->GetInertial()->GetSemimajor();
  double b = fdmex->GetInertial()->GetSemiminor();

  position.SetEllipse(a, b);

  position.SetPositionGeodetic(0.0, 0.0, 0.0);

  orientation = FGQuaternion(0.0, 0.0, 0.0);
  vUVW_NED.InitMatrix();
  vPQR_body.InitMatrix();
  vt=0;

  targetNlfIC = 1.0;

  Tw2b = { 1., 0., 0., 0., 1., 0., 0., 0., 1. };
  Tb2w = { 1., 0., 0., 0., 1., 0., 0., 0., 1. };

  lastSpeedSet = setvt;
  lastAltitudeSet = setasl;
  lastLatitudeSet = setgeoc;
  enginesRunning = 0;
  trimRequested = TrimMode::tNone;
}

//******************************************************************************

void FGInitialCondition::SetVequivalentKtsIC(double ve)
{
  double altitudeASL = GetAltitudeASLFtIC();
  double rho = Atmosphere->GetDensity(altitudeASL);
  double rhoSL = Atmosphere->GetDensitySL();
  SetVtrueFpsIC(ve*ktstofps*sqrt(rhoSL/rho));
  lastSpeedSet = setve;
}

//******************************************************************************

void FGInitialCondition::SetMachIC(double mach)
{
  double altitudeASL = GetAltitudeASLFtIC();
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  SetVtrueFpsIC(mach*soundSpeed);
  lastSpeedSet = setmach;
}

//******************************************************************************

void FGInitialCondition::SetVcalibratedKtsIC(double vcas)
{
  double altitudeASL = GetAltitudeASLFtIC();
  double pressure = Atmosphere->GetPressure(altitudeASL);
  double mach = MachFromVcalibrated(fabs(vcas)*ktstofps, pressure);
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);

  SetVtrueFpsIC(mach * soundSpeed);
  lastSpeedSet = setvc;
}

//******************************************************************************
// Updates alpha and beta according to the aircraft true airspeed in the local
// NED frame.

void FGInitialCondition::calcAeroAngles(const FGColumnVector3& _vt_NED)
{
  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 _vt_BODY = Tl2b * _vt_NED;
  double ua = _vt_BODY(eX);
  double va = _vt_BODY(eY);
  double wa = _vt_BODY(eZ);
  double uwa = sqrt(ua*ua + wa*wa);
  double calpha, cbeta;
  double salpha, sbeta;

  alpha = beta = 0.0;
  calpha = cbeta = 1.0;
  salpha = sbeta = 0.0;

  if( wa != 0 )
    alpha = atan2( wa, ua );

  // alpha cannot be constrained without updating other informations like the
  // true speed or the Euler angles. Otherwise we might end up with an
  // inconsistent state of the aircraft.
  /*alpha = Constrain(fdmex->GetAerodynamics()->GetAlphaCLMin(), alpha,
                    fdmex->GetAerodynamics()->GetAlphaCLMax());*/

  if( va != 0 )
    beta = atan2( va, uwa );

  if (uwa != 0) {
    calpha = ua / uwa;
    salpha = wa / uwa;
  }

  if (vt != 0) {
    cbeta = uwa / vt;
    sbeta = va / vt;
  }

  Tw2b = { calpha*cbeta, -calpha*sbeta,  -salpha,
                  sbeta,         cbeta,      0.0,
           salpha*cbeta, -salpha*sbeta,   calpha };
  Tb2w = Tw2b.Transposed();
}

//******************************************************************************
// Set the ground velocity. Caution it sets the vertical velocity to zero to
// keep backward compatibility.

void FGInitialCondition::SetVgroundFpsIC(double vg)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  vUVW_NED(eU) = vg * orientation.GetCosEuler(ePsi);
  vUVW_NED(eV) = vg * orientation.GetSinEuler(ePsi);
  vUVW_NED(eW) = 0.;
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);

  lastSpeedSet = setvg;
}

//******************************************************************************
// Sets the true airspeed. The amplitude of the airspeed is modified but its
// direction is kept unchanged. If there is no wind, the same is true for the
// ground velocity. If there is some wind, the airspeed direction is unchanged
// but this may result in the ground velocity direction being altered. This is
// for backward compatibility.

void FGInitialCondition::SetVtrueFpsIC(double vtrue)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  if (vt > 0.1)
    _vt_NED *= vtrue / vt;
  else
    _vt_NED = Tb2l * Tw2b * FGColumnVector3(vtrue, 0., 0.);

  vt = vtrue;
  vUVW_NED = _vt_NED - _vWIND_NED;

  calcAeroAngles(_vt_NED);

  lastSpeedSet = setvt;
}

//******************************************************************************
// When the climb rate is modified, we need to update the angles theta and beta
// to keep the true airspeed amplitude, the AoA and the heading unchanged.
// Beta will be modified if the aircraft roll angle is not null.

void FGInitialCondition::SetClimbRateFpsIC(double hdot)
{
  if (fabs(hdot) > vt) {
    cerr << "The climb rate cannot be higher than the true speed." << endl;
    return;
  }

  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _WIND_NED = _vt_NED - vUVW_NED;
  double hdot0 = -_vt_NED(eW);

  if (fabs(hdot0) < vt) { // Is this check really needed ?
    double scale = sqrt((vt*vt-hdot*hdot)/(vt*vt-hdot0*hdot0));
    _vt_NED(eU) *= scale;
    _vt_NED(eV) *= scale;
  }
  _vt_NED(eW) = -hdot;
  vUVW_NED = _vt_NED - _WIND_NED;

  // Updating the angles theta and beta to keep the true airspeed amplitude
  calcThetaBeta(alpha, _vt_NED);
}

//******************************************************************************
// When the AoA is modified, we need to update the angles theta and beta to
// keep the true airspeed amplitude, the climb rate and the heading unchanged.
// Beta will be modified if the aircraft roll angle is not null.

void FGInitialCondition::SetAlphaRadIC(double alfa)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  calcThetaBeta(alfa, _vt_NED);
}

//******************************************************************************
// When the AoA is modified, we need to update the angles theta and beta to
// keep the true airspeed amplitude, the climb rate and the heading unchanged.
// Beta will be modified if the aircraft roll angle is not null.

void FGInitialCondition::calcThetaBeta(double alfa, const FGColumnVector3& _vt_NED)
{
  FGColumnVector3 vOrient = orientation.GetEuler();
  double calpha = cos(alfa), salpha = sin(alfa);
  double cpsi = orientation.GetCosEuler(ePsi), spsi = orientation.GetSinEuler(ePsi);
  double cphi = orientation.GetCosEuler(ePhi), sphi = orientation.GetSinEuler(ePhi);
  FGMatrix33 Tpsi( cpsi, spsi, 0.,
                  -spsi, cpsi, 0.,
                     0.,   0., 1.);
  FGMatrix33 Tphi(1.,   0.,   0.,
                  0., cphi, sphi,
                  0.,-sphi, cphi);
  FGMatrix33 Talpha( calpha, 0., salpha,
                         0., 1.,    0.,
                    -salpha, 0., calpha);

  FGColumnVector3 v0 = Tpsi * _vt_NED;
  FGColumnVector3 n = (Talpha * Tphi).Transposed() * FGColumnVector3(0., 0., 1.);
  FGColumnVector3 y = {0., 1., 0.};
  FGColumnVector3 u = y - DotProduct(y, n) * n;
  FGColumnVector3 p = y * n;

  if (DotProduct(p, v0) < 0) p *= -1.0;
  p.Normalize();

  u *= DotProduct(v0, y) / DotProduct(u, y);

  // There are situations where the desired alpha angle cannot be obtained. This
  // is not a limitation of the algorithm but is due to the mathematical problem
  // not having a solution. This can only be cured by limiting the alpha angle
  // or by modifying an additional angle (psi ?). Since this is anticipated to
  // be a pathological case (mainly when a high roll angle is required) this
  // situation is not addressed below. However if there are complaints about the
  // following error being raised too often, we might need to reconsider this
  // position.
  if (DotProduct(v0, v0) < DotProduct(u, u)) {
    cerr << "Cannot modify angle 'alpha' from " << alpha << " to " << alfa << endl;
    return;
  }

  FGColumnVector3 v1 = u + sqrt(DotProduct(v0, v0) - DotProduct(u, u))*p;

  FGColumnVector3 v0xz(v0(eU), 0., v0(eW));
  FGColumnVector3 v1xz(v1(eU), 0., v1(eW));
  v0xz.Normalize();
  v1xz.Normalize();
  double sinTheta = (v1xz * v0xz)(eY);
  vOrient(eTht) = asin(sinTheta);

  orientation = FGQuaternion(vOrient);

  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 v2 = Talpha * Tl2b * _vt_NED;

  alpha = alfa;
  beta = atan2(v2(eV), v2(eU));
  double cbeta=1.0, sbeta=0.0;
  if (vt != 0.0) {
    cbeta = v2(eU) / vt;
    sbeta = v2(eV) / vt;
  }
  Tw2b = { calpha*cbeta, -calpha*sbeta,  -salpha,
                  sbeta,         cbeta,      0.0,
           salpha*cbeta, -salpha*sbeta,   calpha };
  Tb2w = Tw2b.Transposed();
}

//******************************************************************************
// When the beta angle is modified, we need to update the angles theta and psi
// to keep the true airspeed (amplitude and direction - including the climb rate)
// and the alpha angle unchanged. This may result in the aircraft heading (psi)
// being altered especially if there is cross wind.

void FGInitialCondition::SetBetaRadIC(double bta)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 vOrient = orientation.GetEuler();

  beta = bta;
  double calpha = cos(alpha), salpha = sin(alpha);
  double cbeta = cos(beta), sbeta = sin(beta);
  double cphi = orientation.GetCosEuler(ePhi), sphi = orientation.GetSinEuler(ePhi);
  FGMatrix33 TphiInv(1.,   0.,   0.,
                     0., cphi,-sphi,
                     0., sphi, cphi);

  Tw2b = { calpha*cbeta, -calpha*sbeta,  -salpha,
                  sbeta,         cbeta,      0.0,
           salpha*cbeta, -salpha*sbeta,   calpha };
  Tb2w = Tw2b.Transposed();

  FGColumnVector3 vf = TphiInv * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 v0xy(_vt_NED(eX), _vt_NED(eY), 0.);
  FGColumnVector3 v1xy(sqrt(v0xy(eX)*v0xy(eX)+v0xy(eY)*v0xy(eY)-vf(eY)*vf(eY)),vf(eY),0.);
  v0xy.Normalize();
  v1xy.Normalize();

  if (vf(eX) < 0.) v0xy(eX) *= -1.0;

  double sinPsi = (v1xy * v0xy)(eZ);
  double cosPsi = DotProduct(v0xy, v1xy);
  vOrient(ePsi) = atan2(sinPsi, cosPsi);
  FGMatrix33 Tpsi( cosPsi, sinPsi, 0.,
                  -sinPsi, cosPsi, 0.,
                      0.,     0., 1.);

  FGColumnVector3 v2xz = Tpsi * _vt_NED;
  FGColumnVector3 vfxz = vf;
  v2xz(eV) = vfxz(eV) = 0.0;
  v2xz.Normalize();
  vfxz.Normalize();
  double sinTheta = (v2xz * vfxz)(eY);
  vOrient(eTht) = -asin(sinTheta);

  orientation = FGQuaternion(vOrient);
}

//******************************************************************************
// Modifies the body frame orientation.

void FGInitialCondition::SetEulerAngleRadIC(int idx, double angle)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;
  FGColumnVector3 vOrient = orientation.GetEuler();

  vOrient(idx) = angle;
  orientation = FGQuaternion(vOrient);

  if ((lastSpeedSet != setned) && (lastSpeedSet != setvg)) {
    const FGMatrix33& newTb2l = orientation.GetTInv();
    vUVW_NED = newTb2l * _vUVW_BODY;
    _vt_NED = vUVW_NED + _vWIND_NED;
    vt = _vt_NED.Magnitude();
  }

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies an aircraft velocity component (eU, eV or eW) in the body frame. The
// true airspeed is modified accordingly. If there is some wind, the airspeed
// direction modification may differ from the body velocity modification.

void FGInitialCondition::SetBodyVelFpsIC(int idx, double vel)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  _vUVW_BODY(idx) = vel;
  vUVW_NED = Tb2l * _vUVW_BODY;
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);

  lastSpeedSet = setuvw;
}

//******************************************************************************
// Modifies an aircraft velocity component (eX, eY or eZ) in the local NED frame.
// The true airspeed is modified accordingly. If there is some wind, the airspeed
// direction modification may differ from the local velocity modification.

void FGInitialCondition::SetNEDVelFpsIC(int idx, double vel)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  vUVW_NED(idx) = vel;
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);

  lastSpeedSet = setned;
}

//******************************************************************************
// Set wind amplitude and direction in the local NED frame. The aircraft velocity
// with respect to the ground is not changed but the true airspeed is.

void FGInitialCondition::SetWindNEDFpsIC(double wN, double wE, double wD )
{
  FGColumnVector3 _vt_NED = vUVW_NED + FGColumnVector3(wN, wE, wD);
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Set the cross wind velocity (in knots). Here, 'cross wind' means perpendicular
// to the aircraft heading and parallel to the ground. The aircraft velocity
// with respect to the ground is not changed but the true airspeed is.

void FGInitialCondition::SetCrossWindKtsIC(double cross)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vCROSS(-orientation.GetSinEuler(ePsi), orientation.GetCosEuler(ePsi), 0.);

  // Gram-Schmidt process is used to remove the existing cross wind component
  _vWIND_NED -= DotProduct(_vWIND_NED, _vCROSS) * _vCROSS;
  // Which is now replaced by the new value. The input cross wind is expected
  // in knots, so first convert to fps, which is the internal unit used.
  _vWIND_NED += (cross * ktstofps) * _vCROSS;
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Set the head wind velocity (in knots). Here, 'head wind' means parallel
// to the aircraft heading and to the ground. The aircraft velocity
// with respect to the ground is not changed but the true airspeed is.

void FGInitialCondition::SetHeadWindKtsIC(double head)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  // This is a head wind, so the direction vector for the wind
  // needs to be set opposite to the heading the aircraft
  // is taking. So, the cos and sin of the heading (psi)
  // are negated in the line below.
  FGColumnVector3 _vHEAD(-orientation.GetCosEuler(ePsi), -orientation.GetSinEuler(ePsi), 0.);

  // Gram-Schmidt process is used to remove the existing head wind component
  _vWIND_NED -= DotProduct(_vWIND_NED, _vHEAD) * _vHEAD;
  // Which is now replaced by the new value. The input head wind is expected
  // in knots, so first convert to fps, which is the internal unit used.
  _vWIND_NED += (head * ktstofps) * _vHEAD;
  _vt_NED = vUVW_NED + _vWIND_NED;

  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Set the vertical wind velocity (in knots). The 'vertical' is taken in the
// local NED frame. The aircraft velocity with respect to the ground is not
// changed but the true airspeed is.

void FGInitialCondition::SetWindDownKtsIC(double wD)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  _vt_NED(eW) = vUVW_NED(eW) + wD;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies the wind velocity (in knots) while keeping its direction unchanged.
// The vertical component (in local NED frame) is unmodified. The aircraft
// velocity with respect to the ground is not changed but the true airspeed is.

void FGInitialCondition::SetWindMagKtsIC(double mag)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vHEAD(_vWIND_NED(eU), _vWIND_NED(eV), 0.);
  double windMag = _vHEAD.Magnitude();

  if (windMag > 0.001)
    _vHEAD *= (mag*ktstofps) / windMag;
  else
    _vHEAD = {mag*ktstofps, 0., 0.};

  _vWIND_NED(eU) = _vHEAD(eU);
  _vWIND_NED(eV) = _vHEAD(eV);
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies the wind direction while keeping its velocity unchanged. The vertical
// component (in local NED frame) is unmodified. The aircraft velocity with
// respect to the ground is not changed but the true airspeed is.

void FGInitialCondition::SetWindDirDegIC(double dir)
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  double mag = _vWIND_NED.Magnitude(eU, eV);
  FGColumnVector3 _vHEAD(mag*cos(dir*degtorad), mag*sin(dir*degtorad), 0.);

  _vWIND_NED(eU) = _vHEAD(eU);
  _vWIND_NED(eV) = _vHEAD(eV);
  _vt_NED = vUVW_NED + _vWIND_NED;
  vt = _vt_NED.Magnitude();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************

void FGInitialCondition::SetTerrainElevationFtIC(double elev)
{
  double agl = GetAltitudeAGLFtIC();
  fdmex->GetInertial()->SetTerrainElevation(elev);

  if (lastAltitudeSet == setagl)
    SetAltitudeAGLFtIC(agl);
}

//******************************************************************************

double FGInitialCondition::GetAltitudeASLFtIC(void) const
{
  return position.GetRadius() - position.GetSeaLevelRadius();
}

//******************************************************************************

  double FGInitialCondition::GetAltitudeAGLFtIC(void) const
{
  return fdmex->GetInertial()->GetAltitudeAGL(position);
}

//******************************************************************************

double FGInitialCondition::GetTerrainElevationFtIC(void) const
{
  FGColumnVector3 normal, v, w;
  FGLocation contact;
  double a = fdmex->GetInertial()->GetSemimajor();
  double b = fdmex->GetInertial()->GetSemiminor();
  contact.SetEllipse(a, b);
  fdmex->GetInertial()->GetContactPoint(position, contact, normal, v, w);
  return contact.GetGeodAltitude();
}

//******************************************************************************

void FGInitialCondition::SetAltitudeAGLFtIC(double agl)
{
  double altitudeASL = GetAltitudeASLFtIC();
  double pressure = Atmosphere->GetPressure(altitudeASL);
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  double rho = Atmosphere->GetDensity(altitudeASL);
  double rhoSL = Atmosphere->GetDensitySL();

  double mach0 = vt / soundSpeed;
  double vc0 = VcalibratedFromMach(mach0, pressure);
  double ve0 = vt * sqrt(rho/rhoSL);

  switch(lastLatitudeSet) {
  case setgeod:
    fdmex->GetInertial()->SetAltitudeAGL(position, agl);
    break;
  case setgeoc:
    {
      double a = fdmex->GetInertial()->GetSemimajor();
      double b = fdmex->GetInertial()->GetSemiminor();
      double e2 = 1.0-b*b/(a*a);
      double tanlat = tan(position.GetLatitude());
      double n = e2;
      double prev_n = 1.0;
      int iter = 0;
      double longitude = position.GetLongitude();
      double alt = position.GetGeodAltitude();
      double h = -2.0*max(a,b);
      double geodLat;
      while ((fabs(n-prev_n) > 1E-15 || fabs(h-agl) > 1E-10) && iter < 10) {
        geodLat = atan(tanlat/(1-n));
        position.SetPositionGeodetic(longitude, geodLat, alt);
        h = GetAltitudeAGLFtIC();
        alt += agl-h;
        double sinGeodLat = sin(geodLat);
        double N = a/sqrt(1-e2*sinGeodLat*sinGeodLat);
        prev_n = n;
        n = e2*N/(N+alt);
        iter++;
      }
    }
    break;
  }

  altitudeASL = GetAltitudeASLFtIC();
  soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  rho = Atmosphere->GetDensity(altitudeASL);
  pressure = Atmosphere->GetPressure(altitudeASL);

  switch(lastSpeedSet) {
    case setvc:
      mach0 = MachFromVcalibrated(vc0, pressure);
      SetVtrueFpsIC(mach0 * soundSpeed);
      break;
    case setmach:
      SetVtrueFpsIC(mach0 * soundSpeed);
      break;
    case setve:
      SetVtrueFpsIC(ve0 * sqrt(rhoSL/rho));
      break;
    default: // Make the compiler stop complaining about missing enums
      break;
  }

  lastAltitudeSet = setagl;
}

//******************************************************************************
// Set the altitude SL. If the airspeed has been previously set with parameters
// that are atmosphere dependent (Mach, VCAS, VEAS) then the true airspeed is
// modified to keep the last set speed to its previous value.

void FGInitialCondition::SetAltitudeASLFtIC(double alt)
{
  double altitudeASL = GetAltitudeASLFtIC();
  double pressure = Atmosphere->GetPressure(altitudeASL);
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  double rho = Atmosphere->GetDensity(altitudeASL);
  double rhoSL = Atmosphere->GetDensitySL();

  double mach0 = vt / soundSpeed;
  double vc0 = VcalibratedFromMach(mach0, pressure);
  double ve0 = vt * sqrt(rho/rhoSL);

  switch(lastLatitudeSet) {
  case setgeod:
    {
      // Given an altitude above the mean sea level (or a position radius which
      // is the same) and a geodetic latitude, compute the geodetic altitude.
      double a = fdmex->GetInertial()->GetSemimajor();
      double b = fdmex->GetInertial()->GetSemiminor();
      double e2 = 1.0-b*b/(a*a);
      double geodLatitude = position.GetGeodLatitudeRad();
      double cosGeodLat = cos(geodLatitude);
      double sinGeodLat = sin(geodLatitude);
      double N = a/sqrt(1-e2*sinGeodLat*sinGeodLat);
      double geodAlt = 0.0;
      double n = e2;
      double prev_n = 1.0;
      int iter = 0;
      // Use tan or cotan to solve the geodetic altitude to avoid floating point
      // exceptions.
      if (cosGeodLat > fabs(sinGeodLat)) { // tan() can safely be used.
        double tanGeodLat = sinGeodLat/cosGeodLat;
        double x0 = N*e2*cosGeodLat;
        double x = 0.0;
        while (fabs(n-prev_n) > 1E-15 && iter < 10) {
          double tanLat = (1-n)*tanGeodLat; // See Stevens & Lewis 1.6-14
          double cos2Lat = 1./(1.+tanLat*tanLat);
          double slr = b/sqrt(1.-e2*cos2Lat);
          double R = slr + alt;
          x = R*sqrt(cos2Lat); // OK, cos(latitude) is always positive.
          prev_n = n;
          n = x0/x;
          iter++;
        }
        geodAlt = x/cosGeodLat-N;
      }
      else { // better use cotan (i.e. 1./tan())
        double cotanGeodLat = cosGeodLat/sinGeodLat;
        double z0 = N*e2*sinGeodLat;
        double z = 0.0;
        while (fabs(n-prev_n) > 1E-15 && iter < 10) {
          double cotanLat = cotanGeodLat/(1-n);
          double sin2Lat = 1./(1.+cotanLat*cotanLat);
          double cos2Lat = 1.-sin2Lat;
          double slr = b/sqrt(1.-e2*cos2Lat);
          double R = slr + alt;
          z = R*sign(cotanLat)*sqrt(sin2Lat);
          prev_n = n;
          n = z0/(z0+z);
          iter++;
        }
        geodAlt = z/sinGeodLat-N*(1-e2);
      }
      
      double longitude = position.GetLongitude();
      position.SetPositionGeodetic(longitude, geodLatitude, geodAlt);
    }
    break;
  case setgeoc:
    {
      double slr = position.GetSeaLevelRadius();
      position.SetRadius(slr+alt);
    }
    break;
  }

  altitudeASL = position.GetGeodAltitude();
  soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  rho = Atmosphere->GetDensity(altitudeASL);
  pressure = Atmosphere->GetPressure(altitudeASL);

  switch(lastSpeedSet) {
    case setvc:
      mach0 = MachFromVcalibrated(vc0, pressure);
      SetVtrueFpsIC(mach0 * soundSpeed);
      break;
    case setmach:
      SetVtrueFpsIC(mach0 * soundSpeed);
      break;
    case setve:
      SetVtrueFpsIC(ve0 * sqrt(rhoSL/rho));
      break;
    default: // Make the compiler stop complaining about missing enums
      break;
  }

  lastAltitudeSet = setasl;
}

//******************************************************************************

void FGInitialCondition::SetGeodLatitudeRadIC(double geodLatitude)
{
  double lon = position.GetLongitude();
  lastLatitudeSet = setgeod;

  switch (lastAltitudeSet)
  {
  case setagl:
    {
      double agl = GetAltitudeAGLFtIC();
      position.SetPositionGeodetic(lon, geodLatitude, 0.);
      fdmex->GetInertial()->SetAltitudeAGL(position, agl);
    }
    break;
  case setasl:
    {
      double asl = GetAltitudeASLFtIC();
      position.SetPositionGeodetic(lon, geodLatitude, 0.);
      SetAltitudeASLFtIC(asl);
    }
    break;
  }
}

//******************************************************************************

void FGInitialCondition::SetLatitudeRadIC(double lat)
{
  double altitude;

  lastLatitudeSet = setgeoc;

  switch(lastAltitudeSet) {
  case setagl:
    altitude = GetAltitudeAGLFtIC();
    position.SetLatitude(lat);
    SetAltitudeAGLFtIC(altitude);
    break;
  default:
    altitude = GetAltitudeASLFtIC();
    position.SetLatitude(lat);
    SetAltitudeASLFtIC(altitude);
    break;
  }
}

//******************************************************************************

void FGInitialCondition::SetLongitudeRadIC(double lon)
{
  double altitude;

  switch(lastAltitudeSet) {
  case setagl:
    altitude = GetAltitudeAGLFtIC();
    position.SetLongitude(lon);
    SetAltitudeAGLFtIC(altitude);
    break;
  default:
    position.SetLongitude(lon);
    break;
  }
}

//******************************************************************************

double FGInitialCondition::GetWindDirDegIC(void) const
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED(eV) == 0.0 ? 0.0
                               : atan2(_vWIND_NED(eV), _vWIND_NED(eU))*radtodeg;
}

//******************************************************************************

double FGInitialCondition::GetNEDWindFpsIC(int idx) const
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED(idx);
}

//******************************************************************************

double FGInitialCondition::GetWindFpsIC(void) const
{
  const FGMatrix33& Tb2l = orientation.GetTInv();
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED.Magnitude(eU, eV);
}

//******************************************************************************

double FGInitialCondition::GetBodyWindFpsIC(int idx) const
{
  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 _vt_BODY = Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;
  FGColumnVector3 _vWIND_BODY = _vt_BODY - _vUVW_BODY;

  return _vWIND_BODY(idx);
}

//******************************************************************************

double FGInitialCondition::GetVcalibratedKtsIC(void) const
{
  double altitudeASL = GetAltitudeASLFtIC();
  double pressure = Atmosphere->GetPressure(altitudeASL);
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  double mach = vt / soundSpeed;

  return fpstokts * VcalibratedFromMach(mach, pressure);
}

//******************************************************************************

double FGInitialCondition::GetVequivalentKtsIC(void) const
{
  double altitudeASL = GetAltitudeASLFtIC();
  double rho = Atmosphere->GetDensity(altitudeASL);
  double rhoSL = Atmosphere->GetDensitySL();
  return fpstokts * vt * sqrt(rho/rhoSL);
}

//******************************************************************************

double FGInitialCondition::GetMachIC(void) const
{
  double altitudeASL = GetAltitudeASLFtIC();
  double soundSpeed = Atmosphere->GetSoundSpeed(altitudeASL);
  return vt / soundSpeed;
}

//******************************************************************************

double FGInitialCondition::GetBodyVelFpsIC(int idx) const
{
  const FGMatrix33& Tl2b = orientation.GetT();
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;

  return _vUVW_BODY(idx);
}

//******************************************************************************

bool FGInitialCondition::Load(const SGPath& rstfile, bool useStoredPath)
{
  SGPath init_file_name;
  if(useStoredPath && rstfile.isRelative()) {
    init_file_name = fdmex->GetFullAircraftPath()/rstfile.utf8Str();
  } else {
    init_file_name = rstfile;
  }

  FGXMLFileRead XMLFileRead;
  Element* document = XMLFileRead.LoadXMLDocument(init_file_name);

  // Make sure that the document is valid
  if (!document) {
    cerr << "File: " << init_file_name << " could not be read." << endl;
    exit(-1);
  }

  if (document->GetName() != string("initialize")) {
    cerr << "File: " << init_file_name << " is not a reset file." << endl;
    exit(-1);
  }

  double version = HUGE_VAL;
  bool result = false;

  if (document->HasAttribute("version"))
    version = document->GetAttributeValueAsNumber("version");

  if (version == HUGE_VAL) {
    result = Load_v1(document); // Default to the old version
  } else if (version >= 3.0) {
    cerr << "Only initialization file formats 1 and 2 are currently supported" << endl;
    exit (-1);
  } else if (version >= 2.0) {
    result = Load_v2(document);
  } else if (version >= 1.0) {
    result = Load_v1(document);
  }

  // Check to see if any engines are specified to be initialized in a running state
  Element* running_elements = document->FindElement("running");
  while (running_elements) {
    int engineNumber = int(running_elements->GetDataAsNumber());
    enginesRunning |= engineNumber == -1 ? engineNumber : 1 << engineNumber;
    running_elements = document->FindNextElement("running");
  }

  return result;
}

//******************************************************************************

bool FGInitialCondition::LoadLatitude(Element* position_el)
{
  Element* latitude_el = position_el->FindElement("latitude");

  if (latitude_el) {
    double latitude = position_el->FindElementValueAsNumberConvertTo("latitude", "RAD");

    if (fabs(latitude) > 0.5*M_PI) {
      string unit_type = latitude_el->GetAttributeValue("unit");
      if (unit_type.empty()) unit_type="RAD";

      cerr << latitude_el->ReadFrom() << "The latitude value "
           << latitude_el->GetDataAsNumber() << " " << unit_type
           << " is outside the range [";
      if (unit_type == "DEG")
        cerr << "-90 DEG ; +90 DEG]" << endl;
      else
        cerr << "-PI/2 RAD; +PI/2 RAD]" << endl;

      return false;
    }

    string lat_type = latitude_el->GetAttributeValue("type");

    if (lat_type == "geod" || lat_type == "geodetic") {
      SetGeodLatitudeRadIC(latitude);
      lastLatitudeSet = setgeod;
    }
    else {
      SetLatitudeRadIC(latitude);
      lastLatitudeSet = setgeoc;
    }
  }

  return true;
}

//******************************************************************************

void FGInitialCondition::SetTrimRequest(std::string trim)
{
  std::string& trimOption = to_lower(trim);
  if (trimOption == "1")
    trimRequested = TrimMode::tGround;  // For backwards compatabiity
  else if (trimOption == "longitudinal")
    trimRequested = TrimMode::tLongitudinal;
  else if (trimOption == "full")
    trimRequested = TrimMode::tFull;
  else if (trimOption == "ground")
    trimRequested = TrimMode::tGround;
  else if (trimOption == "pullup")
    trimRequested = TrimMode::tPullup;
  else if (trimOption == "custom")
    trimRequested = TrimMode::tCustom;
  else if (trimOption == "turn")
    trimRequested = TrimMode::tTurn;
}

//******************************************************************************

bool FGInitialCondition::Load_v1(Element* document)
{
  bool result = true;

  if (document->FindElement("longitude"))
    SetLongitudeRadIC(document->FindElementValueAsNumberConvertTo("longitude", "RAD"));
  if (document->FindElement("elevation"))
    SetTerrainElevationFtIC(document->FindElementValueAsNumberConvertTo("elevation", "FT"));

  if (document->FindElement("altitude")) // This is feet above ground level
    SetAltitudeAGLFtIC(document->FindElementValueAsNumberConvertTo("altitude", "FT"));
  else if (document->FindElement("altitudeAGL")) // This is feet above ground level
    SetAltitudeAGLFtIC(document->FindElementValueAsNumberConvertTo("altitudeAGL", "FT"));
  else if (document->FindElement("altitudeMSL")) // This is feet above sea level
    SetAltitudeASLFtIC(document->FindElementValueAsNumberConvertTo("altitudeMSL", "FT"));

  result = LoadLatitude(document);

  FGColumnVector3 vOrient = orientation.GetEuler();

  if (document->FindElement("phi"))
    vOrient(ePhi) = document->FindElementValueAsNumberConvertTo("phi", "RAD");
  if (document->FindElement("theta"))
    vOrient(eTht) = document->FindElementValueAsNumberConvertTo("theta", "RAD");
  if (document->FindElement("psi"))
    vOrient(ePsi) = document->FindElementValueAsNumberConvertTo("psi", "RAD");

  orientation = FGQuaternion(vOrient);

  if (document->FindElement("ubody"))
    SetUBodyFpsIC(document->FindElementValueAsNumberConvertTo("ubody", "FT/SEC"));
  if (document->FindElement("vbody"))
    SetVBodyFpsIC(document->FindElementValueAsNumberConvertTo("vbody", "FT/SEC"));
  if (document->FindElement("wbody"))
    SetWBodyFpsIC(document->FindElementValueAsNumberConvertTo("wbody", "FT/SEC"));
  if (document->FindElement("vnorth"))
    SetVNorthFpsIC(document->FindElementValueAsNumberConvertTo("vnorth", "FT/SEC"));
  if (document->FindElement("veast"))
    SetVEastFpsIC(document->FindElementValueAsNumberConvertTo("veast", "FT/SEC"));
  if (document->FindElement("vdown"))
    SetVDownFpsIC(document->FindElementValueAsNumberConvertTo("vdown", "FT/SEC"));
  if (document->FindElement("vc"))
    SetVcalibratedKtsIC(document->FindElementValueAsNumberConvertTo("vc", "KTS"));
  if (document->FindElement("vt"))
    SetVtrueKtsIC(document->FindElementValueAsNumberConvertTo("vt", "KTS"));
  if (document->FindElement("mach"))
    SetMachIC(document->FindElementValueAsNumber("mach"));
  if (document->FindElement("gamma"))
    SetFlightPathAngleDegIC(document->FindElementValueAsNumberConvertTo("gamma", "DEG"));
  if (document->FindElement("roc"))
    SetClimbRateFpsIC(document->FindElementValueAsNumberConvertTo("roc", "FT/SEC"));
  if (document->FindElement("vground"))
    SetVgroundKtsIC(document->FindElementValueAsNumberConvertTo("vground", "KTS"));
  if (document->FindElement("alpha"))
    SetAlphaDegIC(document->FindElementValueAsNumberConvertTo("alpha", "DEG"));
  if (document->FindElement("beta"))
    SetBetaDegIC(document->FindElementValueAsNumberConvertTo("beta", "DEG"));
  if (document->FindElement("vwind"))
    SetWindMagKtsIC(document->FindElementValueAsNumberConvertTo("vwind", "KTS"));
  if (document->FindElement("winddir"))
    SetWindDirDegIC(document->FindElementValueAsNumberConvertTo("winddir", "DEG"));
  if (document->FindElement("hwind"))
    SetHeadWindKtsIC(document->FindElementValueAsNumberConvertTo("hwind", "KTS"));
  if (document->FindElement("xwind"))
    SetCrossWindKtsIC(document->FindElementValueAsNumberConvertTo("xwind", "KTS"));
  if (document->FindElement("targetNlf"))
    SetTargetNlfIC(document->FindElementValueAsNumber("targetNlf"));
  if (document->FindElement("trim"))
    SetTrimRequest(document->FindElementValue("trim"));

  // Refer to Stevens and Lewis, 1.5-14a, pg. 49.
  // This is the rotation rate of the "Local" frame, expressed in the local frame.
  const FGMatrix33& Tl2b = orientation.GetT();
  double radInv = 1.0 / position.GetRadius();
  FGColumnVector3 vOmegaLocal = {radInv*vUVW_NED(eEast),
                                 -radInv*vUVW_NED(eNorth),
                                 -radInv*vUVW_NED(eEast)*tan(position.GetLatitude())};

  vPQR_body = Tl2b * vOmegaLocal;

  return result;
}

//******************************************************************************

bool FGInitialCondition::Load_v2(Element* document)
{
  FGColumnVector3 vOrient;
  bool result = true;

  // support both earth_position_angle and planet_position_angle, for now.
  if (document->FindElement("earth_position_angle"))
    epa = document->FindElementValueAsNumberConvertTo("earth_position_angle", "RAD");
  if (document->FindElement("planet_position_angle"))
    epa = document->FindElementValueAsNumberConvertTo("planet_position_angle", "RAD");

  // Calculate the inertial to ECEF matrices
  FGMatrix33 Ti2ec(cos(epa), sin(epa), 0.0,
                   -sin(epa), cos(epa), 0.0,
                   0.0, 0.0, 1.0);
  FGMatrix33 Tec2i = Ti2ec.Transposed();

  if (document->FindElement("planet_rotation_rate")) {
    fdmex->GetInertial()->SetOmegaPlanet(document->FindElementValueAsNumberConvertTo("planet_rotation_rate", "RAD"));
    fdmex->GetPropagate()->in.vOmegaPlanet     = fdmex->GetInertial()->GetOmegaPlanet();
    fdmex->GetAccelerations()->in.vOmegaPlanet = fdmex->GetInertial()->GetOmegaPlanet();
  }
  FGColumnVector3 vOmegaEarth = fdmex->GetInertial()->GetOmegaPlanet();

  if (document->FindElement("elevation")) {
    fdmex->GetInertial()->SetTerrainElevation(document->FindElementValueAsNumberConvertTo("elevation", "FT"));
  }

  // Initialize vehicle position
  //
  // Allowable frames:
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)

  Element* position_el = document->FindElement("position");
  if (position_el) {
    string frame = position_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    if (frame == "eci") { // Need to transform vLoc to ECEF for storage and use in FGLocation.
      position = Ti2ec * position_el->FindElementTripletConvertTo("FT");
    } else if (frame == "ecef") {
      if (!position_el->FindElement("x") && !position_el->FindElement("y") && !position_el->FindElement("z")) {
        if (position_el->FindElement("longitude")) {
          SetLongitudeRadIC(position_el->FindElementValueAsNumberConvertTo("longitude", "RAD"));
        }
        if (position_el->FindElement("radius")) {
          position.SetRadius(position_el->FindElementValueAsNumberConvertTo("radius", "FT"));
        } else if (position_el->FindElement("altitudeAGL")) {
          SetAltitudeAGLFtIC(position_el->FindElementValueAsNumberConvertTo("altitudeAGL", "FT"));
        } else if (position_el->FindElement("altitudeMSL")) {
          SetAltitudeASLFtIC(position_el->FindElementValueAsNumberConvertTo("altitudeMSL", "FT"));
        } else {
          cerr << endl << "  No altitude or radius initial condition is given." << endl;
          result = false;
        }

        if (result)
          result = LoadLatitude(position_el);

      } else {
        position = position_el->FindElementTripletConvertTo("FT");
      }
    } else {
      cerr << endl << "  Neither ECI nor ECEF frame is specified for initial position." << endl;
      result = false;
    }
  } else {
    cerr << endl << "  Initial position not specified in this initialization file." << endl;
    result = false;
  }

  // End of position initialization

  // Initialize vehicle orientation
  // Allowable frames
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)
  // - Local
  //
  // Need to convert the provided orientation to a local orientation, using
  // the given orientation and knowledge of the Earth position angle.
  // This could be done using matrices (where in the subscript "b/a",
  // it is meant "b with respect to a", and where b=body frame,
  // i=inertial frame, l=local NED frame and e=ecef frame) as:
  //
  // C_b/l =  C_b/e * C_e/l
  //
  // Using quaternions (note reverse ordering compared to matrix representation):
  //
  // Q_b/l = Q_e/l * Q_b/e
  //
  // Use the specific matrices as needed. The above example of course is for the whole
  // body to local orientation.
  // The new orientation angles can be extracted from the matrix or the quaternion.
  // ToDo: Do we need to deal with normalization of the quaternions here?

  Element* orientation_el = document->FindElement("orientation");
  if (orientation_el) {
    string frame = orientation_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    vOrient = orientation_el->FindElementTripletConvertTo("RAD");
    if (frame == "eci") {

      // In this case, we are supplying the Euler angles for the vehicle with
      // respect to the inertial system, represented by the C_b/i Matrix.
      // We want the body orientation with respect to the local (NED frame):
      //
      // C_b/l = C_b/i * C_i/l
      //
      // Or, using quaternions (note reverse ordering compared to matrix representation):
      //
      // Q_b/l = Q_i/l * Q_b/i

      FGQuaternion QuatI2Body = FGQuaternion(vOrient);
      QuatI2Body.Normalize();
      FGQuaternion QuatLocal2I = Tec2i * position.GetTl2ec();
      QuatLocal2I.Normalize();
      orientation = QuatLocal2I * QuatI2Body;

    } else if (frame == "ecef") {

      // In this case we are given the Euler angles representing the orientation of
      // the body with respect to the ECEF system, represented by the C_b/e Matrix.
      // We want the body orientation with respect to the local (NED frame):
      //
      // C_b/l =  C_b/e * C_e/l
      //
      // Using quaternions (note reverse ordering compared to matrix representation):
      //
      // Q_b/l = Q_e/l * Q_b/e

      FGQuaternion QuatEC2Body(vOrient); // Store relationship of Body frame wrt ECEF frame, Q_b/e
      QuatEC2Body.Normalize();
      FGQuaternion QuatLocal2EC = position.GetTl2ec(); // Get Q_e/l from matrix
      QuatLocal2EC.Normalize();
      orientation = QuatLocal2EC * QuatEC2Body; // Q_b/l = Q_e/l * Q_b/e

    } else if (frame == "local") {

      orientation = FGQuaternion(vOrient);

    } else {

      cerr << endl << fgred << "  Orientation frame type: \"" << frame
           << "\" is not supported!" << reset << endl << endl;
      result = false;

    }
  }

  // Initialize vehicle velocity
  // Allowable frames
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)
  // - Local
  // - Body
  // The vehicle will be defaulted to (0,0,0) in the Body frame if nothing is provided.

  Element* velocity_el = document->FindElement("velocity");
  FGMatrix33 mTec2l = position.GetTec2l();
  const FGMatrix33& Tb2l = orientation.GetTInv();

  if (velocity_el) {

    string frame = velocity_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    FGColumnVector3 vInitVelocity = velocity_el->FindElementTripletConvertTo("FT/SEC");

    if (frame == "eci") {
      FGColumnVector3 omega_cross_r = vOmegaEarth * (Tec2i * position);
      vUVW_NED = mTec2l * (vInitVelocity - omega_cross_r);
      lastSpeedSet = setned;
    } else if (frame == "ecef") {
      vUVW_NED = mTec2l * vInitVelocity;
      lastSpeedSet = setned;
    } else if (frame == "local") {
      vUVW_NED = vInitVelocity;
      lastSpeedSet = setned;
    } else if (frame == "body") {
      vUVW_NED = Tb2l * vInitVelocity;
      lastSpeedSet = setuvw;
    } else {

      cerr << endl << fgred << "  Velocity frame type: \"" << frame
           << "\" is not supported!" << reset << endl << endl;
      result = false;

    }

  } else {

    vUVW_NED.InitMatrix();

  }

  vt = vUVW_NED.Magnitude();

  calcAeroAngles(vUVW_NED);

  // Initialize vehicle body rates
  // Allowable frames
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)
  // - Body

  Element* attrate_el = document->FindElement("attitude_rate");
  const FGMatrix33& Tl2b = orientation.GetT();

  // Refer to Stevens and Lewis, 1.5-14a, pg. 49.
  // This is the rotation rate of the "Local" frame, expressed in the local frame.
  double radInv = 1.0 / position.GetRadius();
  FGColumnVector3 vOmegaLocal = { radInv*vUVW_NED(eEast),
                                  -radInv*vUVW_NED(eNorth),
                                  -radInv*vUVW_NED(eEast)*tan(position.GetLatitude())};

  if (attrate_el) {

    string frame = attrate_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    FGColumnVector3 vAttRate = attrate_el->FindElementTripletConvertTo("RAD/SEC");

    if (frame == "eci") {
      FGMatrix33 Ti2l = position.GetTec2l() * Ti2ec;
      vPQR_body = Tl2b * Ti2l * (vAttRate - vOmegaEarth);
    } else if (frame == "ecef") {
      vPQR_body = Tl2b * position.GetTec2l() * vAttRate;
    } else if (frame == "local") {
      vPQR_body = Tl2b * (vAttRate + vOmegaLocal);
    } else if (frame == "body") {
      vPQR_body = vAttRate;
    } else if (!frame.empty()) { // misspelling of frame

      cerr << endl << fgred << "  Attitude rate frame type: \"" << frame
           << "\" is not supported!" << reset << endl << endl;
      result = false;

    } else if (frame.empty()) {
      vPQR_body = Tl2b * vOmegaLocal;
    }

  } else { // Body frame attitude rate assumed 0 relative to local.
      vPQR_body = Tl2b * vOmegaLocal;
  }

  return result;
}

//******************************************************************************

void FGInitialCondition::bind(FGPropertyManager* PropertyManager)
{
  PropertyManager->Tie("ic/vc-kts", this,
                       &FGInitialCondition::GetVcalibratedKtsIC,
                       &FGInitialCondition::SetVcalibratedKtsIC,
                       true);
  PropertyManager->Tie("ic/ve-kts", this,
                       &FGInitialCondition::GetVequivalentKtsIC,
                       &FGInitialCondition::SetVequivalentKtsIC,
                       true);
  PropertyManager->Tie("ic/vg-kts", this,
                       &FGInitialCondition::GetVgroundKtsIC,
                       &FGInitialCondition::SetVgroundKtsIC,
                       true);
  PropertyManager->Tie("ic/vt-kts", this,
                       &FGInitialCondition::GetVtrueKtsIC,
                       &FGInitialCondition::SetVtrueKtsIC,
                       true);
  PropertyManager->Tie("ic/mach", this,
                       &FGInitialCondition::GetMachIC,
                       &FGInitialCondition::SetMachIC,
                       true);
  PropertyManager->Tie("ic/roc-fpm", this,
                       &FGInitialCondition::GetClimbRateFpmIC,
                       &FGInitialCondition::SetClimbRateFpmIC,
                       true);
  PropertyManager->Tie("ic/gamma-deg", this,
                       &FGInitialCondition::GetFlightPathAngleDegIC,
                       &FGInitialCondition::SetFlightPathAngleDegIC,
                       true);
  PropertyManager->Tie("ic/alpha-deg", this,
                       &FGInitialCondition::GetAlphaDegIC,
                       &FGInitialCondition::SetAlphaDegIC,
                       true);
  PropertyManager->Tie("ic/beta-deg", this,
                       &FGInitialCondition::GetBetaDegIC,
                       &FGInitialCondition::SetBetaDegIC,
                       true);
  PropertyManager->Tie("ic/theta-deg", this,
                       &FGInitialCondition::GetThetaDegIC,
                       &FGInitialCondition::SetThetaDegIC,
                       true);
  PropertyManager->Tie("ic/phi-deg", this,
                       &FGInitialCondition::GetPhiDegIC,
                       &FGInitialCondition::SetPhiDegIC,
                       true);
  PropertyManager->Tie("ic/psi-true-deg", this,
                       &FGInitialCondition::GetPsiDegIC,
                       &FGInitialCondition::SetPsiDegIC,
                       true);
  PropertyManager->Tie("ic/lat-gc-deg", this,
                       &FGInitialCondition::GetLatitudeDegIC,
                       &FGInitialCondition::SetLatitudeDegIC,
                       true);
  PropertyManager->Tie("ic/long-gc-deg", this,
                       &FGInitialCondition::GetLongitudeDegIC,
                       &FGInitialCondition::SetLongitudeDegIC,
                       true);
  PropertyManager->Tie("ic/h-sl-ft", this,
                       &FGInitialCondition::GetAltitudeASLFtIC,
                       &FGInitialCondition::SetAltitudeASLFtIC,
                       true);
  PropertyManager->Tie("ic/h-agl-ft", this,
                       &FGInitialCondition::GetAltitudeAGLFtIC,
                       &FGInitialCondition::SetAltitudeAGLFtIC,
                       true);
  PropertyManager->Tie("ic/terrain-elevation-ft", this,
                       &FGInitialCondition::GetTerrainElevationFtIC,
                       &FGInitialCondition::SetTerrainElevationFtIC,
                       true);
  PropertyManager->Tie("ic/vg-fps", this,
                       &FGInitialCondition::GetVgroundFpsIC,
                       &FGInitialCondition::SetVgroundFpsIC,
                       true);
  PropertyManager->Tie("ic/vt-fps", this,
                       &FGInitialCondition::GetVtrueFpsIC,
                       &FGInitialCondition::SetVtrueFpsIC,
                       true);
  PropertyManager->Tie("ic/vw-bx-fps", this,
                       &FGInitialCondition::GetWindUFpsIC);
  PropertyManager->Tie("ic/vw-by-fps", this,
                       &FGInitialCondition::GetWindVFpsIC);
  PropertyManager->Tie("ic/vw-bz-fps", this,
                       &FGInitialCondition::GetWindWFpsIC);
  PropertyManager->Tie("ic/vw-north-fps", this,
                       &FGInitialCondition::GetWindNFpsIC);
  PropertyManager->Tie("ic/vw-east-fps", this,
                       &FGInitialCondition::GetWindEFpsIC);
  PropertyManager->Tie("ic/vw-down-fps", this,
                       &FGInitialCondition::GetWindDFpsIC);
  PropertyManager->Tie("ic/vw-mag-fps", this,
                       &FGInitialCondition::GetWindFpsIC);
  PropertyManager->Tie("ic/vw-dir-deg", this,
                       &FGInitialCondition::GetWindDirDegIC,
                       &FGInitialCondition::SetWindDirDegIC,
                       true);

  PropertyManager->Tie("ic/roc-fps", this,
                       &FGInitialCondition::GetClimbRateFpsIC,
                       &FGInitialCondition::SetClimbRateFpsIC,
                       true);
  PropertyManager->Tie("ic/u-fps", this,
                       &FGInitialCondition::GetUBodyFpsIC,
                       &FGInitialCondition::SetUBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/v-fps", this,
                       &FGInitialCondition::GetVBodyFpsIC,
                       &FGInitialCondition::SetVBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/w-fps", this,
                       &FGInitialCondition::GetWBodyFpsIC,
                       &FGInitialCondition::SetWBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/vn-fps", this,
                       &FGInitialCondition::GetVNorthFpsIC,
                       &FGInitialCondition::SetVNorthFpsIC,
                       true);
  PropertyManager->Tie("ic/ve-fps", this,
                       &FGInitialCondition::GetVEastFpsIC,
                       &FGInitialCondition::SetVEastFpsIC,
                       true);
  PropertyManager->Tie("ic/vd-fps", this,
                       &FGInitialCondition::GetVDownFpsIC,
                       &FGInitialCondition::SetVDownFpsIC,
                       true);
  PropertyManager->Tie("ic/gamma-rad", this,
                       &FGInitialCondition::GetFlightPathAngleRadIC,
                       &FGInitialCondition::SetFlightPathAngleRadIC,
                       true);
  PropertyManager->Tie("ic/alpha-rad", this,
                       &FGInitialCondition::GetAlphaRadIC,
                       &FGInitialCondition::SetAlphaRadIC,
                       true);
  PropertyManager->Tie("ic/theta-rad", this,
                       &FGInitialCondition::GetThetaRadIC,
                       &FGInitialCondition::SetThetaRadIC,
                       true);
  PropertyManager->Tie("ic/beta-rad", this,
                       &FGInitialCondition::GetBetaRadIC,
                       &FGInitialCondition::SetBetaRadIC,
                       true);
  PropertyManager->Tie("ic/phi-rad", this,
                       &FGInitialCondition::GetPhiRadIC,
                       &FGInitialCondition::SetPhiRadIC,
                       true);
  PropertyManager->Tie("ic/psi-true-rad", this,
                       &FGInitialCondition::GetPsiRadIC,
                       &FGInitialCondition::SetPsiRadIC,
                       true);
  PropertyManager->Tie("ic/lat-gc-rad", this,
                       &FGInitialCondition::GetLatitudeRadIC,
                       &FGInitialCondition::SetLatitudeRadIC,
                       true);
  PropertyManager->Tie("ic/long-gc-rad", this,
                       &FGInitialCondition::GetLongitudeRadIC,
                       &FGInitialCondition::SetLongitudeRadIC,
                       true);
  PropertyManager->Tie("ic/p-rad_sec", this,
                       &FGInitialCondition::GetPRadpsIC,
                       &FGInitialCondition::SetPRadpsIC,
                       true);
  PropertyManager->Tie("ic/q-rad_sec", this,
                       &FGInitialCondition::GetQRadpsIC,
                       &FGInitialCondition::SetQRadpsIC,
                       true);
  PropertyManager->Tie("ic/r-rad_sec", this,
                       &FGInitialCondition::GetRRadpsIC,
                       &FGInitialCondition::SetRRadpsIC,
                       true);
  PropertyManager->Tie("ic/lat-geod-rad", this,
                       &FGInitialCondition::GetGeodLatitudeRadIC,
                       &FGInitialCondition::SetGeodLatitudeRadIC,
                       true);
  PropertyManager->Tie("ic/lat-geod-deg", this,
                       &FGInitialCondition::GetGeodLatitudeDegIC,
                       &FGInitialCondition::SetGeodLatitudeDegIC,
                       true);
  PropertyManager->Tie("ic/geod-alt-ft", &position,
                       &FGLocation::GetGeodAltitude);

  PropertyManager->Tie("ic/targetNlf", this,
                       &FGInitialCondition::GetTargetNlfIC,
                       &FGInitialCondition::SetTargetNlfIC,
                       true);
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

void FGInitialCondition::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGInitialCondition" << endl;
    if (from == 1) cout << "Destroyed:    FGInitialCondition" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
