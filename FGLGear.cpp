/*******************************************************************************

 Module:       FGLGear.cpp
 Author:       Jon S. Berndt
 Date started: 11/18/99
 Purpose:      Encapsulates the landing gear elements
 Called by:    FGAircraft

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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGLGear.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGLGear::FGLGear(FGConfigFile* AC_cfg, FGFDMExec* fdmex) : vXYZ(3),
                                                           vMoment(3),
                                                           Exec(fdmex)
{
  string tmp;
  *AC_cfg >> tmp >> name >> vXYZ(1) >> vXYZ(2) >> vXYZ(3) >> kSpring >> bDamp
                                                    >> statFCoeff >> brakeCoeff;
  State = Exec->GetState();
  Aircraft = Exec->GetAircraft();
  Position = Exec->GetPosition();
  WOW = false;                                                  
}


/******************************************************************************/

FGLGear::~FGLGear(void)
{
}

/******************************************************************************/

FGColumnVector FGLGear::Force(void)
{
  static FGColumnVector vForce(3);
  static FGColumnVector vLocalForce(3);
  static FGColumnVector vLocalGear(3);
  static FGColumnVector vTmpRot(3);

  vTmpRot = vXYZ - Aircraft->GetXYZcg();
  vTmpRot(eX) = -vTmpRot(eX);
  vTmpRot(eZ) = -vTmpRot(eZ);
  vTmpRot = vTmpRot/12.0;

  vLocalGear = State->GetTb2l() * vTmpRot;

  compressLength = vLocalGear(eZ) - Position->GetDistanceAGL();

  if (compressLength > 0.00) {
    WOW = true;
    vLocalForce(eZ) = -compressLength * kSpring;
    vForce = State->GetTl2b() * vLocalForce;

    // currently only aircraft body axis Z-force modeled
    vMoment(eX) = vForce(eZ) * vTmpRot(eY);
    vMoment(eY) = vForce(eZ) * vTmpRot(eX);
    vMoment(eZ) = 0.0;

  } else {

    WOW = false;
    vForce.InitMatrix();
    vMoment.InitMatrix();
  }


  return vForce;
}

/******************************************************************************/

