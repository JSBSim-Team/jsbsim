/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSurface.cpp
 Author:       Erik Hofman
 Date started: 01/15/14
 Purpose:      Base class for all surface properties
 Called by:    GroundReactions

 ------------- Copyright (C) 2014  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This base class for the GroundReactions class defines methoed and holds data
for all surface types.

HISTORY
--------------------------------------------------------------------------------
01/15/14   EMH   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGSurface.h"

using namespace std;

namespace JSBSim {

IDENT(IdSrc,"$Id: FGSurface.cpp,v 1.2 2014/01/16 12:31:50 ehofman Exp $");
IDENT(IdHdr,ID_SURFACE);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGSurface::FGSurface()
{
  frictionFactor = 1.0;
  rollingFriction = 0.02;
  loadCapacity = DBL_MAX;
  loadResistance = DBL_MAX;
  bumpiness = 0.0;
  isSolid = true;
}

FGSurface::FGSurface(FGFDMExec* fdmex = NULL)
{
  frictionFactor = 1.0;
  rollingFriction = 0.02;
  loadCapacity = DBL_MAX;
  loadResistance = DBL_MAX;
  bumpiness = 0.0;
  isSolid = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSurface::~FGSurface()
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGSurface::GetBumpHeight()
{
  if (bumpiness < 0.001) return 0.0f;

  double x = pos[0]*0.1;
  double y = pos[1]*0.1;
  x -= floor(x);
  y -= floor(y);
  x *= 2*M_PI;
  y *= 2*M_PI;
  //now x and y are in the range of 0..2pi
  //we need a function, that is periodically on 2pi and gives some
  //height. This is not very fast, but for a beginning.
  //maybe this should be done by interpolating between some precalculated
  //values
  static const float maxGroundBumpAmplitude=0.4;
  float h = sin(x)+sin(7*x)+sin(8*x)+sin(13*x);
  h += sin(2*y)+sin(5*y)+sin(9*y*x)+sin(17*y);

  return h*(1/8.)*bumpiness*maxGroundBumpAmplitude;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGSurface::GetSurfaceStrings(string delimeter) const
{
  std::ostringstream buf;

  buf << "FrictionFactor" << delimeter
      << "RollingFriction" << delimeter
      << "LoadCapacity" << delimeter
      << "LoadResistance" << delimeter
      << "Bumpiness" << delimeter
      << "IsSolid";
  
  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGSurface::GetSurfaceValues(string delimeter) const
{
  std::ostringstream buf;
 
  buf << GetFrictionFactor() << delimeter
      << GetRollingFriction() << delimeter
      << GetLoadCapacity() << delimeter
      << GetLoadResistance() << delimeter
      << GetBumpiness() << delimeter
      << (GetSolid() ? "1" : "0");

  return buf.str();
}

}

