/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGroundCallback.cpp
 Author:       Mathias Froehlich
 Date started: 05/21/04

 ------ Copyright (C) 2004 Mathias Froehlich (Mathias.Froehlich@web.de) -------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
-------------------------------------------------------------------------------
05/21/00   MF   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "math/FGLocation.h"
#include "FGGroundCallback.h"

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGDefaultGroundCallback::GetAGLevel(double t, const FGLocation& loc,
                                    FGLocation& contact, FGColumnVector3& normal,
                                    FGColumnVector3& vel, FGColumnVector3& angularVel) const
{
  vel.InitMatrix();
  angularVel.InitMatrix();
  FGLocation l = loc;
  l.SetEllipse(a,b);
  double latitude = l.GetGeodLatitudeRad();
  double cosLat = cos(latitude);
  double longitude = l.GetLongitude();
  normal = FGColumnVector3(cosLat*cos(longitude), cosLat*sin(longitude),
                           sin(latitude));
  double loc_radius = loc.GetRadius();  // Get the radius of the given location
                                        // (e.g. the CG)
  contact.SetEllipse(a, b);
  contact.SetPositionGeodetic(longitude, latitude, mTerrainElevation);
  return l.GetGeodAltitude() - mTerrainElevation;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim
