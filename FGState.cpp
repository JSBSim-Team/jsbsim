/*******************************************************************************
                                                                       
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

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  include <cmath>
#endif

#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGState::FGState(FGFDMExec* fdex)
{
  FDMExec = fdex;

  Vt = 0.0;
  latitude = longitude = 0.0;
  adot = bdot = 0.0;
  h = 0.0;
  a = 1000.0;
  qbar = 0.0;
  sim_time = 0.0;
  dt = 1.0/120.0;

  coeffdef["FG_QBAR"]      = 1;
  coeffdef["FG_WINGAREA"]  = 2;
  coeffdef["FG_WINGSPAN"]  = 4;
  coeffdef["FG_CBAR"]      = 8;
  coeffdef["FG_ALPHA"]     = 16;
  coeffdef["FG_ALPHADOT"]  = 32;
  coeffdef["FG_BETA"]      = 64;
  coeffdef["FG_BETADOT"]   = 128;
  coeffdef["FG_PITCHRATE"] = 256;
  coeffdef["FG_ROLLRATE"]  = 512;
  coeffdef["FG_YAWRATE"]   = 1024;
  coeffdef["FG_ELEVATOR"]  = 2048;
  coeffdef["FG_AILERON"]   = 4096;
  coeffdef["FG_RUDDER"]    = 8192;
  coeffdef["FG_MACH"]      = 16384;
  coeffdef["FG_ALTITUDE"]  = 32768L;
  coeffdef["FG_BI2VEL"]    = 65536L;
  coeffdef["FG_CI2VEL"]    = 131072L;
}


FGState::~FGState(void)
{
}


//***************************************************************************
//
// Reset: Assume all angles READ FROM FILE IN DEGREES !!
//

bool FGState::Reset(string path, string acname, string fname)
{
  string resetDef;
  float U, V, W;
  float phi, tht, psi;

  resetDef = path + "/" + acname + "/" + fname;

  ifstream resetfile(resetDef.c_str());

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
  float alpha, beta, gamma;

  latitude = Latitude;
  longitude = Longitude;
  h = H;
  FDMExec->GetAtmosphere()->Run();
  
  gamma = 0.0;
  if (W != 0.0)
    alpha = U*U > 0.0 ? atan2(W, U) : 0.0;
  else
    alpha = 0.0;
  if (V != 0.0)
    beta = U*U+W*W > 0.0 ? atan2(V, (fabs(U)/U)*sqrt(U*U + W*W)) : 0.0;
  else
    beta = 0.0;

  FDMExec->GetTranslation()->SetUVW(U, V, W);
  FDMExec->GetRotation()->SetEuler(phi, tht, psi);
  FDMExec->GetTranslation()->SetABG(alpha, beta, gamma);

  Vt = sqrt(U*U + V*V + W*W);
  qbar = 0.5*(U*U + V*V + W*W)*FDMExec->GetAtmosphere()->GetDensity();

  CalcMatrices(phi, tht, psi);

  FDMExec->GetPosition()->SetT(T[1][1], T[1][2], T[1][3],
                               T[2][1], T[2][2], T[2][3],
                               T[3][1], T[3][2], T[3][3]);
}


void FGState::Initialize(FGInitialCondition *FGIC)
{

  float tht,psi,phi;
  float U,V,W;

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
}


bool FGState::StoreData(string fname)
{
  ofstream datafile(fname.c_str());

  if (datafile) {
    datafile << FDMExec->GetTranslation()->GetU();
    datafile << FDMExec->GetTranslation()->GetV();
    datafile << FDMExec->GetTranslation()->GetW();
    datafile << latitude;
    datafile << longitude;
    datafile << FDMExec->GetRotation()->Getphi();
    datafile << FDMExec->GetRotation()->Gettht();
    datafile << FDMExec->GetRotation()->Getpsi();
    datafile << h;
    datafile.close();
    return true;
  } else {
    cerr << "Could not open dump file " << fname << endl;
    return false;
  }
}


float FGState::GetParameter(string val_string)
{
  return GetParameter(coeffdef[val_string]);
}


int FGState::GetParameterIndex(string val_string)
{
  return coeffdef[val_string];
}


float FGState::GetParameter(int val_idx)
{
  switch(val_idx) {
  case FG_QBAR:
    return Getqbar();
  case FG_WINGAREA:
    return FDMExec->GetAircraft()->GetWingArea();
  case FG_WINGSPAN:
    return FDMExec->GetAircraft()->GetWingSpan();
  case FG_CBAR:
    return FDMExec->GetAircraft()->Getcbar();
  case FG_ALPHA:
    return FDMExec->GetTranslation()->Getalpha();
  case FG_ALPHADOT:
    return Getadot();
  case FG_BETA:
    return FDMExec->GetTranslation()->Getbeta();
  case FG_BETADOT:
    return Getbdot();
  case FG_PITCHRATE:
    return FDMExec->GetRotation()->GetQ();
  case FG_ROLLRATE:
    return FDMExec->GetRotation()->GetP();
  case FG_YAWRATE:
    return FDMExec->GetRotation()->GetR();
  case FG_ELEVATOR:
    return FDMExec->GetFCS()->GetDe();
  case FG_AILERON:
    return FDMExec->GetFCS()->GetDa();
  case FG_RUDDER:
    return FDMExec->GetFCS()->GetDr();
  case FG_MACH:
    return GetMach();
  case FG_ALTITUDE:
    return Geth();
  case FG_BI2VEL:
    return FDMExec->GetAircraft()->GetWingSpan()/(2.0 * GetVt());
  case FG_CI2VEL:
    return FDMExec->GetAircraft()->Getcbar()/(2.0 * GetVt());
  }
  return 0;
}


void FGState::CalcMatrices(float phi, float tht, float psi)
{
  float Q0, Q1, Q2, Q3;
  float thtd2, psid2, phid2;
  float Sthtd2, Spsid2, Sphid2;
  float Cthtd2, Cpsid2, Cphid2;
  float Cphid2Cthtd2;
  float Cphid2Sthtd2;
  float Sphid2Sthtd2;
  float Sphid2Cthtd2;
  float Q0Q0, Q1Q1, Q2Q2, Q3Q3;
  float Q0Q1, Q0Q2, Q0Q3, Q1Q2;
  float Q1Q3, Q2Q3;

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

  Q0 =  Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  Q1 =  Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  Q2 =  Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  Q3 =  Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;

//  Qtrn[1] = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
//  Qtrn[2] = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
//  Qtrn[3] = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
//  Qtrn[4] = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;

  FDMExec->GetRotation()->SetQ0123(Q0, Q1, Q2, Q3);

  Q0Q0 = Q0*Q0;
  Q1Q1 = Q1*Q1;
  Q2Q2 = Q2*Q2;
  Q3Q3 = Q3*Q3;
  Q0Q1 = Q0*Q1;
  Q0Q2 = Q0*Q2;
  Q0Q3 = Q0*Q3;
  Q1Q2 = Q1*Q2;
  Q1Q3 = Q1*Q3;
  Q2Q3 = Q2*Q3;

//  Tb2l[1][1] = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
//  Tb2l[1][2] = 2*(Q1Q2 + Q0Q3);
//  Tb2l[1][3] = 2*(Q1Q3 - Q0Q2);
//  Tb2l[2][1] = 2*(Q1Q2 - Q0Q3);
//  Tb2l[2][2] = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
//  Tb2l[2][3] = 2*(Q2Q3 + Q0Q1);
//  Tb2l[3][1] = 2*(Q1Q3 + Q0Q2);
//  Tb2l[3][2] = 2*(Q2Q3 - Q0Q1);
//  Tb2l[3][3] = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;

  T[1][1] = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
  T[1][2] = 2*(Q1Q2 + Q0Q3);
  T[1][3] = 2*(Q1Q3 - Q0Q2);
  T[2][1] = 2*(Q1Q2 - Q0Q3);
  T[2][2] = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
  T[2][3] = 2*(Q2Q3 + Q0Q1);
  T[3][1] = 2*(Q1Q3 + Q0Q2);
  T[3][2] = 2*(Q2Q3 - Q0Q1);
  T[3][3] = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;
}


void FGState::IntegrateQuat(float P, float Q, float R)
{
    //TODO: Add your source code here
}

