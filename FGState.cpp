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
#define snprintf _snprintf
#endif

#include "FGState.h"

static const char *IdSrc = "$Id: FGState.cpp,v 1.119 2002/07/31 13:41:53 jberndt Exp $";
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

  a = 1000.0;
  sim_time = 0.0;
  dt = 1.0/120.0;

  Aircraft     = FDMExec->GetAircraft();
  Translation  = FDMExec->GetTranslation();
  Rotation     = FDMExec->GetRotation();
  Position     = FDMExec->GetPosition();
  FCS          = FDMExec->GetFCS();
  Output       = FDMExec->GetOutput();
  Atmosphere   = FDMExec->GetAtmosphere();
  Aerodynamics = FDMExec->GetAerodynamics();
  GroundReactions = FDMExec->GetGroundReactions();
  Propulsion      = FDMExec->GetPropulsion();
  PropertyManager = FDMExec->GetPropertyManager();

  for(int i=0;i<3;i++) vQdot_prev[i].InitMatrix();

  InitPropertyMaps();

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
// Reset: Assume all angles READ FROM FILE IN DEGREES !!
//

bool FGState::Reset(string path, string acname, string fname)
{
  string resetDef;
  string token="";

  double U, V, W;
  double phi, tht, psi;
  double latitude, longitude, h;
  double wdir, wmag, wnorth, weast;

# ifndef macintosh
  resetDef = path + "/" + acname + "/" + fname + ".xml";
# else
  resetDef = path + ";" + acname + ";" + fname + ".xml";
# endif

  FGConfigFile resetfile(resetDef);
  if (!resetfile.IsOpen()) return false;

  resetfile.GetNextConfigLine();
  token = resetfile.GetValue();
  if (token != string("initialize")) {
    cerr << "The reset file " << resetDef
         << " does not appear to be a reset file" << endl;
    return false;
  } else {
    resetfile.GetNextConfigLine();
    resetfile >> token;
    cout << "Resetting using: " << token << endl << endl;
  }
  
  while (token != string("/initialize") && token != string("EOF")) {
    if (token == "UBODY") resetfile >> U;
    if (token == "VBODY") resetfile >> V;
    if (token == "WBODY") resetfile >> W;
    if (token == "LATITUDE") resetfile >> latitude;
    if (token == "LONGITUDE") resetfile >> longitude;
    if (token == "PHI") resetfile >> phi;
    if (token == "THETA") resetfile >> tht;
    if (token == "PSI") resetfile >> psi;
    if (token == "ALTITUDE") resetfile >> h;
    if (token == "WINDDIR") resetfile >> wdir;
    if (token == "VWIND") resetfile >> wmag;

    resetfile >> token;
  }
  
  Position->SetLatitude(latitude*degtorad);
  Position->SetLongitude(longitude*degtorad);
  Position->Seth(h);

  wnorth = wmag*ktstofps*cos(wdir*degtorad);
  weast = wmag*ktstofps*sin(wdir*degtorad);
  
  Initialize(U, V, W, phi*degtorad, tht*degtorad, psi*degtorad,
               latitude*degtorad, longitude*degtorad, h, wnorth, weast, 0.0);

  return true;
}

//***************************************************************************
//
// Initialize: Assume all angles GIVEN IN RADIANS !!
//

void FGState::Initialize(double U, double V, double W,
                         double phi, double tht, double psi,
                         double Latitude, double Longitude, double H,
                         double wnorth, double weast, double wdown)
{
  double alpha, beta;
  double qbar, Vt;
  FGColumnVector3 vAeroUVW;

  Position->SetLatitude(Latitude);
  Position->SetLongitude(Longitude);
  Position->Seth(H);

  Atmosphere->Run();
  
  vLocalEuler << phi << tht << psi;
  Rotation->SetEuler(vLocalEuler);

  InitMatrices(phi, tht, psi);
  
  vUVW << U << V << W;
  Translation->SetUVW(vUVW);
  
  Atmosphere->SetWindNED(wnorth, weast, wdown);
  
  vAeroUVW = vUVW + mTl2b*Atmosphere->GetWindNED();
  
  if (vAeroUVW(eW) != 0.0)
    alpha = vAeroUVW(eU)*vAeroUVW(eU) > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
  else
    alpha = 0.0;
  if (vAeroUVW(eV) != 0.0)
    beta = vAeroUVW(eU)*vAeroUVW(eU)+vAeroUVW(eW)*vAeroUVW(eW) > 0.0 ? atan2(vAeroUVW(eV), (fabs(vAeroUVW(eU))/vAeroUVW(eU))*sqrt(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW))) : 0.0;
  else
    beta = 0.0;

  Translation->SetAB(alpha, beta);

  Vt = sqrt(U*U + V*V + W*W);
  Translation->SetVt(Vt);

  Translation->SetMach(Vt/Atmosphere->GetSoundSpeed());

  qbar = 0.5*(U*U + V*V + W*W)*Atmosphere->GetDensity();
  Translation->Setqbar(qbar);

  vLocalVelNED = mTb2l*vUVW;
  Position->SetvVel(vLocalVelNED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Initialize(FGInitialCondition *FGIC)
{
  double tht,psi,phi;
  double U, V, W, h;
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
  wnorth = FGIC->GetWindNFpsIC();
  weast = FGIC->GetWindEFpsIC();
  wdown = FGIC->GetWindDFpsIC();
  
  Position->SetSeaLevelRadius( FGIC->GetSeaLevelRadiusFtIC() );
  Position->SetRunwayRadius( FGIC->GetSeaLevelRadiusFtIC() + 
                                             FGIC->GetTerrainAltitudeFtIC() );

  // need to fix the wind speed args, here.  
  Initialize(U, V, W, phi, tht, psi, latitude, longitude, h, wnorth, weast, wdown);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::InitMatrices(double phi, double tht, double psi)
{
  double thtd2, psid2, phid2;
  double Sthtd2, Spsid2, Sphid2;
  double Cthtd2, Cpsid2, Cphid2;
  double Cphid2Cthtd2;
  double Cphid2Sthtd2;
  double Sphid2Sthtd2;
  double Sphid2Cthtd2;

  thtd2 = tht/2.0;
  psid2 = psi/2.0;
  phid2 = phi/2.0;

  Sthtd2 = sin(thtd2);
  Spsid2 = sin(psid2);
  Sphid2 = sin(phid2);

  Cthtd2 = cos(thtd2);
  Cpsid2 = cos(psid2);
  Cphid2 = cos(phid2);

  Cphid2Cthtd2 = Cphid2*Cthtd2;
  Cphid2Sthtd2 = Cphid2*Sthtd2;
  Sphid2Sthtd2 = Sphid2*Sthtd2;
  Sphid2Cthtd2 = Sphid2*Cthtd2;

  vQtrn(1) = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  vQtrn(2) = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  vQtrn(3) = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  vQtrn(4) = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;

  CalcMatrices();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::CalcMatrices(void)
{
  double Q0Q0, Q1Q1, Q2Q2, Q3Q3;
  double Q0Q1, Q0Q2, Q0Q3, Q1Q2;
  double Q1Q3, Q2Q3;

  Q0Q0 = vQtrn(1)*vQtrn(1);
  Q1Q1 = vQtrn(2)*vQtrn(2);
  Q2Q2 = vQtrn(3)*vQtrn(3);
  Q3Q3 = vQtrn(4)*vQtrn(4);
  Q0Q1 = vQtrn(1)*vQtrn(2);
  Q0Q2 = vQtrn(1)*vQtrn(3);
  Q0Q3 = vQtrn(1)*vQtrn(4);
  Q1Q2 = vQtrn(2)*vQtrn(3);
  Q1Q3 = vQtrn(2)*vQtrn(4);
  Q2Q3 = vQtrn(3)*vQtrn(4);

  mTl2b(1,1) = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
  mTl2b(1,2) = 2*(Q1Q2 + Q0Q3);
  mTl2b(1,3) = 2*(Q1Q3 - Q0Q2);
  mTl2b(2,1) = 2*(Q1Q2 - Q0Q3);
  mTl2b(2,2) = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
  mTl2b(2,3) = 2*(Q2Q3 + Q0Q1);
  mTl2b(3,1) = 2*(Q1Q3 + Q0Q2);
  mTl2b(3,2) = 2*(Q2Q3 - Q0Q1);
  mTl2b(3,3) = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;

  mTb2l = mTl2b;
  mTb2l.T();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::IntegrateQuat(FGColumnVector3 vPQR, int rate)
{
  vQdot(1) = -0.5*(vQtrn(2)*vPQR(eP) + vQtrn(3)*vPQR(eQ) + vQtrn(4)*vPQR(eR));
  vQdot(2) =  0.5*(vQtrn(1)*vPQR(eP) + vQtrn(3)*vPQR(eR) - vQtrn(4)*vPQR(eQ));
  vQdot(3) =  0.5*(vQtrn(1)*vPQR(eQ) + vQtrn(4)*vPQR(eP) - vQtrn(2)*vPQR(eR));
  vQdot(4) =  0.5*(vQtrn(1)*vPQR(eR) + vQtrn(2)*vPQR(eQ) - vQtrn(3)*vPQR(eP));

  vQtrn += Integrate(TRAPZ, dt*rate, vQdot, vQdot_prev);

  vQtrn.Normalize();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGState::CalcEuler(void)
{
  if (mTl2b(3,3) == 0.0) mTl2b(3,3) = 0.0000001;
  if (mTl2b(1,1) == 0.0) mTl2b(1,1) = 0.0000001;

  vEuler(ePhi) = atan2(mTl2b(2,3), mTl2b(3,3));
  vEuler(eTht) = asin(-mTl2b(1,3));
  vEuler(ePsi) = atan2(mTl2b(1,2), mTl2b(1,1));

  if (vEuler(ePsi) < 0.0) vEuler(ePsi) += 2*M_PI;

  return vEuler;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTs2b(void)
{
  double ca, cb, sa, sb;

  double alpha = Translation->Getalpha();
  double beta  = Translation->Getbeta();

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
  
  alpha = Translation->Getalpha();
  beta  = Translation->Getbeta();
  
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
                    FDMExec->GetAuxiliary()->GetVcalibratedKTS(),
                    Translation->GetMach() );
  cout << out;
  snprintf(out,80, "    Altitude: %7.0f ft.  AGL Altitude: %7.0f ft.\n",
                    Position->Geth(),
                    Position->GetDistanceAGL() );
  cout << out;
  snprintf(out,80, "    Angle of Attack: %6.2f deg  Pitch Angle: %6.2f deg\n",
                    Translation->Getalpha()*radtodeg,
                    Rotation->Gettht()*radtodeg );
  cout << out;
  snprintf(out,80, "    Flight Path Angle: %6.2f deg  Climb Rate: %5.0f ft/min\n",
                    Position->GetGamma()*radtodeg,
                    Position->Gethdot()*60 );
  cout << out;                  
  snprintf(out,80, "    Normal Load Factor: %4.2f g's  Pitch Rate: %5.2f deg/s\n",
                    Aircraft->GetNlf(),
                    Rotation->GetPQR(2)*radtodeg );
  cout << out;
  snprintf(out,80, "    Heading: %3.0f deg true  Sideslip: %5.2f deg  Yaw Rate: %5.2f deg/s\n",
                    Rotation->Getpsi()*radtodeg,
                    Translation->Getbeta()*radtodeg,
                    Rotation->GetPQR(3)*radtodeg  );                  
  cout << out;
  snprintf(out,80, "    Bank Angle: %5.2f deg  Roll Rate: %5.2f deg/s\n",
                    Rotation->Getphi()*radtodeg, 
                    Rotation->GetPQR(1)*radtodeg );
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
                    FDMExec->GetAuxiliary()->GetHeadWind()*fpstokts,
                    FDMExec->GetAuxiliary()->GetCrossWind()*fpstokts );
  cout << out; 
  
  snprintf(out,80, "    Ground Speed: %4.0f knots , Ground Track: %3.0f deg true\n",
                    Position->GetVground()*fpstokts,
                    Position->GetGroundTrack()*radtodeg );
  cout << out;                                   
#endif
} 

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::InitPropertyMaps(void)
{
  ParamNameToProp[  "FG_TIME" ]="sim-time-sec";
  ParamNameToProp[  "FG_QBAR" ]="aero/qbar-psf";
  ParamNameToProp[  "FG_QBARUW" ]="aero/qbarUW-psf";
  ParamNameToProp[  "FG_QBARUV" ]="aero/qbarUV-psf";
  ParamNameToProp[  "FG_WINGAREA" ]="metrics/Sw-sqft";
  ParamNameToProp[  "FG_WINGSPAN" ]="metrics/bw-ft";
  ParamNameToProp[  "FG_CBAR" ]="metrics/cbarw-ft";
  ParamNameToProp[  "FG_ALPHA" ]="aero/alpha-rad";
  ParamNameToProp[  "FG_ALPHADOT" ]="aero/alphadot-rad_sec";
  ParamNameToProp[  "FG_BETA" ]="aero/beta-rad";
  ParamNameToProp[  "FG_ABETA" ]="aero/mag-beta-rad";
  ParamNameToProp[  "FG_BETADOT" ]="aero/betadot-rad_sec";
  ParamNameToProp[  "FG_PHI" ]="attitude/phi-rad";
  ParamNameToProp[  "FG_THT" ]="attitude/theta-rad";
  ParamNameToProp[  "FG_PSI" ]="attitude/psi-true-rad";
  ParamNameToProp[  "FG_PITCHRATE" ]="velocities/q-rad_sec";
  ParamNameToProp[  "FG_ROLLRATE" ]="velocities/p-rad_sec";
  ParamNameToProp[  "FG_YAWRATE" ]="velocities/r-rad_sec";
  ParamNameToProp[  "FG_AEROP" ]="velocities/p-aero-rad_sec";
  ParamNameToProp[  "FG_AEROQ" ]="velocities/q-aero-rad_sec";
  ParamNameToProp[  "FG_AEROR" ]="velocities/r-aero-rad_sec";
  ParamNameToProp[  "FG_CL_SQRD" ]="aero/cl-squared-norm";
  ParamNameToProp[  "FG_MACH" ]="velocities/mach-norm";
  ParamNameToProp[  "FG_ALTITUDE" ]="position/h-sl-ft";
  ParamNameToProp[  "FG_BI2VEL" ]="aero/bi2vel";
  ParamNameToProp[  "FG_CI2VEL" ]="aero/ci2vel";
  ParamNameToProp[  "FG_ELEVATOR_POS" ]="fcs/elevator-pos-rad";
  ParamNameToProp[  "FG_AELEVATOR_POS" ]="fcs/mag-elevator-pos-rad";
  ParamNameToProp[  "FG_NELEVATOR_POS" ]="fcs/elevator-pos-norm";
  ParamNameToProp[  "FG_AILERON_POS" ]="fcs/left-aileron-pos-rad";
  ParamNameToProp[  "FG_AAILERON_POS" ]="fcs/mag-aileron-pos-rad";
  ParamNameToProp[  "FG_NAILERON_POS" ]="fcs/left-aileron-pos-norm";
  ParamNameToProp[  "FG_LEFT_AILERON_POS" ]="fcs/left-aileron-pos-rad";
  ParamNameToProp[  "FG_ALEFT_AILERON_POS" ]="fcs/mag-left-aileron-pos-rad";
  ParamNameToProp[  "FG_NLEFT_AILERON_POS" ]="fcs/left-aileron-pos-norm";
  ParamNameToProp[  "FG_RIGHT_AILERON_POS" ]="fcs/right-aileron-pos-rad";
  ParamNameToProp[  "FG_ARIGHT_AILERON_POS" ]="fcs/mag-aileron-pos-rad";
  ParamNameToProp[  "FG_NRIGHT_AILERON_POS" ]="fcs/right-aileron-pos-norm";
  ParamNameToProp[  "FG_RUDDER_POS" ]="fcs/rudder-pos-rad";
  ParamNameToProp[  "FG_ARUDDER_POS" ]="fcs/mag-rudder-pos-rad";
  ParamNameToProp[  "FG_NRUDDER_POS" ]="fcs/rudder-pos-norm";
  ParamNameToProp[  "FG_SPDBRAKE_POS" ]="fcs/speedbrake-pos-rad";
  ParamNameToProp[  "FG_NSPDBRAKE_POS" ]="fcs/speedbrake-pos-norm";
  ParamNameToProp[  "FG_SPOILERS_POS" ]="fcs/spoiler-pos-rad";
  ParamNameToProp[  "FG_NSPOILERS_POS" ]="fcs/spoiler-pos-norm";
  ParamNameToProp[  "FG_FLAPS_POS" ]="fcs/flap-pos-deg";
  ParamNameToProp[  "FG_NFLAPS_POS" ]="fcs/flap-pos-norm";
  ParamNameToProp[  "FG_ELEVATOR_CMD" ]="fcs/elevator-cmd-norm";
  ParamNameToProp[  "FG_AILERON_CMD" ]="fcs/aileron-cmd-norm";
  ParamNameToProp[  "FG_RUDDER_CMD" ]="fcs/rudder-cmd-norm";
  ParamNameToProp[  "FG_SPDBRAKE_CMD" ]="fcs/speedbrake-cmd-norm";
  ParamNameToProp[  "FG_SPOILERS_CMD" ]="fcs/spoiler-cmd-norm";
  ParamNameToProp[  "FG_FLAPS_CMD" ]="fcs/flap-cmd-norm";
  ParamNameToProp[  "FG_THROTTLE_CMD" ]="fcs/throttle-cmd-norm";
  ParamNameToProp[  "FG_THROTTLE_POS" ]="fcs/throttle-pos-norm";
  ParamNameToProp[  "FG_MIXTURE_CMD" ]="fcs/mixture-cmd-norm";
  ParamNameToProp[  "FG_MIXTURE_POS" ]="fcs/mixture-pos-norm";
  ParamNameToProp[  "FG_MAGNETO_CMD" ]="propulsion/magneto_cmd";
  ParamNameToProp[  "FG_STARTER_CMD" ]="propulsion/starter_cmd";
  ParamNameToProp[  "FG_ACTIVE_ENGINE" ]="propulsion/active_engine";
  ParamNameToProp[  "FG_HOVERB" ]="aero/h_b-mac-ft";
  ParamNameToProp[  "FG_PITCH_TRIM_CMD" ]="fcs/pitch-trim-cmd-norm";
  ParamNameToProp[  "FG_YAW_TRIM_CMD" ]="fcs/yaw-trim-cmd-norm";
  ParamNameToProp[  "FG_ROLL_TRIM_CMD" ]="fcs/roll-trim-cmd-norm";
  ParamNameToProp[  "FG_LEFT_BRAKE_CMD" ]="fcs/left_brake";
  ParamNameToProp[  "FG_CENTER_BRAKE_CMD" ]="fcs/center_brake";
  ParamNameToProp[  "FG_RIGHT_BRAKE_CMD" ]="fcs/right_brake";
  ParamNameToProp[  "FG_SET_LOGGING" ]="sim/set_logging";
  ParamNameToProp[  "FG_ALPHAH" ]="aero/alpha-rad";
  ParamNameToProp[  "FG_ALPHAW" ]="aero/alpha-wing-rad";
  ParamNameToProp[  "FG_LBARH" ]="metrics/lh-norm";     
  ParamNameToProp[  "FG_LBARV" ]="metrics/lv-norm";     
  ParamNameToProp[  "FG_HTAILAREA" ]="metrics/Sh-sqft";
  ParamNameToProp[  "FG_VTAILAREA" ]="metrics/Sv-sqft";
  ParamNameToProp[  "FG_VBARH" ]="metrics/vbarh-norm";    
  ParamNameToProp[  "FG_VBARV" ]="metrics/vbarv-norm";     
  ParamNameToProp[  "FG_GEAR_CMD" ]="gear/gear-cmd-norm";
  ParamNameToProp[  "FG_GEAR_POS" ]="gear/gear-pos-norm";
  ParamNameToProp[  "FG_HYSTPARM" ]="aero/stall-hyst-norm";
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

