/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAuxiliary.cpp
 Author:       Tony Peden, Jon Berndt
 Date started: 01/26/99
 Purpose:      Calculates additional parameters needed by the visual system, etc.
 Called by:    FGFDMExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
This class calculates various auxiliary parameters.

REFERENCES
  Anderson, John D. "Introduction to Flight", 3rd Edition, McGraw-Hill, 1989
                    pgs. 112-126
HISTORY
--------------------------------------------------------------------------------
01/26/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>

#include "FGAuxiliary.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.58 2012/09/03 21:36:08 bcoconni Exp $";
static const char *IdHdr = ID_AUXILIARY;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAuxiliary";
  pt = 1.0;
  tat = 1.0;
  tatc = RankineToCelsius(tat);

  vcas = veas = 0.0;
  qbar = qbarUW = qbarUV = 0.0;
  Mach = MachU = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  gamma = Vt = Vground = 0.0;
  psigt = 0.0;
  day_of_year = 1;
  seconds_in_day = 0.0;
  hoverbmac = hoverbcg = 0.0;
  Re = 0.0;
  Nz = 0.0;
  lon_relative_position = lat_relative_position = relative_position = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vAeroUVW.InitMatrix();
  vAeroPQR.InitMatrix();
  vMachUVW.InitMatrix();
  vEuler.InitMatrix();
  vEulerRates.InitMatrix();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::InitModel(void)
{
  pt = in.Pressure;
  tat = in.Temperature;
  tatc = RankineToCelsius(tat);

  vcas = veas = 0.0;
  qbar = qbarUW = qbarUV = 0.0;
  Mach = MachU = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  gamma = Vt = Vground = 0.0;
  psigt = 0.0;
  day_of_year = 1;
  seconds_in_day = 0.0;
  hoverbmac = hoverbcg = 0.0;
  Re = 0.0;
  Nz = 0.0;
  lon_relative_position = lat_relative_position = relative_position = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vAeroUVW.InitMatrix();
  vAeroPQR.InitMatrix();
  vMachUVW.InitMatrix();
  vEuler.InitMatrix();
  vEulerRates.InitMatrix();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAuxiliary::~FGAuxiliary()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::Run(bool Holding)
{
  double A,B,D;

  if (FGModel::Run(Holding)) return true; // return true if error returned from base class
  if (Holding) return false;

  // Rotation

  vEulerRates(eTht) = in.vPQR(eQ)*in.CosPhi - in.vPQR(eR)*in.SinPhi;
  if (in.CosTht != 0.0) {
    vEulerRates(ePsi) = (in.vPQR(eQ)*in.SinPhi + in.vPQR(eR)*in.CosPhi)/in.CosTht;
    vEulerRates(ePhi) = in.vPQR(eP) + vEulerRates(ePsi)*in.SinTht;
  }

  // Combine the wind speed with aircraft speed to obtain wind relative speed
  vAeroPQR = in.vPQR - in.TurbPQR;
  vAeroUVW = in.vUVW - in.Tl2b * in.TotalWindNED;

  Vt = vAeroUVW.Magnitude();
  alpha = beta = adot = bdot = 0;
  double AeroU2 = vAeroUVW(eU)*vAeroUVW(eU);
  double AeroV2 = vAeroUVW(eV)*vAeroUVW(eV);
  double AeroW2 = vAeroUVW(eW)*vAeroUVW(eW);
  double mUW = AeroU2 + AeroW2;
  double Vtdot = (vAeroUVW(eU)*in.vUVWdot(eU) + vAeroUVW(eV)*in.vUVWdot(eV) + vAeroUVW(eW)*in.vUVWdot(eW))/Vt;

  double Vt2 = Vt*Vt;

  if ( Vt > 1.0 ) {
    if (vAeroUVW(eW) != 0.0)
      alpha = AeroU2 > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
    if (vAeroUVW(eV) != 0.0)
      beta  =    mUW > 0.0 ? atan2(vAeroUVW(eV), sqrt(mUW)) : 0.0;

    //double signU=1;
    //if (vAeroUVW(eU) < 0.0) signU=-1;

    if ( mUW >= 1.0 ) {
      adot = (vAeroUVW(eU)*in.vUVWdot(eW) - vAeroUVW(eW)*in.vUVWdot(eU))/mUW;
      // bdot = (signU*mUW*in.vUVWdot(eV)
      //        - vAeroUVW(eV)*(vAeroUVW(eU)*in.vUVWdot(eU) + vAeroUVW(eW)*in.vUVWdot(eW)))/(Vt2*sqrt(mUW));
      bdot = (in.vUVWdot(eV)*Vt - vAeroUVW(eV)*Vtdot)/(Vt*sqrt(mUW));
    }
  }

  UpdateWindMatrices();

  Re = Vt * in.Wingchord / in.KinematicViscosity;

  double densityD2 = 0.5*in.Density;

  qbar = densityD2 * Vt2;
  qbarUW = densityD2 * (mUW);
  qbarUV = densityD2 * (AeroU2 + AeroV2);
  Mach = Vt / in.SoundSpeed;
  MachU = vMachUVW(eU) = vAeroUVW(eU) / in.SoundSpeed;
  vMachUVW(eV) = vAeroUVW(eV) / in.SoundSpeed;
  vMachUVW(eW) = vAeroUVW(eW) / in.SoundSpeed;
  double MachU2 = MachU * MachU;

  // Position

  Vground = sqrt( in.vVel(eNorth)*in.vVel(eNorth) + in.vVel(eEast)*in.vVel(eEast) );

  psigt = atan2(in.vVel(eEast), in.vVel(eNorth));
  if (psigt < 0.0) psigt += 2*M_PI;
  gamma = atan2(-in.vVel(eDown), Vground);

  tat = in.Temperature*(1 + 0.2*Mach*Mach); // Total Temperature, isentropic flow
  tatc = RankineToCelsius(tat);

  if (MachU < 1) {   // Calculate total pressure assuming isentropic flow
    pt = in.Pressure*pow((1 + 0.2*MachU2),3.5);
  } else {
    // Use Rayleigh pitot tube formula for normal shock in front of pitot tube
    B = 5.76 * MachU2 / (5.6*MachU2 - 0.8);
    D = (2.8 * MachU2 - 0.4) * 0.4167;
    pt = in.Pressure*pow(B,3.5)*D;
  }

  A = pow(((pt-in.Pressure)/in.PressureSL + 1),0.28571);
  if (MachU > 0.0) {
    vcas = sqrt(7 * in.PressureSL / in.DensitySL * (A-1));
    veas = sqrt(2 * qbar / in.DensitySL);
    vtrue = 1116.43559 * MachU * sqrt(in.Temperature / 518.67);
  } else {
    vcas = veas = vtrue = 0.0;
  }

  vPilotAccel.InitMatrix();
  vNcg = in.vBodyAccel/in.SLGravity;
  if ( Vt > 1.0 ) {
    // Nz is Acceleration in "g's", along normal axis (-Z body axis)
    Nz = -vNcg(eZ);
    vPilotAccel = in.vBodyAccel + in.vPQRdot * in.ToEyePt;
    vPilotAccel += in.vPQR * (in.vPQR * in.ToEyePt);
  } else {
    // The line below handles low velocity (and on-ground) cases, basically
    // representing the opposite of the force that the landing gear would
    // exert on the ground (which is just the total weight). This eliminates
    // any jitter that could be introduced by the landing gear. Theoretically,
    // this branch could be eliminated, with a penalty of having a short
    // transient at startup (lasting only a fraction of a second).
    vPilotAccel = in.Tl2b * FGColumnVector3( 0.0, 0.0, -in.SLGravity );
    Nz = -vPilotAccel(eZ) / in.SLGravity;
  }

  vNwcg = mTb2w * vNcg;
  vNwcg(eZ) = 1.0 - vNwcg(eZ);

  vPilotAccelN = vPilotAccel / in.SLGravity;

  // VRP computation
  vLocationVRP = in.vLocation.LocalToLocation( in.Tb2l * in.VRPBody );

  // Recompute some derived values now that we know the dependent parameters values ...
  hoverbcg = in.DistanceAGL / in.Wingspan;

  FGColumnVector3 vMac = in.Tb2l * in.RPBody;
  hoverbmac = (in.DistanceAGL + vMac(3)) / in.Wingspan;

  // When all models are executed calculate the distance from the initial point.
  CalculateRelativePosition();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// From Stevens and Lewis, "Aircraft Control and Simulation", 3rd Ed., the
// transformation from body to wind axes is defined (where "a" is alpha and "B"
// is beta):
//
//   cos(a)*cos(B)     sin(B)    sin(a)*cos(B)
//  -cos(a)*sin(B)     cos(B)   -sin(a)*sin(B)
//  -sin(a)              0       cos(a)
//
// The transform from wind to body axes is then,
//
//   cos(a)*cos(B)  -cos(a)*sin(B)  -sin(a)
//          sin(B)          cos(B)     0
//   sin(a)*cos(B)  -sin(a)*sin(B)   cos(a)

void FGAuxiliary::UpdateWindMatrices(void)
{
  double ca, cb, sa, sb;

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTw2b(1,1) =  ca*cb;
  mTw2b(1,2) = -ca*sb;
  mTw2b(1,3) = -sa;
  mTw2b(2,1) =  sb;
  mTw2b(2,2) =  cb;
  mTw2b(2,3) =  0.0;
  mTw2b(3,1) =  sa*cb;
  mTw2b(3,2) = -sa*sb;
  mTw2b(3,3) =  ca;

  mTb2w = mTw2b.Transposed();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// A positive headwind is blowing with you, a negative headwind is blowing against you.
// psi is the direction the wind is blowing *towards*.
// ToDo: should this simply be in the atmosphere class? Same with Get Crosswind.

double FGAuxiliary::GetHeadWind(void) const
{
  return in.Vwind * cos(in.WindPsi - in.Psi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// A positive crosswind is blowing towards the right (from teh perspective of the
// pilot). A negative crosswind is blowing towards the -Y direction (left).
// psi is the direction the wind is blowing *towards*.

double FGAuxiliary::GetCrossWind(void) const
{
  return in.Vwind * sin(in.WindPsi - in.Psi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GethVRP(void) const
{
  return vLocationVRP.GetRadius() - in.ReferenceRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetNlf(void) const
{
  if (in.Mass != 0)
    return (-in.vFw(3))/(in.Mass*slugtolb);
  else
    return 0.;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::CalculateRelativePosition(void)  //ToDo: This belongs elsewhere - perhaps in FGPropagate or Exec
{
  const double earth_radius_mt = in.ReferenceRadius*fttom;
  lat_relative_position=(in.Latitude  - FDMExec->GetIC()->GetLatitudeDegIC() *degtorad)*earth_radius_mt;
  lon_relative_position=(in.Longitude - FDMExec->GetIC()->GetLongitudeDegIC()*degtorad)*earth_radius_mt;
  relative_position = sqrt(lat_relative_position*lat_relative_position + lon_relative_position*lon_relative_position);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::bind(void)
{
  typedef double (FGAuxiliary::*PMF)(int) const;
  typedef double (FGAuxiliary::*PF)(void) const;
  PropertyManager->Tie("propulsion/tat-r", this, &FGAuxiliary::GetTotalTemperature);
  PropertyManager->Tie("propulsion/tat-c", this, &FGAuxiliary::GetTAT_C);
  PropertyManager->Tie("propulsion/pt-lbs_sqft", this, &FGAuxiliary::GetTotalPressure);
  PropertyManager->Tie("velocities/vc-fps", this, &FGAuxiliary::GetVcalibratedFPS);
  PropertyManager->Tie("velocities/vc-kts", this, &FGAuxiliary::GetVcalibratedKTS);
  PropertyManager->Tie("velocities/ve-fps", this, &FGAuxiliary::GetVequivalentFPS);
  PropertyManager->Tie("velocities/ve-kts", this, &FGAuxiliary::GetVequivalentKTS);
  PropertyManager->Tie("velocities/vtrue-fps", this, &FGAuxiliary::GetVtrueFPS);
  PropertyManager->Tie("velocities/vtrue-kts", this, &FGAuxiliary::GetVtrueKTS);
  PropertyManager->Tie("velocities/machU", this, &FGAuxiliary::GetMachU);
  PropertyManager->Tie("velocities/p-aero-rad_sec", this, eX, (PMF)&FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/q-aero-rad_sec", this, eY, (PMF)&FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/r-aero-rad_sec", this, eZ, (PMF)&FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/phidot-rad_sec", this, ePhi, (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/thetadot-rad_sec", this, eTht, (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/psidot-rad_sec", this, ePsi, (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/u-aero-fps", this, eU, (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/v-aero-fps", this, eV, (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/w-aero-fps", this, eW, (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/vt-fps", this, &FGAuxiliary::GetVt);
  PropertyManager->Tie("velocities/mach", this, &FGAuxiliary::GetMach);
  PropertyManager->Tie("velocities/vg-fps", this, &FGAuxiliary::GetVground);
  PropertyManager->Tie("accelerations/a-pilot-x-ft_sec2", this, eX, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-y-ft_sec2", this, eY, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-z-ft_sec2", this, eZ, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/n-pilot-x-norm", this, eX, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-y-norm", this, eY, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-z-norm", this, eZ, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/Nz", this, &FGAuxiliary::GetNz);
  PropertyManager->Tie("forces/load-factor", this, &FGAuxiliary::GetNlf);
  /* PropertyManager->Tie("atmosphere/headwind-fps", this, &FGAuxiliary::GetHeadWind, true);
  PropertyManager->Tie("atmosphere/crosswind-fps", this, &FGAuxiliary::GetCrossWind, true); */
  PropertyManager->Tie("aero/alpha-rad", this, (PF)&FGAuxiliary::Getalpha);
  PropertyManager->Tie("aero/beta-rad", this, (PF)&FGAuxiliary::Getbeta);
  PropertyManager->Tie("aero/mag-beta-rad", this, (PF)&FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/alpha-deg", this, inDegrees, (PMF)&FGAuxiliary::Getalpha);
  PropertyManager->Tie("aero/beta-deg", this, inDegrees, (PMF)&FGAuxiliary::Getbeta);
  PropertyManager->Tie("aero/mag-beta-deg", this, inDegrees, (PMF)&FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/Re", this, &FGAuxiliary::GetReynoldsNumber);
  PropertyManager->Tie("aero/qbar-psf", this, &FGAuxiliary::Getqbar);
  PropertyManager->Tie("aero/qbarUW-psf", this, &FGAuxiliary::GetqbarUW);
  PropertyManager->Tie("aero/qbarUV-psf", this, &FGAuxiliary::GetqbarUV);
  PropertyManager->Tie("aero/alphadot-rad_sec", this, (PF)&FGAuxiliary::Getadot);
  PropertyManager->Tie("aero/betadot-rad_sec", this, (PF)&FGAuxiliary::Getbdot);
  PropertyManager->Tie("aero/alphadot-deg_sec", this, inDegrees, (PMF)&FGAuxiliary::Getadot);
  PropertyManager->Tie("aero/betadot-deg_sec", this, inDegrees, (PMF)&FGAuxiliary::Getbdot);
  PropertyManager->Tie("aero/h_b-cg-ft", this, &FGAuxiliary::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this, &FGAuxiliary::GetHOverBMAC);
  PropertyManager->Tie("flight-path/gamma-rad", this, &FGAuxiliary::GetGamma);
  PropertyManager->Tie("flight-path/psi-gt-rad", this, &FGAuxiliary::GetGroundTrack);

  PropertyManager->Tie("position/distance-from-start-lon-mt", this, &FGAuxiliary::GetLongitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-lat-mt", this, &FGAuxiliary::GetLatitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-mag-mt", this, &FGAuxiliary::GetDistanceRelativePosition);
  PropertyManager->Tie("position/vrp-gc-latitude_deg", &vLocationVRP, &FGLocation::GetLatitudeDeg);
  PropertyManager->Tie("position/vrp-longitude_deg", &vLocationVRP, &FGLocation::GetLongitudeDeg);
  PropertyManager->Tie("position/vrp-radius-ft", &vLocationVRP, &FGLocation::GetRadius);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::BadUnits(void) const
{
  cerr << "Bad units" << endl; return 0.0;
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

void FGAuxiliary::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAuxiliary" << endl;
    if (from == 1) cout << "Destroyed:    FGAuxiliary" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
    if (Mach > 100 || Mach < 0.00)
      cout << "FGPropagate::Mach is out of bounds: " << Mach << endl;
    if (qbar > 1e6 || qbar < 0.00)
      cout << "FGPropagate::qbar is out of bounds: " << qbar << endl;
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
