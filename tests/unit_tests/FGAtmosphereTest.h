#include <limits>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGAtmosphere.h>

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

using namespace JSBSim;

class DummyAtmosphere : public FGAtmosphere
{
public:
  DummyAtmosphere(FGFDMExec* fdm, double t_lapse_rate, double p_lapse_rate)
    : FGAtmosphere(fdm), a_t(t_lapse_rate), a_p(p_lapse_rate)
  {}

  using FGAtmosphere::GetTemperature;
  using FGAtmosphere::GetPressure;

  double GetTemperature(double altitude) const override
  {
    return SLtemperature+a_t*altitude;
  }
  void SetTemperature(double t, double h, eTemperature unit) override
  {
    Temperature = ConvertToRankine(t, unit)-a_t*h;
  }
  double GetPressure(double altitude) const override
  {
    return SLpressure+a_p*altitude;
  }
  double GetR(void) const { return Reng; }
  double GetGamma(void) const { return SHRatio; }
  double GetBeta(void) const { return Beta; }
  double GetSutherlandConstant(void) const { return SutherlandConstant; }
private:
  double a_t, a_p;
};

class FGAtmosphereTest : public CxxTest::TestSuite
{
public:
  void testDefaultValuesBeforeInit()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    TS_ASSERT_DELTA(gamma, 1.4, epsilon);

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);

    TS_ASSERT_EQUALS(atm.GetPressureSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressure(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 0.0);

    const double rho = 1.0/R;
    TS_ASSERT_EQUALS(atm.GetDensitySL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetDensity(), 0.0);
    TS_ASSERT_EQUALS(atm.GetDensity(0.0), rho);
    TS_ASSERT_EQUALS(atm.GetDensityRatio(), 0.0);

    const double a = sqrt(gamma*R);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(), 0.0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(0.0), a);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 0.0);

    TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);

    TS_ASSERT_EQUALS(atm.GetAbsoluteViscosity(), 0.0);
    TS_ASSERT_EQUALS(atm.GetKinematicViscosity(), 0.0);
  }

  void testDefaultValuesAfterInit()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    TS_ASSERT(atm.InitModel());

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), FGAtmosphere::StdDaySLtemperature);
    TS_ASSERT_EQUALS(atm.GetTemperature(), FGAtmosphere::StdDaySLtemperature);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), FGAtmosphere::StdDaySLtemperature);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressureSL(), FGAtmosphere::StdDaySLpressure);
    TS_ASSERT_EQUALS(atm.GetPressure(), FGAtmosphere::StdDaySLpressure);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), FGAtmosphere::StdDaySLpressure);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    TS_ASSERT_DELTA(gamma, 1.4, epsilon);
    const double T = atm.GetTemperature();
    const double SLdensity = FGAtmosphere::StdDaySLpressure/(R*T);
    TS_ASSERT_EQUALS(atm.GetDensity(), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensity(0.0), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensitySL(), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensityRatio(), 1.0);

    const double SLsoundspeed = sqrt(gamma*R*T);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(0.0), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedSL(), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 1.0);

    TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);

    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();
    const double mu = beta*T*sqrt(T)/(k+T);
    const double nu = mu/SLdensity;
    TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
    TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
  }

  void testGetAltitudeParameters()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();
    const double mu0 = beta*T0*sqrt(T0)/(k+T0);
    const double nu0 = mu0/rho0;

    for(double h=-1000.0; h<10000; h+= 1000) {
      double T = T0 + 0.1*h;
      double P = P0 + 1.0*h;

      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);

      // Local values must remain unchanged
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_EQUALS(atm.GetTemperature(), T0);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_EQUALS(atm.GetPressure(), P0);
      TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);
      TS_ASSERT_DELTA(atm.GetDensity(), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), 1.0);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 1.0);
      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu0, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu0, epsilon);
    }
  }

  void testRun()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double T = T0 + 0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testTemperatureOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    auto t_node = pm->GetNode("atmosphere/override/temperature", true);
    const double T = 300.0;
    t_node->setDoubleValue(T);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double Tz = T0+0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), Tz, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), 1.0+0.1*h/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), P/(R*Tz), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), sqrt(gamma*R*Tz), epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testPressureOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    auto p_node = pm->GetNode("atmosphere/override/pressure", true);
    const double P = 3000.0;
    p_node->setDoubleValue(P);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double T = T0 + 0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);

      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P0+h, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), (P0+h)/(R*T), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testDensityOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    auto rho_node = pm->GetNode("atmosphere/override/density", true);
    const double rho = 3000.0;
    rho_node->setDoubleValue(rho);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double T = T0 + 0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), P/(R*T), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testTemperatureSL()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = 300.0;
    const double P0 = FGAtmosphere::StdDaySLpressure;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    atm.SetTemperatureSL(T0, FGAtmosphere::eRankine);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double T = T0+0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testPressureSL()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    const double R = atm.GetR();
    const double gamma = atm.GetGamma();
    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = 3000.0;
    const double rho0 = P0/(R*T0);
    const double a0 = sqrt(gamma*R*T0);
    const double beta = atm.GetBeta();
    const double k = atm.GetSutherlandConstant();

    atm.SetPressureSL(FGAtmosphere::ePSF, P0);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;
      TS_ASSERT(atm.Run(false) == false);

      double T = T0+0.1*h;
      TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gamma*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }
};
