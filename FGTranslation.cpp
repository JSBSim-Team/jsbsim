/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGMassBalance.h"
#include "FGAircraft.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

static const char *IdSrc = "$Id: FGTranslation.cpp,v 1.31 2001/08/30 21:42:45 jberndt Exp $";
static const char *IdHdr = ID_TRANSLATION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGTranslation::FGTranslation(FGFDMExec* fdmex) : FGModel(fdmex),
    vUVW(3),
    vUVWdot(3),
    vNcg(3),
    vlastUVWdot(3),
    mVel(3,3),
    vAero(3)
{
  Name = "FGTranslation";
  qbar = 0;
  Vt = 0.0;
  Mach = 0.0;
  alpha = beta = 0.0;
  adot = bdot = 0.0;

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTranslation::~FGTranslation()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGTranslation" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTranslation::Run(void)
{
  float Tc = 0.5*State->Getdt()*rate;

  if (!FGModel::Run()) {

    mVel(1,1) =  0.0;
    mVel(1,2) = -vUVW(eW);
    mVel(1,3) =  vUVW(eV);
    mVel(2,1) =  vUVW(eW);
    mVel(2,2) =  0.0;
    mVel(2,3) = -vUVW(eU);
    mVel(3,1) = -vUVW(eV);
    mVel(3,2) =  vUVW(eU);
    mVel(3,3) =  0.0;

    vUVWdot = mVel*Rotation->GetPQR() + Aircraft->GetForces()/MassBalance->GetMass();

    vNcg = vUVWdot*INVGRAVITY;

    vUVW += Tc * (vlastUVWdot + vUVWdot);
    vAero = vUVW + State->GetTl2b()*Atmosphere->GetWindNED();

    Vt = vAero.Magnitude();
    if ( Vt > 1) {
      if (vAero(eW) != 0.0)
        alpha = vAero(eU)*vAero(eU) > 0.0 ? atan2(vAero(eW), vAero(eU)) : 0.0;
      if (vAero(eV) != 0.0)
        beta = vAero(eU)*vAero(eU)+vAero(eW)*vAero(eW) > 0.0 ? atan2(vAero(eV),
               sqrt(vAero(eU)*vAero(eU) + vAero(eW)*vAero(eW))) : 0.0;

      // stolen, quite shamelessly, from LaRCsim
      float mUW = (vAero(eU)*vAero(eU) + vAero(eW)*vAero(eW));
      float signU=1;
      if (vAero(eU) != 0.0)
        signU = vAero(eU)/fabs(vAero(eU));

      if ( (mUW == 0.0) || (Vt == 0.0) ) {
        adot = 0.0;
        bdot = 0.0;
      } else {
        adot = (vAero(eU)*vAero(eW) - vAero(eW)*vUVWdot(eU))/mUW;
        bdot = (signU*mUW*vUVWdot(eV) - vAero(eV)*(vAero(eU)*vUVWdot(eU)
                + vAero(eW)*vUVWdot(eW)))/(Vt*Vt*sqrt(mUW));
      }
    } else {
      alpha = beta = adot = bdot = 0;
    }

    qbar = 0.5*Atmosphere->GetDensity()*Vt*Vt;
    Mach = Vt / State->Geta();

    vlastUVWdot = vUVWdot;

    if (debug_lvl > 1) Debug();

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTranslation::Debug(void)
{
  if (debug_lvl & 16) { // Sanity check variables
    if (fabs(vUVW(eU)) > 1e6)
      cout << "FGTranslation::U velocity out of bounds: " << vUVW(eU) << endl;
    if (fabs(vUVW(eV)) > 1e6)
      cout << "FGTranslation::V velocity out of bounds: " << vUVW(eV) << endl;
    if (fabs(vUVW(eW)) > 1e6)
      cout << "FGTranslation::W velocity out of bounds: " << vUVW(eW) << endl;
    if (fabs(vUVWdot(eU)) > 1e4)
      cout << "FGTranslation::U acceleration out of bounds: " << vUVWdot(eU) << endl;
    if (fabs(vUVWdot(eV)) > 1e4)
      cout << "FGTranslation::V acceleration out of bounds: " << vUVWdot(eV) << endl;
    if (fabs(vUVWdot(eW)) > 1e4)
      cout << "FGTranslation::W acceleration out of bounds: " << vUVWdot(eW) << endl;
    if (Mach > 100 || Mach < 0.00)
      cout << "FGTranslation::Mach is out of bounds: " << Mach << endl;
    if (qbar > 1e6 || qbar < 0.00)
      cout << "FGTranslation::qbar is out of bounds: " << qbar << endl;
  }
}

