/*******************************************************************************

 Module:       FGPosition.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
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
This class encapsulates the integration of rates and accelerations to get the
current position of the aircraft.

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

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

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  include <cmath>
#endif

#include "FGPosition.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGPosition::FGPosition(FGFDMExec* fdmex) : FGModel(fdmex),
                                           vUVW(3),
                                           vVel(3)
{
  Name = "FGPosition";
  LongitudeDot = LatitudeDot = RadiusDot = 0.0;
  lastLongitudeDot = lastLatitudeDot = lastRadiusDot = 0.0;
}

/******************************************************************************/

FGPosition::~FGPosition(void)
{
}

/******************************************************************************/

bool FGPosition:: Run(void)
{
  float cosLat;

  if (!FGModel::Run()) {
    GetState();

    vVel = State->GetTb2l()*vUVW;

    cosLat = cos(Latitude);
    if (cosLat != 0) LongitudeDot = vVel(eEast) / (Radius * cosLat);

    LatitudeDot = vVel(eNorth) * invRadius;
    RadiusDot   = -vVel(eDown);

    Longitude += 0.5*dt*rate*(LongitudeDot + lastLongitudeDot);
    Latitude  += 0.5*dt*rate*(LatitudeDot + lastLatitudeDot);
    Radius    += 0.5*dt*rate*(RadiusDot + lastRadiusDot);

    lastLatitudeDot = LatitudeDot;
    lastLongitudeDot = LongitudeDot;
    lastRadiusDot = RadiusDot;

    PutState();
    return false;

  } else {
    return true;
  }
}

/******************************************************************************/

void FGPosition::GetState(void)
{
  dt = State->Getdt();

  vUVW = Translation->GetUVW();

  Latitude = State->Getlatitude();
  Longitude = State->Getlongitude();

  invMass = 1.0 / Aircraft->GetMass();
  invRadius = 1.0 / (State->Geth() + EARTHRAD);
  Radius = State->Geth() + EARTHRAD;
}

/******************************************************************************/

void FGPosition::PutState(void)
{
  State->Setlatitude(Latitude);
  State->Setlongitude(Longitude);
  State->Seth(Radius - EARTHRAD);
}

/******************************************************************************/

