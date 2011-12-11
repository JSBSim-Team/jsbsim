/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGWinds.cpp
 Author:       Jon Berndt, Tony Peden, Andreas Gaeb
 Date started: Extracted from FGAtmosphere, which originated in 1998
               5/2011
 Purpose:      Models winds, gusts, turbulence, and other atmospheric disturbances
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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <cstdlib>
#include "FGWinds.h"
#include "FGFDMExec.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGWinds.cpp,v 1.7 2011/12/11 17:03:05 bcoconni Exp $";
static const char *IdHdr = ID_WINDS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// square a value, but preserve the original sign

static inline double square_signed (double value)
{
    if (value < 0)
        return value * value * -1;
    else
        return value * value;
}

/// simply square a value
static inline double sqr(double x) { return x*x; }

FGWinds::FGWinds(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGWinds";

  MagnitudedAccelDt = MagnitudeAccel = Magnitude = 0.0;
  SetTurbType( ttMilspec );
  TurbGain = 1.0;
  TurbRate = 10.0;
  Rhythmicity = 0.1;
  spike = target_time = strength = 0.0;
  wind_from_clockwise = 0.0;
  psiw = 0.0;

  vGustNED.InitMatrix();
  vTurbulenceNED.InitMatrix();

  // Milspec turbulence model
  windspeed_at_20ft = 0.;
  probability_of_exceedence_index = 0;
  POE_Table = new FGTable(7,12);
  // this is Figure 7 from p. 49 of MIL-F-8785C
  // rows: probability of exceedance curve index, cols: altitude in ft
  *POE_Table
           << 500.0 << 1750.0 << 3750.0 << 7500.0 << 15000.0 << 25000.0 << 35000.0 << 45000.0 << 55000.0 << 65000.0 << 75000.0 << 80000.0
    << 1   <<   3.2 <<    2.2 <<    1.5 <<    0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0
    << 2   <<   4.2 <<    3.6 <<    3.3 <<    1.6 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0
    << 3   <<   6.6 <<    6.9 <<    7.4 <<    6.7 <<     4.6 <<     2.7 <<     0.4 <<     0.0 <<     0.0 <<     0.0 <<     0.0 <<     0.0
    << 4   <<   8.6 <<    9.6 <<   10.6 <<   10.1 <<     8.0 <<     6.6 <<     5.0 <<     4.2 <<     2.7 <<     0.0 <<     0.0 <<     0.0
    << 5   <<  11.8 <<   13.0 <<   16.0 <<   15.1 <<    11.6 <<     9.7 <<     8.1 <<     8.2 <<     7.9 <<     4.9 <<     3.2 <<     2.1
    << 6   <<  15.6 <<   17.6 <<   23.0 <<   23.6 <<    22.1 <<    20.0 <<    16.0 <<    15.1 <<    12.1 <<     7.9 <<     6.2 <<     5.1
    << 7   <<  18.7 <<   21.5 <<   28.4 <<   30.2 <<    30.7 <<    31.0 <<    25.2 <<    23.1 <<    17.5 <<    10.7 <<     8.4 <<     7.2;

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGWinds::~FGWinds()
{
  delete(POE_Table);
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGWinds::InitModel(void)
{
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGWinds::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  if (turbType != ttNone) Turbulence(in.AltitudeASL);
  if (oneMinusCosineGust.gustProfile.Running) CosineGust();

  vTotalWindNED = vWindNED + vGustNED + vCosineGust + vTurbulenceNED;

   // psiw (Wind heading) is the direction the wind is blowing towards
  if (vWindNED(eX) != 0.0) psiw = atan2( vWindNED(eY), vWindNED(eX) );
  if (psiw < 0) psiw += 2*M_PI;

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// psi is the angle that the wind is blowing *towards*

void FGWinds::SetWindspeed(double speed)
{
  if (vWindNED.Magnitude() == 0.0) {
    psiw = 0.0;
    vWindNED(eNorth) = speed;
  } else {
    vWindNED(eNorth) = speed * cos(psiw);
    vWindNED(eEast) = speed * sin(psiw);
    vWindNED(eDown) = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGWinds::GetWindspeed(void) const
{
  return vWindNED.Magnitude();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// psi is the angle that the wind is blowing *towards*

void FGWinds::SetWindPsi(double dir)
{
  double mag = GetWindspeed();
  psiw = dir;
  SetWindspeed(mag);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGWinds::Turbulence(double h)
{
  switch (turbType) {

  case ttCulp: {

    vTurbPQR(eP) = wind_from_clockwise;
    if (TurbGain == 0.0) return;

    // keep the inputs within allowable limts for this model
    if (TurbGain < 0.0) TurbGain = 0.0;
    if (TurbGain > 1.0) TurbGain = 1.0;
    if (TurbRate < 0.0) TurbRate = 0.0;
    if (TurbRate > 30.0) TurbRate = 30.0;
    if (Rhythmicity < 0.0) Rhythmicity = 0.0;
    if (Rhythmicity > 1.0) Rhythmicity = 1.0;

    // generate a sine wave corresponding to turbulence rate in hertz
    double time = FDMExec->GetSimTime();
    double sinewave = sin( time * TurbRate * 6.283185307 );

    double random = 0.0;
    if (target_time == 0.0) {
      strength = random = 1 - 2.0*(double(rand())/double(RAND_MAX));
      target_time = time + 0.71 + (random * 0.5);
    }
    if (time > target_time) {
      spike = 1.0;
      target_time = 0.0;
    }

    // max vertical wind speed in fps, corresponds to TurbGain = 1.0
    double max_vs = 40;

    vTurbulenceNED(1) = vTurbulenceNED(2) = vTurbulenceNED(3) = 0.0;
    double delta = strength * max_vs * TurbGain * (1-Rhythmicity) * spike;

    // Vertical component of turbulence.
    vTurbulenceNED(3) = sinewave * max_vs * TurbGain * Rhythmicity;
    vTurbulenceNED(3)+= delta;
    if (in.DistanceAGL/in.wingspan < 3.0)
        vTurbulenceNED(3) *= in.DistanceAGL/in.wingspan * 0.3333;

    // Yaw component of turbulence.
    vTurbulenceNED(1) = sin( delta * 3.0 );
    vTurbulenceNED(2) = cos( delta * 3.0 );

    // Roll component of turbulence. Clockwise vortex causes left roll.
    vTurbPQR(eP) += delta * 0.04;

    spike = spike * 0.9;
    break;
  }
  case ttMilspec:
  case ttTustin: {

    // an index of zero means turbulence is disabled
    // airspeed occurs as divisor in the code below
    if (probability_of_exceedence_index == 0 || in.V == 0) {
      vTurbulenceNED(1) = vTurbulenceNED(2) = vTurbulenceNED(3) = 0.0;
      vTurbPQR(1) = vTurbPQR(2) = vTurbPQR(3) = 0.0;
      return;
    }

    // Turbulence model according to MIL-F-8785C (Flying Qualities of Piloted Aircraft)
    double b_w = in.wingspan, L_u, L_w, sig_u, sig_w;

      if (b_w == 0.) b_w = 30.;

    // clip height functions at 10 ft
    if (h <= 10.) h = 10;

    // Scale lengths L and amplitudes sigma as function of height
    if (h <= 1000) {
      L_u = h/pow(0.177 + 0.000823*h, 1.2); // MIL-F-8785c, Fig. 10, p. 55
      L_w = h;
      sig_w = 0.1*windspeed_at_20ft;
      sig_u = sig_w/pow(0.177 + 0.000823*h, 0.4); // MIL-F-8785c, Fig. 11, p. 56
    } else if (h <= 2000) {
      // linear interpolation between low altitude and high altitude models
      L_u = L_w = 1000 + (h-1000.)/1000.*750.;
      sig_u = sig_w = 0.1*windspeed_at_20ft
                    + (h-1000.)/1000.*(POE_Table->GetValue(probability_of_exceedence_index, h) - 0.1*windspeed_at_20ft);
    } else {
      L_u = L_w = 1750.; //  MIL-F-8785c, Sec. 3.7.2.1, p. 48
      sig_u = sig_w = POE_Table->GetValue(probability_of_exceedence_index, h);
    }

    // keep values from last timesteps
    // TODO maybe use deque?
    static double
      xi_u_km1 = 0, nu_u_km1 = 0,
      xi_v_km1 = 0, xi_v_km2 = 0, nu_v_km1 = 0, nu_v_km2 = 0,
      xi_w_km1 = 0, xi_w_km2 = 0, nu_w_km1 = 0, nu_w_km2 = 0,
      xi_p_km1 = 0, nu_p_km1 = 0,
      xi_q_km1 = 0, xi_r_km1 = 0;


    double
      T_V = in.totalDeltaT, // for compatibility of nomenclature
      sig_p = 1.9/sqrt(L_w*b_w)*sig_w, // Yeager1998, eq. (8)
      //sig_q = sqrt(M_PI/2/L_w/b_w), // eq. (14)
      //sig_r = sqrt(2*M_PI/3/L_w/b_w), // eq. (17)
      L_p = sqrt(L_w*b_w)/2.6, // eq. (10)
      tau_u = L_u/in.V, // eq. (6)
      tau_w = L_w/in.V, // eq. (3)
      tau_p = L_p/in.V, // eq. (9)
      tau_q = 4*b_w/M_PI/in.V, // eq. (13)
      tau_r =3*b_w/M_PI/in.V, // eq. (17)
      nu_u = GaussianRandomNumber(),
      nu_v = GaussianRandomNumber(),
      nu_w = GaussianRandomNumber(),
      nu_p = GaussianRandomNumber(),
      xi_u=0, xi_v=0, xi_w=0, xi_p=0, xi_q=0, xi_r=0;

    // values of turbulence NED velocities

    if (turbType == ttTustin) {
      // the following is the Tustin formulation of Yeager's report
      double
        omega_w = in.V/L_w, // hidden in nomenclature p. 3
        omega_v = in.V/L_u, // this is defined nowhere
        C_BL  = 1/tau_u/tan(T_V/2/tau_u), // eq. (19)
        C_BLp = 1/tau_p/tan(T_V/2/tau_p), // eq. (22)
        C_BLq = 1/tau_q/tan(T_V/2/tau_q), // eq. (24)
        C_BLr = 1/tau_r/tan(T_V/2/tau_r); // eq. (26)

      // all values calculated so far are strictly positive, except for
      // the random numbers nu_*. This means that in the code below, all
      // divisors are strictly positive, too, and no floating point
      // exception should occur.
      xi_u = -(1 - C_BL*tau_u)/(1 + C_BL*tau_u)*xi_u_km1
           + sig_u*sqrt(2*tau_u/T_V)/(1 + C_BL*tau_u)*(nu_u + nu_u_km1); // eq. (18)
      xi_v = -2*(sqr(omega_v) - sqr(C_BL))/sqr(omega_v + C_BL)*xi_v_km1
           - sqr(omega_v - C_BL)/sqr(omega_v + C_BL) * xi_v_km2
           + sig_u*sqrt(3*omega_v/T_V)/sqr(omega_v + C_BL)*(
                 (C_BL + omega_v/sqrt(3.))*nu_v
               + 2/sqrt(3.)*omega_v*nu_v_km1
               + (omega_v/sqrt(3.) - C_BL)*nu_v_km2); // eq. (20) for v
      xi_w = -2*(sqr(omega_w) - sqr(C_BL))/sqr(omega_w + C_BL)*xi_w_km1
           - sqr(omega_w - C_BL)/sqr(omega_w + C_BL) * xi_w_km2
           + sig_w*sqrt(3*omega_w/T_V)/sqr(omega_w + C_BL)*(
                 (C_BL + omega_w/sqrt(3.))*nu_w
               + 2/sqrt(3.)*omega_w*nu_w_km1
               + (omega_w/sqrt(3.) - C_BL)*nu_w_km2); // eq. (20) for w
      xi_p = -(1 - C_BLp*tau_p)/(1 + C_BLp*tau_p)*xi_p_km1
           + sig_p*sqrt(2*tau_p/T_V)/(1 + C_BLp*tau_p) * (nu_p + nu_p_km1); // eq. (21)
      xi_q = -(1 - 4*b_w*C_BLq/M_PI/in.V)/(1 + 4*b_w*C_BLq/M_PI/in.V) * xi_q_km1
           + C_BLq/in.V/(1 + 4*b_w*C_BLq/M_PI/in.V) * (xi_w - xi_w_km1); // eq. (23)
      xi_r = - (1 - 3*b_w*C_BLr/M_PI/in.V)/(1 + 3*b_w*C_BLr/M_PI/in.V) * xi_r_km1
           + C_BLr/in.V/(1 + 3*b_w*C_BLr/M_PI/in.V) * (xi_v - xi_v_km1); // eq. (25)

    } else if (turbType == ttMilspec) {
      // the following is the MIL-STD-1797A formulation
      // as cited in Yeager's report
      xi_u = (1 - T_V/tau_u)  *xi_u_km1 + sig_u*sqrt(2*T_V/tau_u)*nu_u;  // eq. (30)
      xi_v = (1 - 2*T_V/tau_u)*xi_v_km1 + sig_u*sqrt(4*T_V/tau_u)*nu_v;  // eq. (31)
      xi_w = (1 - 2*T_V/tau_w)*xi_w_km1 + sig_w*sqrt(4*T_V/tau_w)*nu_w;  // eq. (32)
      xi_p = (1 - T_V/tau_p)  *xi_p_km1 + sig_p*sqrt(2*T_V/tau_p)*nu_p;  // eq. (33)
      xi_q = (1 - T_V/tau_q)  *xi_q_km1 + M_PI/4/b_w*(xi_w - xi_w_km1);  // eq. (34)
      xi_r = (1 - T_V/tau_r)  *xi_r_km1 + M_PI/3/b_w*(xi_v - xi_v_km1);  // eq. (35)
    }

    // rotate by wind azimuth and assign the velocities
    double cospsi = cos(psiw), sinpsi = sin(psiw);
    vTurbulenceNED(1) =  cospsi*xi_u + sinpsi*xi_v;
    vTurbulenceNED(2) = -sinpsi*xi_u + cospsi*xi_v;
    vTurbulenceNED(3) = xi_w;

    vTurbPQR(1) =  cospsi*xi_p + sinpsi*xi_q;
    vTurbPQR(2) = -sinpsi*xi_p + cospsi*xi_q;
    vTurbPQR(3) = xi_r;

    // vTurbPQR is in the body fixed frame, not NED
    vTurbPQR = in.Tl2b*vTurbPQR;

    // hand on the values for the next timestep
    xi_u_km1 = xi_u; nu_u_km1 = nu_u;
    xi_v_km2 = xi_v_km1; xi_v_km1 = xi_v; nu_v_km2 = nu_v_km1; nu_v_km1 = nu_v;
    xi_w_km2 = xi_w_km1; xi_w_km1 = xi_w; nu_w_km2 = nu_w_km1; nu_w_km1 = nu_w;
    xi_p_km1 = xi_p; nu_p_km1 = nu_p;
    xi_q_km1 = xi_q;
    xi_r_km1 = xi_r;

  }
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGWinds::CosineGustProfile(double startDuration, double steadyDuration, double endDuration, double elapsedTime)
{
  double factor = 0.0;
  if (elapsedTime >= 0 && elapsedTime <= startDuration) {
    factor = (1.0 - cos(M_PI*elapsedTime/startDuration))/2.0;
  } else if (elapsedTime > startDuration && (elapsedTime <= (startDuration + steadyDuration))) {
    factor = 1.0;
  } else if (elapsedTime > (startDuration + steadyDuration) && elapsedTime <= (startDuration + steadyDuration + endDuration)) {
    factor = (1-cos(M_PI*(1-(elapsedTime-(startDuration + steadyDuration))/endDuration)))/2.0;
  } else {
    factor = 0.0;
  }

  return factor;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGWinds::CosineGust()
{
  struct OneMinusCosineProfile& profile = oneMinusCosineGust.gustProfile;

  double factor = CosineGustProfile( profile.startupDuration,
                                     profile.steadyDuration,
                                     profile.endDuration,
                                     profile.elapsedTime);
  // Normalize the gust wind vector
  oneMinusCosineGust.vWind.Normalize();

  if (oneMinusCosineGust.vWindTransformed.Magnitude() == 0.0) {
    switch (oneMinusCosineGust.gustFrame) {
    case gfBody:
      oneMinusCosineGust.vWindTransformed = in.Tl2b.Inverse() * oneMinusCosineGust.vWind;
      break;
    case gfWind:
      oneMinusCosineGust.vWindTransformed = in.Tl2b.Inverse() * in.Tw2b * oneMinusCosineGust.vWind;
      break;
    case gfLocal:
      // this is the native frame - and the default.
      oneMinusCosineGust.vWindTransformed = oneMinusCosineGust.vWind;
      break;
    default:
      break;
    }
  }

  vCosineGust = factor * oneMinusCosineGust.vWindTransformed * oneMinusCosineGust.magnitude;

  profile.elapsedTime += in.totalDeltaT;

  if (profile.elapsedTime > (profile.startupDuration + profile.steadyDuration + profile.endDuration)) {
    profile.Running = false;
    profile.elapsedTime = 0.0;
    oneMinusCosineGust.vWindTransformed.InitMatrix(0.0);
    vCosineGust.InitMatrix(0);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGWinds::NumberOfUpDownburstCells(int num)
{
  for (unsigned int i=0; i<UpDownBurstCells.size();i++) delete UpDownBurstCells[i];
  UpDownBurstCells.clear();
  if (num >= 0) {
    for (unsigned int i=0; i<num; i++) UpDownBurstCells.push_back(new struct UpDownBurst);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Calculates the distance between a specified point (where presumably the
// Up/Downburst is centered) and the current vehicle location. The distance
// here is calculated from the Haversine formula.

double FGWinds::DistanceFromRingCenter(double lat, double lon)
{
  double deltaLat = in.latitude - lat;
  double deltaLong = in.longitude - lon;
  double dLat2 = deltaLat/2.0;
  double dLong2 = deltaLong/2.0;
  double a = sin(dLat2)*sin(dLat2)
             + cos(lat)*cos(in.latitude)*sin(dLong2)*sin(dLong2);
  double c = 2.0*atan2(sqrt(a), sqrt(1.0 - a));
  double d = in.planetRadius*c;
  return d;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGWinds::UpDownBurst()
{

  for (unsigned int i=0; i<UpDownBurstCells.size(); i++) {
    double d = DistanceFromRingCenter(UpDownBurstCells[i]->ringLatitude, UpDownBurstCells[i]->ringLongitude);

  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGWinds::bind(void)
{
  typedef double (FGWinds::*PMF)(int) const;
  typedef int (FGWinds::*PMFt)(void) const;
  typedef void   (FGWinds::*PMFd)(int,double);
  typedef void   (FGWinds::*PMFi)(int);
  typedef double (FGWinds::*Ptr)(void) const;

  // User-specified steady, constant, wind properties (local navigational/geographic frame: N-E-D)
  PropertyManager->Tie("atmosphere/psiw-rad", this, &FGWinds::GetWindPsi, &FGWinds::SetWindPsi);
  PropertyManager->Tie("atmosphere/wind-north-fps", this, eNorth, (PMF)&FGWinds::GetWindNED,
                                                          (PMFd)&FGWinds::SetWindNED);
  PropertyManager->Tie("atmosphere/wind-east-fps",  this, eEast, (PMF)&FGWinds::GetWindNED,
                                                          (PMFd)&FGWinds::SetWindNED);
  PropertyManager->Tie("atmosphere/wind-down-fps",  this, eDown, (PMF)&FGWinds::GetWindNED,
                                                          (PMFd)&FGWinds::SetWindNED);
  PropertyManager->Tie("atmosphere/wind-mag-fps", this, &FGWinds::GetWindspeed,
                                                        &FGWinds::SetWindspeed);

  // User-specifieded gust (local navigational/geographic frame: N-E-D)
  PropertyManager->Tie("atmosphere/gust-north-fps", this, eNorth, (PMF)&FGWinds::GetGustNED,
                                                          (PMFd)&FGWinds::SetGustNED);
  PropertyManager->Tie("atmosphere/gust-east-fps",  this, eEast, (PMF)&FGWinds::GetGustNED,
                                                          (PMFd)&FGWinds::SetGustNED);
  PropertyManager->Tie("atmosphere/gust-down-fps",  this, eDown, (PMF)&FGWinds::GetGustNED,
                                                          (PMFd)&FGWinds::SetGustNED);

  // User-specified 1 - cosine gust parameters (in specified frame)
  PropertyManager->Tie("atmosphere/cosine-gust/startup-duration-sec", this, (Ptr)0L, &FGWinds::StartupGustDuration);
  PropertyManager->Tie("atmosphere/cosine-gust/steady-duration-sec", this, (Ptr)0L, &FGWinds::SteadyGustDuration);
  PropertyManager->Tie("atmosphere/cosine-gust/end-duration-sec", this, (Ptr)0L, &FGWinds::EndGustDuration);
  PropertyManager->Tie("atmosphere/cosine-gust/magnitude-ft_sec", this, (Ptr)0L, &FGWinds::GustMagnitude);
  PropertyManager->Tie("atmosphere/cosine-gust/frame", this, (PMFt)0L, (PMFi)&FGWinds::GustFrame);
  PropertyManager->Tie("atmosphere/cosine-gust/X-velocity-ft_sec", this, (Ptr)0L, &FGWinds::GustXComponent);
  PropertyManager->Tie("atmosphere/cosine-gust/Y-velocity-ft_sec", this, (Ptr)0L, &FGWinds::GustYComponent);
  PropertyManager->Tie("atmosphere/cosine-gust/Z-velocity-ft_sec", this, (Ptr)0L, &FGWinds::GustZComponent);
  PropertyManager->Tie("atmosphere/cosine-gust/start", this, (PMFt)0L, (PMFi)&FGWinds::StartGust);

  // User-specified Up- Down-burst parameters
  PropertyManager->Tie("atmosphere/updownburst/number-of-cells", this, (PMFt)0L, &FGWinds::NumberOfUpDownburstCells);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);
//  PropertyManager->Tie("atmosphere/updownburst/", this, (Ptr)0L, &FGWinds::);

  // User-specified turbulence (local navigational/geographic frame: N-E-D)
  PropertyManager->Tie("atmosphere/turb-north-fps", this, eNorth, (PMF)&FGWinds::GetTurbNED,
                                                          (PMFd)&FGWinds::SetTurbNED);
  PropertyManager->Tie("atmosphere/turb-east-fps",  this, eEast, (PMF)&FGWinds::GetTurbNED,
                                                          (PMFd)&FGWinds::SetTurbNED);
  PropertyManager->Tie("atmosphere/turb-down-fps",  this, eDown, (PMF)&FGWinds::GetTurbNED,
                                                          (PMFd)&FGWinds::SetTurbNED);
  // Experimental turbulence parameters
  PropertyManager->Tie("atmosphere/p-turb-rad_sec", this,1, (PMF)&FGWinds::GetTurbPQR);
  PropertyManager->Tie("atmosphere/q-turb-rad_sec", this,2, (PMF)&FGWinds::GetTurbPQR);
  PropertyManager->Tie("atmosphere/r-turb-rad_sec", this,3, (PMF)&FGWinds::GetTurbPQR);
  PropertyManager->Tie("atmosphere/turb-type", this, (PMFt)&FGWinds::GetTurbType, (PMFi)&FGWinds::SetTurbType);
  PropertyManager->Tie("atmosphere/turb-rate", this, &FGWinds::GetTurbRate, &FGWinds::SetTurbRate);
  PropertyManager->Tie("atmosphere/turb-gain", this, &FGWinds::GetTurbGain, &FGWinds::SetTurbGain);
  PropertyManager->Tie("atmosphere/turb-rhythmicity", this, &FGWinds::GetRhythmicity,
                                                            &FGWinds::SetRhythmicity);

  // Parameters for milspec turbulence
  PropertyManager->Tie("atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps",
                       this, &FGWinds::GetWindspeed20ft,
                             &FGWinds::SetWindspeed20ft);
  PropertyManager->Tie("atmosphere/turbulence/milspec/severity",
                       this, &FGWinds::GetProbabilityOfExceedence,
                             &FGWinds::SetProbabilityOfExceedence);

  // Total, calculated winds (local navigational/geographic frame: N-E-D). Read only.
  PropertyManager->Tie("atmosphere/total-wind-north-fps", this, eNorth, (PMF)&FGWinds::GetTotalWindNED);
  PropertyManager->Tie("atmosphere/total-wind-east-fps",  this, eEast,  (PMF)&FGWinds::GetTotalWindNED);
  PropertyManager->Tie("atmosphere/total-wind-down-fps",  this, eDown,  (PMF)&FGWinds::GetTotalWindNED);

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

void FGWinds::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGWinds" << endl;
    if (from == 1) cout << "Destroyed:    FGWinds" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 128) { //
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
