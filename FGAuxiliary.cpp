/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAuxiliary.cpp
 Author:       Tony Peden, Jon Berndt
 Date started: 01/26/99
 Purpose:      Calculates additional parameters needed by the visual system, etc.
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAircraft.h"
#include "FGInertial.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.56 2004/05/14 10:40:15 jberndt Exp $";
static const char *IdHdr = ID_AUXILIARY;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAuxiliary";
  vcas = veas = pt = tat = 0;
  psl = rhosl = 1;
  earthPosAngle = 0.0;
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
  LongitudeVRP = LatitudeVRP = 0.0;
  vVRPoffset.InitMatrix();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAuxiliary::~FGAuxiliary()
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::Run()
{
  double A,B,D, hdot_Vt;
  const FGColumnVector3& vPQR = Propagate->GetPQR();
  const FGColumnVector3& vUVW = Propagate->GetUVW();
  const FGColumnVector3& vUVWdot = Propagate->GetUVWdot();
  const FGColumnVector3& vVel = Propagate->GetVel();

  if (!FGModel::Run())
  {
    p = Atmosphere->GetPressure();
    rhosl = Atmosphere->GetDensitySL();
    psl = Atmosphere->GetPressureSL();
    sat = Atmosphere->GetTemperature();

// Rotation

    double cTht = Propagate->GetCostht();
    double cPhi = Propagate->GetCosphi();
    double sPhi = Propagate->GetSinphi();

    vEulerRates(eTht) = vPQR(eQ)*cPhi - vPQR(eR)*sPhi;
    if (cTht != 0.0) {
      vEulerRates(ePsi) = (vPQR(eQ)*sPhi + vPQR(eR)*cPhi)/cTht;
      vEulerRates(ePhi) = vPQR(eP) + vEulerRates(ePsi)*sPhi;
    }

    vAeroPQR = vPQR + Atmosphere->GetTurbPQR();

// Translation

    vAeroUVW = vUVW + Propagate->GetTl2b()*Atmosphere->GetWindNED();

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

    qbar = 0.5*Atmosphere->GetDensity()*Vt*Vt;
    qbarUW = 0.5*Atmosphere->GetDensity()*(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW));
    qbarUV = 0.5*Atmosphere->GetDensity()*(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eV)*vAeroUVW(eV));
    Mach = Vt / Atmosphere->GetSoundSpeed();
    MachU = vMachUVW(eU) = vAeroUVW(eU) / Atmosphere->GetSoundSpeed();
    vMachUVW(eV) = vAeroUVW(eV) / Atmosphere->GetSoundSpeed();
    vMachUVW(eW) = vAeroUVW(eW) / Atmosphere->GetSoundSpeed();

