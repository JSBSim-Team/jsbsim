/*******************************************************************************

 Header:       FGInitialCondition.cpp
 Author:       Tony Peden, Bertrand Coconnier
 Date started: 7/1/99

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------

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
7/1/99   TP   Created
11/25/10 BC   Complete revision - Use minimal set of variables to prevent
              inconsistent states. Wind is correctly handled.


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to take a set of initial conditions and provide
a kinematically consistent set of body axis velocity components, euler
angles, and altitude.  This class does not attempt to trim the model i.e.
the sim will most likely start in a very dynamic state (unless, of course,
you have chosen your IC's wisely) even after setting it up with this class.

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGInitialCondition.h"
#include "FGFDMExec.h"
#include "math/FGQuaternion.h"
#include "models/FGInertial.h"
#include "models/FGAtmosphere.h"
#include "models/FGPropagate.h"
#include "models/FGPropulsion.h"
#include "input_output/FGPropertyManager.h"
#include "input_output/string_utilities.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGInitialCondition.cpp,v 1.55 2011/01/19 20:49:20 bcoconni Exp $";
static const char *IdHdr = ID_INITIALCONDITION;

//******************************************************************************

FGInitialCondition::FGInitialCondition(FGFDMExec *FDMExec) : fdmex(FDMExec)
{
  InitializeIC();

  if(FDMExec != NULL ) {
    PropertyManager=fdmex->GetPropertyManager();
    Constructing = true;
    bind();
    Constructing = false;
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

  p = p0;  q = q0;  r = r0;
  alpha = alpha0;  beta = beta0;
  phi = phi0;  theta = theta0;  psi = psi0;

  position.SetPosition(lonRad0, latRad0, altAGLFt0 + terrain_elevation + sea_level_radius);

  FGQuaternion Quat(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

  vUVW_NED = Tb2l * FGColumnVector3(u0, v0, w0);
  vt = vUVW_NED.Magnitude();

  Tw2b = FGMatrix33(calpha*cbeta, -calpha*sbeta,  -salpha,
                           sbeta,         cbeta,      0.0,
                    salpha*cbeta, -salpha*sbeta,   calpha);
  Tb2w = Tw2b.Transposed();

  SetFlightPathAngleRadIC(gamma0);
}

//******************************************************************************

void FGInitialCondition::InitializeIC(void)
{
  alpha=beta=0;
  theta=phi=psi=0;
  position.SetPosition(0., 0., sea_level_radius);
  position.SetEarthPositionAngle(fdmex->GetInertial()->GetEarthPositionAngle());
  vUVW_NED.InitMatrix();
  p=q=r=0;
  vt=0;
  sea_level_radius = fdmex->GetInertial()->GetRefRadius();
  terrain_elevation = 0;

  targetNlfIC = 1.0;

  Tw2b.InitMatrix(1., 0., 0., 0., 1., 0., 0., 0., 1.);
  Tb2w.InitMatrix(1., 0., 0., 0., 1., 0., 0., 0., 1.);
  Tl2b.InitMatrix(1., 0., 0., 0., 1., 0., 0., 0., 1.);
  Tb2l.InitMatrix(1., 0., 0., 0., 1., 0., 0., 0., 1.);
}

//******************************************************************************

void FGInitialCondition::WriteStateFile(int num)
{
  if (Constructing) return;

  string filename = fdmex->GetFullAircraftPath();

  if (filename.empty())
    filename = "initfile.xml";
  else
    filename.append("/initfile.xml");
  
  ofstream outfile(filename.c_str());
  FGPropagate* Propagate = fdmex->GetPropagate();
  
  if (outfile.is_open()) {
    outfile << "<?xml version=\"1.0\"?>" << endl;
    outfile << "<initialize name=\"reset00\">" << endl;
    outfile << "  <ubody unit=\"FT/SEC\"> " << Propagate->GetUVW(eX) << " </ubody> " << endl;
    outfile << "  <vbody unit=\"FT/SEC\"> " << Propagate->GetUVW(eY) << " </vbody> " << endl;
    outfile << "  <wbody unit=\"FT/SEC\"> " << Propagate->GetUVW(eZ) << " </wbody> " << endl;
    outfile << "  <phi unit=\"DEG\"> " << Propagate->GetEuler(ePhi) << " </phi>" << endl;
    outfile << "  <theta unit=\"DEG\"> " << Propagate->GetEuler(eTht) << " </theta>" << endl;
    outfile << "  <psi unit=\"DEG\"> " << Propagate->GetEuler(ePsi) << " </psi>" << endl;
    outfile << "  <longitude unit=\"DEG\"> " << Propagate->GetLongitudeDeg() << " </longitude>" << endl;
    outfile << "  <latitude unit=\"DEG\"> " << Propagate->GetLatitudeDeg() << " </latitude>" << endl;
    outfile << "  <altitude unit=\"FT\"> " << Propagate->GetDistanceAGL() << " </altitude>" << endl;
    outfile << "</initialize>" << endl;
    outfile.close();
  } else {
    cerr << "Could not open and/or write the state to the initial conditions file: " << filename << endl;
  }
}

//******************************************************************************

void FGInitialCondition::SetVequivalentKtsIC(double ve)
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double rho = fdmex->GetAtmosphere()->GetDensity(altitudeASL);
  double rhoSL = fdmex->GetAtmosphere()->GetDensitySL();
  SetVtrueFpsIC(ve*ktstofps/sqrt(rho/rhoSL));
  lastSpeedSet = setve;
}

//******************************************************************************

void FGInitialCondition::SetMachIC(double mach)
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  double soundSpeed = sqrt(SHRatio*Reng*temperature);
  SetVtrueFpsIC(mach*soundSpeed);
  lastSpeedSet = setmach;
}

//******************************************************************************

void FGInitialCondition::SetVcalibratedKtsIC(double vcas)
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double mach = getMachFromVcas(fabs(vcas)*ktstofps);
  double temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  double soundSpeed = sqrt(SHRatio*Reng*temperature);

  SetVtrueFpsIC(mach*soundSpeed);
  lastSpeedSet = setvc;
}

//******************************************************************************
// Updates alpha and beta according to the aircraft true airspeed in the local
// NED frame.

void FGInitialCondition::calcAeroAngles(const FGColumnVector3& _vt_NED)
{
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
  // true speed or the Euler angles. Otherwise we might end up with an inconsistent
  // state of the aircraft.
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

  Tw2b = FGMatrix33(calpha*cbeta, -calpha*sbeta,  -salpha,
                           sbeta,         cbeta,      0.0,
                    salpha*cbeta, -salpha*sbeta,   calpha);
  Tb2w = Tw2b.Transposed();
}

//******************************************************************************
// Set the ground velocity. Caution it sets the vertical velocity to zero to
// keep backward compatibility.

void FGInitialCondition::SetVgroundFpsIC(double vg)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  vUVW_NED(eU) = vg*cos(psi);
  vUVW_NED(eV) = vg*sin(psi);
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

  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _WIND_NED = _vt_NED - vUVW_NED;
  double hdot0 = _vt_NED(eW);

  if (fabs(hdot0) < vt) {
    double scale = sqrt((vt*vt-hdot*hdot)/(vt*vt-hdot0*hdot0));
    _vt_NED(eU) *= scale;
    _vt_NED(eV) *= scale;
  }
  _vt_NED(eW) = hdot;
  vUVW_NED = _vt_NED - _WIND_NED;

  // The AoA is not modified here but the function SetAlphaRadIC is updating the
  // same angles than SetClimbRateFpsIC needs to update.
  // TODO : create a subroutine that only shares the relevant code.
  SetAlphaRadIC(alpha);
}

//******************************************************************************
// When the AoA is modified, we need to update the angles theta and beta to
// keep the true airspeed amplitude, the climb rate and the heading unchanged.
// Beta will be modified if the aircraft roll angle is not null.

void FGInitialCondition::SetAlphaRadIC(double alfa)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  double calpha = cos(alfa), salpha = sin(alfa);
  double cpsi = cos(psi), spsi = sin(psi);
  double cphi = cos(phi), sphi = sin(phi);
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
  FGColumnVector3 y = FGColumnVector3(0., 1., 0.);
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
  theta = asin(sinTheta);

  FGQuaternion Quat(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

  FGColumnVector3 v2 = Talpha * Quat.GetT() * _vt_NED;

  alpha = alfa;
  beta = atan2(v2(eV), v2(eU));
  double cbeta=0.0, sbeta=0.0;
  if (vt != 0.0) {
    cbeta = v2(eU) / vt;
    sbeta = v2(eV) / vt;
  }
  Tw2b = FGMatrix33(calpha*cbeta, -calpha*sbeta,  -salpha,
                           sbeta,         cbeta,      0.0,
                    salpha*cbeta, -salpha*sbeta,   calpha);
  Tb2w = Tw2b.Transposed();
}

//******************************************************************************
// When the beta angle is modified, we need to update the angles theta and psi
// to keep the true airspeed (amplitude and direction - including the climb rate)
// and the alpha angle unchanged. This may result in the aircraft heading (psi)
// being altered especially if there is cross wind.

void FGInitialCondition::SetBetaRadIC(double bta)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  beta = bta;
  double calpha = cos(alpha), salpha = sin(alpha);
  double cbeta = cos(beta), sbeta = sin(beta);

  Tw2b = FGMatrix33(calpha*cbeta, -calpha*sbeta,  -salpha,
                           sbeta,         cbeta,      0.0,
                    salpha*cbeta, -salpha*sbeta,   calpha);
  Tb2w = Tw2b.Transposed();

  FGColumnVector3 vf = FGQuaternion(eX, phi).GetTInv() * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 v0xy(_vt_NED(eX), _vt_NED(eY), 0.);
  FGColumnVector3 v1xy(sqrt(v0xy(eX)*v0xy(eX)+v0xy(eY)*v0xy(eY)-vf(eY)*vf(eY)),vf(eY),0.);
  v0xy.Normalize();
  v1xy.Normalize();

  if (vf(eX) < 0.) v0xy(eX) *= -1.0;

  double sinPsi = (v1xy * v0xy)(eZ);
  double cosPsi = DotProduct(v0xy, v1xy);
  psi = atan2(sinPsi, cosPsi);
  FGMatrix33 Tpsi( cosPsi, sinPsi, 0.,
                  -sinPsi, cosPsi, 0.,
                      0.,     0., 1.);

  FGColumnVector3 v2xz = Tpsi * _vt_NED;
  FGColumnVector3 vfxz = vf;
  v2xz(eV) = vfxz(eV) = 0.0;
  v2xz.Normalize();
  vfxz.Normalize();
  double sinTheta = (v2xz * vfxz)(eY);
  theta = -asin(sinTheta);

  FGQuaternion Quat(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();
}

//******************************************************************************
// Modifies the body frame orientation (roll angle phi). The true airspeed in
// the local NED frame is kept unchanged. Hence the true airspeed in the body
// frame is modified.

void FGInitialCondition::SetPhiRadIC(double fi)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  phi = fi;
  FGQuaternion Quat = FGQuaternion(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies the body frame orientation (pitch angle theta). The true airspeed in
// the local NED frame is kept unchanged. Hence the true airspeed in the body
// frame is modified.

void FGInitialCondition::SetThetaRadIC(double teta)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  theta = teta;
  FGQuaternion Quat = FGQuaternion(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies the body frame orientation (yaw angle psi). The true airspeed in
// the local NED frame is kept unchanged. Hence the true airspeed in the body
// frame is modified.

void FGInitialCondition::SetPsiRadIC(double psy)
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);

  psi = psy;
  FGQuaternion Quat = FGQuaternion(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

  calcAeroAngles(_vt_NED);
}

//******************************************************************************
// Modifies an aircraft velocity component (eU, eV or eW) in the body frame. The
// true airspeed is modified accordingly. If there is some wind, the airspeed
// direction modification may differ from the body velocity modification.

void FGInitialCondition::SetBodyVelFpsIC(int idx, double vel)
{
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
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vCROSS(-sin(psi), cos(psi), 0.);

  // Gram-Schmidt process is used to remove the existing cross wind component
  _vWIND_NED -= DotProduct(_vWIND_NED, _vCROSS) * _vCROSS;
  // which is now replaced by the new value.
  _vWIND_NED += cross * _vCROSS;
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
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vHEAD(cos(psi), sin(psi), 0.);

  // Gram-Schmidt process is used to remove the existing cross wind component
  _vWIND_NED -= DotProduct(_vWIND_NED, _vHEAD) * _vHEAD;
  // which is now replaced by the new value.
  _vWIND_NED += head * _vHEAD;
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
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;
  FGColumnVector3 _vHEAD(_vWIND_NED(eU), _vWIND_NED(eV), 0.);
  double windMag = _vHEAD.Magnitude();

  if (windMag > 0.001)
    _vHEAD *= mag / windMag;
  else
    _vHEAD = FGColumnVector3(mag, 0., 0.);

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
// Set the altitude SL. If the airspeed has been previously set with parameters
// that are atmosphere dependent (Mach, VCAS, VEAS) then the true airspeed is
// modified to keep the last set speed to its previous value.

void FGInitialCondition::SetAltitudeASLFtIC(double alt)
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  double soundSpeed = sqrt(SHRatio*Reng*temperature);
  double rho = fdmex->GetAtmosphere()->GetDensity(altitudeASL);
  double rhoSL = fdmex->GetAtmosphere()->GetDensitySL();

  double mach0 = vt / soundSpeed;
  double vc0 = calcVcas(mach0);
  double ve0 = vt * sqrt(rho/rhoSL);

  altitudeASL=alt;
  temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  soundSpeed = sqrt(SHRatio*Reng*temperature);
  rho = fdmex->GetAtmosphere()->GetDensity(altitudeASL);

  switch(lastSpeedSet) {
    case setvc:
      mach0 = getMachFromVcas(vc0);
    case setmach:
      SetVtrueFpsIC(mach0 * soundSpeed);
      break;
    case setve:
      SetVtrueFpsIC(ve0 * sqrt(rho/rhoSL));
      break;
  }

  position.SetRadius(alt + sea_level_radius);
}

//******************************************************************************
// Calculate the VCAS. Uses the Rayleigh formula for supersonic speeds
// (See "Introduction to Aerodynamics of a Compressible Fluid - H.W. Liepmann,
// A.E. Puckett - Wiley & sons (1947)" §5.4 pp 75-80)

double FGInitialCondition::calcVcas(double Mach) const
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double p=fdmex->GetAtmosphere()->GetPressure(altitudeASL);
  double psl=fdmex->GetAtmosphere()->GetPressureSL();
  double rhosl=fdmex->GetAtmosphere()->GetDensitySL();
  double pt,A,vcas;

  if (Mach < 0) Mach=0;
  if (Mach < 1)    //calculate total pressure assuming isentropic flow
    pt=p*pow((1 + 0.2*Mach*Mach),3.5);
  else {
    // shock in front of pitot tube, we'll assume its normal and use
    // the Rayleigh Pitot Tube Formula, i.e. the ratio of total
    // pressure behind the shock to the static pressure in front of
    // the normal shock assumption should not be a bad one -- most supersonic
    // aircraft place the pitot probe out front so that it is the forward
    // most point on the aircraft.  The real shock would, of course, take
    // on something like the shape of a rounded-off cone but, here again,
    // the assumption should be good since the opening of the pitot probe
    // is very small and, therefore, the effects of the shock curvature
    // should be small as well. AFAIK, this approach is fairly well accepted
    // within the aerospace community

    // The denominator below is zero for Mach ~ 0.38, for which
    // we'll never be here, so we're safe

    pt = p*166.92158*pow(Mach,7.0)/pow(7*Mach*Mach-1,2.5);
  }

  A = pow(((pt-p)/psl+1),0.28571);
  vcas = sqrt(7*psl/rhosl*(A-1));
  //cout << "calcVcas: vcas= " << vcas*fpstokts << " mach= " << Mach << " pressure: " << pt << endl;
  return vcas;
}

//******************************************************************************
// Reverse the VCAS formula to obtain the corresponding Mach number. For subsonic
// speeds, the reversed formula has a closed form. For supersonic speeds, the
// formula is reversed by the Newton-Raphson algorithm.

double FGInitialCondition::getMachFromVcas(double vcas)
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double p=fdmex->GetAtmosphere()->GetPressure(altitudeASL);
  double psl=fdmex->GetAtmosphere()->GetPressureSL();
  double rhosl=fdmex->GetAtmosphere()->GetDensitySL();

  double pt = p + psl*(pow(1+vcas*vcas*rhosl/(7.0*psl),3.5)-1);

  if (pt/p < 1.89293)
    return sqrt(5.0*(pow(pt/p, 0.2857143) -1)); // Mach < 1
  else {
    // Mach >= 1
    double mach = sqrt(0.77666*pt/p); // Initial guess is based on a quadratic approximation of the Rayleigh formula
    double delta = 1.;
    double target = pt/(166.92158*p);
    int iter = 0;

    // Find the root with Newton-Raphson. Since the differential is never zero,
    // the function is monotonic and has only one root with a multiplicity of one.
    // Convergence is certain.
    while (delta > 1E-5 && iter < 10) {
      double m2 = mach*mach; // Mach^2
      double m6 = m2*m2*m2;  // Mach^6
      delta = mach*m6/pow(7.0*m2-1.0,2.5) - target;
      double diff = 7.0*m6*(2.0*m2-1)/pow(7.0*m2-1.0,3.5); // Never zero when Mach >= 1
      mach -= delta/diff;
      iter++;
    }

    return mach;
  }
}

//******************************************************************************

double FGInitialCondition::GetWindDirDegIC(void) const
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED(eV) == 0.0 ? 0.0
                               : atan2(_vWIND_NED(eV), _vWIND_NED(eU))*radtodeg;
}

//******************************************************************************

double FGInitialCondition::GetNEDWindFpsIC(int idx) const
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED(idx);
}

//******************************************************************************

double FGInitialCondition::GetWindFpsIC(void) const
{
  FGColumnVector3 _vt_NED = Tb2l * Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vWIND_NED = _vt_NED - vUVW_NED;

  return _vWIND_NED.Magnitude(eU, eV);
}

//******************************************************************************

double FGInitialCondition::GetBodyWindFpsIC(int idx) const
{
  FGColumnVector3 _vt_BODY = Tw2b * FGColumnVector3(vt, 0., 0.);
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;
  FGColumnVector3 _vWIND_BODY = _vt_BODY - _vUVW_BODY;

  return _vWIND_BODY(idx);
}

//******************************************************************************

double FGInitialCondition::GetVcalibratedKtsIC(void) const
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  double soundSpeed = sqrt(SHRatio*Reng*temperature);
  double mach = vt / soundSpeed;
  return fpstokts * calcVcas(mach);
}

//******************************************************************************

double FGInitialCondition::GetVequivalentKtsIC(void) const
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double rho = fdmex->GetAtmosphere()->GetDensity(altitudeASL);
  double rhoSL = fdmex->GetAtmosphere()->GetDensitySL();
  return fpstokts * vt * sqrt(rho/rhoSL);
}

//******************************************************************************

double FGInitialCondition::GetMachIC(void) const
{
  double altitudeASL = position.GetRadius() - sea_level_radius;
  double temperature = fdmex->GetAtmosphere()->GetTemperature(altitudeASL);
  double soundSpeed = sqrt(SHRatio*Reng*temperature);
  return vt / soundSpeed;
}

//******************************************************************************

double FGInitialCondition::GetBodyVelFpsIC(int idx) const
{
  FGColumnVector3 _vUVW_BODY = Tl2b * vUVW_NED;

  return _vUVW_BODY(idx);
}

//******************************************************************************

bool FGInitialCondition::Load(string rstfile, bool useStoredPath)
{
  string sep = "/";
  if( useStoredPath ) {
    init_file_name = fdmex->GetFullAircraftPath() + sep + rstfile + ".xml";
  } else {
    init_file_name = rstfile;
  }

  document = LoadXMLDocument(init_file_name);

  // Make sure that the document is valid
  if (!document) {
    cerr << "File: " << init_file_name << " could not be read." << endl;
    exit(-1);
  }

  if (document->GetName() != string("initialize")) {
    cerr << "File: " << init_file_name << " is not a reset file." << endl;
    exit(-1);
  }

  double version = document->GetAttributeValueAsNumber("version");
  bool result = false;

  if (version == HUGE_VAL) {
    result = Load_v1(); // Default to the old version
  } else if (version >= 3.0) {
    cerr << "Only initialization file formats 1 and 2 are currently supported" << endl;
    exit (-1);
  } else if (version >= 2.0) {
    result = Load_v2();
  } else if (version >= 1.0) {
    result = Load_v1();
  }

  // Check to see if any engines are specified to be initialized in a running state
  FGPropulsion* propulsion = fdmex->GetPropulsion();
  Element* running_elements = document->FindElement("running");
  while (running_elements) {
    int n = int(running_elements->GetDataAsNumber());
    try {
      propulsion->InitRunning(n);
    } catch (string str) {
      cerr << str << endl;
      result = false;
    }
    running_elements = document->FindNextElement("running");
  }

  fdmex->RunIC();
  fdmex->GetPropagate()->DumpState();

  return result;
}

//******************************************************************************

bool FGInitialCondition::Load_v1(void)
{
  bool result = true;

  if (document->FindElement("latitude"))
    position.SetLatitude(document->FindElementValueAsNumberConvertTo("latitude", "RAD"));
  if (document->FindElement("longitude"))
    position.SetLongitude(document->FindElementValueAsNumberConvertTo("longitude", "RAD"));
  if (document->FindElement("elevation"))
    terrain_elevation = document->FindElementValueAsNumberConvertTo("elevation", "FT");

  if (document->FindElement("altitude")) // This is feet above ground level
    position.SetRadius(document->FindElementValueAsNumberConvertTo("altitude", "FT") + terrain_elevation + sea_level_radius);
  else if (document->FindElement("altitudeAGL")) // This is feet above ground level
    position.SetRadius(document->FindElementValueAsNumberConvertTo("altitudeAGL", "FT") + terrain_elevation + sea_level_radius);
  else if (document->FindElement("altitudeMSL")) // This is feet above sea level
    position.SetRadius(document->FindElementValueAsNumberConvertTo("altitudeMSL", "FT") + sea_level_radius);

  if (document->FindElement("phi"))
    phi = document->FindElementValueAsNumberConvertTo("phi", "RAD");
  if (document->FindElement("theta"))
    theta = document->FindElementValueAsNumberConvertTo("theta", "RAD");
  if (document->FindElement("psi"))
    psi = document->FindElementValueAsNumberConvertTo("psi", "RAD");

  FGQuaternion Quat(phi, theta, psi);
  Quat.Normalize();
  Tl2b = Quat.GetT();
  Tb2l = Quat.GetTInv();

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
  {
    SetTargetNlfIC(document->FindElementValueAsNumber("targetNlf"));
  }

  return result;
}

//******************************************************************************

bool FGInitialCondition::Load_v2(void)
{
  FGColumnVector3 vOrient;
  bool result = true;
  FGColumnVector3 vOmegaEarth = FGColumnVector3(0.0, 0.0, fdmex->GetInertial()->omega());

  if (document->FindElement("earth_position_angle"))
    position.SetEarthPositionAngle(document->FindElementValueAsNumberConvertTo("earth_position_angle", "RAD"));

  if (document->FindElement("elevation"))
    terrain_elevation = document->FindElementValueAsNumberConvertTo("elevation", "FT");

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
      position = position.GetTi2ec() * position_el->FindElementTripletConvertTo("FT");
    } else if (frame == "ecef") {
      if (!position_el->FindElement("x") && !position_el->FindElement("y") && !position_el->FindElement("z")) {
        if (position_el->FindElement("radius")) {
          position.SetRadius(position_el->FindElementValueAsNumberConvertTo("radius", "FT"));
        } else if (position_el->FindElement("altitudeAGL")) {
          position.SetRadius(sea_level_radius + terrain_elevation + position_el->FindElementValueAsNumberConvertTo("altitude", "FT"));
        } else if (position_el->FindElement("altitudeMSL")) {
          position.SetRadius(sea_level_radius + position_el->FindElementValueAsNumberConvertTo("altitudeMSL", "FT"));
        } else {
          cerr << endl << "  No altitude or radius initial condition is given." << endl;
          result = false;
        }
        if (position_el->FindElement("longitude"))
          position.SetLongitude(position_el->FindElementValueAsNumberConvertTo("longitude", "RAD"));
        if (position_el->FindElement("latitude"))
          position.SetLatitude(position_el->FindElementValueAsNumberConvertTo("latitude", "RAD"));
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
  // i=inertial frame, and e=ecef frame) as:
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
  FGQuaternion QuatLocal2Body;
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
      // Q_b/l = Q_e/l * Q_b/i

      FGQuaternion QuatI2Body = FGQuaternion(vOrient);
      QuatI2Body.Normalize();
      FGQuaternion QuatLocal2I = position.GetTl2i();
      QuatLocal2I.Normalize();
      QuatLocal2Body = QuatLocal2I * QuatI2Body;

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
      QuatLocal2Body = QuatLocal2EC * QuatEC2Body; // Q_b/l = Q_e/l * Q_b/e

    } else if (frame == "local") {

      QuatLocal2Body = FGQuaternion(vOrient);

    } else {

      cerr << endl << fgred << "  Orientation frame type: \"" << frame
           << "\" is not supported!" << reset << endl << endl;
      result = false;

    }
  }

  QuatLocal2Body.Normalize();
  phi = QuatLocal2Body.GetEuler(ePhi);
  theta = QuatLocal2Body.GetEuler(eTht);
  psi = QuatLocal2Body.GetEuler(ePsi);
  Tl2b = QuatLocal2Body.GetT();
  Tb2l = QuatLocal2Body.GetTInv();

  // Initialize vehicle velocity
  // Allowable frames
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)
  // - Local
  // - Body
  // The vehicle will be defaulted to (0,0,0) in the Body frame if nothing is provided.
  
  Element* velocity_el = document->FindElement("velocity");
  FGColumnVector3 vInitVelocity = FGColumnVector3(0.0, 0.0, 0.0);
  FGMatrix33 mTec2l = position.GetTec2l();
  if (velocity_el) {

    string frame = velocity_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    FGColumnVector3 vInitVelocity = velocity_el->FindElementTripletConvertTo("FT/SEC");

    if (frame == "eci") {
      FGColumnVector3 omega_cross_r = vOmegaEarth * (position.GetTec2i() * position);
      vUVW_NED = mTec2l * (vInitVelocity - omega_cross_r);
    } else if (frame == "ecef") {
      vUVW_NED = mTec2l * vInitVelocity;
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

    vUVW_NED = Tb2l * vInitVelocity;

  }

  vt = vUVW_NED.Magnitude();

  calcAeroAngles(vUVW_NED);

  // Initialize vehicle body rates
  // Allowable frames
  // - ECI (Earth Centered Inertial)
  // - ECEF (Earth Centered, Earth Fixed)
  // - Body
  
  FGColumnVector3 vLocalRate;
  Element* attrate_el = document->FindElement("attitude_rate");
  double radInv = 1.0 / position.GetRadius();
  FGColumnVector3 vOmegaLocal = FGColumnVector3(
   radInv*vUVW_NED(eEast),
  -radInv*vUVW_NED(eNorth),
  -radInv*vUVW_NED(eEast)*position.GetTanLatitude() );

  if (attrate_el) {

    string frame = attrate_el->GetAttributeValue("frame");
    frame = to_lower(frame);
    FGColumnVector3 vAttRate = attrate_el->FindElementTripletConvertTo("RAD/SEC");

    if (frame == "eci") {
      vLocalRate = Tl2b * (position.GetTi2l() * (vAttRate - vOmegaEarth) - vOmegaLocal);
    } else if (frame == "ecef") {
      vLocalRate = Tl2b * (position.GetTec2l() * vAttRate - vOmegaLocal);
    } else if (frame == "local") {
      vLocalRate = vAttRate;
    } else if (!frame.empty()) { // misspelling of frame
      
      cerr << endl << fgred << "  Attitude rate frame type: \"" << frame
           << "\" is not supported!" << reset << endl << endl;
      result = false;

    } else if (frame.empty()) {
    
    }
    
  } else { // Body frame attitude rate assumed 0 relative to local.
      vLocalRate.InitMatrix();
  }

  p = vLocalRate(eP);
  q = vLocalRate(eQ);
  r = vLocalRate(eR);

  return result;
}

//******************************************************************************

void FGInitialCondition::bind(void)
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
                       &FGInitialCondition::GetPsiDegIC );
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
  PropertyManager->Tie("ic/sea-level-radius-ft", this,
                       &FGInitialCondition::GetSeaLevelRadiusFtIC,
                       &FGInitialCondition::SetSeaLevelRadiusFtIC,
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
                       &FGInitialCondition::GetPsiRadIC);
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

  typedef int (FGInitialCondition::*iPMF)(void) const;
  PropertyManager->Tie("simulation/write-state-file",
                       this,
                       (iPMF)0,
                       &FGInitialCondition::WriteStateFile);

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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
