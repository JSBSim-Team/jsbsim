/*******************************************************************************

 Module:       FGRotation.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Integrates the rotational EOM
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
This class integrates the rotational EOM.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

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

  The order of rotations used in this class corresponds to a 3-2-1 sequence,
  or Y-P-R, or Z-Y-X, if you prefer.

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

#ifndef M_PI
/* get a definition for pi */
#include <Include/fg_constants.h>
#define M_PI FG_PI
#endif

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGRotation::FGRotation(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGRotation";
  Q0dot = Q1dot = Q2dot = Q3dot = 0.0;
  Pdot = Qdot = Rdot = 0.0;
}


FGRotation::~FGRotation(void)
{
}


bool FGRotation::Run(void)
{
  float L2, N1, iQtot, sum;

  if (!FGModel::Run()) {
    GetState();

    lastPdot = Pdot;
    lastQdot = Qdot;
    lastRdot = Rdot;

    L2 = L + Ixz*P*Q - (Izz-Iyy)*R*Q;
    N1 = N - (Iyy-Ixx)*P*Q - Ixz*R*Q;

    Pdot = (L2*Izz - N1*Ixz) / (Ixx*Izz - Ixz*Ixz);
    Qdot = (M - (Ixx-Izz)*P*R - Ixz*(P*P - R*R))/Iyy;
    Rdot = (N1*Ixx + L2*Ixz) / (Ixx*Izz - Ixz*Ixz);

    P += dt*rate*(lastPdot + Pdot)/2.0;
    Q += dt*rate*(lastQdot + Qdot)/2.0;
    R += dt*rate*(lastRdot + Rdot)/2.0;

    lastQ0dot = Q0dot;
    lastQ1dot = Q1dot;
    lastQ2dot = Q2dot;
    lastQ3dot = Q3dot;

    Q0dot = -0.5*(Q1*P + Q2*Q + Q3*R);
    Q1dot =  0.5*(Q0*P + Q2*R - Q3*Q);
    Q2dot =  0.5*(Q0*Q + Q3*P - Q1*R);
    Q3dot =  0.5*(Q0*R + Q1*Q - Q2*P);

    Q0 += 0.5*dt*rate*(lastQ0dot + Q0dot);
    Q1 += 0.5*dt*rate*(lastQ1dot + Q1dot);
    Q2 += 0.5*dt*rate*(lastQ2dot + Q2dot);
    Q3 += 0.5*dt*rate*(lastQ3dot + Q3dot);

    sum = Q0*Q0 + Q1*Q1 + Q2*Q2 + Q3*Q3;

    iQtot = 1.0 / sqrt(sum);

    Q0 *= iQtot;
    Q1 *= iQtot;
    Q2 *= iQtot;
    Q3 *= iQtot;

    if (T[3][3] == 0)
      phi = 0.0;
    else
      phi = atan2(T[2][3], T[3][3]);

    tht = asin(-T[1][3]);

    if (T[1][1] == 0.0)
      psi = 0.0;
    else
      psi = atan2(T[1][2], T[1][1]);

    if (psi < 0.0) psi += 2*M_PI;

    PutState();
  } else {
  }
  return false;
}


void FGRotation::GetState(void)
{
  dt = State->Getdt();

  L = Aircraft->GetL();
  M = Aircraft->GetM();
  N = Aircraft->GetN();

  Ixx = Aircraft->GetIxx();
  Iyy = Aircraft->GetIyy();
  Izz = Aircraft->GetIzz();
  Ixz = Aircraft->GetIxz();

  for (int r=1;r<=3;r++)
    for (int c=1;c<=3;c++)
      T[r][c] = Position->GetT(r,c);
}


void FGRotation::PutState(void)
{
}

