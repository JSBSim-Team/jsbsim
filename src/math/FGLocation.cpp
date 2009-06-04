/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLocation.cpp
 Author:       Jon S. Berndt
 Date started: 04/04/2004
 Purpose:      Store an arbitrary location on the globe

 ------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----

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
------------------------------------------------------------------------------
This class encapsulates an arbitrary position in the globe with its accessors.
It has vector properties, so you can add multiply ....

HISTORY
------------------------------------------------------------------------------
04/04/2004   MF    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cmath>

#include "FGLocation.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGLocation.cpp,v 1.14 2009/06/04 02:56:34 jberndt Exp $";
static const char *IdHdr = ID_LOCATION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLocation::FGLocation(void)
{
  mCacheValid = false;
  initial_longitude = 0.0;
  a = 0.0;
  b = 0.0;
  a2 = 0.0;
  b2 = 0.0;
  e2 = 1.0;
  e = 1.0;
  eps2 = -1.0;
  f = 1.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLocation::FGLocation(double lon, double lat, double radius)
{
  mCacheValid = false;

  double sinLat = sin(lat);
  double cosLat = cos(lat);
  double sinLon = sin(lon);
  double cosLon = cos(lon);
  initial_longitude = lon;
  a = 0.0;
  b = 0.0;
  a2 = 0.0;
  b2 = 0.0;
  e2 = 1.0;
  e = 1.0;
  eps2 = -1.0;
  f = 1.0;
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

  // Need to figure out how to set the initial_longitude here

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

void FGLocation::SetPosition(double lon, double lat, double radius)
{
  mCacheValid = false;

  double sinLat = sin(lat);
  double cosLat = cos(lat);
  double sinLon = sin(lon);
  double cosLon = cos(lon);
  initial_longitude = lon;
  mECLoc = FGColumnVector3( radius*cosLat*cosLon,
                            radius*cosLat*sinLon,
                            radius*sinLat );
  ComputeDerived();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetPositionGeodetic(double lon, double lat, double height)
{
  mCacheValid = false;
  
  mGeodLat = lat;
  mLon = lon;
  GeodeticAltitude = height;

  initial_longitude = mLon;

  double RN = a / sqrt(1.0 - e2*sin(mGeodLat)*sin(mGeodLat));

  mECLoc(eX) = (RN + GeodeticAltitude)*cos(mGeodLat)*cos(mLon);
  mECLoc(eY) = (RN + GeodeticAltitude)*cos(mGeodLat)*sin(mLon);
  mECLoc(eZ) = ((1 - e2)*RN + GeodeticAltitude)*sin(mGeodLat);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetEllipse(double semimajor, double semiminor)
{
  mCacheValid = false;

  a = semimajor;
  b = semiminor;
  a2 = a*a;
  b2 = b*b;
  e2 = 1.0 - b2/a2;
  e = sqrt(e2);
  eps2 = a2/b2 - 1.0;
  f = 1.0 - b/a;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the ECEF to ECI transformation matrix using Stevens and Lewis "Aircraft
// Control and Simulation", second edition, eqn. 1.4-12, pg. 39

const FGMatrix33& FGLocation::GetTec2i(double epa)
{
  double mu = epa - initial_longitude;
  mTec2i = FGMatrix33( cos(mu), -sin(mu), 0.0,
                       sin(mu),  cos(mu), 0.0,
                            0.0,     0.0, 1.0 );
  return mTec2i;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGLocation::GetTi2ec(double epa)
{
  double mu = epa - initial_longitude;
  mTi2ec = FGMatrix33( cos(mu), sin(mu), 0.0,
                      -sin(mu), cos(mu), 0.0,
                           0.0,      0.0, 1.0 );
  return mTi2ec;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::ComputeDerivedUnconditional(void) const
{
  // The radius is just the Euclidean norm of the vector.
  mRadius = mECLoc.Magnitude();

  // The distance of the location to the Z-axis, which is the axis
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
  // See Stevens and Lewis, "Aircraft Control and Simulation", Second Edition,
  // Eqn. 1.4-13, page 40. 
  mTec2l = FGMatrix33( -cosLon*sinLat, -sinLon*sinLat,  cosLat,
                           -sinLon   ,     cosLon    ,    0.0 ,
                       -cosLon*cosLat, -sinLon*cosLat, -sinLat  );

  mTl2ec = mTec2l.Transposed();
  
  // Calculate the geodetic latitude base on AIAA Journal of Guidance and Control paper,
  // "Improved Method for Calculating Exact Geodetic Latitude and Altitude", and
  // "Improved Method for Calculating Exact Geodetic Latitude and Altitude, Revisited",
  // author: I. Sofair

  if (a != 0.0 && b != 0.0) {
    double c, p, q, s, t, u, v, w, z, p2, u2, r02, r0;
    double Ne, P, Q0, Q, signz0, sqrt_q; 
    p  = fabs(mECLoc(eZ))/eps2;
    r02 = mECLoc(eX)*mECLoc(eX) + mECLoc(eY)*mECLoc(eY);
    s  = r02/(e2*eps2);
    p2 = p*p;
    q  = p2 - b2 + s;
    sqrt_q = sqrt(q);
    if (q>0)
    {
      u  = p/sqrt_q;
      u2 = p2/q;
      v  = b2*u2/q;
      P  = 27.0*v*s/q;
      Q0 = sqrt(P+1) + sqrt(P);
      Q  = pow(Q0, 0.66666666667);
      t  = (1.0 + Q + 1.0/Q)/6.0;
      c  = sqrt(u2 - 1 + 2.0*t);
      w  = (c - u)/2.0;
      signz0 = mECLoc(eZ)>=0?1.0:-1.0;
      z  = signz0*sqrt_q*(w+sqrt(sqrt(t*t+v)-u*w-0.5*t-0.25));
      Ne = a*sqrt(1+eps2*z*z/b2);
      mGeodLat = asin((eps2+1.0)*(z/Ne));
      r0 = sqrt(r02);
      GeodeticAltitude = r0*cos(mGeodLat) + mECLoc(eZ)*sin(mGeodLat) - a2/Ne;
    }
  }

  // Mark the cached values as valid
  mCacheValid = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim
