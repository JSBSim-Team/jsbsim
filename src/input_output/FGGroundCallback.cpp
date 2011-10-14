/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGroundCallback.cpp
 Author:       Mathias Froehlich
 Date started: 05/21/04

 ------ Copyright (C) 2004 Mathias Froehlich (Mathias.Froehlich@web.de) -------

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

HISTORY
-------------------------------------------------------------------------------
05/21/00   MF   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "math/FGColumnVector3.h"
#include "math/FGLocation.h"
#include "FGGroundCallback.h"

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGDefaultGroundCallback::FGDefaultGroundCallback(double referenceRadius)
{
  mSeaLevelRadius = referenceRadius; // Sea level radius
  mTerrainLevelRadius = mSeaLevelRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGDefaultGroundCallback::GetAltitude(const FGLocation& loc) const
{
  return loc.GetRadius() - mSeaLevelRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGDefaultGroundCallback::GetAGLevel(double t, const FGLocation& loc,
                                    FGLocation& contact, FGColumnVector3& normal,
                                    FGColumnVector3& vel, FGColumnVector3& angularVel) const
{
  vel = FGColumnVector3(0.0, 0.0, 0.0);
  angularVel = FGColumnVector3(0.0, 0.0, 0.0);
  normal = FGColumnVector3(loc).Normalize();
  double loc_radius = loc.GetRadius();  // Get the radius of the given location
                                        // (e.g. the CG)
  double agl = loc_radius - mTerrainLevelRadius;
  contact = (mTerrainLevelRadius/loc_radius)*FGColumnVector3(loc);
  return agl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim
