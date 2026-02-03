/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAuxiliary.cpp
 Author:       Tony Peden, Jon Berndt
 Date started: 01/26/99
 Purpose:      Calculates additional parameters needed by the visual system, etc.
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
#include "initialization/FGInitialCondition.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"
#include "FGInertial.h"
#include "FGAtmosphere.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAuxiliary";
  pt = FGAtmosphere::StdDaySLpressure;     // ISA SL pressure
  tat = FGAtmosphere::StdDaySLtemperature; // ISA SL temperature
  tatc = RankineToCelsius(tat);

  vcas = veas = 0.0;
  qbar = qbarUW = qbarUV = 0.0;
  Mach = MachU = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  gamma = Vt = Vground = 0.0;
  psigt = 0.0;
  hoverbmac = hoverbcg = 0.0;
  Re = 0.0;
  Nx = Ny = Nz = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vAeroUVW.InitMatrix();
  vAeroPQR.InitMatrix();
  vMachUVW.InitMatrix();
  vEulerRates.InitMatrix();
  vNEUFromStart.InitMatrix();
  NEUCalcValid = false;

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

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
  hoverbmac = hoverbcg = 0.0;
  Re = 0.0;
  Nz = Ny = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vAeroUVW.InitMatrix();
  vAeroPQR.InitMatrix();
  vMachUVW.InitMatrix();
  vEulerRates.InitMatrix();
  vNEUFromStart.InitMatrix();
  NEUCalcValid = false;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::SetInitialState(const FGInitialCondition* ic)
{
  NEUStartLocation = ic->GetPosition();
  NEUStartLocation.SetPositionGeodetic(NEUStartLocation.GetLongitude(), NEUStartLocation.GetGeodLatitudeRad(), 0.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAuxiliary::~FGAuxiliary()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::Run(bool Holding)
{
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

  alpha = beta = adot = bdot = 0;
  double AeroU2 = vAeroUVW(eU)*vAeroUVW(eU);
  double AeroV2 = vAeroUVW(eV)*vAeroUVW(eV);
  double AeroW2 = vAeroUVW(eW)*vAeroUVW(eW);
  double mUW = AeroU2 + AeroW2;

  double Vt2 = mUW + AeroV2;
  Vt = sqrt(Vt2);

  if ( Vt > 0.001 ) {
    beta = atan2(vAeroUVW(eV), sqrt(mUW));

    if ( mUW >= 1E-6 ) {
      alpha = atan2(vAeroUVW(eW), vAeroUVW(eU));
      double Vtdot = (vAeroUVW(eU)*in.vUVWdot(eU) + vAeroUVW(eV)*in.vUVWdot(eV) + vAeroUVW(eW)*in.vUVWdot(eW))/Vt;
      adot = (vAeroUVW(eU)*in.vUVWdot(eW) - vAeroUVW(eW)*in.vUVWdot(eU))/mUW;
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

  Vground = sqrt( in.vVel(eNorth)*in.vVel(eNorth) + in.vVel(eEast)*in.vVel(eEast) );

  psigt = atan2(in.vVel(eEast), in.vVel(eNorth));
  if (psigt < 0.0) psigt += 2*M_PI;
  gamma = atan2(-in.vVel(eDown), Vground);

  tat = in.Temperature*(1 + 0.2*Mach*Mach); // Total Temperature, isentropic flow
  tatc = RankineToCelsius(tat);

  pt = PitotTotalPressure(Mach, in.Pressure);

  if (abs(Mach) > 0.0) {
    vcas = VcalibratedFromMach(Mach, in.Pressure);
    veas = sqrt(2 * qbar / FGAtmosphere::StdDaySLdensity);
  }
  else
    vcas = veas = 0.0;

  vPilotAccel.InitMatrix();
  vNcg = in.vBodyAccel/in.StandardGravity;
  // Nz is Acceleration in "g's", along normal axis (-Z body axis)
  Nz = -vNcg(eZ);
  Ny =  vNcg(eY);
  Nx =  vNcg(eX);
  vPilotAccel = in.vBodyAccel + in.vPQRidot * in.ToEyePt;
  vPilotAccel += in.vPQRi * (in.vPQRi * in.ToEyePt);

  vNwcg = mTb2w * vNcg;
  vNwcg(eZ) = 1.0 - vNwcg(eZ);

  vPilotAccelN = vPilotAccel / in.StandardGravity;

  // VRP computation
  vLocationVRP = in.vLocation.LocalToLocation( in.Tb2l * in.VRPBody );

  // Recompute some derived values now that we know the dependent parameters values ...
  hoverbcg = in.DistanceAGL / in.Wingspan;

  FGColumnVector3 vMac = in.Tb2l * in.RPBody;
  hoverbmac = (in.DistanceAGL - vMac(3)) / in.Wingspan;

  // New timestep so vNEUFromStart is no longer valid since we only calculate it on demand
  NEUCalcValid = false;

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::PitotTotalPressure(double mach, double pressure) const
{
  constexpr double SHRatio = FGAtmosphere::SHRatio;
  constexpr double a = (SHRatio-1.0) / 2.0;
  constexpr double b = SHRatio / (SHRatio-1.0);
  constexpr double c = 2.0*b;
  constexpr double d = 1.0 / (SHRatio-1.0);
  const double coeff = pow(0.5*(SHRatio+1.0), b)
                     * pow((SHRatio+1.0)/(SHRatio-1.0), d);

  if (mach < 0) return pressure;
  if (mach < 1)    //calculate total pressure assuming isentropic flow
    return pressure*pow((1.0 + a*mach*mach), b);
  else {
    // Shock in front of pitot tube, we'll assume its normal and use the
    // Rayleigh Pitot Tube Formula, i.e. the ratio of total pressure behind the
    // shock to the static pressure in front of the normal shock assumption
    // should not be a bad one -- most supersonic aircraft place the pitot probe
    // out front so that it is the forward most point on the aircraft.
    // The real shock would, of course, take on something like the shape of a
    // rounded-off cone but, here again, the assumption should be good since the
    // opening of the pitot probe is very small and, therefore, the effects of
    // the shock curvature should be small as well. AFAIK, this approach is
    // fairly well accepted within the aerospace community

    // The denominator below is zero for Mach ~ 0.38, for which
    // we'll never be here, so we're safe

    return pressure*coeff*pow(mach, c)/pow(c*mach*mach-1.0, d);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Based on the formulas in the US Air Force Aircraft Performance Flight Testing
// Manual (AFFTC-TIH-99-01). In particular sections 4.6 to 4.8.

double FGAuxiliary::MachFromImpactPressure(double qc, double pressure) const
{
  constexpr double SHRatio = FGAtmosphere::SHRatio;
  constexpr double a = 2.0/(SHRatio-1.0);
  constexpr double b = (SHRatio-1.0)/SHRatio;
  constexpr double c = 2.0/b;
  constexpr double d = 0.5*a;
  const double coeff = pow(0.5*(SHRatio+1.0), -0.25*c)
                     * pow(0.5*(SHRatio+1.0)/SHRatio, -0.5*d);

  double A = qc / pressure + 1;
  double M = sqrt(a*(pow(A, b) - 1.0));  // Equation (4.12)

  if (M > 1.0)
    for (unsigned int i = 0; i<10; i++)
      M = coeff*sqrt(A*pow(1 - 1.0 / (c*M*M), d));  // Equation (4.17)

  return M;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::VcalibratedFromMach(double mach, double pressure) const
{
  double qc = PitotTotalPressure(mach, pressure) - pressure;
  return in.StdDaySLsoundspeed * MachFromImpactPressure(qc, FGAtmosphere::StdDaySLpressure);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::MachFromVcalibrated(double vcas, double pressure) const
{
  constexpr double StdDaySLpressure = FGAtmosphere::StdDaySLpressure;
  double qc = PitotTotalPressure(vcas / in.StdDaySLsoundspeed, StdDaySLpressure) - StdDaySLpressure;
  return MachFromImpactPressure(qc, pressure);
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

double FGAuxiliary::GetNlf(void) const
{
  if (in.Mass != 0)
    return (in.vFw(3))/(in.Mass*slugtolb);
  else
    return 0.;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetLongitudeRelativePosition(void) const
{
  return in.vLocation.GetDistanceTo(FDMExec->GetIC()->GetLongitudeRadIC(),
                                    in.vLocation.GetGeodLatitudeRad())*fttom;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetLatitudeRelativePosition(void) const
{
  return in.vLocation.GetDistanceTo(in.vLocation.GetLongitude(),
                                    FDMExec->GetIC()->GetGeodLatitudeRadIC())*fttom;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetDistanceRelativePosition(void) const
{
  auto ic = FDMExec->GetIC();
  return in.vLocation.GetDistanceTo(ic->GetLongitudeRadIC(),
                                    ic->GetGeodLatitudeRadIC())*fttom;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGAuxiliary::GetNEUPositionFromStart() const
{ 
  if (!NEUCalcValid) {
    // Position tracking in local frame with local frame origin at lat, lon of initial condition
    // and at 0 altitude relative to the reference ellipsoid. Position is NEU (North, East, UP) in feet.
    vNEUFromStart = NEUStartLocation.LocationToLocal(in.vLocation);
    vNEUFromStart(3) *= -1.0;  // Flip sign for Up, so + for altitude above reference ellipsoid
    NEUCalcValid = true;
  }

  return vNEUFromStart; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::bind(void)
{
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
  PropertyManager->Tie("velocities/p-aero-rad_sec", this, eX, &FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/q-aero-rad_sec", this, eY, &FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/r-aero-rad_sec", this, eZ, &FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/phidot-rad_sec", this, ePhi, &FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/thetadot-rad_sec", this, eTht, &FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/psidot-rad_sec", this, ePsi, &FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/u-aero-fps", this, eU, &FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/v-aero-fps", this, eV, &FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/w-aero-fps", this, eW, &FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/vt-fps", this, &FGAuxiliary::GetVt);
  PropertyManager->Tie("velocities/mach", this, &FGAuxiliary::GetMach);
  PropertyManager->Tie("velocities/vg-fps", this, &FGAuxiliary::GetVground);
  PropertyManager->Tie("accelerations/a-pilot-x-ft_sec2", this, eX, &FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-y-ft_sec2", this, eY, &FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-z-ft_sec2", this, eZ, &FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/n-pilot-x-norm", this, eX, &FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-y-norm", this, eY, &FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-z-norm", this, eZ, &FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/Nx", this, &FGAuxiliary::GetNx);
  PropertyManager->Tie("accelerations/Ny", this, &FGAuxiliary::GetNy);
  PropertyManager->Tie("accelerations/Nz", this, &FGAuxiliary::GetNz);
  PropertyManager->Tie("forces/load-factor", this, &FGAuxiliary::GetNlf);
  PropertyManager->Tie("aero/alpha-rad", this, &FGAuxiliary::Getalpha);
  PropertyManager->Tie("aero/beta-rad", this, &FGAuxiliary::Getbeta);
  PropertyManager->Tie("aero/mag-beta-rad", this, &FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/alpha-deg", this, inDegrees, &FGAuxiliary::Getalpha);
  PropertyManager->Tie("aero/beta-deg", this, inDegrees, &FGAuxiliary::Getbeta);
  PropertyManager->Tie("aero/mag-beta-deg", this, inDegrees, &FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/Re", this, &FGAuxiliary::GetReynoldsNumber);
  PropertyManager->Tie("aero/qbar-psf", this, &FGAuxiliary::Getqbar);
  PropertyManager->Tie("aero/qbarUW-psf", this, &FGAuxiliary::GetqbarUW);
  PropertyManager->Tie("aero/qbarUV-psf", this, &FGAuxiliary::GetqbarUV);
  PropertyManager->Tie("aero/alphadot-rad_sec", this, &FGAuxiliary::Getadot);
  PropertyManager->Tie("aero/betadot-rad_sec", this, &FGAuxiliary::Getbdot);
  PropertyManager->Tie("aero/alphadot-deg_sec", this, inDegrees, &FGAuxiliary::Getadot);
  PropertyManager->Tie("aero/betadot-deg_sec", this, inDegrees, &FGAuxiliary::Getbdot);
  PropertyManager->Tie("aero/h_b-cg-ft", this, &FGAuxiliary::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this, &FGAuxiliary::GetHOverBMAC);
  PropertyManager->Tie("flight-path/gamma-rad", this, &FGAuxiliary::GetGamma);
  PropertyManager->Tie("flight-path/gamma-deg", this, inDegrees, &FGAuxiliary::GetGamma);
  PropertyManager->Tie("flight-path/psi-gt-rad", this, &FGAuxiliary::GetGroundTrack);

  PropertyManager->Tie("position/distance-from-start-lon-mt", this, &FGAuxiliary::GetLongitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-lat-mt", this, &FGAuxiliary::GetLatitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-mag-mt", this, &FGAuxiliary::GetDistanceRelativePosition);
  PropertyManager->Tie("position/vrp-gc-latitude_deg", &vLocationVRP, &FGLocation::GetLatitudeDeg);
  PropertyManager->Tie("position/vrp-longitude_deg", &vLocationVRP, &FGLocation::GetLongitudeDeg);
  PropertyManager->Tie("position/vrp-radius-ft", &vLocationVRP, &FGLocation::GetRadius);

  PropertyManager->Tie("position/from-start-neu-n-ft", this, eX, &FGAuxiliary::GetNEUPositionFromStart);
  PropertyManager->Tie("position/from-start-neu-e-ft", this, eY, &FGAuxiliary::GetNEUPositionFromStart);
  PropertyManager->Tie("position/from-start-neu-u-ft", this, eZ, &FGAuxiliary::GetNEUPositionFromStart);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::BadUnits(void) const
{
  FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
  log << "Bad units" << endl;
  return 0.0;
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
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGAuxiliary" << endl;
    if (from == 1) log << "Destroyed:    FGAuxiliary" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (Mach > 100 || Mach < 0.00)
      log << "FGPropagate::Mach is out of bounds: " << Mach << endl;
    if (qbar > 1e6 || qbar < 0.00)
      log << "FGPropagate::qbar is out of bounds: " << qbar << endl;
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

} // namespace JSBSim
