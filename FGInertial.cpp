/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGInertial.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the inertial frame forces (coriolis and centrifugal)

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGInertial.h"
#include "FGPosition.h"
#include "FGMassBalance.h"

static const char *IdSrc = "$Id: FGInertial.cpp,v 1.20 2001/12/11 05:33:09 jberndt Exp $";
static const char *IdHdr = ID_INERTIAL;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGInertial::FGInertial(FGFDMExec* fgex) : FGModel(fgex)
{
  Name = "FGInertial";

  vRadius.InitMatrix();
  
  // Defaults
  RotationRate    = 0.00007272205217;
  GM              = 14.06252720E15;
  RadiusReference = 20925650.00;
  gAccelReference = GM/(RadiusReference*RadiusReference);
  gAccel          = GM/(RadiusReference*RadiusReference);

  if (debug_lvl > 0) Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInertial::~FGInertial(void)
{
  if (debug_lvl > 0) Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::Run(void)
{
  double stht, ctht, sphi, cphi;

  if (!FGModel::Run()) {

    gAccel = GM / (Position->GetRadius()*Position->GetRadius());

    stht = sin(Rotation->GetEuler(eTht));
    ctht = cos(Rotation->GetEuler(eTht));
    sphi = sin(Rotation->GetEuler(ePhi));
    cphi = cos(Rotation->GetEuler(ePhi));

    vGravity(eX) = vForces(eX) = -gravity()*stht;
    vGravity(eY) = vForces(eY) =  gravity()*sphi*ctht;
    vGravity(eZ) = vForces(eZ) =  gravity()*cphi*ctht;
    
    // The following equation for vOmegaLocal terms shows the angular velocity
    // calculation _for_the_local_frame_ given the earth's rotation (first set)
    // at the current latitude, and also the component due to the aircraft
    // motion over the curved surface of the earth (second set).

    vOmegaLocal(eX) = omega() * cos(Position->GetLatitude());
    vOmegaLocal(eY) = 0.0;
    vOmegaLocal(eZ) = omega() * -sin(Position->GetLatitude());

    vOmegaLocal(eX) +=  Position->GetVe() / Position->GetRadius();
    vOmegaLocal(eY) += -Position->GetVn() / Position->GetRadius();
    vOmegaLocal(eZ) +=  0.00;

//    vForces = State->GetTl2b()*(-2.0*vOmegaLocal * Position->GetVel());

    vRadius(3) = Position->GetRadius();
    vForces += State->GetTl2b()*(vOmegaLocal * (vOmegaLocal * vRadius));

    vForces *= MassBalance->GetMass(); // IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::LoadInertial(FGConfigFile* AC_cfg)
{
  return true;
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

void FGInertial::Debug(int from)
{
  if (debug_lvl & 1 ) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGInertial" << endl;
    if (from == 1) cout << "Destroyed:    FGInertial" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
}

