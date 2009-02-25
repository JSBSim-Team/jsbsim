/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAuxiliary.cpp
 Author:       Tony Peden, Jon Berndt
 Date started: 01/26/99
 Purpose:      Calculates additional parameters needed by the visual system, etc.
 Called by:    FGSimExec

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

#include "FGAuxiliary.h"
#include "FGAerodynamics.h"
#include "FGPropagate.h"
#include "FGAtmosphere.h"
#include <FGFDMExec.h>
#include "FGAircraft.h"
#include "FGInertial.h"
#include "FGExternalReactions.h"
#include "FGBuoyantForces.h"
#include "FGGroundReactions.h"
#include "FGPropulsion.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.33 2009/02/25 12:16:42 jberndt Exp $";
static const char *IdHdr = ID_AUXILIARY;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAuxiliary";
  vcas = veas = pt = tat = 0;
  psl = rhosl = 1;
  qbar = 0;
  qbarUW = 0.0;
  qbarUV = 0.0;
  Re = 0.0;
  Mach = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  gamma = Vt = Vground = 0.0;
  psigt = 0.0;
  day_of_year = 1;
  seconds_in_day = 0.0;
  hoverbmac = hoverbcg = 0.0;
  tatc = RankineToCelsius(tat);

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vToEyePt.InitMatrix();
  vAeroPQR.InitMatrix();
  vEulerRates.InitMatrix();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  vcas = veas = pt = tat = 0;
  psl = rhosl = 1;
  qbar = 0;
  qbarUW = 0.0;
  qbarUV = 0.0;
  Mach = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  gamma = Vt = Vground = 0.0;
  psigt = 0.0;
  day_of_year = 1;
  seconds_in_day = 0.0;
  hoverbmac = hoverbcg = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vToEyePt.InitMatrix();
  vAeroPQR.InitMatrix();
  vEulerRates.InitMatrix();

  return true;
}
  
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAuxiliary::~FGAuxiliary()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::Run()
{
  double A,B,D;

  if (FGModel::Run()) return true; // return true if error returned from base class
  if (FDMExec->Holding()) return false;

  const FGColumnVector3& vPQR = Propagate->GetPQR();
  const FGColumnVector3& vUVW = Propagate->GetUVW();
  const FGColumnVector3& vUVWdot = Propagate->GetUVWdot();
  const FGColumnVector3& vVel = Propagate->GetVel();

  p = Atmosphere->GetPressure();
  rhosl = Atmosphere->GetDensitySL();
  psl = Atmosphere->GetPressureSL();
  sat = Atmosphere->GetTemperature();

// Rotation

  double cTht = Propagate->GetCosEuler(eTht);
  double sTht = Propagate->GetSinEuler(eTht);
  double cPhi = Propagate->GetCosEuler(ePhi);
  double sPhi = Propagate->GetSinEuler(ePhi);

  vEulerRates(eTht) = vPQR(eQ)*cPhi - vPQR(eR)*sPhi;
  if (cTht != 0.0) {
    vEulerRates(ePsi) = (vPQR(eQ)*sPhi + vPQR(eR)*cPhi)/cTht;
    vEulerRates(ePhi) = vPQR(eP) + vEulerRates(ePsi)*sTht;
  }

// 12/16/2005, JSB: For ground handling purposes, at this time, let's ramp
// in the effects of wind from 10 fps to 30 fps when there is weight on the
// landing gear wheels.

  if (GroundReactions->GetWOW() && vUVW(eU) < 10) {
    vAeroPQR = vPQR;
    vAeroUVW = vUVW;
  } else if (GroundReactions->GetWOW() && vUVW(eU) < 30) {
    double factor = (vUVW(eU) - 10.0)/20.0;
    vAeroPQR = vPQR - factor*Atmosphere->GetTurbPQR();
    vAeroUVW = vUVW - factor*Propagate->GetTl2b()*Atmosphere->GetTotalWindNED();
  } else {
    FGColumnVector3 wind = Propagate->GetTl2b()*Atmosphere->GetTotalWindNED();
    vAeroPQR = vPQR - Atmosphere->GetTurbPQR();
    vAeroUVW = vUVW - wind;
  }

  Vt = vAeroUVW.Magnitude();
  if ( Vt > 0.05) {
    if (vAeroUVW(eW) != 0.0)
      alpha = vAeroUVW(eU)*vAeroUVW(eU) > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
    if (vAeroUVW(eV) != 0.0)
      beta = vAeroUVW(eU)*vAeroUVW(eU)+vAeroUVW(eW)*vAeroUVW(eW) > 0.0 ? atan2(vAeroUVW(eV),
             sqrt(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW))) : 0.0;

    double mUW = (vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW));
    double signU=1;
    if (vAeroUVW(eU) != 0.0)
      signU = vAeroUVW(eU)/fabs(vAeroUVW(eU));

    if ( (mUW == 0.0) || (Vt == 0.0) ) {
      adot = 0.0;
      bdot = 0.0;
    } else {
      adot = (vAeroUVW(eU)*vUVWdot(eW) - vAeroUVW(eW)*vUVWdot(eU))/mUW;
      bdot = (signU*mUW*vUVWdot(eV) - vAeroUVW(eV)*(vAeroUVW(eU)*vUVWdot(eU)
              + vAeroUVW(eW)*vUVWdot(eW)))/(Vt*Vt*sqrt(mUW));
    }
  } else {
    alpha = beta = adot = bdot = 0;
  }

  Re = Vt * Aircraft->Getcbar() / Atmosphere->GetKinematicViscosity();

  qbar = 0.5*Atmosphere->GetDensity()*Vt*Vt;
  qbarUW = 0.5*Atmosphere->GetDensity()*(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW));
  qbarUV = 0.5*Atmosphere->GetDensity()*(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eV)*vAeroUVW(eV));
  Mach = Vt / Atmosphere->GetSoundSpeed();
  MachU = vMachUVW(eU) = vAeroUVW(eU) / Atmosphere->GetSoundSpeed();
  vMachUVW(eV) = vAeroUVW(eV) / Atmosphere->GetSoundSpeed();
  vMachUVW(eW) = vAeroUVW(eW) / Atmosphere->GetSoundSpeed();

