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
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGTranslation.cpp,v 1.60 2004/03/23 12:32:53 jberndt Exp $";
static const char *IdHdr = ID_TRANSLATION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGTranslation::FGTranslation(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGTranslation";

  vUVWdot.InitMatrix();
  vUVWdot_prev[0].InitMatrix();
  vUVWdot_prev[1].InitMatrix();
  vUVWdot_prev[2].InitMatrix();
  vUVWdot_prev[3].InitMatrix();

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTranslation::~FGTranslation(void)
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTranslation::Run(void)
{
  if (!FGModel::Run()) {

    vUVWdot = vUVW*Rotation->GetPQR() + Aircraft->GetBodyAccel();
    vUVW += State->Integrate(FGState::TRAPZ, State->Getdt()*rate, vUVWdot, vUVWdot_prev);

    if (debug_lvl > 1) Debug(1);

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTranslation::bind(void)
{
  typedef double (FGTranslation::*PMF)(int) const;
  PropertyManager->Tie("velocities/u-fps", this,1,
                       (PMF)&FGTranslation::GetUVW /*,
                       &FGTranslation::SetUVW,
                       true */);
  PropertyManager->Tie("velocities/v-fps", this,2,
                       (PMF)&FGTranslation::GetUVW /*,
                       &FGTranslation::SetUVW,
                       true*/);
  PropertyManager->Tie("velocities/w-fps", this,3,
                       (PMF)&FGTranslation::GetUVW /*,
                       &FGTranslation::SetUVW,
                       true*/);
  PropertyManager->Tie("accelerations/udot-fps", this,1,
                       (PMF)&FGTranslation::GetUVWdot);
  PropertyManager->Tie("accelerations/vdot-fps", this,2,
                       (PMF)&FGTranslation::GetUVWdot);
  PropertyManager->Tie("accelerations/wdot-fps", this,3,
                       (PMF)&FGTranslation::GetUVWdot);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTranslation::unbind(void)
{
  PropertyManager->Untie("velocities/u-fps");
  PropertyManager->Untie("velocities/v-fps");
  PropertyManager->Untie("velocities/w-fps");
  PropertyManager->Untie("accelerations/udot-fps");
  PropertyManager->Untie("accelerations/vdot-fps");
  PropertyManager->Untie("accelerations/wdot-fps");
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

void FGTranslation::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTranslation" << endl;
    if (from == 1) cout << "Destroyed:    FGTranslation" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
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
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
