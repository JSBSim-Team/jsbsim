/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGLocation.h
 Author:       Jon S. Berndt, Mathias Froehlich
 Date started: 04/04/2004

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

HISTORY
-------------------------------------------------------------------------------
04/04/2004   MF   Created from code previously in the old positions class.
11/01/2011   ORT  Encapsulated ground callback code in FGLocation and removed
                  it from FGFDMExec.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGLOCATION_H
#define FGLOCATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cassert>

#include "FGJSBBase.h"
#include "FGColumnVector3.h"
#include "FGMatrix33.h"

/* Setting the -ffast-math compilation flag is highly discouraged */
#ifdef __FAST_MATH__
#error Usage of -ffast-math is strongly discouraged
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** FGLocation holds an arbitrary location in the Earth centered Earth fixed
    reference frame (ECEF). The coordinate frame ECEF has its center in the
    middle of the earth. The X-axis points from the center of the Earth towards
    a location with zero latitude and longitude on the Earth surface. The Y-axis
    points from the center of the Earth towards a location with zero latitude
    and 90 deg East longitude on the Earth surface. The Z-axis points from the
    Earth center to the geographic north pole.

    This class provides access functions to set and get the location as either
    the simple X, Y and Z values in ft or longitude/latitude and the radial
    distance of the location from the Earth center.

    It is common to associate a parent frame with a location. This frame is
    usually called the local horizontal frame or simply the local frame. It is
    also called the NED frame (North, East, Down), as well as the Navigation
    frame. This frame has its X/Y plane parallel to the surface of the Earth.
    The X-axis points towards north, the Y-axis points east and the Z-axis
    is normal to the reference spheroid (WGS84 for Earth).

    Since the local frame is determined by the location (and NOT by the
    orientation of the vehicle IN any frame), this class also provides the
    rotation matrices required to transform from the Earth centered (ECEF) frame
    to the local horizontal frame and back. This class "owns" the
    transformations that go from the ECEF frame to and from the local frame.
    Again, this is because the ECEF, and local frames do not involve the actual
    orientation of the vehicle - only the location on the Earth surface. There
    are conversion functions for conversion of position vectors given in the one
    frame to positions in the other frame.

    The Earth centered reference frame is NOT an inertial frame since it rotates
    with the Earth.

    The cartesian coordinates (X,Y,Z) in the Earth centered frame are the master
    values. All other values are computed from these master values and are
    cached as long as the location is changed by access through a non-const
    member function. Values are cached to improve performance. It is best
    practice to work with a natural set of master values. Other parameters that
    are derived from these master values are calculated only when needed, and IF
    they are needed and calculated, then they are cached (stored and remembered)
    so they do not need to be re-calculated until the master values they are
    derived from are themselves changed (and become stale).

    Accuracy and round off

    Given,

    - that we model a vehicle near the Earth
    - that the Earth surface average radius is about 2*10^7, ft
    - that we use double values for the representation of the location

    we have an accuracy of about

    1e-16*2e7ft/1 = 2e-9 ft

    left. This should be sufficient for our needs. Note that this is the same
    relative accuracy we would have when we compute directly with
    lon/lat/radius. For the radius value this is clear. For the lon/lat pair
    this is easy to see. Take for example KSFO located at about 37.61 deg north
    122.35 deg west, which corresponds to 0.65642 rad north and 2.13541 rad
    west. Both values are of magnitude of about 1. But 1 ft corresponds to about
    1/(2e7*2*pi) = 7.9577e-09 rad. So the left accuracy with this representation
    is also about 1*1e-16/7.9577e-09 = 1.2566e-08 which is of the same magnitude
    as the representation chosen here.

    The advantage of this representation is that it is a linear space without
    singularities. The singularities are the north and south pole and most
    notably the non-steady jump at -pi to pi. It is harder to track this jump
    correctly especially when we need to work with error norms and derivatives
    of the equations of motion within the time-stepping code. Also, the rate of
    change is of the same magnitude for all components in this representation
    which is an advantage for numerical stability in implicit time-stepping.

    Note: Both GEOCENTRIC and GEODETIC latitudes can be used. In order to get
    best matching relative to a map, geodetic latitude must be used.

    @see Stevens and Lewis, "Aircraft Control and Simulation", Second edition
    @see W. C. Durham "Aircraft Dynamics & Control", section 2.2

    @author Mathias Froehlich
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGLocation : public FGJSBBase
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

  /** Constructor to initialize the location with the cartesian coordinates
      (X,Y,Z) contained in the input FGColumnVector3. Distances are in feet,
      the position is expressed in the ECEF frame.
      @param lv vector that contain the cartesian coordinates*/
  FGLocation(const FGColumnVector3& lv);

  /** Copy constructor. */
  FGLocation(const FGLocation& l);

  /** Set the longitude.
      @param longitude Longitude in rad to set.
      Sets the longitude of the location represented with this class instance to
      the value of the given argument. The value is meant to be in rad. The
      latitude and the radius value are preserved with this call with the
      exception of radius being equal to zero. If the radius is previously set
      to zero it is changed to be equal to 1.0 past this call.
      Longitude is positive east and negative west.
      The arguments should be within the bounds of -pi <= lon <= pi.
      The behavior of this function with arguments outside this range is left as
      an exercise to the gentle reader ... */
  void SetLongitude(double longitude);

  /** Set the GEOCENTRIC latitude.
      @param latitude GEOCENTRIC latitude in rad to set.
      Sets the latitude of the location represented with this class instance to
      the value of the given argument. The value is meant to be in rad. The
      longitude and the radius value are preserved with this call with the
      exception of radius being equal to zero. If the radius is previously set
      to zero it is changed to be equal to 1.0 past this call.
      Latitude is positive north and negative south.
      The arguments should be within the bounds of -pi/2 <= lat <= pi/2.
      The behavior of this function with arguments outside this range is left as
      an exercise to the gentle reader ... */
  void SetLatitude(double latitude);

  /** Set the distance from the center of the earth.
      @param radius Radius in ft to set.
      Sets the radius of the location represented with this class instance to
      the value of the given argument. The value is meant to be in ft. The
      latitude and longitude values are preserved with this call with the
      exception of radius being equal to zero. If the radius is previously set
      to zero, latitude and longitude is set equal to zero past this call.
      The argument should be positive.
      The behavior of this function called with a negative argument is left as
      an exercise to the gentle reader ... */
  void SetRadius(double radius);

  /** Sets the longitude, latitude and the distance from the center of the earth.
      @param lon longitude in radians
      @param lat GEOCENTRIC latitude in radians
      @param radius distance from center of earth to vehicle in feet*/
  void SetPosition(double lon, double lat, double radius);

  /** Sets the longitude, latitude and the distance above the reference spheroid.
      @param lon longitude in radians
      @param lat GEODETIC latitude in radians
      @param height distance above the reference ellipsoid to vehicle in feet*/
  void SetPositionGeodetic(double lon, double lat, double height);

  /** Sets the semimajor and semiminor axis lengths for this planet.
      The eccentricity and flattening are calculated from the semimajor
      and semiminor axis lengths.
      @param semimajor planet semi-major axis in ft.
      @param semiminor planet semi-minor axis in ft.*/
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

  /** Get the GEOCENTRIC latitude in radians.
      @return the geocentric latitude in rad of the location represented with
      this class instance. The returned values are in the range between
      -pi/2 <= lon <= pi/2. Latitude is positive north and negative south. */
  double GetLatitude() const { ComputeDerived(); return mLat; }

  /** Get the GEODETIC latitude in radians.
      @return the geodetic latitude in rad of the location represented with this
      class instance. The returned values are in the range between
      -pi/2 <= lon <= pi/2. Latitude is positive north and negative south. */
  double GetGeodLatitudeRad(void) const {
    assert(mEllipseSet);
    ComputeDerived(); return mGeodLat;
  }

  /** Get the GEOCENTRIC latitude in degrees.
      @return the geocentric latitude in deg of the location represented with
      this class instance. The returned value is in the range between
      -90 <= lon <= 90. Latitude is positive north and negative south. */
  double GetLatitudeDeg() const { ComputeDerived(); return radtodeg*mLat; }

  /** Get the GEODETIC latitude in degrees.
      @return the geodetic latitude in degrees of the location represented by
      this class instance. The returned value is in the range between
      -90 <= lon <= 90. Latitude is positive north and negative south. */
  double GetGeodLatitudeDeg(void) const {
    assert(mEllipseSet);
    ComputeDerived(); return radtodeg*mGeodLat;
  }

  /** Gets the geodetic altitude in feet. */
  double GetGeodAltitude(void) const {
    assert(mEllipseSet);
    ComputeDerived(); return GeodeticAltitude;
  }

  /** Get the sea level radius in feet below the current location. */
  double GetSeaLevelRadius(void) const;

  /** Get the distance from the center of the earth in feet.
      @return the distance of the location represented with this class
      instance to the center of the earth in ft. The radius value is
      always positive. */
  double GetRadius() const { ComputeDerived(); return mRadius; }

  /** Transform matrix from local horizontal to earth centered frame.
      @return a const reference to the rotation matrix of the transform from
      the local horizontal frame to the earth centered frame. */
  const FGMatrix33& GetTl2ec(void) const { ComputeDerived(); return mTl2ec; }

  /** Transform matrix from the earth centered to local horizontal frame.
      @return a const reference to the rotation matrix of the transform from
      the earth centered frame to the local horizontal frame. */
  const FGMatrix33& GetTec2l(void) const { ComputeDerived(); return mTec2l; }

  /** Get the geodetic distance between the current location and a given
      location. This corresponds to the shortest distance between the two
      locations. Earth curvature is taken into account.
      @param target_longitude the target longitude in radians
      @param target_latitude the target latitude in radians
      @return The geodetic distance between the two locations */
  double GetDistanceTo(double target_longitude, double target_latitude) const;
  
  /** Get the heading that should be followed from the current location to
      a given location along the shortest path. Earth curvature is taken into
      account.
      @param target_longitude the target longitude in radians
      @param target_latitude the target latitude in radians
      @return The heading that should be followed to reach the targeted
              location along the shortest path */
  double GetHeadingTo(double target_longitude, double target_latitude) const;

  /** Conversion from Local frame coordinates to a location in the
      earth centered and fixed frame.
      This function calculates the FGLocation of an object which position
      relative to the vehicle is given as in input.
      @param lvec Vector in the local horizontal coordinate frame
      @return The location in the earth centered and fixed frame */
  FGLocation LocalToLocation(const FGColumnVector3& lvec) const {
    ComputeDerived(); return mTl2ec*lvec + mECLoc;
  }

  /** Conversion from a location in the earth centered and fixed frame
      to local horizontal frame coordinates.
      This function calculates the relative position between the vehicle and
      the input vector and returns the result expressed in the local frame.
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
  double operator()(unsigned int idx) const { return mECLoc.Entry(idx); }

  /** Write access the entries of the vector.
      @param idx the component index.
      @return a reference to the vector entry at the given index.
      Indices are counted starting with 1.
      Note that the index given in the argument is unchecked. */
  double& operator()(unsigned int idx) { mCacheValid = false; return mECLoc.Entry(idx); }

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

  /** Sets this location via the supplied vector.
      The location can be set by an Earth-centered, Earth-fixed (ECEF) frame
      position vector. The cache is marked as invalid, so any future requests
      for selected important data will cause the parameters to be calculated.
      @param v the ECEF column vector in feet.
      @return a reference to the FGLocation object. */
  const FGLocation& operator=(const FGColumnVector3& v)
  {
    mECLoc(eX) = v(eX);
    mECLoc(eY) = v(eY);
    mECLoc(eZ) = v(eZ);
    mCacheValid = false;
    //ComputeDerived();
    return *this;
  }

  /** Sets this location via the supplied location object.
      @param l A location object reference.
      @return a reference to the FGLocation object. */
  FGLocation& operator=(const FGLocation& l);

  /** This operator returns true if the ECEF location vectors for the two
      location objects are equal. */
  bool operator==(const FGLocation& l) const {
    return mECLoc == l.mECLoc;
  }

  /** This operator returns true if the ECEF location vectors for the two
      location objects are not equal. */
  bool operator!=(const FGLocation& l) const { return ! operator==(l); }

  /** This operator adds the ECEF position vectors.
      The cartesian coordinates of the supplied vector (right side) are added to
      the ECEF position vector on the left side of the equality, and a reference
      to this object is returned. */
  const FGLocation& operator+=(const FGLocation &l) {
    mCacheValid = false;
    mECLoc += l.mECLoc;
    return *this;
  }

  /** This operator substracts the ECEF position vectors.
      The cartesian coordinates of the supplied vector (right side) are
      substracted from the ECEF position vector on the left side of the
      equality, and a reference to this object is returned. */
  const FGLocation& operator-=(const FGLocation &l) {
    mCacheValid = false;
    mECLoc -= l.mECLoc;
    return *this;
  }

  /** This operator scales the ECEF position vector.
      The cartesian coordinates of the ECEF position vector on the left side of
      the equality are scaled by the supplied value (right side), and a
      reference to this object is returned. */
  const FGLocation& operator*=(double scalar) {
    mCacheValid = false;
    mECLoc *= scalar;
    return *this;
  }

  /** This operator scales the ECEF position vector.
      The cartesian coordinates of the ECEF position vector on the left side of
      the equality are scaled by the inverse of the supplied value (right side),
      and a reference to this object is returned. */
  const FGLocation& operator/=(double scalar) {
    return operator*=(1.0/scalar);
  }

  /** This operator adds two ECEF position vectors.
      A new object is returned that defines a position which is the sum of the
      cartesian coordinates of the two positions provided. */
  FGLocation operator+(const FGLocation& l) const {
    FGLocation result(mECLoc + l.mECLoc);
    if (mEllipseSet) result.SetEllipse(a, ec*a);
    return result;
  }

  /** This operator substracts two ECEF position vectors.
      A new object is returned that defines a position which is the difference
      of the cartesian coordinates of the two positions provided. */
  FGLocation operator-(const FGLocation& l) const {
    FGLocation result(mECLoc - l.mECLoc);
    if (mEllipseSet) result.SetEllipse(a, ec*a);
    return result;
  }

  /** This operator scales an ECEF position vector.
      A new object is returned that defines a position made of the cartesian
      coordinates of the provided ECEF position scaled by the supplied scalar
      value. */
  FGLocation operator*(double scalar) const {
    FGLocation result(scalar*mECLoc);
    if (mEllipseSet) result.SetEllipse(a, ec*a);
    return result;
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

  /** The cached rotation matrices from and to the associated frames. */
  mutable FGMatrix33 mTl2ec;
  mutable FGMatrix33 mTec2l;

  /* Terms for geodetic latitude calculation. Values are from WGS84 model */
  double a;    // Earth semimajor axis in feet
  double e2;   // Earth eccentricity squared
  double c;
  double ec;
  double ec2;

  /** A data validity flag.
      This class implements caching of the derived values like the
      orthogonal rotation matrices or the lon/lat/radius values. For caching we
      carry a flag which signals if the values are valid or not.
      The C++ keyword "mutable" tells the compiler that the data member is
      allowed to change during a const member function. */
  mutable bool mCacheValid;
  // Flag that checks that geodetic methods are called after SetEllipse() has
  // been called.
  bool mEllipseSet = false;
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
