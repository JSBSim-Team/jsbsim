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
}


FGState::~FGState(void)
{
}


//***************************************************************************
//
// Reset: Assume all angles READ FROM FILE IN DEGREES !!
//

bool FGState::Reset(string path, string fname)
{
  string resetDef;
  float U, V, W;
  float phi, tht, psi;

  resetDef = path + "/" + FDMExec->GetAircraft()->GetAircraftName() + "/" + fname;

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
  float Q0, Q1, Q2, Q3;
  float T[4][4];
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

  T[1][1] = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
  T[1][2] = 2*(Q1Q2 + Q0Q3);
  T[1][3] = 2*(Q1Q3 - Q0Q2);
  T[2][1] = 2*(Q1Q2 - Q0Q3);
  T[2][2] = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
  T[2][3] = 2*(Q2Q3 + Q0Q1);
  T[3][1] = 2*(Q1Q3 + Q0Q2);
  T[3][2] = 2*(Q2Q3 - Q0Q1);
  T[3][3] = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;

  FDMExec->GetPosition()->SetT(T[1][1], T[1][2], T[1][3],
                               T[2][1], T[2][2], T[2][3],
                               T[3][1], T[3][2], T[3][3]);
  DisplayData();
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


bool FGState::DumpData(string fname)
{
  ofstream datafile(fname.c_str());

  if (datafile) {
    datafile << "U: " << FDMExec->GetTranslation()->GetU() << endl;
    datafile << "V: " << FDMExec->GetTranslation()->GetV() << endl;
    datafile << "W: " << FDMExec->GetTranslation()->GetW() << endl;
    datafile << "P: " << FDMExec->GetRotation()->GetP() << endl;
    datafile << "Q: " << FDMExec->GetRotation()->GetQ() << endl;
    datafile << "R: " << FDMExec->GetRotation()->GetR() << endl;
    datafile << "L: " << FDMExec->GetAircraft()->GetL() << endl;
    datafile << "M: " << FDMExec->GetAircraft()->GetM() << endl;
    datafile << "N: " << FDMExec->GetAircraft()->GetN() << endl;
    datafile << "latitude: " << latitude << endl;
    datafile << "longitude: " << longitude << endl;
    datafile << "alpha: " << FDMExec->GetTranslation()->Getalpha() << endl;
    datafile << "beta: " << FDMExec->GetTranslation()->Getbeta() << endl;
    datafile << "gamma: " << FDMExec->GetTranslation()->Getgamma() << endl;
    datafile << "phi: " << FDMExec->GetRotation()->Getphi() << endl;
    datafile << "tht: " << FDMExec->GetRotation()->Gettht() << endl;
    datafile << "psi: " << FDMExec->GetRotation()->Getpsi() << endl;
    datafile << "Pdot: " << FDMExec->GetRotation()->GetPdot() << endl;
    datafile << "Qdot: " << FDMExec->GetRotation()->GetQdot() << endl;
    datafile << "Rdot: " << FDMExec->GetRotation()->GetRdot() << endl;
    datafile << "h: " << h << endl;
    datafile << "a: " << a << endl;
    datafile << "rho: " << FDMExec->GetAtmosphere()->GetDensity() << endl;
    datafile << "qbar: " << qbar << endl;
    datafile << "sim_time: " << sim_time << endl;
    datafile << "dt: " << dt << endl;
    datafile << "m: " << FDMExec->GetAircraft()->GetMass() << endl;
    datafile.close();
    return true;
  } else {
    return false;
  }
}


bool FGState::DisplayData(void)
{
  cout << "U: " << FDMExec->GetTranslation()->GetU() << endl;
  cout << "V: " << FDMExec->GetTranslation()->GetV() << endl;
  cout << "W: " << FDMExec->GetTranslation()->GetW() << endl;
  cout << "P: " << FDMExec->GetRotation()->GetP()*RADTODEG << endl;
  cout << "Q: " << FDMExec->GetRotation()->GetQ()*RADTODEG << endl;
  cout << "R: " << FDMExec->GetRotation()->GetR()*RADTODEG << endl;
  cout << "L: " << FDMExec->GetAircraft()->GetL() << endl;
  cout << "M: " << FDMExec->GetAircraft()->GetM() << endl;
  cout << "N: " << FDMExec->GetAircraft()->GetN() << endl;
  cout << "Vt: " << Vt << endl;
  cout << "latitude: " << latitude << endl;
  cout << "longitude: " << longitude << endl;
  cout << "alpha: " << FDMExec->GetTranslation()->Getalpha()*RADTODEG << endl;
  cout << "beta: " << FDMExec->GetTranslation()->Getbeta()*RADTODEG << endl;
  cout << "gamma: " << FDMExec->GetTranslation()->Getgamma()*RADTODEG << endl;
  cout << "phi: " << FDMExec->GetRotation()->Getphi()*RADTODEG << endl;
  cout << "tht: " << FDMExec->GetRotation()->Gettht()*RADTODEG << endl;
  cout << "psi: " << FDMExec->GetRotation()->Getpsi()*RADTODEG << endl;
  cout << "Pdot: " << FDMExec->GetRotation()->GetPdot()*RADTODEG << endl;
  cout << "Qdot: " << FDMExec->GetRotation()->GetQdot()*RADTODEG << endl;
  cout << "Rdot: " << FDMExec->GetRotation()->GetRdot()*RADTODEG << endl;
  cout << "h: " << h << endl;
  cout << "a: " << a << endl;
  cout << "rho: " << FDMExec->GetAtmosphere()->GetDensity() << endl;
  cout << "qbar: " << qbar << endl;
  cout << "sim_time: " << sim_time << endl;
  cout << "dt: " << dt << endl;
  cout << "m: " << FDMExec->GetAircraft()->GetMass() << endl;

  return true;
}
