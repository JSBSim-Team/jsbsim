/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMassBalance.cpp
 Author:       Jon S. Berndt
 Date started: 09/12/2000
 Purpose:      This module models weight and balance

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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

This class models the change in weight and balance of the aircraft due to fuel
burnoff, etc.

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGMassBalance.h"

static const char *IdSrc = "$Id: FGMassBalance.cpp,v 1.11 2001/04/19 22:05:21 jberndt Exp $";
static const char *IdHdr = ID_MASSBALANCE;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMassBalance::FGMassBalance(FGFDMExec* fdmex) : FGModel(fdmex)
{
  if (debug_lvl & 2) cout << "Instantiated: FGMassBalance" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMassBalance::~FGMassBalance(void)
{
  if (debug_lvl & 2) cout << "Destroyed:    FGMassBalance" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMassBalance::Run(void)
{
  if (!FGModel::Run()) {

    Weight = EmptyWeight + Propulsion->GetTanksWeight();

    Mass = Weight / GRAVITY;

// Calculate new CG here.

    vXYZcg = (Propulsion->GetTanksCG() + EmptyWeight*vbaseXYZcg) / Weight;

// Calculate new moments of inertia here

    Ixx = baseIxx + Propulsion->GetTanksIxx(vXYZcg);
    Iyy = baseIyy + Propulsion->GetTanksIyy(vXYZcg);
    Izz = baseIzz + Propulsion->GetTanksIzz(vXYZcg);
    Ixz = baseIxz + Propulsion->GetTanksIxz(vXYZcg);

    if (debug_lvl > 1) Debug();

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMassBalance::Debug(void)
{
  if (debug_lvl & 16) { // Sanity check variables
    if (EmptyWeight <= 0.0 || EmptyWeight > 1e9)
      cout << "MassBalance::EmptyWeight out of bounds: " << EmptyWeight << endl;
    if (Weight <= 0.0 || Weight > 1e9)
      cout << "MassBalance::Weight out of bounds: " << Weight << endl;
    if (Mass <= 0.0 || Mass > 1e9)
      cout << "MassBalance::Mass out of bounds: " << Mass << endl;
  }
}

