/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGState.cpp
 Author:       Jon Berndt
 Date started: 11/17/98
 Called by:    FGFDMExec and accessed by all models.

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
See header file.

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#ifdef _WIN32
//#define snprintf _snprintf
#endif

#include "FGState.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGState.cpp,v 1.135 2004/04/17 21:21:26 jberndt Exp $";
static const char *IdHdr = ID_STATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MACROS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGState::FGState(FGFDMExec* fdex)
{
  FDMExec = fdex;

  sim_time = 0.0;
  dt = 1.0/120.0;

  Aircraft     = FDMExec->GetAircraft();
  Propagate    = FDMExec->GetPropagate();
  Auxiliary    = FDMExec->GetAuxiliary();
  FCS          = FDMExec->GetFCS();
  Output       = FDMExec->GetOutput();
  Atmosphere   = FDMExec->GetAtmosphere();
  Aerodynamics = FDMExec->GetAerodynamics();
  GroundReactions = FDMExec->GetGroundReactions();
  Propulsion      = FDMExec->GetPropulsion();
  PropertyManager = FDMExec->GetPropertyManager();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGState::~FGState()
{
  unbind();
  Debug(1);
}

//***************************************************************************
//
// Initialize: Assume all angles GIVEN IN RADIANS !!
//

void FGState::Initialize(double U, double V, double W,
                         double p, double q, double r,
                         double phi, double tht, double psi,
                         double Latitude, double Longitude, double H,
                         double wnorth, double weast, double wdown)
{
  double alpha, beta;
  double qbar, Vt;
  FGColumnVector3 vAeroUVW;
  FGColumnVector3 vUVW;

  Propagate->SetLatitude(Latitude);
  Propagate->SetLongitude(Longitude);
  Propagate->Seth(H);

  Atmosphere->Run();

  Propagate->SetEuler( FGColumnVector3(phi, tht, psi) );
  Propagate->SetPQR( p, q, r );

  vUVW << U << V << W;
  Propagate->SetUVW(vUVW);

  Atmosphere->SetWindNED(wnorth, weast, wdown);

  vAeroUVW = vUVW + Propagate->GetTl2b()*Atmosphere->GetWindNED();

  if (vAeroUVW(eW) != 0.0)
    alpha = vAeroUVW(eU)*vAeroUVW(eU) > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
  else
    alpha = 0.0;
  if (vAeroUVW(eV) != 0.0)
    beta = vAeroUVW(eU)*vAeroUVW(eU)+vAeroUVW(eW)*vAeroUVW(eW) > 0.0 ? atan2(vAeroUVW(eV), (fabs(vAeroUVW(eU))/vAeroUVW(eU))*sqrt(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW))) : 0.0;
  else
    beta = 0.0;

  Auxiliary->SetAB(alpha, beta);

  Vt = sqrt(U*U + V*V + W*W);
  Auxiliary->SetVt(Vt);

  Auxiliary->SetMach(Vt/Atmosphere->GetSoundSpeed());

  qbar = 0.5*(U*U + V*V + W*W)*Atmosphere->GetDensity();
  Auxiliary->Setqbar(qbar);

  FGColumnVector3 vLocalVelNED = Propagate->GetTb2l()*vUVW;
  Propagate->SetvVel(vLocalVelNED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Initialize(FGInitialCondition *FGIC)
{
  double tht,psi,phi;
  double U, V, W, h;
  double p, q, r;
  double latitude, longitude;
  double wnorth,weast, wdown;

  latitude = FGIC->GetLatitudeRadIC();
  longitude = FGIC->GetLongitudeRadIC();
  h = FGIC->GetAltitudeFtIC();
  U = FGIC->GetUBodyFpsIC();
  V = FGIC->GetVBodyFpsIC();
  W = FGIC->GetWBodyFpsIC();
  tht = FGIC->GetThetaRadIC();
  phi = FGIC->GetPhiRadIC();
  psi = FGIC->GetPsiRadIC();
  p = FGIC->GetPRadpsIC();
  q = FGIC->GetQRadpsIC();
  r = FGIC->GetRRadpsIC();
  wnorth = FGIC->GetWindNFpsIC();
  weast = FGIC->GetWindEFpsIC();
  wdown = FGIC->GetWindDFpsIC();

  Propagate->SetSeaLevelRadius( FGIC->GetSeaLevelRadiusFtIC() );
  Propagate->SetRunwayRadius( FGIC->GetSeaLevelRadiusFtIC() +
                                             FGIC->GetTerrainAltitudeFtIC() );

  // need to fix the wind speed args, here.
  Initialize(U, V, W, p, q, r, phi, tht, psi, latitude, longitude, h, wnorth, weast, wdown);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTs2b(void)
{
  double ca, cb, sa, sb;

  double alpha = Auxiliary->Getalpha();
  double beta  = Auxiliary->Getbeta();

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTs2b(1,1) = ca*cb;
  mTs2b(1,2) = -ca*sb;
  mTs2b(1,3) = -sa;
  mTs2b(2,1) = sb;
  mTs2b(2,2) = cb;
  mTs2b(2,3) = 0.0;
  mTs2b(3,1) = sa*cb;
  mTs2b(3,2) = -sa*sb;
  mTs2b(3,3) = ca;

  return mTs2b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTb2s(void)
{
  float alpha,beta;
  float ca, cb, sa, sb;

  alpha = Auxiliary->Getalpha();
  beta  = Auxiliary->Getbeta();

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTb2s(1,1) = ca*cb;
  mTb2s(1,2) = sb;
  mTb2s(1,3) = sa*cb;
  mTb2s(2,1) = -ca*sb;
  mTb2s(2,2) = cb;
  mTb2s(2,3) = -sa*sb;
  mTb2s(3,1) = -sa;
  mTb2s(3,2) = 0.0;
  mTb2s(3,3) = ca;

  return mTb2s;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::ReportState(void)
{
#if 0
#if !defined(__BORLANDCPP__)
  char out[80], flap[10], gear[12];

  cout << endl << "  JSBSim State" << endl;
  snprintf(out,80,"    Weight: %7.0f lbs.  CG: %5.1f, %5.1f, %5.1f inches\n",
                   FDMExec->GetMassBalance()->GetWeight(),
                   FDMExec->GetMassBalance()->GetXYZcg(1),
                   FDMExec->GetMassBalance()->GetXYZcg(2),
                   FDMExec->GetMassBalance()->GetXYZcg(3));
  cout << out;
  if ( FCS->GetDfPos() <= 0.01)
    snprintf(flap,10,"Up");
  else
    snprintf(flap,10,"%2.0f",FCS->GetDfPos());

  if (FCS->GetGearPos() < 0.01)
    snprintf(gear,12,"Up");
  else if (FCS->GetGearPos() > 0.99)
    snprintf(gear,12,"Down");
  else
    snprintf(gear,12,"In Transit");

  snprintf(out,80, "    Flaps: %3s  Gear: %12s\n",flap,gear);
  cout << out;
  snprintf(out,80, "    Speed: %4.0f KCAS  Mach: %5.2f\n",
                    Auxiliary->GetVcalibratedKTS(),
                    Auxiliary->GetMach() );
  cout << out;
  snprintf(out,80, "    Altitude: %7.0f ft.  AGL Altitude: %7.0f ft.\n",
                    Propagate->Geth(),
                    Propagate->GetDistanceAGL() );
  cout << out;
  snprintf(out,80, "    Angle of Attack: %6.2f deg  Pitch Angle: %6.2f deg\n",
                    Auxiliary->Getalpha()*radtodeg,
                    Propagate->Gettht()*radtodeg );
  cout << out;
  snprintf(out,80, "    Flight Path Angle: %6.2f deg  Climb Rate: %5.0f ft/min\n",
                    Auxiliary->GetGamma()*radtodeg,
                    Propagate->Gethdot()*60 );
  cout << out;
  snprintf(out,80, "    Normal Load Factor: %4.2f g's  Pitch Rate: %5.2f deg/s\n",
                    Aircraft->GetNlf(),
                    Propagate->GetPQR(2)*radtodeg );
  cout << out;
  snprintf(out,80, "    Heading: %3.0f deg true  Sideslip: %5.2f deg  Yaw Rate: %5.2f deg/s\n",
                    Propagate->Getpsi()*radtodeg,
                    Auxiliary->Getbeta()*radtodeg,
                    Propagate->GetPQR(3)*radtodeg  );
  cout << out;
  snprintf(out,80, "    Bank Angle: %5.2f deg  Roll Rate: %5.2f deg/s\n",
                    Propagate->Getphi()*radtodeg,
                    Propagate->GetPQR(1)*radtodeg );
  cout << out;
  snprintf(out,80, "    Elevator: %5.2f deg  Left Aileron: %5.2f deg  Rudder: %5.2f deg\n",
                    FCS->GetDePos(ofRad)*radtodeg,
                    FCS->GetDaLPos(ofRad)*radtodeg,
                    FCS->GetDrPos(ofRad)*radtodeg );
  cout << out;
  snprintf(out,80, "    Throttle: %5.2f%c\n",
                    FCS->GetThrottlePos(0)*100,'%' );
  cout << out;

  snprintf(out,80, "    Wind Components: %5.2f kts head wind, %5.2f kts cross wind\n",
                    Auxiliary->GetHeadWind()*fpstokts,
                    Auxiliary->GetCrossWind()*fpstokts );
  cout << out;

  snprintf(out,80, "    Ground Speed: %4.0f knots , Ground Track: %3.0f deg true\n",
                    Auxiliary->GetVground()*fpstokts,
                    Auxiliary->GetGroundTrack()*radtodeg );
  cout << out;
#endif
#endif
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::bind(void)
{
  PropertyManager->Tie("sim-time-sec",this,
                        &FGState::Getsim_time);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::unbind(void)
{
  PropertyManager->Untie("sim-time-sec");
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

void FGState::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGState" << endl;
    if (from == 1) cout << "Destroyed:    FGState" << endl;
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
