/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLocation.cpp
 Author:       Jon S. Berndt
 Date started: 04/04/2004
 Purpose:      Store an arbitrary location on the globe

 ------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----

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
------------------------------------------------------------------------------
This class encapsulates an arbitrary position in the globe with its accessors.
It has vector properties, so you can add multiply ....

HISTORY
------------------------------------------------------------------------------
04/04/2004   MF    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGLocation.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGLocation.cpp,v 1.3 2004/08/21 17:34:01 frohlich Exp $";
static const char *IdHdr = ID_LOCATION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLocation::FGLocation(double lon, double lat, double radius)
{
  mCacheValid = false;

  double sinLat = sin(lat);
  double cosLat = cos(lat);
  double sinLon = sin(lon);
  double cosLon = cos(lon);
  mECLoc = FGColumnVector3( radius*cosLat*cosLon,
                            radius*cosLat*sinLon,
                            radius*sinLat );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetLongitude(double longitude)
{
  double rtmp = mECLoc.Magnitude(eX, eY);
  // Check if we have zero radius.
  // If so set it to 1, so that we can set a position
  if (0.0 == mECLoc.Magnitude())
    rtmp = 1.0;

  // Fast return if we are on the north or south pole ...
  if (rtmp == 0.0)
    return;

  mCacheValid = false;

  mECLoc(eX) = rtmp*cos(longitude);
  mECLoc(eY) = rtmp*sin(longitude);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetLatitude(double latitude)
{
  mCacheValid = false;

  double r = mECLoc.Magnitude();
  if (r == 0.0) {
    mECLoc(eX) = 1.0;
    r = 1.0;
  }

  double rtmp = mECLoc.Magnitude(eX, eY);
  if (rtmp != 0.0) {
    double fac = r/rtmp*cos(latitude);
    mECLoc(eX) *= fac;
    mECLoc(eY) *= fac;
  } else {
    mECLoc(eX) = r*cos(latitude);
    mECLoc(eY) = 0.0;
  }
  mECLoc(eZ) = r*sin(latitude);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetRadius(double radius)
{
  mCacheValid = false;

  double rold = mECLoc.Magnitude();
  if (rold == 0.0)
    mECLoc(eX) = radius;
  else
    mECLoc *= radius/rold;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::ComputeDerivedUnconditional(void) const
{
  // The radius is just the Euclidean norm of the vector.
  mRadius = mECLoc.Magnitude();

  // The distance of the location to the y-axis, which is the axis
  // through the poles.
  double rxy = sqrt(mECLoc(eX)*mECLoc(eX) + mECLoc(eY)*mECLoc(eY));

  // Compute the sin/cos values of the longitude
  double sinLon, cosLon;
  if (rxy == 0.0) {
    sinLon = 0.0;
    cosLon = 1.0;
  } else {
    sinLon = mECLoc(eY)/rxy;
    cosLon = mECLoc(eX)/rxy;
  }

  // Compute the sin/cos values of the latitude
  double sinLat, cosLat;
  if (mRadius == 0.0)  {
    sinLat = 0.0;
    cosLat = 1.0;
  } else {
    sinLat = mECLoc(eZ)/mRadius;
    cosLat = rxy/mRadius;
  }

  // Compute the longitude and latitude itself
  if ( mECLoc( eX ) == 0.0 && mECLoc( eY ) == 0.0 )
    mLon = 0.0;
  else
    mLon = atan2( mECLoc( eY ), mECLoc( eX ) );

  if ( rxy == 0.0 && mECLoc( eZ ) == 0.0 )
    mLat = 0.0;
  else
    mLat = atan2( mECLoc(eZ), rxy );

  // Compute the transform matrices from and to the earth centered frame.
  // see Durham Chapter 4, problem 1, page 52
  mTec2l = FGMatrix33( -cosLon*sinLat, -sinLon*sinLat,  cosLat,
                           -sinLon   ,     cosLon    ,    0.0 ,
                       -cosLon*cosLat, -sinLon*cosLat, -sinLat  );

  mTl2ec = mTec2l.Transposed();

  // Mark the cached values as valid
  mCacheValid = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::bind(FGPropertyManager* PropertyManager, const string& prefix) const
{
  PropertyManager->Tie(prefix + "lat-gc-rad", (FGLocation*)this,
                       &FGLocation::GetLatitude);
  PropertyManager->Tie(prefix + "lat-gc-deg", (FGLocation*)this,
                       &FGLocation::GetLatitudeDeg);
  PropertyManager->Tie(prefix + "long-gc-rad", (FGLocation*)this,
                       &FGLocation::GetLongitude);
  PropertyManager->Tie(prefix + "long-gc-deg", (FGLocation*)this,
                       &FGLocation::GetLongitudeDeg);
  PropertyManager->Tie(prefix + "radius-ft", (FGLocation*)this,
                       &FGLocation::GetRadius);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::unbind(FGPropertyManager* PropertyManager, const string& prefix) const
{
  PropertyManager->Untie(prefix + "lat-gc-rad");
  PropertyManager->Untie(prefix + "lat-gc-deg");
  PropertyManager->Untie(prefix + "long-gc-rad");
  PropertyManager->Untie(prefix + "long-gc-deg");
  PropertyManager->Untie(prefix + "radius-ft");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim
