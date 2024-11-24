#include <limits>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <initialization/FGInitialCondition.h>
#include "TestAssertions.h"

using namespace JSBSim;

const double epsilon = 100. * std::numeric_limits<double>::epsilon();
const FGColumnVector3 zero {0.0, 0.0, 0.0};
constexpr double ktstofps = 1852./(3600*0.3048);

class FGInitialConditionTest : public CxxTest::TestSuite
{
public:
  void testDefaultConstructor() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    TS_ASSERT_EQUALS(ic.GetLatitudeDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetLatitudeRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetLongitudeDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetLongitudeRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetGeodLatitudeDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetGeodLatitudeRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetThetaDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetThetaRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetPhiDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetPhiRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetPsiDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetPsiRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetAltitudeASLFtIC(), 0.0);
#ifdef __arm64__
    TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC(), 0.0, 1E-8);
    TS_ASSERT_DELTA(ic.GetTerrainElevationFtIC(), 0.0, 1E-8);
#else
    TS_ASSERT_EQUALS(ic.GetAltitudeAGLFtIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetTerrainElevationFtIC(), 0.0);
#endif
    TS_ASSERT_EQUALS(ic.GetEarthPositionAngleIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVcalibratedKtsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVequivalentKtsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVgroundFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVtrueFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetMachIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetClimbRateFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetFlightPathAngleDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetFlightPathAngleRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetAlphaDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetAlphaRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetBetaDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetBetaDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetBetaRadIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindMagFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindDirDegIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindUFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindVFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindWFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindNFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindEFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWindDFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetUBodyFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVBodyFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetWBodyFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVNorthFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVEastFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetVDownFpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetPRadpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetQRadpsIC(), 0.0);
    TS_ASSERT_EQUALS(ic.GetRRadpsIC(), 0.0);
    TS_ASSERT_VECTOR_EQUALS(ic.GetWindNEDFpsIC(), zero);
    TS_ASSERT_VECTOR_EQUALS(ic.GetUVWFpsIC(), zero);
    TS_ASSERT_VECTOR_EQUALS(ic.GetPQRRadpsIC(), zero);
  }

  void testSetPositionASL() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      ic.SetLongitudeDegIC(lon);

      // Altitude first, then latitude
      for(double asl=1.; asl <= 1000001.; asl += 10000.) {
        ic.SetAltitudeASLFtIC(asl);
        for(double lat=-90.; lat <=90.; lat += 10.) {
          ic.SetLatitudeDegIC(lat);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // For some reasons, MinGW32 and MSVC are less accurate than other platforms.
#if defined(_MSC_VER) || defined(__MINGW32__)
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 4E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 4E-8);
#else
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 2E-8);
#endif
          TS_ASSERT_DELTA(ic.GetLatitudeDegIC(), lat, epsilon*10.);
          TS_ASSERT_DELTA(ic.GetLatitudeRadIC(), lat*M_PI/180., epsilon);
        }
      }

      // Latitude first, then altitude
      for(double lat=-90.; lat <=90.; lat += 10.) {
        ic.SetLatitudeDegIC(lat);
        for(double asl=1.; asl <= 1000001.; asl += 10000.) {
          ic.SetAltitudeASLFtIC(asl);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetLatitudeDegIC(), lat, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLatitudeRadIC(), lat*M_PI/180., epsilon);
        }
      }
    }
  }

  void testSetPositionAGL() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    ic.SetTerrainElevationFtIC(2000.);

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      ic.SetLongitudeDegIC(lon);

      // Altitude first, then latitude
      for(double agl=1.; agl <= 1000001.; agl += 10000.) {
        ic.SetAltitudeAGLFtIC(agl);
        for(double lat=-90.; lat <=90.; lat += 10.) {
          ic.SetLatitudeDegIC(lat);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/(agl+2000.), 1.0, 2E-8);
          // For some reasons, MinGW32, MSVC and MacOS are less accurate than Linux.
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__APPLE__)
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 4E-8);
#else
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 2E-8);
#endif
          TS_ASSERT_DELTA(ic.GetLatitudeDegIC(), lat, epsilon*10.);
          TS_ASSERT_DELTA(ic.GetLatitudeRadIC(), lat*M_PI/180., epsilon);
        }

        ic.SetAltitudeAGLFtIC(-2000.);
        for(double lat=-90.; lat <=90.; lat += 10.) {
          ic.SetLatitudeDegIC(lat);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC(), 0.0, 3E-8);
          // For some reasons, MinGW32 is less accurate than other platforms.