// Position

  Vground = sqrt( vVel(eNorth)*vVel(eNorth) + vVel(eEast)*vVel(eEast) );

  psigt = atan2(vVel(eEast), vVel(eNorth));
  if (psigt < 0.0) psigt += 2*M_PI;
  gamma = atan2(-vVel(eDown), Vground);

  tat = sat*(1 + 0.2*Mach*Mach); // Total Temperature, isentropic flow
  tatc = RankineToCelsius(tat);

  if (MachU < 1) {   // Calculate total pressure assuming isentropic flow
    pt = p*pow((1 + 0.2*MachU*MachU),3.5);
  } else {
    // Use Rayleigh pitot tube formula for normal shock in front of pitot tube
    B = 5.76*MachU*MachU/(5.6*MachU*MachU - 0.8);
    D = (2.8*MachU*MachU-0.4)*0.4167;
    pt = p*pow(B,3.5)*D;
  }

  A = pow(((pt-p)/psl+1),0.28571);
  if (MachU > 0.0) {
    vcas = sqrt(7*psl/rhosl*(A-1));
    veas = sqrt(2*qbar/rhosl);
  } else {
    vcas = veas = 0.0;
  }

  vPilotAccel.InitMatrix();
  if ( Vt > 1.0 ) {
     vAircraftAccel = Aerodynamics->GetForces()
                    + Propulsion->GetForces()
                    + GroundReactions->GetForces()
                    + ExternalReactions->GetForces()
                    + BuoyantForces->GetForces();

     vAircraftAccel /= MassBalance->GetMass();
     // Nz is Acceleration in "g's", along normal axis (-Z body axis)
     Nz = -vAircraftAccel(eZ)/Inertial->gravity();
     vToEyePt = MassBalance->StructuralToBody(Aircraft->GetXYZep());
     vPilotAccel = vAircraftAccel + Propagate->GetPQRdot() * vToEyePt;
     vPilotAccel += vPQR * (vPQR * vToEyePt);
  } else {
     // The line below handles low velocity (and on-ground) cases, basically
     // representing the opposite of the force that the landing gear would
     // exert on the ground (which is just the total weight). This eliminates
     // any jitter that could be introduced by the landing gear. Theoretically,
     // this branch could be eliminated, with a penalty of having a short
     // transient at startup (lasting only a fraction of a second).
     vPilotAccel = Propagate->GetTl2b() * FGColumnVector3( 0.0, 0.0, -Inertial->gravity() );
     Nz = -vPilotAccel(eZ)/Inertial->gravity();
  }

  vPilotAccelN = vPilotAccel/Inertial->gravity();

  // VRP computation
  const FGLocation& vLocation = Propagate->GetLocation();
  FGColumnVector3 vrpStructural = Aircraft->GetXYZvrp();
  FGColumnVector3 vrpBody = MassBalance->StructuralToBody( vrpStructural );
  FGColumnVector3 vrpLocal = Propagate->GetTb2l() * vrpBody;
  vLocationVRP = vLocation.LocalToLocation( vrpLocal );

  // Recompute some derived values now that we know the dependent parameters values ...
  hoverbcg = Propagate->GetDistanceAGL() / Aircraft->GetWingSpan();

  FGColumnVector3 vMac = Propagate->GetTb2l()*MassBalance->StructuralToBody(Aircraft->GetXYZrp());
  hoverbmac = (Propagate->GetDistanceAGL() + vMac(3)) / Aircraft->GetWingSpan();

  // when all model are executed, 
  // please calculate the distance from the initial point

  CalculateRelativePosition();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// A positive headwind is blowing with you, a negative headwind is blowing against you.
// psi is the direction the wind is blowing *towards*.

double FGAuxiliary::GetHeadWind(void) const
{
  double psiw,vw;

  psiw = Atmosphere->GetWindPsi();
  vw = Atmosphere->GetTotalWindNED().Magnitude();

  return vw*cos(psiw - Propagate->GetEuler(ePsi));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// A positive crosswind is blowing towards the right (from teh perspective of the
// pilot). A negative crosswind is blowing towards the -Y direction (left).
// psi is the direction the wind is blowing *towards*.

double FGAuxiliary::GetCrossWind(void) const
{
  double psiw,vw;

  psiw = Atmosphere->GetWindPsi();
  vw = Atmosphere->GetTotalWindNED().Magnitude();

  return  vw*sin(psiw - Propagate->GetEuler(ePsi));
}

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
  PropertyManager->Tie("velocities/vt-fps", this, &FGAuxiliary::GetVt, &FGAuxiliary::SetVt, true);
  PropertyManager->Tie("velocities/mach", this, &FGAuxiliary::GetMach, &FGAuxiliary::SetMach, true);
  PropertyManager->Tie("velocities/vg-fps", this, &FGAuxiliary::GetVground);
  PropertyManager->Tie("accelerations/a-pilot-x-ft_sec2", this, eX, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-y-ft_sec2", this, eY, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-z-ft_sec2", this, eZ, (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/n-pilot-x-norm", this, eX, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-y-norm", this, eY, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-z-norm", this, eZ, (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/Nz", this, &FGAuxiliary::GetNz);
  /* PropertyManager->Tie("atmosphere/headwind-fps", this, &FGAuxiliary::GetHeadWind, true);
  PropertyManager->Tie("atmosphere/crosswind-fps", this, &FGAuxiliary::GetCrossWind, true); */
  PropertyManager->Tie("aero/alpha-rad", this, (PF)&FGAuxiliary::Getalpha, &FGAuxiliary::Setalpha, true);
  PropertyManager->Tie("aero/beta-rad", this, (PF)&FGAuxiliary::Getbeta, &FGAuxiliary::Setbeta, true);
  PropertyManager->Tie("aero/mag-beta-rad", this, (PF)&FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/alpha-deg", this, inDegrees, (PMF)&FGAuxiliary::Getalpha);
  PropertyManager->Tie("aero/beta-deg", this, inDegrees, (PMF)&FGAuxiliary::Getbeta);
  PropertyManager->Tie("aero/mag-beta-deg", this, inDegrees, (PMF)&FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/Re", this, &FGAuxiliary::GetReynoldsNumber);
  PropertyManager->Tie("aero/qbar-psf", this, &FGAuxiliary::Getqbar, &FGAuxiliary::Setqbar, true);
  PropertyManager->Tie("aero/qbarUW-psf", this, &FGAuxiliary::GetqbarUW, &FGAuxiliary::SetqbarUW, true);
  PropertyManager->Tie("aero/qbarUV-psf", this, &FGAuxiliary::GetqbarUV, &FGAuxiliary::SetqbarUV, true);
  PropertyManager->Tie("aero/alphadot-rad_sec", this, (PF)&FGAuxiliary::Getadot, &FGAuxiliary::Setadot, true);
  PropertyManager->Tie("aero/betadot-rad_sec", this, (PF)&FGAuxiliary::Getbdot, &FGAuxiliary::Setbdot, true);
  PropertyManager->Tie("aero/alphadot-deg_sec", this, inDegrees, (PMF)&FGAuxiliary::Getadot);
  PropertyManager->Tie("aero/betadot-deg_sec", this, inDegrees, (PMF)&FGAuxiliary::Getbdot);
  PropertyManager->Tie("aero/h_b-cg-ft", this, &FGAuxiliary::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this, &FGAuxiliary::GetHOverBMAC);
  PropertyManager->Tie("flight-path/gamma-rad", this, &FGAuxiliary::GetGamma, &FGAuxiliary::SetGamma);
  PropertyManager->Tie("flight-path/psi-gt-rad", this, &FGAuxiliary::GetGroundTrack);

  PropertyManager->Tie("position/distance-from-start-lon-mt", this, &FGAuxiliary::GetLongitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-lat-mt", this, &FGAuxiliary::GetLatitudeRelativePosition);
  PropertyManager->Tie("position/distance-from-start-mag-mt", this, &FGAuxiliary::GetDistanceRelativePosition);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::CalculateRelativePosition(void)
{ 
  const double earth_radius_mt = Inertial->GetRefRadius()*fttom;
  lat_relative_position=(FDMExec->GetPropagate()->GetLatitude()  - FDMExec->GetIC()->GetLatitudeDegIC() *degtorad)*earth_radius_mt;
  lon_relative_position=(FDMExec->GetPropagate()->GetLongitude() - FDMExec->GetIC()->GetLongitudeDegIC()*degtorad)*earth_radius_mt;
  relative_position = sqrt(lat_relative_position*lat_relative_position + lon_relative_position*lon_relative_position);
};

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
