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


FGRotation::FGRotation(FGFDMExec* fdmex) : FGModel(fdmex),
                                           vPQR(3),
                                           vEuler(3),
                                           vMoments(3)
{
  Name = "FGRotation";
}

/******************************************************************************/

FGRotation::~FGRotation(void)
{
}

/******************************************************************************/

bool FGRotation::Run(void)
{
  float L2, N1;
  static FGColumnVector vlastPQRdot(3);
  static FGColumnVector vPQRdot(3);

  if (!FGModel::Run()) {
    GetState();

    L2 = vMoments(eL) + Ixz*vPQR(eP)*vPQR(eQ) - (Izz-Iyy)*vPQR(eR)*vPQR(eQ);
    N1 = vMoments(eN) - (Iyy-Ixx)*vPQR(eP)*vPQR(eQ) - Ixz*vPQR(eR)*vPQR(eQ);

    vPQRdot(eP) = (L2*Izz - N1*Ixz) / (Ixx*Izz - Ixz*Ixz);
    vPQRdot(eQ) = (vMoments(eM) - (Ixx-Izz)*vPQR(eP)*vPQR(eR) - Ixz*(vPQR(eP)*vPQR(eP) - vPQR(eR)*vPQR(eR)))/Iyy;
    vPQRdot(eR) = (N1*Ixx + L2*Ixz) / (Ixx*Izz - Ixz*Ixz);

    vPQR += dt*rate*(vlastPQRdot + vPQRdot)/2.0;

    State->IntegrateQuat(vPQR, rate);

    vEuler = State->CalcEuler();

    vlastPQRdot = vPQRdot;

  } else {
  }
  return false;
}

/******************************************************************************/

void FGRotation::GetState(void)
{
  dt = State->Getdt();

  vMoments = Aircraft->GetMoments();

  Ixx = Aircraft->GetIxx();
  Iyy = Aircraft->GetIyy();
  Izz = Aircraft->GetIzz();
  Ixz = Aircraft->GetIxz();
}

/******************************************************************************/

