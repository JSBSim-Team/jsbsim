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

static const char *IdSrc = "$Id: FGInertial.cpp,v 1.10 2001/04/26 23:46:46 jberndt Exp $";
static const char *IdHdr = ID_INERTIAL;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGInertial::FGInertial(FGFDMExec* fgex) : FGModel(fgex),
    vForces(3),
    vOmegaLocal(3),
    vRadius(3)
{
  vRadius.InitMatrix();

  if (debug_lvl & 2) cout << "Instantiated: FGInertial" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInertial::~FGInertial(void)
{
  if (debug_lvl & 2) cout << "Destroyed:    FGInertial" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::Run(void)
{
  if (!FGModel::Run()) {

    // The following equation for vOmegaLocal terms shows the angular velocity
    // calculation for the local frame given the earth's rotation (first term)
    // at the current latitude, and also the component due to the aircraft
    // motion over the curved surface of the earth (second term).

    vOmegaLocal(eX) = OMEGA_EARTH * cos(Position->GetLatitude())
                      + Position->GetVe() / Position->GetRadius();
    vOmegaLocal(eY) = 0.0 - Position->GetVn() / Position->GetRadius();
    vOmegaLocal(eZ) = OMEGA_EARTH * -sin(Position->GetLatitude());

    vForces = (2.0*vOmegaLocal * Position->GetVel()) * MassBalance->GetMass();
cout << "Coriolis: " << vForces << endl;

    vRadius(3) = Position->GetRadius();
    vForces = (vOmegaLocal * (vOmegaLocal * vRadius)) * MassBalance->GetMass();
cout << "Centripetal: " << vForces << endl;

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::LoadInertial(FGConfigFile* AC_cfg)
{
//
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGInertial::Debug(void)
{
    //TODO: Add your source code here
}

