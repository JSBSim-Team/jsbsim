#include <limits>
#include <cxxtest/TestSuite.h>
#include <math/FGLocation.h>
#include <math/FGQuaternion.h>
#include "TestAssertions.h"

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

double NormalizedAngle(double angle) {
  if (angle > M_PI) angle -= 2.0*M_PI;
  if (angle <= -M_PI) angle += 2.0*M_PI;
  return angle;
}

void CheckLocation(const JSBSim::FGLocation& loc,
                   JSBSim::FGColumnVector3 vec) {
  const JSBSim::FGQuaternion qloc(2, -0.5*M_PI);
  JSBSim::FGQuaternion q;
  double r = vec.Magnitude();

  TS_ASSERT_DELTA(vec(1), loc(1), r*epsilon);
  TS_ASSERT_DELTA(vec(2), loc(2), r*epsilon);
  TS_ASSERT_DELTA(vec(3), loc(3), r*epsilon);
  TS_ASSERT_DELTA(r, loc.GetRadius(), r*epsilon);

  vec.Normalize();
  double lon = atan2(vec(2), vec(1));
  double lat = asin(vec(3));

  TS_ASSERT_DELTA(lon, loc.GetLongitude(), epsilon);
  TS_ASSERT_DELTA(lat, loc.GetLatitude(), epsilon);
  TS_ASSERT_DELTA(sin(lon), loc.GetSinLongitude(), epsilon);
  TS_ASSERT_DELTA(cos(lon), loc.GetCosLongitude(), epsilon);
  TS_ASSERT_DELTA(sin(lat), loc.GetSinLatitude(), epsilon);
  TS_ASSERT_DELTA(cos(lat), loc.GetCosLatitude(), epsilon);
  TS_ASSERT_DELTA(tan(lat), loc.GetTanLatitude(), epsilon);

  q = JSBSim::FGQuaternion(0., -lat, lon);
  JSBSim::FGMatrix33 m = (q * qloc).GetT();
  TS_ASSERT_MATRIX_EQUALS(m, loc.GetTec2l());
  TS_ASSERT_MATRIX_EQUALS(m.Transposed(), loc.GetTl2ec());
}

class  FGLocationTest : public CxxTest::TestSuite
{
public:
  void testConstructors() {
    JSBSim::FGLocation l0;
    TS_ASSERT_EQUALS(1.0, l0(1));
    TS_ASSERT_EQUALS(0.0, l0(2));
    TS_ASSERT_EQUALS(0.0, l0(3));
    TS_ASSERT_EQUALS(1.0, l0.Entry(1));
    TS_ASSERT_EQUALS(0.0, l0.Entry(2));
    TS_ASSERT_EQUALS(0.0, l0.Entry(3));
    TS_ASSERT_EQUALS(0.0, l0.GetLongitude());
    TS_ASSERT_EQUALS(0.0, l0.GetLatitude());
    TS_ASSERT_EQUALS(0.0, l0.GetLongitudeDeg());
    TS_ASSERT_EQUALS(0.0, l0.GetLatitudeDeg());
    TS_ASSERT_EQUALS(1.0, l0.GetRadius());
    TS_ASSERT_EQUALS(0.0, l0.GetSinLongitude());
    TS_ASSERT_EQUALS(1.0, l0.GetCosLongitude());
    TS_ASSERT_EQUALS(0.0, l0.GetSinLatitude());
    TS_ASSERT_EQUALS(1.0, l0.GetCosLatitude());
    TS_ASSERT_EQUALS(0.0, l0.GetTanLatitude());

    l0.SetEllipse(1., 1.);
    TS_ASSERT_EQUALS(0.0, l0.GetGeodLatitudeRad());
    TS_ASSERT_EQUALS(0.0, l0.GetGeodLatitudeDeg());
    TS_ASSERT_EQUALS(0.0, l0.GetGeodAltitude());

    double lat = -0.25*M_PI;
    double lon = M_PI/6.0;
    JSBSim::FGLocation l(lon, lat, 1.0);
    TS_ASSERT_DELTA(lon, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(lat, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetRadius(), epsilon);
    TS_ASSERT_DELTA(30.0, l.GetLongitudeDeg(), epsilon);
    TS_ASSERT_DELTA(-45.0, l.GetLatitudeDeg(), epsilon);
    TS_ASSERT_DELTA(0.5, l.GetSinLongitude(), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(3.0), l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(-0.5*sqrt(2.0), l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(2.0), l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(-1.0, l.GetTanLatitude(), epsilon);

    l.SetEllipse(1., 1.);
    TS_ASSERT_EQUALS(lat, l.GetGeodLatitudeRad());
    TS_ASSERT_EQUALS(-45.0, l.GetGeodLatitudeDeg());
    TS_ASSERT_DELTA(0.0, l.GetGeodAltitude(), epsilon);

    const JSBSim::FGQuaternion qloc(2, -0.5*M_PI);
    JSBSim::FGQuaternion q(0., -lat, lon);
    JSBSim::FGMatrix33 m = (q * qloc).GetT();
    TS_ASSERT_MATRIX_EQUALS(m, l.GetTec2l());
    TS_ASSERT_MATRIX_EQUALS(m.Transposed(), l.GetTl2ec());

    JSBSim::FGColumnVector3 v(1.,0.,1.);
    JSBSim::FGLocation lv1(v);
    TS_ASSERT_EQUALS(v(1), lv1(1));
    TS_ASSERT_EQUALS(v(2), lv1(2));
    TS_ASSERT_EQUALS(v(3), lv1(3));
    TS_ASSERT_DELTA(0.0, lv1.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.25*M_PI, lv1.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(sqrt(2.0), lv1.GetRadius(), epsilon);
    JSBSim::FGQuaternion qlat(2, -lv1.GetLatitude());
    m = (qlat * qloc).GetT();
    TS_ASSERT_MATRIX_EQUALS(m, lv1.GetTec2l());
    TS_ASSERT_MATRIX_EQUALS(m.Transposed(), lv1.GetTl2ec());

    v = JSBSim::FGColumnVector3(1.,1.,0.);
    JSBSim::FGLocation lv2(v);
    TS_ASSERT_EQUALS(v(1), lv2(1));
    TS_ASSERT_EQUALS(v(2), lv2(2));
    TS_ASSERT_EQUALS(v(3), lv2(3));
    TS_ASSERT_DELTA(0.25*M_PI, lv2.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, lv2.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(sqrt(2.0), lv2.GetRadius(), epsilon);
    JSBSim::FGQuaternion qlon(3, lv2.GetLongitude());
    m = (qlon * qloc).GetT();
    TS_ASSERT_MATRIX_EQUALS(m, lv2.GetTec2l());
    TS_ASSERT_MATRIX_EQUALS(m.Transposed(), lv2.GetTl2ec());

    v = JSBSim::FGColumnVector3(1.5,-2.,3.);
    JSBSim::FGLocation lv3(v);

    CheckLocation(lv3, v);
  }

  void testCopyConstructor() {
    JSBSim::FGColumnVector3 v(1.5, -2.0, 3.0);
    JSBSim::FGLocation l(v);
    JSBSim::FGLocation lv(l);

    TS_ASSERT_DELTA(l(1), lv(1), epsilon);
    TS_ASSERT_DELTA(l(2), lv(2), epsilon);
    TS_ASSERT_DELTA(l(3), lv(3), epsilon);

    CheckLocation(l, v);
    CheckLocation(lv, v);

    // Check that FGLocation use a copy of the values contained in the vector v
    // If a value of v is modified, then the FGLocation instances shall not be
    // affected.
    JSBSim::FGColumnVector3 v0 = v;
    v(2) = 1.0;
    TS_ASSERT_DELTA(l(1), lv(1), epsilon);
    TS_ASSERT_DELTA(-2.0, lv(2), epsilon);
    TS_ASSERT_DELTA(1.0, v(2), epsilon);
    TS_ASSERT_DELTA(l(3), lv(3), epsilon);

    CheckLocation(l, v0);
    CheckLocation(lv, v0);

    // Check that the copy 'lv' is not altered if the FGLocation 'l' is modified
    l(2) = 1.0;
    CheckLocation(l, v);
    CheckLocation(lv, v0);

    // Check the copy constructor for an FGLocation with cached values.
    JSBSim::FGLocation lv2(l);

    TS_ASSERT_DELTA(l(1), lv2(1), epsilon);
    TS_ASSERT_DELTA(l(2), lv2(2), epsilon);
    TS_ASSERT_DELTA(l(3), lv2(3), epsilon);

    CheckLocation(lv2, v);
  }

  void testEquality() {
    const JSBSim::FGColumnVector3 v(1.5, -2.0, 3.0);
    JSBSim::FGLocation l(v);
    const JSBSim::FGLocation lv(l);

    TS_ASSERT_EQUALS(l, lv);

    for (unsigned int i=1; i < 4; i++) {
      l = lv;
      l(i) = lv.Entry(i) + 1.0;
      TS_ASSERT_DIFFERS(l, lv);

      for (unsigned int j=1; j < 4; j++) {
        if (i == j)
          l(i) = lv.Entry(i);
        else
          l(j) = lv.Entry(j) + 1.0;
      }

      TS_ASSERT_DIFFERS(l, lv);
    }
  }

  void testAssignment() {
    JSBSim::FGColumnVector3 v(1.5, -2.0, 3.0);
    JSBSim::FGLocation lv(v);
    JSBSim::FGLocation l;

    TS_ASSERT_EQUALS(1.0, l(1));
    TS_ASSERT_EQUALS(0.0, l(2));
    TS_ASSERT_EQUALS(0.0, l(3));

    l = lv;
    TS_ASSERT_EQUALS(l(1), lv(1));
    TS_ASSERT_EQUALS(l(2), lv(2));
    TS_ASSERT_EQUALS(l(3), lv(3));
    CheckLocation(l, v);

    // Make sure that l and lv are distinct copies
    lv(1) = -3.4;
    TS_ASSERT_EQUALS(v(1), l(1));
    TS_ASSERT_EQUALS(v(2), l(2));
    TS_ASSERT_EQUALS(v(3), l(3));
    lv(1) = 1.5;

    for (unsigned int i=1; i < 4; i++) {
      l = lv;
      double x = v(i) + 1.0;
      l.Entry(i) = x;

      for (unsigned int j=1; j < 4; j++) {
        if (i == j) {
          TS_ASSERT_EQUALS(l(i), x);
          TS_ASSERT_EQUALS(l.Entry(i), x);
        }
        else {
          TS_ASSERT_EQUALS(l(j), v(j));
          TS_ASSERT_EQUALS(l.Entry(j), v(j));
        }
      }

      CheckLocation(l, JSBSim::FGColumnVector3(l(1), l(2), l(3)));
    }

    l = v;
    TS_ASSERT_EQUALS(l(1), v(1));
    TS_ASSERT_EQUALS(l(2), v(2));
    TS_ASSERT_EQUALS(l(3), v(3));
    CheckLocation(l, v);

    // Make sure that l and v are distinct copies
    v(2) = -3.4;
    TS_ASSERT_EQUALS(lv(1), l(1));
    TS_ASSERT_EQUALS(lv(2), l(2));
    TS_ASSERT_EQUALS(lv(3), l(3));
    v(2) = -2.0;

    for (unsigned int i=1; i < 4; i++) {
      l = v;
      double x = v(i) + 1.0;
      l(i) = x;

      for (unsigned int j=1; j < 4; j++) {
        if (i == j) {
          TS_ASSERT_EQUALS(l(i), x);
          TS_ASSERT_EQUALS(l.Entry(i), x);
        }
        else {
          TS_ASSERT_EQUALS(l(j), v(j));
          TS_ASSERT_EQUALS(l.Entry(j), v(j));
        }
      }

      CheckLocation(l, JSBSim::FGColumnVector3(l(1), l(2), l(3)));
    }

    // Check the copy assignment operator for an FGLocation with cached values.
    l = v;
    CheckLocation(l, v);

    lv = l;

    TS_ASSERT_DELTA(l(1), lv(1), epsilon);
    TS_ASSERT_DELTA(l(2), lv(2), epsilon);
    TS_ASSERT_DELTA(l(3), lv(3), epsilon);

    CheckLocation(lv, v);
  }

  void testOperations() {
    const JSBSim::FGColumnVector3 v(1.5, -2.0, 3.0);
    const JSBSim::FGLocation l(v);
    JSBSim::FGLocation l2(l);

    l2 += l;

    TS_ASSERT_EQUALS(l2(1), 2.0*l(1));
    TS_ASSERT_EQUALS(l2(2), 2.0*l(2));
    TS_ASSERT_EQUALS(l2(3), 2.0*l(3));
    CheckLocation(l2, 2.0*v);

    JSBSim::FGColumnVector3 v2 = l2;
    TS_ASSERT_VECTOR_EQUALS(v2, 2.0*v);

    l2 -= l;

    TS_ASSERT_EQUALS(l2(1), l(1));
    TS_ASSERT_EQUALS(l2(2), l(2));
    TS_ASSERT_EQUALS(l2(3), l(3));
    CheckLocation(l2, v);

    v2 = l2;
    TS_ASSERT_VECTOR_EQUALS(v2, v);

    l2 *= 3.5;

    TS_ASSERT_EQUALS(l2(1), 3.5*l(1));
    TS_ASSERT_EQUALS(l2(2), 3.5*l(2));
    TS_ASSERT_EQUALS(l2(3), 3.5*l(3));
    CheckLocation(l2, 3.5*v);

    l2 /= 7.0;

    TS_ASSERT_EQUALS(l2(1), 0.5*l(1));
    TS_ASSERT_EQUALS(l2(2), 0.5*l(2));
    TS_ASSERT_EQUALS(l2(3), 0.5*l(3));
    CheckLocation(l2, 0.5*v);

    l2 = l * 2.0;

    TS_ASSERT_EQUALS(l2(1), 2.0*l(1));
    TS_ASSERT_EQUALS(l2(2), 2.0*l(2));
    TS_ASSERT_EQUALS(l2(3), 2.0*l(3));
    CheckLocation(l2, 2.0 * v);

    l2 = 1.5 * l;

    TS_ASSERT_EQUALS(l2(1), 1.5*l(1));
    TS_ASSERT_EQUALS(l2(2), 1.5*l(2));
    TS_ASSERT_EQUALS(l2(3), 1.5*l(3));
    CheckLocation(l2, 1.5*v);

    l2 = 0.7 * l + l;

    TS_ASSERT_EQUALS(l2(1), 1.7*l(1));
    TS_ASSERT_EQUALS(l2(2), 1.7*l(2));
    TS_ASSERT_EQUALS(l2(3), 1.7*l(3));
    CheckLocation(l2, 1.7*v);

    l2 = 0.5 * l - l;

    TS_ASSERT_EQUALS(l2(1), -0.5*l(1));
    TS_ASSERT_EQUALS(l2(2), -0.5*l(2));
    TS_ASSERT_EQUALS(l2(3), -0.5*l(3));
    CheckLocation(l2, -0.5*v);
  }

  void testLocalLocation() {
    const JSBSim::FGColumnVector3 v(1.5, -2.0, 3.0);
    const JSBSim::FGColumnVector3 z(0.0, 0.0, 1.0);
    JSBSim::FGColumnVector3 v0(0.0, 0.0, -1.0);
    JSBSim::FGLocation l(v);

    JSBSim::FGLocation l2 = l.LocalToLocation(v0);
    TS_ASSERT_DELTA(l2.GetRadius(), v.Magnitude() + 1.0, epsilon);
    TS_ASSERT_VECTOR_EQUALS(v * l2, JSBSim::FGColumnVector3());
    TS_ASSERT_VECTOR_EQUALS(l.LocationToLocal(l2), v0);

    JSBSim::FGColumnVector3 east = (z * v).Normalize();
    v0 = JSBSim::FGColumnVector3(0.0, 1.0, 0.0);
    l2 = l.LocalToLocation(v0);
    TS_ASSERT_DELTA(l(3), l2(3), epsilon);
    TS_ASSERT_VECTOR_EQUALS(JSBSim::FGColumnVector3(l2), east+l);
    TS_ASSERT_VECTOR_EQUALS(l.LocationToLocal(l2), v0);

    JSBSim::FGColumnVector3 north = (v * east).Normalize();
    v0 = JSBSim::FGColumnVector3(1.0, 0.0, 0.0);
    l2 = l.LocalToLocation(v0);
    TS_ASSERT_VECTOR_EQUALS(JSBSim::FGColumnVector3(l2), north+l);
    TS_ASSERT_VECTOR_EQUALS(l.LocationToLocal(l2), v0);

    JSBSim::FGColumnVector3 down = (-1.0*v).Normalize();
    v0 = JSBSim::FGColumnVector3(1.0, 2.1, -0.5);
    l2 = l.LocalToLocation(v0);
    TS_ASSERT_VECTOR_EQUALS(JSBSim::FGColumnVector3(l2),
                            v0(1)*north+v0(2)*east+v0(3)*down+l);
    TS_ASSERT_VECTOR_EQUALS(l.LocationToLocal(l2), v0);
  }

  void testPosition() {
    const JSBSim::FGQuaternion qloc(2, -0.5*M_PI);
    JSBSim::FGLocation l;
    JSBSim::FGQuaternion q;
    JSBSim::FGMatrix33 m;
    JSBSim::FGColumnVector3 v;

    for (int ilat=-5; ilat <=5; ilat++) {
      l.SetRadius(1.0);
      TS_ASSERT_DELTA(1.0, l.GetRadius(), epsilon);
      double lat = ilat*M_PI/12.0;
      l.SetLatitude(lat);
      TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
      TS_ASSERT_DELTA(lat, l.GetLatitude(), epsilon);
      TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);
      TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
      TS_ASSERT_DELTA(sin(lat), l.GetSinLatitude(), epsilon);
      TS_ASSERT_DELTA(cos(lat), l.GetCosLatitude(), epsilon);
      TS_ASSERT_DELTA(tan(lat), l.GetTanLatitude(), epsilon);

      q = JSBSim::FGQuaternion(0.0, -lat, 0.0);
      m = (q * qloc).GetT();
      TS_ASSERT_MATRIX_EQUALS(m, l.GetTec2l());
      TS_ASSERT_MATRIX_EQUALS(m.Transposed(), l.GetTl2ec());

      for (unsigned int ilon=0; ilon < 12; ilon++) {
        double r = ilon + 1;
        double lon = NormalizedAngle(ilon*M_PI/6.0);
        l.SetLongitude(lon);
        TS_ASSERT_DELTA(lon, l.GetLongitude(), epsilon);
        TS_ASSERT_DELTA(lat, l.GetLatitude(), epsilon);
        TS_ASSERT_DELTA(sin(lon), l.GetSinLongitude(), epsilon);
        TS_ASSERT_DELTA(cos(lon), l.GetCosLongitude(), epsilon);
        TS_ASSERT_DELTA(sin(lat), l.GetSinLatitude(), epsilon);
        TS_ASSERT_DELTA(cos(lat), l.GetCosLatitude(), epsilon);
        TS_ASSERT_DELTA(tan(lat), l.GetTanLatitude(), epsilon);

        q = JSBSim::FGQuaternion(0.0, -lat, lon);
        m = (q * qloc).GetT();
        TS_ASSERT_MATRIX_EQUALS(m, l.GetTec2l());
        TS_ASSERT_MATRIX_EQUALS(m.Transposed(), l.GetTl2ec());

        l.SetRadius(r);
        TS_ASSERT_DELTA(r, l.GetRadius(), epsilon);
        v = m.Transposed() * JSBSim::FGColumnVector3(0.0, 0.0, -r);
        TS_ASSERT_VECTOR_EQUALS(v, JSBSim::FGColumnVector3(l));
      }

      l.SetLongitude(0.0);
    }

    for (int ilat=-5; ilat <=5; ilat++) {
      double lat = ilat*M_PI/12.0;
      for (unsigned int ilon=0; ilon < 12; ilon++) {
        double r = ilon + 1;
        double lon = NormalizedAngle(ilon*M_PI/6.0);

        l.SetPosition(lon, lat, r);
        TS_ASSERT_DELTA(lon, l.GetLongitude(), epsilon);
        TS_ASSERT_DELTA(lat, l.GetLatitude(), epsilon);
        TS_ASSERT_DELTA(sin(lon), l.GetSinLongitude(), epsilon);
        TS_ASSERT_DELTA(cos(lon), l.GetCosLongitude(), epsilon);
        TS_ASSERT_DELTA(sin(lat), l.GetSinLatitude(), epsilon);
        TS_ASSERT_DELTA(cos(lat), l.GetCosLatitude(), epsilon);
        TS_ASSERT_DELTA(tan(lat), l.GetTanLatitude(), epsilon);

        q = JSBSim::FGQuaternion(0.0, -lat, lon);
        m = (q * qloc).GetT();
        v = m.Transposed() * JSBSim::FGColumnVector3(0.0, 0.0, -r);
        TS_ASSERT_MATRIX_EQUALS(m, l.GetTec2l());
        TS_ASSERT_MATRIX_EQUALS(m.Transposed(), l.GetTl2ec());
        TS_ASSERT_DELTA(r, l.GetRadius(), epsilon);
        TS_ASSERT_VECTOR_EQUALS(v, JSBSim::FGColumnVector3(l));
      }
    }

    // Check the condition where the location is at the center of the Earth
    v.InitMatrix();
    l = v;
    TS_ASSERT_DELTA(0.0, l.GetRadius(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    l.SetRadius(1.0);
    CheckLocation(l, JSBSim::FGColumnVector3(1., 0., 0.).Normalize());

    l = v;
    l.SetLatitude(M_PI*0.25);
    CheckLocation(l, JSBSim::FGColumnVector3(1., 0., 1.).Normalize());

    l = v;
    l.SetLongitude(M_PI*0.25);
    CheckLocation(l, JSBSim::FGColumnVector3(1., 1., 0.).Normalize());

    // Check the location definition does not depend on the order in which the
    // latitude & longitude are specified
    v(1) = 1.0;

    l = v;
    l.SetLongitude(M_PI/3.0);
    l.SetLatitude(M_PI/6.0);

    JSBSim::FGLocation lbis(v);
    lbis.SetLatitude(M_PI/6.0);
    lbis.SetLongitude(M_PI/3.0);
    TS_ASSERT_DELTA(l(1), lbis(1), epsilon);
    TS_ASSERT_DELTA(l(2), lbis(2), epsilon);
    TS_ASSERT_DELTA(l(3), lbis(3), epsilon);
  }

  void testGeodetic() {
    const double a = 20925646.32546; // WGS84 semimajor axis length in feet
    const double b = 20855486.5951;  // WGS84 semiminor axis length in feet
    JSBSim::FGLocation l;

    l.SetEllipse(a, b);

    for (int ilat=-5; ilat <=5; ilat++) {
      double glat = ilat*M_PI/12.0;
      for (unsigned int ilon=0; ilon < 12; ilon++) {
        double h = ilon + 1.0;
        double lon = NormalizedAngle(ilon*M_PI/6.0);
        double ac = a * cos(glat);
        double bs = b * sin(glat);
        double N = a*a / sqrt(ac*ac + bs*bs);
        l(1) = (N+h)*cos(glat)*cos(lon);
        l(2) = (N+h)*cos(glat)*sin(lon);
        l(3) = (b*b*N/(a*a)+h)*sin(glat);
        TS_ASSERT_DELTA(lon, l.GetLongitude(), epsilon);
        TS_ASSERT_DELTA(sin(lon), l.GetSinLongitude(), epsilon);
        TS_ASSERT_DELTA(cos(lon), l.GetCosLongitude(), epsilon);
        TS_ASSERT_DELTA(glat, l.GetGeodLatitudeRad(), epsilon);
        TS_ASSERT_DELTA(h, l.GetGeodAltitude(), 1E-8);
      }
    }

    for (int ilat=-5; ilat <=5; ilat++) {
      double glat = ilat*M_PI/12.0;
      for (unsigned int ilon=0; ilon < 12; ilon++) {
        double h = ilon + 1.0;
        double lon = NormalizedAngle(ilon*M_PI/6.0);
        double ac = a * cos(glat);
        double bs = b * sin(glat);
        double N = a*a / sqrt(ac*ac + bs*bs);
        double x = (N+h)*cos(glat)*cos(lon);
        double y = (N+h)*cos(glat)*sin(lon);
        double z = (b*b*N/(a*a)+h)*sin(glat);
        l.SetPositionGeodetic(lon, glat, h);
        TS_ASSERT_DELTA(x, l(1), epsilon*fabs(x));
        TS_ASSERT_DELTA(y, l(2), epsilon*fabs(y));
        TS_ASSERT_DELTA(z, l(3), epsilon*fabs(z));
        CheckLocation(l, JSBSim::FGColumnVector3(x, y, z));
        TS_ASSERT_DELTA(lon, l.GetLongitude(), epsilon);
        TS_ASSERT_DELTA(sin(lon), l.GetSinLongitude(), epsilon);
        TS_ASSERT_DELTA(cos(lon), l.GetCosLongitude(), epsilon);
        TS_ASSERT_DELTA(glat, l.GetGeodLatitudeRad(), epsilon);
        TS_ASSERT_DELTA(h, l.GetGeodAltitude(), 1E-8);
      }
    }
  }

  void testPoles() {
    JSBSim::FGColumnVector3 v(0.0, 0.0, 1.0); // North pole
    JSBSim::FGLocation l(v);

    TS_ASSERT_DELTA(M_PI*0.5, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    // Check that SetLongitude is a no-op when applied at the North pole
    l.SetLongitude(M_PI/6.0);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    l.SetLatitude(M_PI/3.0);
    TS_ASSERT_DELTA(M_PI/3.0, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.5, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(3.0), l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(sqrt(3.0), l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    // South Pole
    l = -1.0 * v;
    TS_ASSERT_DELTA(-M_PI*0.5, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(-1.0, l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    // Check that SetLongitude is a no-op when applied at the South pole
    l.SetLongitude(M_PI/6.0);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(-1.0, l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    l.SetLatitude(-M_PI/3.0);
    TS_ASSERT_DELTA(-M_PI/3.0, l.GetLatitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
    TS_ASSERT_DELTA(0.5, l.GetCosLatitude(), epsilon);
    TS_ASSERT_DELTA(-0.5*sqrt(3.0), l.GetSinLatitude(), epsilon);
    TS_ASSERT_DELTA(-sqrt(3.0), l.GetTanLatitude(), epsilon);
    TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);

    // Geodetic calculations next to the North Pole
    const double a = 20925646.32546; // WGS84 semimajor axis length in feet
    const double b = 20855486.5951;  // WGS84 semiminor axis length in feet
    l.SetEllipse(a, b);
    l = b * v;
    TS_ASSERT_DELTA(90.0, l.GetGeodLatitudeDeg(), epsilon);
    TS_ASSERT_DELTA(M_PI*0.5, l.GetGeodLatitudeRad(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetGeodAltitude(), 1E-8);

    // Check locations next to the North Pole
    for (unsigned int i=1; i < 1000; i++) {
      double h = 10.0;
      double glat = 0.5*M_PI-i*1E-9;
      double ac = a * cos(glat);
      double bs = b * sin(glat);
      double N = a*a / sqrt(ac*ac + bs*bs);
      double x = (N+h)*cos(glat);
      double z = (b*b*N/(a*a)+h)*sin(glat);
      l.SetPositionGeodetic(0., glat, h);
      TS_ASSERT_DELTA(x, l(1), epsilon*fabs(x));
      TS_ASSERT_DELTA(0.0, l(2), epsilon);
      TS_ASSERT_DELTA(z, l(3), epsilon*fabs(z));
      TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
      TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);
      TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
      TS_ASSERT_DELTA(glat, l.GetGeodLatitudeRad(), epsilon);
      TS_ASSERT_DELTA(h, l.GetGeodAltitude(), 1E-8);
    }

    // Geodetic calculations next to the South Pole
    l = -b * v;
    TS_ASSERT_DELTA(-90.0, l.GetGeodLatitudeDeg(), epsilon);
    TS_ASSERT_DELTA(-0.5*M_PI, l.GetGeodLatitudeRad(), epsilon);
    TS_ASSERT_DELTA(0.0, l.GetGeodAltitude(), 1E-8);

    // Check locations next to the South Pole
    for (unsigned int i=1; i < 1000; i++) {
      double h = 10.0;
      double glat = -0.5*M_PI+i*1E-9;
      double ac = a * cos(glat);
      double bs = b * sin(glat);
      double N = a*a / sqrt(ac*ac + bs*bs);
      double x = (N+h)*cos(glat);
      double z = (b*b*N/(a*a)+h)*sin(glat);
      l.SetPositionGeodetic(0., glat, h);
      TS_ASSERT_DELTA(x, l(1), epsilon*fabs(x));
      TS_ASSERT_DELTA(0.0, l(2), epsilon);
      TS_ASSERT_DELTA(z, l(3), epsilon*fabs(z));
      TS_ASSERT_DELTA(0.0, l.GetLongitude(), epsilon);
      TS_ASSERT_DELTA(0.0, l.GetSinLongitude(), epsilon);
      TS_ASSERT_DELTA(1.0, l.GetCosLongitude(), epsilon);
      TS_ASSERT_DELTA(glat, l.GetGeodLatitudeRad(), epsilon);
      TS_ASSERT_DELTA(h, l.GetGeodAltitude(), 1E-8);
    }
  }
};
