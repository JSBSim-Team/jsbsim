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
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAircraft.h"
#include "FGInertial.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.45 2004/03/23 12:04:15 jberndt Exp $";
static const char *IdHdr = ID_AUXILIARY;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAuxiliary";
  vcas = veas = Mach = qbar = pt = tat = 0;
  psl = rhosl = 1;
  earthPosAngle = 0.0;

  vPilotAccel.InitMatrix();
  vPilotAccelN.InitMatrix();
  vToEyePt.InitMatrix();
  vAeroPQR.InitMatrix();
  vEuler.InitMatrix();
  vEulerRates.InitMatrix();

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
  double A,B,D;
  FGColumnVector3& vPQR = Rotation->GetPQR();

  if (!FGModel::Run())
  {
    qbar = Translation->Getqbar();
    Mach = Translation->GetMach();
    MachU= Translation->GetMachU();
    p = Atmosphere->GetPressure();
    rhosl = Atmosphere->GetDensitySL();
    psl = Atmosphere->GetPressureSL();
    sat = Atmosphere->GetTemperature();

    vAeroPQR = vPQR + Atmosphere->GetTurbPQR();
    vEuler = State->CalcEuler();

    double cTht = cos(vEuler(eTht));
    double cPhi = cos(vEuler(ePhi)),   sPhi = sin(vEuler(ePhi));

    vEulerRates(eTht) = vPQR(eQ)*cPhi - vPQR(eR)*sPhi;
    if (cTht != 0.0) {
      vEulerRates(ePsi) = (vPQR(eQ)*sPhi + vPQR(eR)*cPhi)/cTht;
      vEulerRates(ePhi) = vPQR(eP) + vEulerRates(ePsi)*sPhi;
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
    if ( Translation->GetVt() > 1 ) {
       vPilotAccel =  Aerodynamics->GetForces()
                      +  Propulsion->GetForces()
                      +  GroundReactions->GetForces();
       vPilotAccel /= MassBalance->GetMass();
       vToEyePt = MassBalance->StructuralToBody(Aircraft->GetXYZep());
       vPilotAccel += Rotation->GetPQRdot() * vToEyePt;
       vPilotAccel += vPQR * (vPQR * vToEyePt);
    } else {
       vPilotAccel = -1*( State->GetTl2b() * Inertial->GetGravity() );
    }

    vPilotAccelN = vPilotAccel/Inertial->gravity();

    earthPosAngle += State->Getdt()*Inertial->omega();

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

  return vw*cos(psiw - vEuler(ePsi));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAuxiliary::GetCrossWind(void)
{
  double psiw,vw;

  psiw = Atmosphere->GetWindPsi();
  vw = Atmosphere->GetWindNED().Magnitude();

  return  vw*sin(psiw - vEuler(ePsi));
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
  PropertyManager->Tie("attitude/roll-rad", this,1,
                       (PMF)&FGAuxiliary::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this,2,
                       (PMF)&FGAuxiliary::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this,3,
                       (PMF)&FGAuxiliary::GetEuler);
  PropertyManager->Tie("velocities/phidot-rad_sec", this,1,
                       (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/thetadot-rad_sec", this,2,
                       (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("velocities/psidot-rad_sec", this,3,
                       (PMF)&FGAuxiliary::GetEulerRates);
  PropertyManager->Tie("attitude/phi-rad", this,
                       &FGAuxiliary::Getphi);
  PropertyManager->Tie("attitude/theta-rad", this,
                       &FGAuxiliary::Gettht);
  PropertyManager->Tie("attitude/psi-true-rad", this,
                       &FGAuxiliary::Getpsi);

  /* PropertyManager->Tie("atmosphere/headwind-fps", this,
                       &FGAuxiliary::GetHeadWind,
                       true);
  PropertyManager->Tie("atmosphere/crosswind-fps", this,
                       &FGAuxiliary::GetCrossWind,
                       true); */
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
  PropertyManager->Untie("attitude/roll-rad");
  PropertyManager->Untie("attitude/pitch-rad");
  PropertyManager->Untie("attitude/heading-true-rad");
  PropertyManager->Untie("velocities/phidot-rad_sec");
  PropertyManager->Untie("velocities/thetadot-rad_sec");
  PropertyManager->Untie("velocities/psidot-rad_sec");
  PropertyManager->Untie("attitude/phi-rad");
  PropertyManager->Untie("attitude/theta-rad");
  PropertyManager->Untie("attitude/psi-true-rad");
  /* PropertyManager->Untie("atmosphere/headwind-fps");
  PropertyManager->Untie("atmosphere/crosswind-fps"); */

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
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
