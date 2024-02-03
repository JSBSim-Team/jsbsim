#include <limits>
#include <memory>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <initialization/FGInitialCondition.h>
#include <math/FGLocation.h>
#include <models/FGInertial.h>
#include <models/FGPropagate.h>
#include <input_output/FGGroundCallback.h>
#include "TestAssertions.h"

const double epsilon = 100. * std::numeric_limits<double>::epsilon();
const double RadiusReference = 20925646.32546;
const double a = 20925646.32546; // WGS84 semimajor axis length in feet
const double b = 20855486.5951;  // WGS84 semiminor axis length in feet

using namespace JSBSim;

// A class that does not set the ellipse parameters of the 'contact' location
// to check that such a ground callback does not crash JSBSim.
class DummyGroundCallback : public FGDefaultGroundCallback
{
public:
  DummyGroundCallback(double a, double b) : FGDefaultGroundCallback(a, b) {}
  double GetAGLevel(double t, const FGLocation& location,
                    FGLocation& contact,
                    FGColumnVector3& normal, FGColumnVector3& v,
                    FGColumnVector3& w) const override
  {
    FGLocation c;
    double h = FGDefaultGroundCallback::GetAGLevel(t, location, c, normal, v, w);
    // The ellipse parameters are intentionally not copied from c to contact.
    contact(1) = c(1);
    contact(2) = c(2);
    contact(3) = c(3);
    return h;
  }
};

class FGGroundCallbackTest : public CxxTest::TestSuite
{
public:
  void testSphericalEarthSurface() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(RadiusReference,
                                                                     RadiusReference));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc = FGLocation(lon_rad, lat_rad, RadiusReference);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA(0.0, agl, 1e-8);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        FGColumnVector3 vLoc = loc;
        FGColumnVector3 vContact = contact;
        TS_ASSERT_DELTA(vContact.Magnitude(), RadiusReference, epsilon);
        TS_ASSERT_DELTA(vLoc(1), vContact(1), 1e-8);
        TS_ASSERT_DELTA(vLoc(2), vContact(2), 1e-8);
        TS_ASSERT_DELTA(vLoc(3), vContact(3), 1e-8);
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
        vContact.Normalize();
        TS_ASSERT_VECTOR_EQUALS(vContact, normal);
      }
    }
  }

  void testSphericalEarthAltitude() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(RadiusReference,
                                                                     RadiusReference));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};
    double h = 100000.;

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc = FGLocation(lon_rad, lat_rad, RadiusReference+h);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA(h/agl, 1.0, epsilon*100.);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        FGColumnVector3 vLoc = loc;
        FGColumnVector3 vContact = contact;
#ifdef __arm64__
        TS_ASSERT_DELTA(vContact.Magnitude()/RadiusReference, 1.0, epsilon);
#else
        TS_ASSERT_DELTA(vContact.Magnitude(), RadiusReference, epsilon);
