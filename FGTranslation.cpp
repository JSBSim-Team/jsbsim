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


FGTranslation::FGTranslation(FGFDMExec* fdmex) : FGModel(fdmex),
    vUVW(3),
    vWindUVW(3),
    vUVWdot(3),
    vNcg(3),
    vPQR(3),
    vForces(3),
    vEuler(3)
{
  Name = "FGTranslation";
  qbar = 0;
  Vt = 0.0;
  Mach = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;
  rho = 0.002378;
}

/******************************************************************************/

FGTranslation::~FGTranslation(void) {}

/******************************************************************************/

bool FGTranslation::Run(void) {
  static FGColumnVector vlastUVWdot(3);
  static FGMatrix       mVel(3,3);

  if (!FGModel::Run()) {

    GetState();

    mVel(1,1) =  0.0;
    mVel(1,2) = -vUVW(eW);
    mVel(1,3) =  vUVW(eV);
    mVel(2,1) =  vUVW(eW);
    mVel(2,2) =  0.0;
    mVel(2,3) = -vUVW(eU);
    mVel(3,1) = -vUVW(eV);
    mVel(3,2) =  vUVW(eU);
    mVel(3,3) =  0.0;

    vUVWdot = mVel*vPQR + vForces/Mass;
    
    vNcg=vUVWdot*INVGRAVITY;

    vUVW += 0.5*dt*rate*(vlastUVWdot + vUVWdot) + vWindUVW;
    
    Vt = vUVW.Magnitude();

    if (vUVW(eW) != 0.0)
      alpha = vUVW(eU)*vUVW(eU) > 0.0 ? atan2(vUVW(eW), vUVW(eU)) : 0.0;
    if (vUVW(eV) != 0.0)
      beta = vUVW(eU)*vUVW(eU)+vUVW(eW)*vUVW(eW) > 0.0 ? atan2(vUVW(eV),
             sqrt(vUVW(eU)*vUVW(eU) + vUVW(eW)*vUVW(eW))) : 0.0;
    
     
	
	  // stolen, quite shamelessly, from LaRCsim
    float mUW = (vUVW(eU)*vUVW(eU) + vUVW(eW)*vUVW(eW));
    float signU=1;
    if (vUVW(eU) != 0.0)
		  signU = vUVW(eU)/fabs(vUVW(eU));

	  if( (mUW == 0.0) || (Vt == 0.0) ) {
		  adot = 0.0;
		  bdot = 0.0;
	  } else {
		  adot = (vUVW(eU)*vUVWdot(eW) - vUVW(eW)*vUVWdot(eU))/mUW;
		  bdot = (signU*mUW*vUVWdot(eV) - vUVW(eV)*(vUVW(eU)*vUVWdot(eU) 
              + vUVW(eW)*vUVWdot(eW)))/(Vt*Vt*sqrt(mUW));
	  }
    //
    
    qbar = 0.5*rho*Vt*Vt;

    Mach = Vt / State->Geta();

    vlastUVWdot = vUVWdot;

  } else {}

  return false;
}

/******************************************************************************/

void FGTranslation::GetState(void) {
  dt = State->Getdt();

  vPQR = Rotation->GetPQR();
  vForces = Aircraft->GetForces();

  Mass = Aircraft->GetMass();
  rho = Atmosphere->GetDensity();

  vEuler = Rotation->GetEuler();

  vWindUVW = Atmosphere->GetWindUVW();
}