// Position

    Vground = sqrt( vVel(eNorth)*vVel(eNorth) + vVel(eEast)*vVel(eEast) );

    if (vVel(eNorth) == 0) psigt = 0;
    else psigt =  atan2(vVel(eEast), vVel(eNorth));

    if (psigt < 0.0) psigt += 2*M_PI;

    if (Vt != 0) {
      hdot_Vt = -vVel(eDown)/Vt;
      if (fabs(hdot_Vt) <= 1) gamma = asin(hdot_Vt);
    } else {
      gamma = 0.0;
    }

    tat = sat*(1 + 0.2*Mach*Mach); // Total Temperature, isentropic flow
    tatc = RankineToCelsius(tat);

    if (Mach < 1) {   // Calculate total pressure assuming isentropic flow
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
       vPilotAccel =  Aerodynamics->GetForces()
                      +  Propulsion->GetForces()
                      +  GroundReactions->GetForces();
       vPilotAccel /= MassBalance->GetMass();
       vToEyePt = MassBalance->StructuralToBody(Aircraft->GetXYZep());
       vPilotAccel += Propagate->GetPQRdot() * vToEyePt;
       vPilotAccel += vPQR * (vPQR * vToEyePt);
    } else {
       vPilotAccel = -1*( Propagate->GetTl2b() * Inertial->GetGravity() );
    }

    vPilotAccelN = vPilotAccel/Inertial->gravity();

    earthPosAngle += State->Getdt()*Inertial->omega();

    const FGColumnVector3& vLocation = Propagate->GetLocation();
    vVRPoffset = Propagate->GetTb2l() * MassBalance->StructuralToBody(Aircraft->GetXYZvrp());

    // vVRP  - the vector to the Visual Reference Point - now contains the
    // offset from the CG to the VRP, in units of feet, in the Local coordinate
    // frame, where X points north, Y points East, and Z points down. This needs
    // to be converted to Lat/Lon/Alt, now.

    if (cos(vLocation(eLat)) != 0)
      vLocationVRP(eLong) = vVRPoffset(eEast) / (vLocation(eRad) * cos(vLocation(eLat))) + vLocation(eLong);

    vLocationVRP(eLat) = vVRPoffset(eNorth) / vLocation(eRad) + vLocation(eLat);
    vLocationVRP(eRad) = Propagate->Geth() - vVRPoffset(eDown); // this is really a height, not a radius

    // Recompute some derived values now that we know the dependent parameters values ...
    hoverbcg = Propagate->GetDistanceAGL() / Aircraft->GetWingSpan();

    FGColumnVector3 vMac = Propagate->GetTb2l()*MassBalance->StructuralToBody(Aircraft->GetXYZrp());
    hoverbmac = (Propagate->GetDistanceAGL() + vMac(3)) / Aircraft->GetWingSpan();

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetHeadWind(void)
{
  double psiw,vw;

  psiw = Atmosphere->GetWindPsi();
  vw = Atmosphere->GetWindNED().Magnitude();

  return vw*cos(psiw - Propagate->Getpsi());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetCrossWind(void)
{
  double psiw,vw;

  psiw = Atmosphere->GetWindPsi();
  vw = Atmosphere->GetWindNED().Magnitude();

  return  vw*sin(psiw - Propagate->Getpsi());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::bind(void)
{
  typedef double (FGAuxiliary::*PMF)(int) const;
  PropertyManager->Tie("velocities/vc-fps", this,
                       &FGAuxiliary::GetVcalibratedFPS);
  PropertyManager->Tie("velocities/vc-kts", this,
                       &FGAuxiliary::GetVcalibratedKTS);
  PropertyManager->Tie("velocities/ve-fps", this,
                       &FGAuxiliary::GetVequivalentFPS);
  PropertyManager->Tie("velocities/ve-kts", this,
                       &FGAuxiliary::GetVequivalentKTS);
  PropertyManager->Tie("velocities/machU", this,
                       &FGAuxiliary::GetMachU);
  PropertyManager->Tie("velocities/tat-r", this,
                       &FGAuxiliary::GetTotalTemperature);
  PropertyManager->Tie("velocities/tat-c", this,
                       &FGAuxiliary::GetTAT_C);
  PropertyManager->Tie("velocities/pt-lbs_sqft", this,
                       &FGAuxiliary::GetTotalPressure);
  PropertyManager->Tie("velocities/p-aero-rad_sec", this,1,
                       (PMF)&FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/q-aero-rad_sec", this,2,
                       (PMF)&FGAuxiliary::GetAeroPQR);
  PropertyManager->Tie("velocities/r-aero-rad_sec", this,3,
                       (PMF)&FGAuxiliary::GetAeroPQR);

  PropertyManager->Tie("accelerations/a-pilot-x-ft_sec2", this,1,
                       (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-y-ft_sec2", this,2,
                       (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/a-pilot-z-ft_sec2", this,3,
                       (PMF)&FGAuxiliary::GetPilotAccel);
  PropertyManager->Tie("accelerations/n-pilot-x-norm", this,1,
                       (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-y-norm", this,2,
                       (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("accelerations/n-pilot-z-norm", this,3,
                       (PMF)&FGAuxiliary::GetNpilot);
  PropertyManager->Tie("position/epa-rad", this,
                       &FGAuxiliary::GetEarthPositionAngle);
  PropertyManager->Tie("velocities/phidot-rad_sec", this,1,
                       (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/thetadot-rad_sec", this,2,
                       (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/psidot-rad_sec", this,3,
                       (PMF)&FGAuxiliary::GetEulerRates);

  /* PropertyManager->Tie("atmosphere/headwind-fps", this,
                       &FGAuxiliary::GetHeadWind,
                       true);
  PropertyManager->Tie("atmosphere/crosswind-fps", this,
                       &FGAuxiliary::GetCrossWind,
                       true); */
  PropertyManager->Tie("velocities/u-aero-fps", this,1,
                      (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/v-aero-fps", this,2,
                      (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("velocities/w-aero-fps", this,3,
                      (PMF)&FGAuxiliary::GetAeroUVW);
  PropertyManager->Tie("aero/alpha-rad", this,
                      &FGAuxiliary::Getalpha,
                      &FGAuxiliary::Setalpha,
                      true);
  PropertyManager->Tie("aero/beta-rad", this,
                      &FGAuxiliary::Getbeta,
                      &FGAuxiliary::Setbeta,
                      true);
  PropertyManager->Tie("aero/mag-beta-rad", this,
                      &FGAuxiliary::GetMagBeta);
  PropertyManager->Tie("aero/qbar-psf", this,
                      &FGAuxiliary::Getqbar,
                      &FGAuxiliary::Setqbar,
                      true);
  PropertyManager->Tie("aero/qbarUW-psf", this,
                      &FGAuxiliary::GetqbarUW,
                      &FGAuxiliary::SetqbarUW,
                      true);
  PropertyManager->Tie("aero/qbarUV-psf", this,
                      &FGAuxiliary::GetqbarUV,
                      &FGAuxiliary::SetqbarUV,
                      true);
  PropertyManager->Tie("velocities/vt-fps", this,
                      &FGAuxiliary::GetVt,
                      &FGAuxiliary::SetVt,
                      true);
  PropertyManager->Tie("velocities/mach-norm", this,
                      &FGAuxiliary::GetMach,
                      &FGAuxiliary::SetMach,
                      true);
  PropertyManager->Tie("aero/alphadot-rad_sec", this,
                      &FGAuxiliary::Getadot,
                      &FGAuxiliary::Setadot,
                      true);
  PropertyManager->Tie("aero/betadot-rad_sec", this,
                      &FGAuxiliary::Getbdot,
                      &FGAuxiliary::Setbdot,
                      true);
  PropertyManager->Tie("flight-path/gamma-rad", this,
                      &FGAuxiliary::GetGamma,
                      &FGAuxiliary::SetGamma);
  PropertyManager->Tie("velocities/vg-fps", this,
                      &FGAuxiliary::GetVground);
  PropertyManager->Tie("flight-path/psi-gt-rad", this,
                      &FGAuxiliary::GetGroundTrack);
  PropertyManager->Tie("aero/h_b-cg-ft", this, &FGAuxiliary::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this, &FGAuxiliary::GetHOverBMAC);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::unbind(void)
{
  PropertyManager->Untie("velocities/vc-fps");
  PropertyManager->Untie("velocities/vc-kts");
  PropertyManager->Untie("velocities/ve-fps");
  PropertyManager->Untie("velocities/ve-kts");
  PropertyManager->Untie("velocities/machU");
  PropertyManager->Untie("velocities/tat-r");
  PropertyManager->Untie("velocities/tat-c");
  PropertyManager->Untie("velocities/p-aero-rad_sec");
  PropertyManager->Untie("velocities/q-aero-rad_sec");
  PropertyManager->Untie("velocities/r-aero-rad_sec");
  PropertyManager->Untie("velocities/pt-lbs_sqft");
  PropertyManager->Untie("accelerations/a-pilot-x-ft_sec2");
  PropertyManager->Untie("accelerations/a-pilot-y-ft_sec2");
  PropertyManager->Untie("accelerations/a-pilot-z-ft_sec2");
  PropertyManager->Untie("accelerations/n-pilot-x-norm");
  PropertyManager->Untie("accelerations/n-pilot-y-norm");
  PropertyManager->Untie("accelerations/n-pilot-z-norm");
  PropertyManager->Untie("position/epa-rad");
  PropertyManager->Untie("velocities/phidot-rad_sec");
  PropertyManager->Untie("velocities/thetadot-rad_sec");
  PropertyManager->Untie("velocities/psidot-rad_sec");
  /* PropertyManager->Untie("atmosphere/headwind-fps");
  PropertyManager->Untie("atmosphere/crosswind-fps"); */
  PropertyManager->Untie("velocities/u-aero-fps");
  PropertyManager->Untie("velocities/v-aero-fps");
  PropertyManager->Untie("velocities/w-aero-fps");
  PropertyManager->Untie("aero/alpha-rad");
  PropertyManager->Untie("aero/beta-rad");
  PropertyManager->Untie("aero/qbar-psf");
  PropertyManager->Untie("aero/qbarUW-psf");
  PropertyManager->Untie("aero/qbarUV-psf");
  PropertyManager->Untie("velocities/vt-fps");
  PropertyManager->Untie("velocities/mach-norm");
  PropertyManager->Untie("aero/alphadot-rad_sec");
  PropertyManager->Untie("aero/betadot-rad_sec");
  PropertyManager->Untie("aero/mag-beta-rad");
  PropertyManager->Untie("flight-path/gamma-rad");
  PropertyManager->Untie("velocities/vg-fps");
  PropertyManager->Untie("flight-path/psi-gt-rad");
  PropertyManager->Untie("aero/h_b-cg-ft");
  PropertyManager->Untie("aero/h_b-mac-ft");
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
