/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGLocation.cpp
 Author:       Jon S. Berndt
 Date started: 04/04/2004
 Purpose:      Store an arbitrary location on the globe

 ------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----
 -------           (C) 2011  Ola Røer Thorsen (ola@silentwings.no) -----------

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

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
This class encapsulates an arbitrary position in the globe with its accessors.
It has vector properties, so you can add multiply ....

HISTORY
------------------------------------------------------------------------------
04/04/2004   MF    Created
11/01/2011   ORT   Encapsulated ground callback code in FGLocation and removed
                   it from FGFDMExec.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cmath>
#include <GeographicLib/Math.hpp>
#include <GeographicLib/Geodesic.hpp>

#include "FGLocation.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGLocation::FGLocation(void)
  : mECLoc(1.0, 0.0, 0.0), mCacheValid(false)
{
  e2 = c = 0.0;
  a = ec = ec2 = 1.0;

  mLon = mLat = mRadius = 0.0;
  mGeodLat = GeodeticAltitude = 0.0;

  mTl2ec.InitMatrix();
  mTec2l.InitMatrix();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLocation::FGLocation(double lon, double lat, double radius)
  : mCacheValid(false)
{
  e2 = c = 0.0;
  a = ec = ec2 = 1.0;

  mLon = mLat = mRadius = 0.0;
  mGeodLat = GeodeticAltitude = 0.0;

  mTl2ec.InitMatrix();
  mTec2l.InitMatrix();

  double sinLat = sin(lat);
  double cosLat = cos(lat);
  double sinLon = sin(lon);
  double cosLon = cos(lon);
  mECLoc = { radius*cosLat*cosLon,
             radius*cosLat*sinLon,
             radius*sinLat };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLocation::FGLocation(const FGColumnVector3& lv)
  : mECLoc(lv), mCacheValid(false)
{
  e2 = c = 0.0;
  a = ec = ec2 = 1.0;

  mLon = mLat = mRadius = 0.0;
  mGeodLat = GeodeticAltitude = 0.0;

  mTl2ec.InitMatrix();
  mTec2l.InitMatrix();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLocation::FGLocation(const FGLocation& l)
  : mECLoc(l.mECLoc), mCacheValid(l.mCacheValid)
{
  a = l.a;
  e2 = l.e2;
  c = l.c;
  ec = l.ec;
  ec2 = l.ec2;
  mEllipseSet = l.mEllipseSet;

  /*ag
   * if the cache is not valid, all of the following values are unset.
   * They will be calculated once ComputeDerivedUnconditional is called.
   * If unset, they may possibly contain NaN and could thus trigger floating
   * point exceptions.
   */
  if (!mCacheValid) return;

  mLon = l.mLon;
  mLat = l.mLat;
  mRadius = l.mRadius;

  mTl2ec = l.mTl2ec;
  mTec2l = l.mTec2l;

  mGeodLat = l.mGeodLat;
  GeodeticAltitude = l.GeodeticAltitude;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGLocation& FGLocation::operator=(const FGLocation& l)
{
  mECLoc = l.mECLoc;
  mCacheValid = l.mCacheValid;
  mEllipseSet = l.mEllipseSet;

  a = l.a;
  e2 = l.e2;
  c = l.c;
  ec = l.ec;
  ec2 = l.ec2;

  //ag See comment in constructor above
  if (!mCacheValid) return *this;

  mLon = l.mLon;
  mLat = l.mLat;
  mRadius = l.mRadius;

  mTl2ec = l.mTl2ec;
  mTec2l = l.mTec2l;

  mGeodLat = l.mGeodLat;
  GeodeticAltitude = l.GeodeticAltitude;

  return *this;
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

void FGLocation::SetPosition(double lon, double lat, double radius)
{
  mCacheValid = false;

  double sinLat = sin(lat);
  double cosLat = cos(lat);
  double sinLon = sin(lon);
  double cosLon = cos(lon);

  mECLoc = { radius*cosLat*cosLon,
             radius*cosLat*sinLon,
             radius*sinLat };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetPositionGeodetic(double lon, double lat, double height)
{
  assert(mEllipseSet);
  mCacheValid = false;

  double slat = sin(lat);
  double clat = cos(lat);
  double RN = a / sqrt(1.0 - e2*slat*slat);

  mECLoc(eX) = (RN + height)*clat*cos(lon);
  mECLoc(eY) = (RN + height)*clat*sin(lon);
  mECLoc(eZ) = ((1 - e2)*RN + height)*slat;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::SetEllipse(double semimajor, double semiminor)
{
  mCacheValid = false;
  mEllipseSet = true;

  a = semimajor;
  ec = semiminor/a;
  ec2 = ec * ec;
  e2 = 1.0 - ec2;
  c = a * e2;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGLocation::GetSeaLevelRadius(void) const
{
  assert(mEllipseSet);
  ComputeDerived();
  double cosLat = cos(mLat);
  return a*ec/sqrt(1.0-e2*cosLat*cosLat);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGLocation::ComputeDerivedUnconditional(void) const
{
  // The radius is just the Euclidean norm of the vector.
  mRadius = mECLoc.Magnitude();

  // The distance of the location to the Z-axis, which is the axis
  // through the poles.
  double rxy = mECLoc.Magnitude(eX, eY);

  // Compute the longitude and its sin/cos values.
  double sinLon, cosLon;
  if (rxy == 0.0) {
    sinLon = 0.0;
    cosLon = 1.0;
    mLon = 0.0;
  } else {
    sinLon = mECLoc(eY)/rxy;
    cosLon = mECLoc(eX)/rxy;
    mLon = atan2(mECLoc(eY), mECLoc(eX));
  }

  // Compute the geocentric & geodetic latitudes.
  double sinLat, cosLat;
  if (mRadius == 0.0)  {
    mLat = 0.0;
    sinLat = 0.0;
    cosLat = 1.0;
    if (mEllipseSet) {
      mGeodLat = 0.0;
      GeodeticAltitude = -a;
    }
  }
  else {
    mLat = atan2( mECLoc(eZ), rxy );

    // Calculate the geodetic latitude based on "Transformation from Cartesian to
    // geodetic coordinates accelerated by Halley's method", Fukushima T. (2006)
    // Journal of Geodesy, Vol. 79, pp. 689-693
    // Unlike I. Sofair's method which uses a closed form solution, Fukushima's
    // method is an iterative method whose convergence is so fast that only one
    // iteration suffices. In addition, Fukushima's method has a much better
    // numerical stability over Sofair's method at the North and South poles and
    // it also gives the correct result for a spherical Earth.
    if (mEllipseSet) {
      double s0 = fabs(mECLoc(eZ));
      double zc = ec * s0;
      double c0 = ec * rxy;
      double c02 = c0 * c0;
      double s02 = s0 * s0;
      double a02 = c02 + s02;
      double a0 = sqrt(a02);
      double a03 = a02 * a0;
      double s1 = zc*a03 + c*s02*s0;
      double c1 = rxy*a03 - c*c02*c0;
      double cs0c0 = c*c0*s0;
      double b0 = 1.5*cs0c0*((rxy*s0-zc*c0)*a0-cs0c0);
      s1 = s1*a03-b0*s0;
      double cc = ec*(c1*a03-b0*c0);
      mGeodLat = sign(mECLoc(eZ))*atan(s1 / cc);
      double s12 = s1 * s1;
      double cc2 = cc * cc;
      double norm = sqrt(s12 + cc2);
      cosLat = cc / norm;
      sinLat = sign(mECLoc(eZ)) * s1 / norm;
      GeodeticAltitude = (rxy*cc + s0*s1 - a*sqrt(ec2*s12 + cc2)) / norm;
    }
    else {
      sinLat = mECLoc(eZ)/mRadius;
      cosLat = rxy/mRadius;
    }
  }

  // Compute the transform matrices from and to the earth centered frame.
  // See Stevens and Lewis, "Aircraft Control and Simulation", Second Edition,
  // Eqn. 1.4-13, page 40. In Stevens and Lewis notation, this is C_n/e - the
  // orientation of the navigation (local) frame relative to the ECEF frame,
  // and a transformation from ECEF to nav (local) frame.

  mTec2l = { -cosLon*sinLat, -sinLon*sinLat,  cosLat,
                 -sinLon   ,     cosLon    ,    0.0 ,
             -cosLon*cosLat, -sinLon*cosLat, -sinLat  };

  // In Stevens and Lewis notation, this is C_e/n - the
  // orientation of the ECEF frame relative to the nav (local) frame,
  // and a transformation from nav (local) to ECEF frame.

  mTl2ec = mTec2l.Transposed();

  // Mark the cached values as valid
  mCacheValid = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGLocation::GetDistanceTo(double target_longitude,
                                 double target_latitude) const
{
  assert(mEllipseSet);
  ComputeDerived();
  GeographicLib::Geodesic geod(a, 1 - ec);
  GeographicLib::Math::real distance;
  geod.Inverse(mGeodLat * radtodeg, mLon * radtodeg, target_latitude * radtodeg,
               target_longitude * radtodeg, distance);

  return distance;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGLocation::GetHeadingTo(double target_longitude,
                                double target_latitude) const
{
  assert(mEllipseSet);
  ComputeDerived();
  GeographicLib::Geodesic geod(a, 1 - ec);
  GeographicLib::Math::real heading, azimuth2;
  geod.Inverse(mGeodLat * radtodeg, mLon * radtodeg, target_latitude * radtodeg,
               target_longitude * radtodeg, heading, azimuth2);

  return heading * degtorad;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

} // namespace JSBSim
