/*******************************************************************************

 Module:       FGTranslation.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Integrates the translational EOM
 Called by:    FDMExec

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
This class integrates the translational EOM.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created
 7/23/99   TP    Added data member and modified Run and PutState to calcuate 
 	  	  	       Mach number

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

#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGTranslation::FGTranslation(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGTranslation";
  Udot = Vdot = Wdot = 0.0;
}


FGTranslation::~FGTranslation(void)
{
}


bool FGTranslation::Run(void)
{
  if (!FGModel::Run()) {

    GetState();

    lastUdot = Udot;
    lastVdot = Vdot;
    lastWdot = Wdot;

    Udot = V*R - W*Q + Fx/Mass;
    Vdot = W*P - U*R + Fy/Mass;
    Wdot = U*Q - V*P + Fz/Mass;

    U += 0.5*dt*rate*(lastUdot + Udot);
    V += 0.5*dt*rate*(lastVdot + Vdot);
    W += 0.5*dt*rate*(lastWdot + Wdot);

    Vt = U*U+V*V+W*W > 0.0 ? sqrt(U*U + V*V + W*W) : 0.0;

    if (W != 0.0)
      alpha = U*U > 0.0 ? atan2(W, U) : 0.0;
    if (V != 0.0)
      beta = U*U+W*W > 0.0 ? atan2(V, (fabs(U)/U)*sqrt(U*U + W*W)) : 0.0;

    qbar = 0.5*rho*Vt*Vt;

    mach = Vt / State->Geta();

    PutState();
  } else {
  }
  return false;
}


void FGTranslation::GetState(void)
{
  dt = State->Getdt();

  P = Rotation->GetP();
  Q = Rotation->GetQ();
  R = Rotation->GetR();

  Fx = Aircraft->GetFx();
  Fy = Aircraft->GetFy();
  Fz = Aircraft->GetFz();

  Mass = Aircraft->GetMass();
  rho = Atmosphere->GetDensity();

  phi = Rotation->Getphi();
  tht = Rotation->Gettht();
  psi = Rotation->Getpsi();
}


void FGTranslation::PutState(void)
{
  State->SetVt(Vt);
  State->Setqbar(qbar);
  State->SetMach(mach);
}

