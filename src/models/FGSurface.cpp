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

#include "input_output/FGPropertyManager.h"
#include "models/FGSurface.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGSurface::FGSurface(FGFDMExec* fdmex)
{
  resetValues();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSurface::resetValues(void)
{
  staticFFactor = 1.0;
  rollingFFactor = 1.0;
  maximumForce = DBL_MAX;
  bumpiness = 0.0;
  isSolid = true;
  pos[0] = 0.0;
  pos[1] = 0.0;
  pos[2] = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSurface::bind(FGPropertyManager* PropertyManager)
{
  string base_property_name = "ground";
  string property_name;

  property_name = base_property_name + "/solid";
  PropertyManager->Tie( property_name.c_str(), &isSolid);
  property_name = base_property_name + "/bumpiness";
  PropertyManager->Tie( property_name.c_str(), &bumpiness);
  property_name = base_property_name + "/maximum-force-lbs";
  PropertyManager->Tie( property_name.c_str(), &maximumForce);
  property_name = base_property_name + "/rolling_friction-factor";
  PropertyManager->Tie( property_name.c_str(), &rollingFFactor);
  property_name = base_property_name + "/static-friction-factor";
  PropertyManager->Tie( property_name.c_str(), &staticFFactor);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGSurface::GetBumpHeight()
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
  static const double maxGroundBumpAmplitude=0.4;
  double h = sin(x)+sin(7*x)+sin(8*x)+sin(13*x);
  h += sin(2*y)+sin(5*y)+sin(9*y*x)+sin(17*y);

  return h*(1/8.)*bumpiness*maxGroundBumpAmplitude;
}

} // namespace JSBSim
