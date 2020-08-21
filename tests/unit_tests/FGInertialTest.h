#include <limits>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGInertial.h>
#include "TestAssertions.h"

using namespace JSBSim;

const double epsilon = 100. * std::numeric_limits<double>::epsilon();
constexpr double degtorad = M_PI / 180.;

class FGInertialTest : public CxxTest::TestSuite
{
public:
  void testTransformationMatricesSphericalEarth() {
    FGFDMExec fdmex;
    FGInertial* planet = fdmex.GetInertial();
    fdmex.SetPropertyValue("simulation/gravity-model", 0);
    FGLocation loc;
    FGMatrix33 Tl2ec, Tec2l;
    double radius = planet->GetSemimajor();

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      for(double lat=-90.; lat <= 90.; lat += 30.) {
        loc.SetPosition(lon*degtorad, lat*degtorad, radius);
        Tl2ec = planet->GetTl2ec(loc);
        Tec2l = planet->GetTec2l(loc);
        TS_ASSERT_MATRIX_EQUALS(loc.GetTl2ec(), Tl2ec);
        TS_ASSERT_MATRIX_EQUALS(loc.GetTec2l(), Tec2l);
      }
    }
  }
};
