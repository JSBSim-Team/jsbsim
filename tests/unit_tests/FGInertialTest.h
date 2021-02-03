#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGInertial.h>
#include "TestAssertions.h"

using namespace JSBSim;

const double epsilon = 1e-5;
constexpr double degtorad = M_PI / 180.;

class FGInertialTest : public CxxTest::TestSuite
{
public:
  void testTransformationMatricesSphericalEarth() {
    FGFDMExec fdmex;
    auto planet = fdmex.GetInertial();
    fdmex.SetPropertyValue("simulation/gravity-model", 0);
    FGLocation loc;
    FGMatrix33 Tec2l;
    double radius = planet->GetSemimajor();

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      double longitude = lon * degtorad;
      double cosLon = cos(longitude);
      double sinLon = sin(longitude);

      for(double lat=-90.; lat <= 90.; lat += 30.) {
        double latitude = lat * degtorad;
        loc.SetPosition(longitude, latitude, radius);
        double cosLat = cos(latitude);
        double sinLat = sin(latitude);
        Tec2l = { -cosLon*sinLat, -sinLon*sinLat,  cosLat,
                      -sinLon   ,     cosLon    ,    0.0 ,
                  -cosLon*cosLat, -sinLon*cosLat, -sinLat  };
        TS_ASSERT_MATRIX_EQUALS(planet->GetTec2l(loc), Tec2l);
        TS_ASSERT_MATRIX_EQUALS(planet->GetTl2ec(loc), Tec2l.Transposed());
      }
    }
  }

  void testTransformationMatricesWGS84Earth() {
    FGFDMExec fdmex;
    auto planet = fdmex.GetInertial();
    fdmex.SetPropertyValue("simulation/gravity-model", 1);
    FGLocation loc;
    FGMatrix33 Tec2l;

    loc.SetEllipse(planet->GetSemimajor(), planet->GetSemiminor());

    for(double lat=-90.; lat <= 90.; lat += 30.) {
      double latitude = lat * degtorad;
      double cosLat = cos(latitude);
      double sinLat = sin(latitude);

      for(double lon=-180.; lon <= 180.; lon += 30.) {
        double longitude = lon * degtorad;
        loc.SetPositionGeodetic(longitude, latitude, 0.0);
        double cosLon = cos(longitude);
        double sinLon = sin(longitude);
        Tec2l = { -cosLon*sinLat, -sinLon*sinLat,  cosLat,
                      -sinLon   ,     cosLon    ,    0.0 ,
                  -cosLon*cosLat, -sinLon*cosLat, -sinLat  };
        TS_ASSERT_MATRIX_EQUALS(planet->GetTec2l(loc), Tec2l);
        TS_ASSERT_MATRIX_EQUALS(planet->GetTl2ec(loc), Tec2l.Transposed());
      }
    }
  }
};
