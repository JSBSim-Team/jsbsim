/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGLocation.h
 Author:       Jon S. Berndt, Mathias Froehlich
 Date started: 04/04/2004

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

HISTORY
-------------------------------------------------------------------------------
04/04/2004   MF   Created from code previously in the old positions class.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGLOCATION_H
#define FGLOCATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FGJSBBase.h>
#include <input_output/FGPropertyManager.h>
#include "FGColumnVector3.h"
#include "FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_LOCATION "$Id: FGLocation.h,v 1.14 2009/06/04 02:56:34 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Holds an arbitrary location in the earth centered reference frame.
    This coordinate frame has its center in the middle of the earth.
    Its x-axis points from the center of the earth towards a location
    with zero latitude and longitude on the earths surface. The y-axis
    points from the center of the earth towards a location with zero
    latitude and 90deg longitude on the earths surface. The z-axis
    points from the earths center to the geographic north pole.

    This class provides access functions to set and get the location as
    either the simple x, y and z values in ft or longitude/latitude and
    the radial distance of the location from the earth center.

    It is common to associate a parent frame with a location. This
    frame is usually called the local horizontal frame or simply the local
    frame. This frame has its x/y plane parallel to the surface of the earth
    (with the assumption of a spherical earth). The x-axis points
    towards north, the y-axis points towards east and the z-axis
    points to the center of the earth.

    Since this frame is determined by the location, this class also
    provides the rotation matrices required to transform from the
    earth centered frame to the local horizontal frame and back. There
    are also conversion functions for conversion of position vectors
    given in the one frame to positions in the other frame.

    The earth centered reference frame is *NOT* an inertial frame
    since it rotates with the earth.

    The coordinates in the earth centered frame are the master values.
    All other values are computed from these master values and are
    cached as long as the location is changed by access through a
    non-const member function. Values are cached to improve performance.
    It is best practice to work with a natural set of master values.
    Other parameters that are derived from these master values are calculated
    only when needed, and IF they are needed and calculated, then they are
    cached (stored and remembered) so they do not need to be re-calculated
    until the master values they are derived from are themselves changed
    (and become stale).

    Accuracy and round off:

    Given that we model a vehicle near the earth, the earths surface
    radius is about 2*10^7, ft and that we use double values for the
    representation of the location, we have an accuracy of about
    1e-16*2e7ft/1=2e-9ft left. This should be sufficient for our needs.
    Note that this is the same relative accuracy we would have when we
    compute directly with lon/lat/radius. For the radius value this
    is clear. For the lon/lat pair this is easy to see. Take for
    example KSFO located at about 37.61deg north 122.35deg west, which
    corresponds to 0.65642rad north and 2.13541rad west. Both values
    are of magnitude of about 1. But 1ft corresponds to about
    1/(2e7*2*pi)=7.9577e-09rad. So the left accuracy with this
    representation is also about 1*1e-16/7.9577e-09=1.2566e-08 which
    is of the same magnitude as the representation chosen here.

    The advantage of this representation is that it is a linear space
    without singularities. The singularities are the north and south
    pole and most notably the non-steady jump at -pi to pi. It is
    harder to track this jump correctly especially when we need to
    work with error norms and derivatives of the equations of motion
    within the time-stepping code. Also, the rate of change is of the
    same magnitude for all components in this representation which is
    an advantage for numerical stability in implicit time-stepping too.

    Note: The latitude is a GEOCENTRIC value. FlightGear
    converts latitude to a geodetic value and uses that. In order to get best
    matching relative to a map, geocentric latitude must be converted to geodetic.

    @see Stevens and Lewis, "Aircraft Control and Simulation", Second edition
    @see W. C. Durham "Aircraft Dynamics & Control", section 2.2

    @author Mathias Froehlich
    @version $Id: FGLocation.h,v 1.14 2009/06/04 02:56:34 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGLocation : virtual FGJSBBase
{
public:
  /** Default constructor. */
  FGLocation(void);

  /** Constructor to set the longitude, latitude and the distance
      from the center of the earth.
      @param lon longitude
      @param lat GEOCENTRIC latitude
      @param radius distance from center of earth to vehicle in feet*/
  FGLocation(double lon, double lat, double radius);

  /** Column constructor. */
  FGLocation(const FGColumnVector3& lv) : mECLoc(lv), mCacheValid(false)
  {
    a = 0.0;
    b = 0.0;
    a2 = 0.0;
    b2 = 0.0;
    e2 = 1.0;
    e = 1.0;
    eps2 = -1.0;
    f = 1.0;
  }

  /** Copy constructor. */
  FGLocation(const FGLocation& l)
    : mECLoc(l.mECLoc), mCacheValid(l.mCacheValid)
  {
//    if (!mCacheValid) return; // This doesn't seem right.

    mLon = l.mLon;
    mLat = l.mLat;
    mRadius = l.mRadius;

    mTl2ec = l.mTl2ec;
    mTec2l = l.mTec2l;

    a = l.a;
    b = l.b;
    a2 = l.a2;
    b2 = l.b2;
    e2 = l.e2;
    e = l.e;
    eps2 = l.eps2;
    f = l.f;

    initial_longitude = l.initial_longitude;
  }

  /** Set the longitude.
      @param longitude Longitude in rad to set.
      Sets the longitude of the location represented with this class
      instance to the value of the given argument. The value is meant
      to be in rad. The latitude and the radius value are preserved
      with this call with the exception of radius being equal to
      zero. If the radius is previously set to zero it is changed to be
      equal to 1.0 past this call. Longitude is positive east and negative west. */
  void SetLongitude(double longitude);

  /** Set the latitude.
      @param latitude Latitude in rad to set.
      Sets the latitude of the location represented with this class
      instance to the value of the given argument. The value is meant
      to be in rad. The longitude and the radius value are preserved
      with this call with the exception of radius being equal to
      zero. If the radius is previously set to zero it is changed to be
      equal to 1.0 past this call.
      Latitude is positive north and negative south.
      The arguments should be within the bounds of -pi/2 <= lat <= pi/2.
      The behavior of this function with arguments outside this range is
      left as an exercise to the gentle reader ... */
  void SetLatitude(double latitude);

  /** Set the distance from the center of the earth.
      @param radius Radius in ft to set.
      Sets the radius of the location represented with this class
      instance to the value of the given argument. The value is meant
      to be in ft. The latitude and longitude values are preserved
      with this call with the exception of radius being equal to
      zero. If the radius is previously set to zero, latitude and
      longitude is set equal to zero past this call.
      The argument should be positive.
      The behavior of this function called with a negative argument is
      left as an exercise to the gentle reader ... */
  void SetRadius(double radius);

  /** Sets the longitude, latitude and the distance from the center of the earth.
      @param lon longitude in radians
      @param lat GEOCENTRIC latitude in radians
      @param radius distance from center of earth to vehicle in feet*/
  void SetPosition(double lon, double lat, double radius);

  /** Sets the longitude, latitude and the distance above the reference ellipsoid.
      @param lon longitude in radians
      @param lat GEODETIC latitude in radians
      @param height distance above the reference ellipsoid to vehicle in feet*/
  void SetPositionGeodetic(double lon, double lat, double height);

  /** Sets the semimajor and semiminor axis lengths for this planet.
      The eccentricity and flattening are calculated from the semimajor
      and semiminor axis lengths */
  void SetEllipse(double semimajor, double semiminor);

  /** Get the longitude.
      @return the longitude in rad of the location represented with this
      class instance. The returned values are in the range between
      -pi <= lon <= pi. Longitude is positive east and negative west. */
  double GetLongitude() const { ComputeDerived(); return mLon; }

  /** Get the longitude.
      @return the longitude in deg of the location represented with this
      class instance. The returned values are in the range between
      -180 <= lon <= 180.  Longitude is positive east and negative west. */
  double GetLongitudeDeg() const { ComputeDerived(); return radtodeg*mLon; }

  /** Get the sine of Longitude. */
  double GetSinLongitude() const { ComputeDerived(); return -mTec2l(2,1); }

  /** Get the cosine of Longitude. */
  double GetCosLongitude() const { ComputeDerived(); return mTec2l(2,2); }

  /** Get the latitude.
      @return the latitude in rad of the location represented with this
      class instance. The returned values are in the range between
      -pi/2 <= lon <= pi/2. Latitude is positive north and negative south. */
  double GetLatitude() const { ComputeDerived(); return mLat; }

  /** Get the geodetic latitude.
      @return the geodetic latitude in rad of the location represented with this
      class instance. The returned values are in the range between
      -pi/2 <= lon <= pi/2. Latitude is positive north and negative south. */
  double GetGeodLatitudeRad(void) const { ComputeDerived(); return mGeodLat; }

  /** Get the latitude.
      @return the latitude in deg of the location represented with this
      class instance. The returned value is in the range between
      -90 <= lon <= 90. Latitude is positive north and negative south. */
  double GetLatitudeDeg() const { ComputeDerived(); return radtodeg*mLat; }

  /** Get the geodetic latitude in degrees.
      @return the geodetic latitude in degrees of the location represented by
      this class instance. The returned value is in the range between
      -90 <= lon <= 90. Latitude is positive north and negative south. */
  double GetGeodLatitudeDeg(void) const { ComputeDerived(); return radtodeg*mGeodLat; }

  /** Gets the geodetic altitude in feet. */
  double GetGeodAltitude(void) const { return GeodeticAltitude;}

  /** Get the sine of Latitude. */
  double GetSinLatitude() const { ComputeDerived(); return -mTec2l(3,3); }

  /** Get the cosine of Latitude. */
  double GetCosLatitude() const { ComputeDerived(); return mTec2l(1,3); }

  /** Get the cosine of Latitude. */
  double GetTanLatitude() const {
    ComputeDerived();
    double cLat = mTec2l(1,3);
    if (cLat == 0.0)
      return 0.0;
    else
      return -mTec2l(3,3)/cLat;
  }

  /** Get the distance from the center of the earth.
      @return the distance of the location represented with this class
      instance to the center of the earth in ft. The radius value is
      always positive. */
  double GetRadius() const { ComputeDerived(); return mRadius; }

  /** Transform matrix from local horizontal to earth centered frame.
      Returns a const reference to the rotation matrix of the transform from
      the local horizontal frame to the earth centered frame. */
  const FGMatrix33& GetTl2ec(void) const { ComputeDerived(); return mTl2ec; }

  /** Transform matrix from the earth centered to local horizontal frame.
      Returns a const reference to the rotation matrix of the transform from
      the earth centered frame to the local horizontal frame. */
  const FGMatrix33& GetTec2l(void) const { ComputeDerived(); return mTec2l; }

  /** Transform matrix from inertial to earth centered frame.
      Returns a const reference to the rotation matrix of the transform from
      the inertial frame to the earth centered frame (ECI to ECEF). */
  const FGMatrix33& GetTi2ec(double epa);

  /** Transform matrix from the earth centered to inertial frame.
      Returns a const reference to the rotation matrix of the transform from
      the earth centered frame to the inertial frame (ECEF to ECI). */
  const FGMatrix33& GetTec2i(double epa);

  /** Conversion from Local frame coordinates to a location in the
      earth centered and fixed frame.
      @param lvec Vector in the local horizontal coordinate frame
      @return The location in the earth centered and fixed frame */
  FGLocation LocalToLocation(const FGColumnVector3& lvec) const {
    ComputeDerived(); return mTl2ec*lvec + mECLoc;
  }

  /** Conversion from a location in the earth centered and fixed frame
      to local horizontal frame coordinates.
      @param ecvec Vector in the earth centered and fixed frame
      @return The vector in the local horizontal coordinate frame */
  FGColumnVector3 LocationToLocal(const FGColumnVector3& ecvec) const {
    ComputeDerived(); return mTec2l*(ecvec - mECLoc);
  }

  // For time-stepping, locations have vector properties...

  /** Read access the entries of the vector.
      @param idx the component index.
      Return the value of the matrix entry at the given index.
      Indices are counted starting with 1.
      Note that the index given in the argument is unchecked. */
  double operator()(unsigned int idx) const { return Entry(idx); }

  /** Write access the entries of the vector.
      @param idx the component index.
      @return a reference to the vector entry at the given index.
      Indices are counted starting with 1.
      Note that the index given in the argument is unchecked. */
  double& operator()(unsigned int idx) { return Entry(idx); }

  /** Read access the entries of the vector.
      @param idx the component index.
      @return the value of the matrix entry at the given index.
      Indices are counted starting with 1.
      This function is just a shortcut for the <tt>double
      operator()(unsigned int idx) const</tt> function. It is
      used internally to access the elements in a more convenient way.
      Note that the index given in the argument is unchecked. */
  double Entry(unsigned int idx) const { return mECLoc.Entry(idx); }

  /** Write access the entries of the vector.
      @param idx the component index.
      @return a reference to the vector entry at the given index.
      Indices are counted starting with 1.
      This function is just a shortcut for the double&
      operator()(unsigned int idx) function. It is
      used internally to access the elements in a more convenient way.
      Note that the index given in the argument is unchecked. */
  double& Entry(unsigned int idx) {
    mCacheValid = false; return mECLoc.Entry(idx);
  }

  const FGLocation& operator=(const FGColumnVector3& v)
  {
    mECLoc(eX) = v(eX);
    mECLoc(eY) = v(eY);
    mECLoc(eZ) = v(eZ);
    mCacheValid = false;
    ComputeDerived();
    return *this;
  }

  const FGLocation& operator=(const FGLocation& l)
  {
    mECLoc = l.mECLoc;
    mCacheValid = l.mCacheValid;

//    if (!mCacheValid) return *this; // Why is this here for an assignment operator?

    mLon = l.mLon;
    mLat = l.mLat;
    mRadius = l.mRadius;

    mTl2ec = l.mTl2ec;
    mTec2l = l.mTec2l;

    a = l.a;
    b = l.b;
    a2 = l.a2;
    b2 = l.b2;
    e2 = l.e2;
    e = l.e;
    eps2 = l.eps2;
    f = l.f;

    initial_longitude = l.initial_longitude;

    return *this;
  }
  bool operator==(const FGLocation& l) const {
    return mECLoc == l.mECLoc;
  }
  bool operator!=(const FGLocation& l) const { return ! operator==(l); }
  const FGLocation& operator+=(const FGLocation &l) {
    mCacheValid = false;
    mECLoc += l.mECLoc;
    return *this;
  }
  const FGLocation& operator-=(const FGLocation &l) {
    mCacheValid = false;
    mECLoc -= l.mECLoc;
    return *this;
  }
  const FGLocation& operator*=(double scalar) {
    mCacheValid = false;
    mECLoc *= scalar;
    return *this;
  }
  const FGLocation& operator/=(double scalar) {
    return operator*=(1.0/scalar);
  }
  FGLocation operator+(const FGLocation& l) const {
    return FGLocation(mECLoc + l.mECLoc);
  }
  FGLocation operator-(const FGLocation& l) const {
    return FGLocation(mECLoc - l.mECLoc);
  }

  FGLocation operator*(double scalar) const {
    return FGLocation(scalar*mECLoc);
  }

  /** Cast to a simple 3d vector */
  operator const FGColumnVector3&() const {
    return mECLoc;
  }

private:
  /** Computation of derived values.
      This function re-computes the derived values like lat/lon and
      transformation matrices. It does this unconditionally. */
  void ComputeDerivedUnconditional(void) const;

  /** Computation of derived values.
      This function checks if the derived values like lat/lon and
      transformation matrices are already computed. If so, it
      returns. If they need to be computed this is done here. */
  void ComputeDerived(void) const {
    if (!mCacheValid)
      ComputeDerivedUnconditional();
  }

  /** The coordinates in the earth centered frame. This is the master copy.
      The coordinate frame has its center in the middle of the earth.
      Its x-axis points from the center of the earth towards a
      location with zero latitude and longitude on the earths
      surface. The y-axis points from the center of the earth towards a
      location with zero latitude and 90deg longitude on the earths
      surface. The z-axis points from the earths center to the
      geographic north pole.
      @see W. C. Durham "Aircraft Dynamics & Control", section 2.2 */
  FGColumnVector3 mECLoc;

  /** The cached lon/lat/radius values. */
  mutable double mLon;
  mutable double mLat;
  mutable double mRadius;
  mutable double mGeodLat;
  mutable double GeodeticAltitude;
  
  double initial_longitude;

  /** The cached rotation matrices from and to the associated frames. */
  mutable FGMatrix33 mTl2ec;
  mutable FGMatrix33 mTec2l;
  mutable FGMatrix33 mTi2ec;
  mutable FGMatrix33 mTec2i;
  
  /* Terms for geodetic latitude calculation. Values are from WGS84 model */
  double a;    // Earth semimajor axis in feet (6,378,137.0 meters)
  double b;    // Earth semiminor axis in feet (6,356,752.3142 meters)
  double a2;
  double b2;
  double e;    // Earth eccentricity
  double e2;   // Earth eccentricity squared
  double eps2; //
  double f;    // Flattening

  /** A data validity flag.
      This class implements caching of the derived values like the
      orthogonal rotation matrices or the lon/lat/radius values. For caching we
      carry a flag which signals if the values are valid or not.
      The C++ keyword "mutable" tells the compiler that the data member is
      allowed to change during a const member function. */
  mutable bool mCacheValid;
};

/** Scalar multiplication.

    @param scalar scalar value to multiply with.
    @param l Vector to multiply.

    Multiply the Vector with a scalar value. */
inline FGLocation operator*(double scalar, const FGLocation& l)
{
  return l.operator*(scalar);
}

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
