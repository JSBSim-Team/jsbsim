#include <limits>
#include <memory>
#include <cxxtest/TestSuite.h>

#include <math/FGLocation.h>
#include <input_output/FGGroundCallback.h>
#include "TestAssertions.h"

const double epsilon = 100. * std::numeric_limits<double>::epsilon();
const double RadiusReference = 20925646.32546;
const double a = 20925646.32546; // WGS84 semimajor axis length in feet
const double b = 20855486.5951;  // WGS84 semiminor axis length in feet

using namespace JSBSim;

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
        TS_ASSERT_DELTA(vContact.Magnitude(), RadiusReference, epsilon);
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
};
