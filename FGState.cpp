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
#  include <cmath>
#endif

#ifndef M_PI 
#  include <simgear/constants.h>
#  define M_PI SG_PI
#endif

#include "FGState.h"

static const char *IdSrc = "$Id: FGState.cpp,v 1.65 2001/07/26 23:11:04 jberndt Exp $";
static const char *IdHdr = ID_STATE;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MACROS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define RegisterVariable(ID,DEF) coeffdef[#ID] = ID; paramdef[ID] = DEF

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// For every term registered here there must be a corresponding handler in
// GetParameter() below that retrieves that parameter. Also, there must be an
// entry in the enum eParam definition in FGDefs.h. The ID is what must be used
// in any config file entry which references that item.

FGState::FGState(FGFDMExec* fdex) : mTb2l(3,3),
    mTl2b(3,3),
    mTs2b(3,3),
    vQtrn(4),
    vlastQdot(4),
    vQdot(4),
    vTmp(4),
    vEuler(3),
    vUVW(3),
    vLocalVelNED(3),
    vLocalEuler(3)
{
  FDMExec = fdex;

  a = 1000.0;
  sim_time = 0.0;
  dt = 1.0/120.0;
  ActiveEngine = -1;

  Aircraft     = FDMExec->GetAircraft();
  Translation  = FDMExec->GetTranslation();
  Rotation     = FDMExec->GetRotation();
  Position     = FDMExec->GetPosition();
  FCS          = FDMExec->GetFCS();
  Output       = FDMExec->GetOutput();
  Atmosphere   = FDMExec->GetAtmosphere();
  Aerodynamics = FDMExec->GetAerodynamics();

  RegisterVariable(FG_TIME,           " time "           );
  RegisterVariable(FG_QBAR,           " qbar "           );
  RegisterVariable(FG_WINGAREA,       " wing_area "      );
  RegisterVariable(FG_WINGSPAN,       " wingspan "       );
  RegisterVariable(FG_CBAR,           " cbar "           );
  RegisterVariable(FG_ALPHA,          " alpha "          );
  RegisterVariable(FG_ALPHADOT,       " alphadot "       );
  RegisterVariable(FG_BETA,           " beta "           );
  RegisterVariable(FG_BETADOT,        " betadot "        );
  RegisterVariable(FG_PHI,            " roll_angle "     );
  RegisterVariable(FG_THT,            " pitch_angle "    );
  RegisterVariable(FG_PSI,            " heading_angle "  );
  RegisterVariable(FG_PITCHRATE,      " pitch_rate "     );
  RegisterVariable(FG_ROLLRATE,       " roll_rate "      );
  RegisterVariable(FG_YAWRATE,        " yaw_rate "       );
  RegisterVariable(FG_CL_SQRD,        " Clift_sqrd "     );
  RegisterVariable(FG_MACH,           " mach "           );
  RegisterVariable(FG_ALTITUDE,       " altitude "       );
  RegisterVariable(FG_BI2VEL,         " BI2Vel "         );
  RegisterVariable(FG_CI2VEL,         " CI2Vel "         );
  RegisterVariable(FG_ELEVATOR_POS,   " elevator_pos "   );
  RegisterVariable(FG_AILERON_POS,    " aileron_pos "    );
  RegisterVariable(FG_RUDDER_POS,     " rudder_pos "     );
  RegisterVariable(FG_SPDBRAKE_POS,   " speedbrake_pos " );
  RegisterVariable(FG_SPOILERS_POS,   " spoiler_pos "    );
  RegisterVariable(FG_FLAPS_POS,      " flaps_pos "      );
  RegisterVariable(FG_ELEVATOR_CMD,   " elevator_cmd "   );
  RegisterVariable(FG_AILERON_CMD,    " aileron_cmd "    );
  RegisterVariable(FG_RUDDER_CMD,     " rudder_cmd "     );
  RegisterVariable(FG_SPDBRAKE_CMD,   " speedbrake_cmd " );
  RegisterVariable(FG_SPOILERS_CMD,   " spoiler_cmd "    );
  RegisterVariable(FG_FLAPS_CMD,      " flaps_cmd "      );
  RegisterVariable(FG_THROTTLE_CMD,   " throttle_cmd "   );
  RegisterVariable(FG_THROTTLE_POS,   " throttle_pos "   );
  RegisterVariable(FG_ACTIVE_ENGINE,  " active_engine "  );
  RegisterVariable(FG_HOVERB,         " height/span "    );
  RegisterVariable(FG_PITCH_TRIM_CMD, " pitch_trim_cmd " );
  RegisterVariable(FG_LEFT_BRAKE_CMD, " left_brake_cmd " );
  RegisterVariable(FG_RIGHT_BRAKE_CMD," right_brake_cmd ");
  RegisterVariable(FG_CENTER_BRAKE_CMD," center_brake_cmd ");
  RegisterVariable(FG_SET_LOGGING,    " data_logging "   );

  if (debug_lvl & 2) cout << "Instantiated: FGState" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGState::~FGState()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGState" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGState::GetParameter(eParam val_idx) {
  float scratch;
  
  switch(val_idx) {
  case FG_TIME:
    return sim_time;
  case FG_QBAR:
    return Translation->Getqbar();
  case FG_WINGAREA:
    return Aircraft->GetWingArea();
  case FG_WINGSPAN:
    return Aircraft->GetWingSpan();
  case FG_CBAR:
    return Aircraft->Getcbar();
  case FG_ALPHA:
    return Translation->Getalpha();
  case FG_ALPHADOT:
    return Translation->Getadot();
  case FG_BETA:
    return Translation->Getbeta();
  case FG_BETADOT:
    return Translation->Getbdot();
  case FG_PHI:
    return Rotation->Getphi();
  case FG_THT:
    return Rotation->Gettht();
  case FG_PSI:
    return Rotation->Getpsi();
  case FG_PITCHRATE:
    return Rotation->GetPQR(eQ);
  case FG_ROLLRATE:
    return Rotation->GetPQR(eP);
  case FG_YAWRATE:
    return Rotation->GetPQR(eR);
  case FG_CL_SQRD:
    if (Translation->Getqbar() > 0.00)
      scratch = Aerodynamics->GetvLastFs(eLift)/(Aircraft->GetWingArea()*Translation->Getqbar());
    else
      scratch = 0.0;
    return scratch*scratch;					   
  case FG_ELEVATOR_POS:
    return FCS->GetDePos();
  case FG_AILERON_POS:
    return FCS->GetDaPos();
  case FG_RUDDER_POS:
    return FCS->GetDrPos();
  case FG_SPDBRAKE_POS:
    return FCS->GetDsbPos();
  case FG_SPOILERS_POS:
    return FCS->GetDspPos();
  case FG_FLAPS_POS:
    return FCS->GetDfPos();
  case FG_ELEVATOR_CMD:
    return FCS->GetDeCmd();
  case FG_AILERON_CMD:
    return FCS->GetDaCmd();
  case FG_RUDDER_CMD:
    return FCS->GetDrCmd();
  case FG_SPDBRAKE_CMD:
    return FCS->GetDsbCmd();
  case FG_SPOILERS_CMD:
    return FCS->GetDspCmd();
  case FG_FLAPS_CMD:
    return FCS->GetDfCmd();
  case FG_MACH:
    return Translation->GetMach();
  case FG_ALTITUDE:
    return Position->Geth();
  case FG_BI2VEL:
    if(Translation->GetVt() > 0)
        return Aircraft->GetWingSpan()/(2.0 * Translation->GetVt());
    else
        return 0;
  case FG_CI2VEL:
    if(Translation->GetVt() > 0)
        return Aircraft->Getcbar()/(2.0 * Translation->GetVt());
    else
        return 0;
  case FG_THROTTLE_CMD:
    if (ActiveEngine < 0) return FCS->GetThrottleCmd(0);
    else return FCS->GetThrottleCmd(ActiveEngine);
  case FG_THROTTLE_POS:
    if (ActiveEngine < 0) return FCS->GetThrottlePos(0);
    else return FCS->GetThrottlePos(ActiveEngine);
  case FG_HOVERB:
    return Position->GetHOverB();
  case FG_PITCH_TRIM_CMD:
    return FCS->GetPitchTrimCmd();
  default:
    cerr << "FGState::GetParameter() - No handler for parameter " << val_idx << endl;
    return 0.0;
  }
  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGState::GetParameter(string val_string) {
  return GetParameter(coeffdef[val_string]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

eParam FGState::GetParameterIndex(string val_string) {
  return coeffdef[val_string];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::SetParameter(eParam val_idx, float val) {
  switch(val_idx) {
  case FG_ELEVATOR_POS:
    FCS->SetDePos(val);
    break;
  case FG_AILERON_POS:
    FCS->SetDaPos(val);
    break;
  case FG_RUDDER_POS:
    FCS->SetDrPos(val);
    break;
  case FG_SPDBRAKE_POS:
    FCS->SetDsbPos(val);
    break;
  case FG_SPOILERS_POS:
    FCS->SetDspPos(val);
    break;
  case FG_FLAPS_POS:
    FCS->SetDfPos(val);
    break;
  case FG_THROTTLE_POS:
    FCS->SetThrottlePos(ActiveEngine,val);
    break;

  case FG_ELEVATOR_CMD:
    FCS->SetDeCmd(val);
    break;
  case FG_AILERON_CMD:
    FCS->SetDaCmd(val);
    break;
  case FG_RUDDER_CMD:
    FCS->SetDrCmd(val);
    break;
  case FG_SPDBRAKE_CMD:
    FCS->SetDsbCmd(val);
    break;
  case FG_SPOILERS_CMD:
    FCS->SetDspCmd(val);
    break;
  case FG_FLAPS_CMD:
    FCS->SetDfCmd(val);
    break;
  case FG_THROTTLE_CMD:
    FCS->SetThrottleCmd(ActiveEngine,val);
    break;

  case FG_ACTIVE_ENGINE:
    ActiveEngine = (int)val;
    break;

  case FG_LEFT_BRAKE_CMD:
    FCS->SetLBrake(val);
    break;
  case FG_CENTER_BRAKE_CMD:
    FCS->SetCBrake(val);
    break;
  case FG_RIGHT_BRAKE_CMD:
    FCS->SetRBrake(val);
    break;

  case FG_SET_LOGGING:
    if      (val < -0.01) Output->Disable();
    else if (val >  0.01) Output->Enable();
    else                  Output->Toggle();
    break;

  default:
    cerr << "Parameter '" << val_idx << "' (" << paramdef[val_idx] << ") not handled" << endl;
  }
}

//***************************************************************************
//
// Reset: Assume all angles READ FROM FILE IN DEGREES !!
//

bool FGState::Reset(string path, string acname, string fname) {
  string resetDef;
  float U, V, W;
  float phi, tht, psi;
  float latitude, longitude, h;

  resetDef = path + "/" + acname + "/" + fname + ".xml";

#if defined ( sgi ) && !defined( __GNUC__ )
  ifstream resetfile(resetDef.c_str(), ios::in );
#else
  ifstream resetfile(resetDef.c_str(), ios::in | ios::binary );
#endif

  if (resetfile) {
    resetfile >> U;
    resetfile >> V;
    resetfile >> W;
    resetfile >> latitude;
    resetfile >> longitude;
    resetfile >> phi;
    resetfile >> tht;
    resetfile >> psi;
    resetfile >> h;
    resetfile.close();

    Position->SetLatitude(latitude*DEGTORAD);
    Position->SetLongitude(longitude*DEGTORAD);
    Position->Seth(h);

    Initialize(U, V, W, phi*DEGTORAD, tht*DEGTORAD, psi*DEGTORAD,
               latitude*DEGTORAD, longitude*DEGTORAD, h);

    return true;
  } else {
    cerr << "Unable to load reset file " << fname << endl;
    return false;
  }
}

//***************************************************************************
//
// Initialize: Assume all angles GIVEN IN RADIANS !!
//

void FGState::Initialize(float U, float V, float W,
                         float phi, float tht, float psi,
                         float Latitude, float Longitude, float H)
{
  float alpha, beta;
  float qbar, Vt;

  Position->SetLatitude(Latitude);
  Position->SetLongitude(Longitude);
  Position->Seth(H);

  Atmosphere->Run();

  if (W != 0.0)
    alpha = U*U > 0.0 ? atan2(W, U) : 0.0;
  else
    alpha = 0.0;
  if (V != 0.0)
    beta = U*U+W*W > 0.0 ? atan2(V, (fabs(U)/U)*sqrt(U*U + W*W)) : 0.0;
  else
    beta = 0.0;

  vUVW << U << V << W;
  Translation->SetUVW(vUVW);

  vLocalEuler << phi << tht << psi;
  Rotation->SetEuler(vLocalEuler);

  Translation->SetAB(alpha, beta);

  Vt = sqrt(U*U + V*V + W*W);
  Translation->SetVt(Vt);

  Translation->SetMach(Vt/Atmosphere->GetSoundSpeed());

  qbar = 0.5*(U*U + V*V + W*W)*Atmosphere->GetDensity();
  Translation->Setqbar(qbar);

  InitMatrices(phi, tht, psi);

  vLocalVelNED = mTb2l*vUVW;
  Position->SetvVel(vLocalVelNED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Initialize(FGInitialCondition *FGIC) {

  float tht,psi,phi;
  float U, V, W, h;
  float latitude, longitude;

  latitude = FGIC->GetLatitudeRadIC();
  longitude = FGIC->GetLongitudeRadIC();
  h = FGIC->GetAltitudeFtIC();
  U = FGIC->GetUBodyFpsIC();
  V = FGIC->GetVBodyFpsIC();
  W = FGIC->GetWBodyFpsIC();
  tht = FGIC->GetThetaRadIC();
  phi = FGIC->GetPhiRadIC();
  psi = FGIC->GetPsiRadIC();

  Initialize(U, V, W, phi, tht, psi, latitude, longitude, h);
  
  Position->SetSeaLevelRadius( FGIC->GetSeaLevelRadiusFtIC() );
  Position->SetRunwayRadius( FGIC->GetSeaLevelRadiusFtIC() + 
                                             FGIC->GetTerrainAltitudeFtIC() );

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGState::StoreData(string fname) {
  ofstream datafile(fname.c_str());

  if (datafile) {
    datafile << Translation->GetUVW(eU);
    datafile << Translation->GetUVW(eV);
    datafile << Translation->GetUVW(eW);
    datafile << Position->GetLatitude();
    datafile << Position->GetLongitude();
    datafile << Rotation->GetEuler(ePhi);
    datafile << Rotation->GetEuler(eTht);
    datafile << Rotation->GetEuler(ePsi);
    datafile << Position->Geth();
    datafile.close();
    return true;
  } else {
    cerr << "Could not open dump file " << fname << endl;
    return false;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::InitMatrices(float phi, float tht, float psi) {
  float thtd2, psid2, phid2;
  float Sthtd2, Spsid2, Sphid2;
  float Cthtd2, Cpsid2, Cphid2;
  float Cphid2Cthtd2;
  float Cphid2Sthtd2;
  float Sphid2Sthtd2;
  float Sphid2Cthtd2;

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

void FGState::CalcMatrices(void) {
  float Q0Q0, Q1Q1, Q2Q2, Q3Q3;
  float Q0Q1, Q0Q2, Q0Q3, Q1Q2;
  float Q1Q3, Q2Q3;

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

void FGState::IntegrateQuat(FGColumnVector vPQR, int rate) {
  vQdot(1) = -0.5*(vQtrn(2)*vPQR(eP) + vQtrn(3)*vPQR(eQ) + vQtrn(4)*vPQR(eR));
  vQdot(2) =  0.5*(vQtrn(1)*vPQR(eP) + vQtrn(3)*vPQR(eR) - vQtrn(4)*vPQR(eQ));
  vQdot(3) =  0.5*(vQtrn(1)*vPQR(eQ) + vQtrn(4)*vPQR(eP) - vQtrn(2)*vPQR(eR));
  vQdot(4) =  0.5*(vQtrn(1)*vPQR(eR) + vQtrn(2)*vPQR(eQ) - vQtrn(3)*vPQR(eP));
  vQtrn += 0.5*dt*rate*(vlastQdot + vQdot);

  vQtrn.Normalize();

  vlastQdot = vQdot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector& FGState::CalcEuler(void) {
  if (mTl2b(3,3) == 0.0) mTl2b(3,3) = 0.0000001;
  if (mTl2b(1,1) == 0.0) mTl2b(1,1) = 0.0000001;

  vEuler(ePhi) = atan2(mTl2b(2,3), mTl2b(3,3));
  vEuler(eTht) = asin(-mTl2b(1,3));
  vEuler(ePsi) = atan2(mTl2b(1,2), mTl2b(1,1));

  if (vEuler(ePsi) < 0.0) vEuler(ePsi) += 2*M_PI;

  return vEuler;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix& FGState::GetTs2b(float alpha, float beta)
{
  float ca, cb, sa, sb;

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTs2b(1,1) = -ca*cb;
  mTs2b(1,2) = -ca*sb;
  mTs2b(1,3) = sa;
  mTs2b(2,1) = -sb;
  mTs2b(2,2) = cb;
  mTs2b(2,3) = 0.0;
  mTs2b(3,1) = -sa*cb;
  mTs2b(3,2) = -sa*sb;
  mTs2b(3,3) = -ca;

  return mTs2b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Debug(void)
{
    //TODO: Add your source code here
}