#endif
        FGColumnVector3 vtest = vLoc/(1.+h/RadiusReference);
        TS_ASSERT_DELTA(vtest(1), vContact(1), 1e-8);
        TS_ASSERT_DELTA(vtest(2), vContact(2), 1e-8);
        TS_ASSERT_DELTA(vtest(3), vContact(3), 1e-8);
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
        vContact.Normalize();
        TS_ASSERT_VECTOR_EQUALS(vContact, normal);
      }
    }
  }

  void testSphericalEarthAltitudeWithTerrainElevation() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(RadiusReference,
                                                                     RadiusReference));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};
    double h = 100000.;
    double elevation = 2000.;

    cb->SetTerrainElevation(elevation);

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc = FGLocation(lon_rad, lat_rad, RadiusReference+h);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA((h-elevation)/agl, 1.0, epsilon*100.);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        FGColumnVector3 vLoc = loc;
        FGColumnVector3 vContact = contact;
        TS_ASSERT_DELTA(vContact.Magnitude()/(RadiusReference+elevation), 1.0,
                        epsilon);
        TS_ASSERT_VECTOR_EQUALS(vLoc/(RadiusReference+h),
                                vContact/(RadiusReference+elevation));
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
        vContact.Normalize();
        TS_ASSERT_VECTOR_EQUALS(vContact, normal);
      }
    }
  }

  void testWGS84EarthSurface() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(a, b));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};

    loc.SetEllipse(a, b);
    contact.SetEllipse(a, b);

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc.SetPositionGeodetic(lon_rad, lat_rad, 0.0);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA(0.0, agl, 1e-8);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        FGColumnVector3 vLoc = loc;
        FGColumnVector3 vContact = contact;
        TS_ASSERT_DELTA(vLoc(1), vContact(1), 1e-8);
        TS_ASSERT_DELTA(vLoc(2), vContact(2), 1e-8);
        TS_ASSERT_DELTA(vLoc(3), vContact(3), 1e-8);
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
      }
    }
  }

  void testWGS84EarthAltitude() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(a, b));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};
    double h = 100000.;

    loc.SetEllipse(a, b);
    contact.SetEllipse(a, b);

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc.SetPositionGeodetic(lon_rad, lat_rad, h);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA(h, agl, 1e-8);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
        FGColumnVector3 vLoc = loc-h*normal;
        FGColumnVector3 vContact = contact;
        TS_ASSERT_DELTA(vLoc(1), vContact(1), 1e-7);
        TS_ASSERT_DELTA(vLoc(2), vContact(2), 1e-7);
        TS_ASSERT_DELTA(vLoc(3), vContact(3), 1e-7);
      }
    }
  }

  void testWGS84EarthAltitudeWithTerrainElevation() {
    std::unique_ptr<FGGroundCallback> cb(new FGDefaultGroundCallback(a, b));
    FGLocation loc, contact;
    FGColumnVector3 normal, v, w;
    FGColumnVector3 zero {0., 0., 0.};
    double h = 100000.;
    double elevation = 2000.;

    loc.SetEllipse(a, b);
    contact.SetEllipse(a, b);
    cb->SetTerrainElevation(elevation);

    // Check that, for a point located, on the sea level radius the AGL is 0.0
    for(double lat = -90.0; lat <= 90.; lat += 30.) {
      for(double lon = 0.0; lon <=360.; lon += 45.){
        double lon_rad = lon*M_PI/180.;
        double lat_rad = lat*M_PI/180.;
        loc.SetPositionGeodetic(lon_rad, lat_rad, h);
        double agl = cb->GetAGLevel(loc, contact, normal, v, w);
        TS_ASSERT_DELTA(h-elevation, agl, 1e-8);
        TS_ASSERT_VECTOR_EQUALS(v, zero);
        TS_ASSERT_VECTOR_EQUALS(w, zero);
        TS_ASSERT_DELTA(normal(1), cos(lat_rad)*cos(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(2), cos(lat_rad)*sin(lon_rad), epsilon);
        TS_ASSERT_DELTA(normal(3), sin(lat_rad), epsilon);
        FGColumnVector3 vLoc = loc-(h-elevation)*normal;
        FGColumnVector3 vContact = contact;
        TS_ASSERT_DELTA(vLoc(1), vContact(1), 1e-7);
        TS_ASSERT_DELTA(vLoc(2), vContact(2), 1e-7);
        TS_ASSERT_DELTA(vLoc(3), vContact(3), 1e-7);
      }
    }
  }

  // Regression test for FlightGear.
  //
  // Check that JSBSim does not crash (assertion "ellipse not set") when using
  // a ground callback that does not set the ellipse parameters of the 'contact'
  // location in its GetAGLevel method.
  void testGroundCallback() {
    FGFDMExec fdmex;
    auto propagate = fdmex.GetPropagate();
    auto planet = fdmex.GetInertial();
    DummyGroundCallback* cb = new DummyGroundCallback(planet->GetSemimajor(),
                                                      planet->GetSemiminor());
    planet->SetGroundCallback(cb);
    auto IC = fdmex.GetIC();
    TS_ASSERT_DELTA(IC->GetTerrainElevationFtIC(), 0.0, 1E-8);
    TS_ASSERT_DELTA(propagate->GetTerrainElevation(), 0.0, 1E-8);
    FGLocation loc;
    loc.SetEllipse(planet->GetSemimajor(), planet->GetSemiminor());
    planet->SetAltitudeAGL(loc, 1.0);
    TS_ASSERT_DELTA(loc.GetGeodAltitude(), 1.0, 1E-8);
  }
};
