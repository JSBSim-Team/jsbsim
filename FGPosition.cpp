/*******************************************************************************

 Module:       FGPosition.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
 Called by:    FGFDMExec

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
This class encapsulates the integration of rates and accelerations to get the
current position of the aircraft.

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
    Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
    School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
    JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
    Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
    1982 ISBN 0-471-08936-2

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

#include "FGPosition.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGPosition::FGPosition(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGPosition";
  AccelN = AccelE = AccelD = 0.0;
  LongitudeDot = LatitudeDot = RadiusDot = 0.0;
}


FGPosition::~FGPosition(void)
{
}


bool FGPosition:: Run(void)
{
  float /*tanLat,*/ cosLat;

  if (!FGModel::Run()) {
    GetState();
    T[1][1] = Q0*Q0 + Q1*Q1 - Q2*Q2 - Q3*Q3;                    // Page A-11
    T[1][2] = 2*(Q1*Q2 + Q0*Q3);                                // From
    T[1][3] = 2*(Q1*Q3 - Q0*Q2);                                // Reference [2]
    T[2][1] = 2*(Q1*Q2 - Q0*Q3);
    T[2][2] = Q0*Q0 - Q1*Q1 + Q2*Q2 - Q3*Q3;
    T[2][3] = 2*(Q2*Q3 + Q0*Q1);
    T[3][1] = 2*(Q1*Q3 + Q0*Q2);
    T[3][2] = 2*(Q2*Q3 - Q0*Q1);
    T[3][3] = Q0*Q0 - Q1*Q1 - Q2*Q2 + Q3*Q3;

    Fn = T[1][1]*Fx + T[2][1]*Fy + T[3][1]*Fz;                  // Eqn. 3.5
    Fe = T[1][2]*Fx + T[2][2]*Fy + T[3][2]*Fz;                  // From
    Fd = T[1][3]*Fx + T[2][3]*Fy + T[3][3]*Fz;                  // Reference [3]

//    tanLat = tan(Latitude);                                     // I made this up
    cosLat = cos(Latitude);

    lastAccelN = AccelN;
    lastAccelE = AccelE;
    lastAccelD = AccelD;

    Vn = T[1][1]*U + T[2][1]*V + T[3][1]*W;
    Ve = T[1][2]*U + T[2][2]*V + T[3][2]*W;
    Vd = T[1][3]*U + T[2][3]*V + T[3][3]*W;

/*    AccelN = invMass * Fn + invRadius * (Vn*Vd - Ve*Ve*tanLat); // Eqn. 3.6
    AccelE = invMass * Fe + invRadius * (Ve*Vd + Vn*Ve*tanLat); // From
    AccelD = invMass * Fd - invRadius * (Vn*Vn + Ve*Ve);        // Reference [3]

    Vn += 0.5*dt*rate*(3.0*AccelN - lastAccelN);                // Eqn. 3.7
    Ve += 0.5*dt*rate*(3.0*AccelE - lastAccelE);                // From
    Vd += 0.5*dt*rate*(3.0*AccelD - lastAccelD);                // Reference [3]

    Vee = Ve - OMEGAEARTH * (Radius) * cosLat;                  // From Eq. 3.8
*/                                                                // Reference [3]
    lastLatitudeDot = LatitudeDot;
    lastLongitudeDot = LongitudeDot;
    lastRadiusDot = RadiusDot;

    if (cosLat != 0) LongitudeDot = Ve / (Radius * cosLat);
    LatitudeDot = Vn * invRadius;
    RadiusDot = -Vd;

    Longitude += 0.5*dt*rate*(LongitudeDot + lastLongitudeDot);
    Latitude  += 0.5*dt*rate*(LatitudeDot + lastLatitudeDot);
    Radius    += 0.5*dt*rate*(RadiusDot + lastRadiusDot);

    PutState();
    return false;
  } else {
    return true;
  }
}


void FGPosition::GetState(void)
{
  dt = State->Getdt();

  Q0 = Rotation->GetQ0();
  Q1 = Rotation->GetQ1();
  Q2 = Rotation->GetQ2();
  Q3 = Rotation->GetQ3();

  Fx = Aircraft->GetFx();
  Fy = Aircraft->GetFy();
  Fz = Aircraft->GetFz();

  U = Translation->GetU();
  V = Translation->GetV();
  W = Translation->GetW();

  Latitude = State->Getlatitude();
  Longitude = State->Getlongitude();

  invMass = 1.0 / Aircraft->GetMass();
  invRadius = 1.0 / (State->Geth() + EARTHRAD);
  Radius = State->Geth() + EARTHRAD;
}


void FGPosition::PutState(void)
{
  State->Setlatitude(Latitude);
  State->Setlongitude(Longitude);
  State->Seth(Radius - EARTHRAD);
}

