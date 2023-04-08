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
    return ValidateTemperature(SLtemperature+a_t*altitude, "", true);
  }
  void SetTemperature(double t, double h, eTemperature unit) override
  {
    SetTemperatureSL(ConvertToRankine(t, unit)-a_t*h, eRankine);
  }
  double GetPressure(double altitude) const override
  {
    return ValidatePressure(SLpressure+a_p*altitude, "", true);
  }
  // Getters for the protected members
  static constexpr double GetR(void) { return Reng0; }
  static constexpr double GetGamma(void) { return SHRatio; }
  static constexpr double GetBeta(void) { return Beta; }
  static constexpr double GetSutherlandConstant(void) { return SutherlandConstant; }
  static constexpr double GetPSFtoPa(void) { return psftopa; }
  static constexpr double GetPSFtoInHg(void) { return psftoinhg; }
  static constexpr double GetFpsToKts(void) { return fpstokts; }
private:
  double a_t, a_p;
};

constexpr double R = DummyAtmosphere::GetR();
constexpr double gama = DummyAtmosphere::GetGamma();
constexpr double beta = DummyAtmosphere::GetBeta();
constexpr double k = DummyAtmosphere::GetSutherlandConstant();
constexpr double psftopa = DummyAtmosphere::GetPSFtoPa();
constexpr double psftombar = psftopa/100.;
constexpr double psftoinhg = DummyAtmosphere::GetPSFtoInHg();
constexpr double fpstokts = DummyAtmosphere::GetFpsToKts();

class FGAtmosphereTest : public CxxTest::TestSuite
{
public:
  void testDefaultValuesBeforeInit()
  {
    FGFDMExec fdmex;
    FGJSBBase::debug_lvl = 2;
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetTotalTemperature(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTAT_C(), -272.15);

    TS_ASSERT_EQUALS(atm.GetPressureSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressure(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 0.0);
    TS_ASSERT_EQUALS(atm.GetTotalPressure(), 0.0);

    const double rho = 1.0/(R*1.8);
    TS_ASSERT_EQUALS(atm.GetDensitySL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetDensity(), 0.0);
    TS_ASSERT_EQUALS(atm.GetDensity(0.0), rho);
    TS_ASSERT_EQUALS(atm.GetDensityRatio(), 0.0);

    const double a = sqrt(gama*R*1.8);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(), 0.0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(0.0), a);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 0.0);

    TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);

    TS_ASSERT_EQUALS(atm.GetAbsoluteViscosity(), 0.0);
    TS_ASSERT_EQUALS(atm.GetKinematicViscosity(), 0.0);

    TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
    TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);

    FGJSBBase::debug_lvl = 0;
  }

  void testDefaultValuesAfterInit()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    TS_ASSERT(atm.InitModel());

    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
    TS_ASSERT_EQUALS(atm.GetTemperature(), T0);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetTotalTemperature(), T0);
    TS_ASSERT_DELTA(atm.GetTAT_C(), 15.0, 10.*epsilon);

    TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
    TS_ASSERT_EQUALS(atm.GetPressure(), P0);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTotalPressure(), P0);

    const double SLdensity = P0/(R*T0);
    TS_ASSERT_EQUALS(atm.GetDensity(), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensity(0.0), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensitySL(), SLdensity);
    TS_ASSERT_EQUALS(atm.GetDensityRatio(), 1.0);

    const double SLsoundspeed = sqrt(gama*R*T0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(0.0), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedSL(), SLsoundspeed);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 1.0);

    TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);

    TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
    TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);

    const double mu = beta*T0*sqrt(T0)/(k+T0);
    const double nu = mu/SLdensity;
    TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
    TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
  }

  void testGetAltitudeParameters()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);
    const double mu0 = beta*T0*sqrt(T0)/(k+T0);
    const double nu0 = mu0/rho0;

    for(double h=-1000.0; h<10000; h+= 1000) {
      double T = T0 + 0.1*h;
      double P = P0 + 1.0*h;

      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_EQUALS(atm.GetTotalTemperature(), T0);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T0), 1.0, epsilon);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_EQUALS(atm.GetTotalPressure(), P0);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);

      double a = sqrt(gama*R*T);
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
      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testRun()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
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

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testTemperatureOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), P/(R*Tz), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), sqrt(gama*R*Tz), epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testPressureOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P0+h, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), (P0+h)/(R*T), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
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

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testDensityOverride()
  {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), P/(R*T), epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
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

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testSetTemperatureSL()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = 300.0;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
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

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
    }
  }

  void testSetPressureSL()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = 3000.0;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);

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
      TS_ASSERT_DELTA(atm.GetTotalTemperature(), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(T), 1.0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(atm.GetTotalPressure(), P, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

      double a = sqrt(gama*R*T);
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

      TS_ASSERT_EQUALS(atm.GetVcalibratedFPS(), 0.0);
      TS_ASSERT_EQUALS(atm.GetVcalibratedKTS(), 0.0);
  }

  void testPressureConversion()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    double P0 = 900.0;  // mbar
    atm.SetPressureSL(FGAtmosphere::eMillibars, P0);
    TS_ASSERT_DELTA(atm.GetPressureSL()*psftombar / P0, 1.0, 1e-5);
    TS_ASSERT_DELTA(atm.GetPressureSL(FGAtmosphere::eMillibars) / P0, 1.0, 1e-5);

    P0 *= 100.0;  // Pa
    atm.SetPressureSL(FGAtmosphere::ePascals, P0);
    TS_ASSERT_DELTA(atm.GetPressureSL()*psftopa / P0, 1.0, 1e-5);
    TS_ASSERT_DELTA(atm.GetPressureSL(FGAtmosphere::ePascals) / P0, 1.0, 1e-5);

    P0 = 25.0;  // inHg
    atm.SetPressureSL(FGAtmosphere::eInchesHg, P0);
    TS_ASSERT_DELTA(atm.GetPressureSL()*psftoinhg / P0, 1.0, 1e-3);
    TS_ASSERT_DELTA(atm.GetPressureSL(FGAtmosphere::eInchesHg) / P0, 1.0, 1e-3);

    // Illegal units
    TS_ASSERT_THROWS(atm.SetPressureSL(FGAtmosphere::eNoPressUnit, P0), BaseException&);
    TS_ASSERT_THROWS(atm.GetPressureSL(FGAtmosphere::eNoPressUnit), BaseException&);
  }

  void testTemperatureConversion()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    double T0 = 250.0;  // K
    atm.SetTemperatureSL(T0, FGAtmosphere::eKelvin);
    TS_ASSERT_DELTA(atm.GetTemperatureSL()*5.0/9.0, T0, epsilon);

    T0 = -30.0;  // Celsius
    atm.SetTemperatureSL(T0, FGAtmosphere::eCelsius);
    TS_ASSERT_DELTA(atm.GetTemperatureSL()*5.0/9.0-273.15, T0, epsilon);

    T0 = 10.0;  // Fahrenheit
    atm.SetTemperatureSL(T0, FGAtmosphere::eFahrenheit);
    TS_ASSERT_DELTA(atm.GetTemperatureSL()-459.67, T0, epsilon);

    // Illegal units
    TS_ASSERT_THROWS(atm.SetTemperatureSL(T0, FGAtmosphere::eNoTempUnit), BaseException&);
  }

  void testAltitudeParametersValidation()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    atm.in.altitudeASL = 1000;
    TS_ASSERT(atm.Run(false) == false);

    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.8);
    TS_ASSERT_DELTA(atm.GetPressure()*psftopa*1e15, 1.0, 1e-5);
  }

  void testSeaLevelParametersValidation()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    atm.SetTemperatureSL(0.0, FGAtmosphere::eKelvin);
    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.8);

    atm.SetPressureSL(FGAtmosphere::ePascals, 0.0);
    TS_ASSERT_DELTA(atm.GetPressureSL()*psftopa*1e15, 1.0, 1e-5);
  }

  void testProbeAtADifferentAltitude()
  {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    TS_ASSERT_EQUALS(atm.GetTemperature(1000.), 1.8);
    TS_ASSERT_DELTA(atm.GetPressure(1000.)*psftopa*1e15, 1.0, 1e-5);
  }

  void testPitotTotalPressure() {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    atm.InitModel();

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double p1 = atm.GetPressureSL();
    double t1 = atm.GetTemperatureSL();
    double rho1 = atm.GetDensitySL();
    constexpr double Cp = gama*R/(gama-1.0);

    // Based on formulas from Modern Compressible Flow (3rd edition)
    // - John D. Anderson
    for(double M1=0; M1<3.0; M1+=0.25) {
      double a1 = sqrt(gama*R*t1);
      double u1 = M1*a1;
      // Total temperature
      double T0 = t1+u1*u1/(2.0*Cp);
      // Compute conditions downstream (at the pitot tube)
      double u2 = u1;
      if (M1 >= 1.0){
        // Assess the normal shock effect knowing that a_star=u1*u2
        double a_star = sqrt((a1*a1/(gama-1.0)+0.5*u1*u1)*2*(gama-1.0)/(gama+1.0)); // equation (3.32) p.81
        u2 = a_star*a_star/u1;// equation (3.47) p.89
      }
      double t2 = T0-u2*u2/(2*Cp);
      double P2 = atm.PitotTotalPressure(M1, p1);
      double p2 = P2*pow(t2/T0, gama/(gama-1.0));
      double rho2 = p2/(R*t2);

      // mass conservation
      TS_ASSERT_DELTA(rho1*u1, rho2*u2, epsilon);
      // momentum conservation
      TS_ASSERT_DELTA(p1+rho1*u1*u1, p2+rho2*u2*u2, 1000.*epsilon);
      // energy conservation
      TS_ASSERT_DELTA(Cp*t1+0.5*u1*u1, Cp*t2+0.5*u2*u2, epsilon);
    }
  }

  void testMachFromImpactPressure() {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    atm.InitModel();

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double p1 = atm.GetPressureSL();
    double t1 = atm.GetTemperatureSL();
    double rho1 = atm.GetDensitySL();
    constexpr double Cp = gama*R/(gama-1.0);

    // Based on formulas from Modern Compressible Flow (3rd edition)
    // - John D. Anderson
    for(double M1=0; M1<3.0; M1+=0.25) {
      double a1 = sqrt(gama*R*t1);
      double u1 = M1*a1;
      // Total temperature
      double T0 = t1+u1*u1/(2.0*Cp);
      // Compute conditions downstream (at the pitot tube)
      double u2 = u1;
      if (M1 >= 1.0) {
        // Assess the normal shock effect knowing that a_star=u1*u2
        double a_star = sqrt((a1*a1/(gama-1.0)+0.5*u1*u1)*2*(gama-1.0)/(gama+1.0)); // equation (3.32) p.81
        u2 = a_star*a_star/u1;// equation (3.47) p.89
      }
      double t2 = T0-u2*u2/(2*Cp);
      double rho2 = M1 == 0.0 ? rho1 : rho1*u1/u2;
      double p2 = rho2*R*t2;
      double P2 = p2*pow(T0/t2, gama/(gama-1.0));
      double mach1 = atm.MachFromImpactPressure(P2-p1, p1);
      double a2 = sqrt(gama*R*t2);
      double M2 = u2/a2;
      double mach2 = atm.MachFromImpactPressure(P2-p2, p2);

      // mass conservation
      TS_ASSERT_DELTA(rho1*u1, rho2*u2, epsilon);
      // momentum conservation
      TS_ASSERT_DELTA(p1+rho1*u1*u1, p2+rho2*u2*u2, 1000.*epsilon);
      // energy conservation
      TS_ASSERT_DELTA(Cp*t1+0.5*u1*u1, Cp*t2+0.5*u2*u2, epsilon);
      // Check the Mach computations
      TS_ASSERT_DELTA(mach1, M1, 1e-7);
      TS_ASSERT_DELTA(mach2, M2, 1e-7);
    }
  }

  void testCASConversion() {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, -0.1, -1.0);
    atm.InitModel();

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double t1 = atm.GetTemperatureSL();

    TS_ASSERT_DELTA(atm.VcalibratedFromMach(0.0, 0.0), 0.0, epsilon);
    TS_ASSERT_DELTA(atm.MachFromVcalibrated(0.0, 0.0), 0.0, epsilon);

    // Check that VCAS match the true airspeed at sea level
    for(double M1=0.1; M1<3.0; M1+=0.25) {
      double u1 = M1*sqrt(gama*R*t1);
      TS_ASSERT_DELTA(atm.VcalibratedFromMach(M1, 0.0)/u1, 1.0, 1e-7);
      TS_ASSERT_DELTA(atm.MachFromVcalibrated(u1, 0.0)/M1, 1.0, 1e-7);
    }

    // Check the VCAS computation at an altitude of 1000 ft
    double asl = atm.GetSoundSpeedSL();

    TS_ASSERT_DELTA(atm.VcalibratedFromMach(0.0, 1000.), 0.0, epsilon);
    TS_ASSERT_DELTA(atm.MachFromVcalibrated(0.0, 1000.), 0.0, epsilon);

    for(double M=0.1; M<3.0; M+=0.25) {
      double vcas = M*asl;
      double M1 = atm.MachFromVcalibrated(vcas, 1000.);
      TS_ASSERT_DELTA(atm.VcalibratedFromMach(M1, 1000.)/vcas, 1.0, 1e-7);
    }

    double psl = atm.GetPressureSL();
    double p1 = atm.GetPressure(1000.);
    t1 = atm.GetTemperature(1000.);
    double rho1 = atm.GetDensity(1000.);
    constexpr double Cp = gama*R/(gama-1.0);

    // Based on formulas from Modern Compressible Flow (3rd edition)
    // - John D. Anderson
    for(double M1=0.1; M1<3.0; M1+=0.25) {
      double a1 = sqrt(gama*R*t1);
      double u1 = M1*a1;
      // Total temperature
      double T0 = t1+u1*u1/(2.0*Cp);
      // Compute conditions downstream (at the pitot tube)
      double u2 = u1;
      if (M1 >= 1.0) {
        // Assess the normal shock effect knowing that a_star=u1*u2
        double a_star = sqrt((a1*a1/(gama-1.0)+0.5*u1*u1)*2*(gama-1.0)/(gama+1.0)); // equation (3.32) p.81
        u2 = a_star*a_star/u1;// equation (3.47) p.89
      }
      double t2 = T0-u2*u2/(2*Cp);
      double rho2 = M1 == 0.0 ? rho1 : rho1*u1/u2;
      double p2 = rho2*R*t2;
      double P2 = p2*pow(T0/t2, gama/(gama-1.0));
      double mach = atm.MachFromImpactPressure(P2-p1, psl);

      TS_ASSERT_DELTA(atm.VcalibratedFromMach(M1, 1000.)/(mach*asl), 1.0, 1e-8);
    }
  }

  void testCASComputation() {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);
    constexpr double k1 = 0.5*(gama-1.0);

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;

      double T = T0 + 0.1*h;
      double a = sqrt(gama*R*T);
      double P = P0 + 1.0*h;
      double rho = P/(R*T);
      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;

      for(double M1=0.1; M1<3.0; M1+=0.25) {
        atm.in.vUVW = {M1*a, 0.0, 0.0};
        double Tt = T*(1.0+k1*M1*M1);
        TS_ASSERT(atm.Run(false) == false);
        TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
        TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
        TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
        TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
        TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
        TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
        TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);
        TS_ASSERT_DELTA(atm.GetTotalTemperature() / Tt, 1.0, epsilon);
        TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(Tt), 1.0, epsilon);

        double Pt = atm.PitotTotalPressure(M1, P);
        TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
        TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
        TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
        TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
        TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
        TS_ASSERT_DELTA(atm.GetTotalPressure() / Pt, 1.0, epsilon);

        TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
        TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
        TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
        TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
        TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

        TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

        TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
        TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

        TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
        TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);

        double vc = atm.VcalibratedFromMach(M1, h);
        TS_ASSERT_DELTA(atm.GetVcalibratedFPS()/vc, 1.0, epsilon);
        TS_ASSERT_DELTA(atm.GetVcalibratedKTS() / (vc*fpstokts), 1.0, epsilon);
      }
    }
  }

  void testCASComputationWithWind() {
    FGFDMExec fdmex;
    auto atm = DummyAtmosphere(&fdmex, 0.1, 1.0);
    TS_ASSERT(atm.InitModel());

    constexpr double T0 = FGAtmosphere::StdDaySLtemperature;
    constexpr double P0 = FGAtmosphere::StdDaySLpressure;
    constexpr double rho0 = P0/(R*T0);
    const double a0 = sqrt(gama*R*T0);
    constexpr double k1 = 0.5*(gama-1.0);

    atm.in.Tl2b = { 1.0, 0.0, 0.0,
                    0.0, 2.0, 0.0,
                    0.0, 0.0, 1.0}; // Double the Y component
    // Wind is 10 ft/s tailwind and 25 ft/s crosswind
    atm.in.TotalWindNED = {-10.0, 15.0, 40.0};

    for(double h=-1000.0; h<10000; h+= 1000) {
      atm.in.altitudeASL = h;

      double T = T0 + 0.1*h;
      double a = sqrt(gama*R*T);
      double P = P0 + 1.0*h;
      double rho = P/(R*T);
      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;

      for(double M1=0.1; M1<3.0; M1+=0.25) {
        atm.in.vUVW = {M1*a, 0.0, 0.0};
        double Vg = M1*a+10;
        double V = sqrt(Vg*Vg+2500.0);
        double mach = V / a;
        double Tt = T*(1.0+k1*mach*mach);
        TS_ASSERT(atm.Run(false) == false);
        TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
        TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
        TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
        TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
        TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
        TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
        TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);
        TS_ASSERT_DELTA(atm.GetTotalTemperature() / Tt, 1.0, epsilon);
        TS_ASSERT_DELTA(atm.GetTAT_C()/FGJSBBase::RankineToCelsius(Tt), 1.0, epsilon);

        double Pt = atm.PitotTotalPressure(mach, P);
        TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
        TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
        TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
        TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
        TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
        TS_ASSERT_DELTA(atm.GetTotalPressure() / Pt, 1.0, epsilon);

        TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
        TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
        TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
        TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
        TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);

        TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
        TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);

        TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
        TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);

        TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
        TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);

        double vc = atm.VcalibratedFromMach(mach, h);
        TS_ASSERT_DELTA(atm.GetVcalibratedFPS()/vc, 1.0, epsilon);
        TS_ASSERT_DELTA(atm.GetVcalibratedKTS() / (vc*fpstokts), 1.0, epsilon);
      }
    }
  }
};
