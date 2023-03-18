#include <string>
#include <limits>
#include <cxxtest/TestSuite.h>
#include <FGJSBBase.h>

class FGJSBBaseTest : public CxxTest::TestSuite, public JSBSim::FGJSBBase
{
public:
  void testNumericRoutines() {
    double dx = 1.0;
    float  fx = 1.0;
    double dy = dx + std::numeric_limits<double>::epsilon();
    float  fy = fx + std::numeric_limits<float>::epsilon();
    TS_ASSERT(EqualToRoundoff(dx, dy));
    TS_ASSERT(EqualToRoundoff(dx, fy));
    TS_ASSERT(EqualToRoundoff(fx, fy));
    TS_ASSERT(EqualToRoundoff(fx, dy));
    TS_ASSERT_EQUALS(sign(1.235), 1.0);
    TS_ASSERT_EQUALS(sign(0.0), 1.0);
    TS_ASSERT_EQUALS(sign(-1E-5), -1.0);
    TS_ASSERT_EQUALS(Constrain(0.0,-1E-5,1.0), 0.0);
    TS_ASSERT_EQUALS(Constrain(0.0,  0.5,1.0), 0.5);
    TS_ASSERT_EQUALS(Constrain(0.0, 10.0,1.0), 1.0);
    Filter f0;
    Filter f(1.0, 1E-5);
    double x = f.execute(3.0);
  }

  void testTemperatureConversion() {
    TS_ASSERT(EqualToRoundoff(KelvinToFahrenheit(0.0), -459.4));
    TS_ASSERT(EqualToRoundoff(KelvinToFahrenheit(288.15), 59.27));
    TS_ASSERT(EqualToRoundoff(CelsiusToRankine(0.0), 491.67));
    TS_ASSERT(EqualToRoundoff(CelsiusToRankine(15.0), 518.67));
    TS_ASSERT(EqualToRoundoff(RankineToCelsius(491.67), 0.0));
    TS_ASSERT_DELTA(RankineToCelsius(518.67), 15.0, 1E-8);
    TS_ASSERT(EqualToRoundoff(KelvinToRankine(0.0), 0.0));
    TS_ASSERT(EqualToRoundoff(KelvinToRankine(288.15), 518.67));
    TS_ASSERT(EqualToRoundoff(RankineToKelvin(0.0), 0.0));
    TS_ASSERT_DELTA(RankineToKelvin(518.67), 288.15, 1E-8);
    TS_ASSERT(EqualToRoundoff(CelsiusToFahrenheit(0.0), 32.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToFahrenheit(15.0), 59.0));
    TS_ASSERT(EqualToRoundoff(FahrenheitToCelsius(32.0), 0.0));
    TS_ASSERT_DELTA(FahrenheitToCelsius(59.0), 15.0, 1E-8);
    TS_ASSERT(EqualToRoundoff(KelvinToCelsius(0.0), -273.15));
    TS_ASSERT(EqualToRoundoff(KelvinToCelsius(288.15), 15.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToKelvin(-273.15), 0.0));
    TS_ASSERT(EqualToRoundoff(CelsiusToKelvin(15.0), 288.15));
  }

  void testMisc() {
    std::string version = GetVersion();
    disableHighLighting();
  }

  void testRandomNumberGenerator() {
    JSBSim::RandomNumberGenerator generator(17);

    double u0 = generator.GetUniformRandomNumber();
    double u1 = generator.GetUniformRandomNumber();
    double u2 = generator.GetUniformRandomNumber();

    double x0 = generator.GetNormalRandomNumber();
    double x1 = generator.GetNormalRandomNumber();
    double x2 = generator.GetNormalRandomNumber();

    // Check that the seed() method correctly resets the random number generator
    generator.seed(17);
    double v0 = generator.GetUniformRandomNumber();
    double v1 = generator.GetUniformRandomNumber();
    double v2 = generator.GetUniformRandomNumber();

    double y0 = generator.GetNormalRandomNumber();
    double y1 = generator.GetNormalRandomNumber();
    double y2 = generator.GetNormalRandomNumber();

    TS_ASSERT_EQUALS(u0, v0);
    TS_ASSERT_EQUALS(u1, v1);
    TS_ASSERT_EQUALS(u2, v2);

    TS_ASSERT_EQUALS(x0, y0);
    TS_ASSERT_EQUALS(x1, y1);
    TS_ASSERT_EQUALS(x2, y2);
  }
};