#ifdef __MINGW32__
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/2000., -1.0, 4E-8);
#else
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/2000., -1.0, 2E-8);
#endif
          TS_ASSERT_DELTA(ic.GetLatitudeDegIC(), lat, epsilon*10.);
          TS_ASSERT_DELTA(ic.GetLatitudeRadIC(), lat*M_PI/180., epsilon);
        }
      }

      // Latitude first, then altitude
      for(double lat=-90.; lat <=90.; lat += 10.) {
        ic.SetLatitudeDegIC(lat);
        for(double agl=1.; agl <= 1000001.; agl += 10000.) {
          ic.SetAltitudeAGLFtIC(agl);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/(agl+2000.), 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetLatitudeDegIC(), lat, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLatitudeRadIC(), lat*M_PI/180., epsilon);
        }
      }
    }
  }

  void testSetGeodeticLatitudeAndASL() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      ic.SetLongitudeDegIC(lon);

      // Altitude first, then latitude
      for(double asl=1.; asl <= 1000001.; asl += 10000.) {
        ic.SetAltitudeASLFtIC(asl);
        for(double lat=-90.; lat <=90.; lat += 10.) {
          ic.SetGeodLatitudeDegIC(lat);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // For some reasons, MinGW32 and MSVC are less accurate than other platforms.
#if defined(_MSC_VER) || defined(__MINGW32__)
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 4E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 4E-8);
#else
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 2E-8);
#endif
          TS_ASSERT_DELTA(ic.GetGeodLatitudeDegIC(), lat, epsilon*1000.);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeRadIC(), lat*M_PI/180., epsilon*10.);
        }
      }

      // Latitude first, then altitude
      for(double lat=-90.; lat <=90.; lat += 10.) {
        ic.SetGeodLatitudeDegIC(lat);
        for(double asl=1.; asl <= 1000001.; asl += 10000.) {
          ic.SetAltitudeASLFtIC(asl);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          // TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/asl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeDegIC(), lat, 1E-9);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeRadIC(), lat*M_PI/180., epsilon*1000.);
        }
      }
    }
  }

  void testSetGeodeticLatitudeAndAGL() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    for(double lon=-180.; lon <= 180.; lon += 30.) {
      ic.SetLongitudeDegIC(lon);

      // Altitude first, then latitude
      for(double agl=1.; agl <= 1000001.; agl += 10000.) {
        ic.SetAltitudeAGLFtIC(agl);
        for(double lat=-90.; lat <=90.; lat += 10.) {
          ic.SetGeodLatitudeDegIC(lat);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // For some reasons, MinGW32 and MSVC are less accurate than other platforms.
#if defined(_MSC_VER) || defined(__MINGW32__)
          // TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 4E-8);
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 4E-8);
#else
          // TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 4E-8);
#endif
          TS_ASSERT_DELTA(ic.GetGeodLatitudeDegIC(), lat, epsilon*1000.);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeRadIC(), lat*M_PI/180., epsilon*10.);
        }
      }

      // Latitude first, then altitude
      for(double lat=-90.; lat <=90.; lat += 10.) {
        ic.SetGeodLatitudeDegIC(lat);
        for(double agl=1.; agl <= 1000001.; agl += 10000.) {
          ic.SetAltitudeAGLFtIC(agl);

          TS_ASSERT_DELTA(ic.GetLongitudeDegIC(), lon, epsilon*100.);
          TS_ASSERT_DELTA(ic.GetLongitudeRadIC(), lon*M_PI/180., epsilon);
          // TS_ASSERT_DELTA(ic.GetAltitudeASLFtIC()/asl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetAltitudeAGLFtIC()/agl, 1.0, 2E-8);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeDegIC(), lat, 1E-9);
          TS_ASSERT_DELTA(ic.GetGeodLatitudeRadIC(), lat*M_PI/180., epsilon*1000.);
        }
      }
    }
  }

  void testBodyVelocity() {
    using namespace std;

    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    ic.SetUBodyFpsIC(100.);
    TS_ASSERT_DELTA(ic.GetUBodyFpsIC(), 100., epsilon);
    TS_ASSERT_DELTA(ic.GetVBodyFpsIC(), 0., epsilon);
    TS_ASSERT_DELTA(ic.GetWBodyFpsIC(), 0., epsilon);
    TS_ASSERT_DELTA(ic.GetVtrueFpsIC(), 100., epsilon);
    TS_ASSERT_DELTA(ic.GetVgroundFpsIC(), 100., epsilon);
    TS_ASSERT_DELTA(ic.GetAlphaDegIC(), 0.0, epsilon);
    TS_ASSERT_DELTA(ic.GetBetaDegIC(), 0.0, epsilon);

    for(double theta=-90.; theta <= 90.; theta+=10.) {
      ic.SetThetaDegIC(theta);

      TS_ASSERT_DELTA(ic.GetUBodyFpsIC(), 100., epsilon*10.);
      TS_ASSERT_DELTA(ic.GetVBodyFpsIC(), 0., epsilon);
      TS_ASSERT_DELTA(ic.GetWBodyFpsIC(), 0., epsilon);
#ifdef __arm64__
      TS_ASSERT_DELTA(ic.GetVNorthFpsIC(), 100.*cos(theta*M_PI/180.), epsilon*10.);
      TS_ASSERT_DELTA(ic.GetVgroundFpsIC(), abs(100.*cos(theta*M_PI/180.)),
                      epsilon*10.);
#else
      TS_ASSERT_DELTA(ic.GetVNorthFpsIC(), 100.*cos(theta*M_PI/180.), epsilon);
      TS_ASSERT_DELTA(ic.GetVgroundFpsIC(), abs(100.*cos(theta*M_PI/180.)),
                      epsilon);
#endif
      TS_ASSERT_DELTA(ic.GetVEastFpsIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetVDownFpsIC(), -100.*sin(theta*M_PI/180.),
                      epsilon*10.);
      TS_ASSERT_DELTA(ic.GetAlphaDegIC(), 0.0, epsilon*10.);
      TS_ASSERT_DELTA(ic.GetBetaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetVtrueFpsIC(), 100., epsilon*10.);
      TS_ASSERT_DELTA(ic.GetPhiDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetThetaDegIC(), theta, epsilon*10.);
      TS_ASSERT_DELTA(ic.GetPsiDegIC(), 0.0, epsilon);
    }

    ic.SetThetaRadIC(0.0);
    for(double phi=-180.; phi <= 180.; phi+=10.) {
      ic.SetPhiDegIC(phi);

      TS_ASSERT_DELTA(ic.GetUBodyFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetVBodyFpsIC(), 0., epsilon);
      TS_ASSERT_DELTA(ic.GetWBodyFpsIC(), 0., epsilon);
      TS_ASSERT_DELTA(ic.GetVtrueFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetVgroundFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetAlphaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetBetaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetPhiDegIC(), phi, epsilon);
      TS_ASSERT_DELTA(ic.GetThetaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetPsiDegIC(), 0.0, epsilon);
    }

    ic.SetPhiDegIC(0.0);
    for(double psi=0.; psi <= 360.; psi+=10.) {
      ic.SetPsiDegIC(psi);

      TS_ASSERT_DELTA(ic.GetUBodyFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetVBodyFpsIC(), 0., epsilon*10.);
      TS_ASSERT_DELTA(ic.GetWBodyFpsIC(), 0., epsilon);
      TS_ASSERT_DELTA(ic.GetVtrueFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetVgroundFpsIC(), 100., epsilon*100.);
      TS_ASSERT_DELTA(ic.GetAlphaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetBetaDegIC(), 0.0, epsilon*10.);
      TS_ASSERT_DELTA(ic.GetPhiDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetThetaDegIC(), 0.0, epsilon);
      TS_ASSERT_DELTA(ic.GetPsiDegIC(), psi, epsilon*10.);
    }
  }

  void testWindVelocity() {
    FGFDMExec fdmex;
    FGInitialCondition ic(&fdmex);

    ic.SetWindDownKtsIC(1.0);
    TS_ASSERT_DELTA(ic.GetWindDFpsIC(), ktstofps, epsilon);

    ic.SetWindNEDFpsIC(1.0, 2.0, 3.0);
    TS_ASSERT_VECTOR_EQUALS(ic.GetWindNEDFpsIC(), FGColumnVector3(1.0, 2.0, 3.0));
    TS_ASSERT_DELTA(ic.GetWindNFpsIC(), 1.0, epsilon);
    TS_ASSERT_DELTA(ic.GetWindEFpsIC(), 2.0, epsilon);
    TS_ASSERT_DELTA(ic.GetWindDFpsIC(), 3.0, epsilon);
    TS_ASSERT_DELTA(ic.GetWindMagFpsIC(), sqrt(5.0), epsilon);
    TS_ASSERT_DELTA(ic.GetWindDirDegIC(), atan2(2.0, 1.0)*180./M_PI, epsilon);

    double mag = ic.GetWindMagFpsIC();
    ic.SetWindDirDegIC(30.);
    TS_ASSERT_DELTA(ic.GetWindNFpsIC(), 0.5*mag*sqrt(3.0), epsilon);
    TS_ASSERT_DELTA(ic.GetWindEFpsIC(), 0.5*mag, epsilon);
    TS_ASSERT_DELTA(ic.GetWindDFpsIC(), 3.0, epsilon);

    ic.SetWindMagKtsIC(7.0);
    TS_ASSERT_DELTA(ic.GetWindNFpsIC(), 3.5*sqrt(3.0)*ktstofps, epsilon);
    TS_ASSERT_DELTA(ic.GetWindEFpsIC(), 3.5*ktstofps, epsilon);
    TS_ASSERT_DELTA(ic.GetWindDFpsIC(), 3.0, epsilon);

    ic.SetWindMagFpsIC(7.0);
    TS_ASSERT_DELTA(ic.GetWindNFpsIC(), 3.5 * sqrt(3.0), epsilon);
    TS_ASSERT_DELTA(ic.GetWindEFpsIC(), 3.5, epsilon);
    TS_ASSERT_DELTA(ic.GetWindDFpsIC(), 3.0, epsilon);
  }
};
