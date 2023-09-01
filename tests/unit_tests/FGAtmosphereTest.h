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

  ~DummyAtmosphere() { PropertyManager->Unbind(this); }

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
  static constexpr double GetBeta(void) { return Beta; }
  static constexpr double GetSutherlandConstant(void) { return SutherlandConstant; }
  static constexpr double GetPSFtoPa(void) { return psftopa; }
  static constexpr double GetPSFtoInHg(void) { return psftoinhg; }
private:
  double a_t, a_p;
};

constexpr double R = DummyAtmosphere::GetR();
constexpr double gama = FGAtmosphere::SHRatio;
constexpr double beta = DummyAtmosphere::GetBeta();
constexpr double k = DummyAtmosphere::GetSutherlandConstant();
constexpr double psftopa = DummyAtmosphere::GetPSFtoPa();
constexpr double psftombar = psftopa/100.;
constexpr double psftoinhg = DummyAtmosphere::GetPSFtoInHg();

class FGAtmosphereTest : public CxxTest::TestSuite
{
public:
  FGFDMExec fdmex;

  FGAtmosphereTest() {
    auto atm = fdmex.GetAtmosphere();
    fdmex.GetPropertyManager()->Unbind(atm);
  }

  void testDefaultValuesBeforeInit()
  {
    FGJSBBase::debug_lvl = 2;
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);

    TS_ASSERT_EQUALS(atm.GetPressureSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressure(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 0.0);

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

    FGJSBBase::debug_lvl = 0;
  }

  void testDefaultValuesAfterInit()
  {
    auto atm = DummyAtmosphere(&fdmex, 1.0, 1.0);

    TS_ASSERT(atm.InitModel());

    const double T0 = FGAtmosphere::StdDaySLtemperature;
    const double P0 = FGAtmosphere::StdDaySLpressure;

    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), T0);
    TS_ASSERT_EQUALS(atm.GetTemperature(), T0);
    TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
    TS_ASSERT_EQUALS(atm.GetPressure(), P0);
    TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);

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

    const double mu = beta*T0*sqrt(T0)/(k+T0);
    const double nu = mu/SLdensity;
    TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
    TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
  }

  void testGetAltitudeParameters()
  {
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
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);

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
    }
  }

  void testRun()
  {
    auto pm = fdmex.GetPropertyManager();
    auto T_node = pm->GetNode("atmosphere/T-R");
    auto rho_node = pm->GetNode("atmosphere/rho-slugs_ft3");
    auto P_node = pm->GetNode("atmosphere/P-psf");
    auto a_node = pm->GetNode("atmosphere/a-fps");
    auto T0_node = pm->GetNode("atmosphere/T-sl-R");
    auto rho0_node = pm->GetNode("atmosphere/rho-sl-slugs_ft3");
    auto a0_node = pm->GetNode("atmosphere/a-sl-fps");
    auto theta_node = pm->GetNode("atmosphere/theta");
    auto sigma_node = pm->GetNode("atmosphere/sigma");
    auto delta_node = pm->GetNode("atmosphere/delta");
    auto a_ratio_node = pm->GetNode("atmosphere/a-ratio");
    auto density_altitude_node = pm->GetNode("atmosphere/density-altitude");
    auto pressure_altitude_node = pm->GetNode("atmosphere/pressure-altitude");

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
      TS_ASSERT_DELTA(T0_node->getDoubleValue(), T0, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperature(), T, epsilon);
      TS_ASSERT_DELTA(T_node->getDoubleValue(), T, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperature(0.0), T0);
      TS_ASSERT_DELTA(atm.GetTemperature(h), T, epsilon);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(), T/T0, epsilon);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(0.0), 1.0);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio(h), T/T0, epsilon);
      TS_ASSERT_DELTA(theta_node->getDoubleValue(), T/T0, epsilon);

      double P = P0 + 1.0*h;
      TS_ASSERT_EQUALS(atm.GetPressureSL(), P0);
      TS_ASSERT_DELTA(atm.GetPressure(), P, epsilon);
      TS_ASSERT_DELTA(P_node->getDoubleValue(), P, epsilon);
      TS_ASSERT_EQUALS(atm.GetPressure(0.0), P0);
      TS_ASSERT_DELTA(atm.GetPressure(h), P, epsilon);
      TS_ASSERT_DELTA(atm.GetPressureRatio(), P/P0, epsilon);
      TS_ASSERT_DELTA(delta_node->getDoubleValue(), P/P0, epsilon);

      double rho = P/(R*T);
      TS_ASSERT_DELTA(atm.GetDensity(), rho, epsilon);
      TS_ASSERT_DELTA(rho_node->getDoubleValue(), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(0.0), rho0, epsilon);
      TS_ASSERT_DELTA(atm.GetDensity(h), rho, epsilon);
      TS_ASSERT_DELTA(atm.GetDensitySL(), rho0, epsilon);
      TS_ASSERT_DELTA(rho0_node->getDoubleValue(), rho0, epsilon);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), rho/rho0);
      TS_ASSERT_DELTA(sigma_node->getDoubleValue(), rho/rho0, epsilon);

      double a = sqrt(gama*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(), a, epsilon);
      TS_ASSERT_DELTA(a_node->getDoubleValue(), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(0.0), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h), a, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedSL(), a0, epsilon);
      TS_ASSERT_DELTA(a0_node->getDoubleValue(), a0, epsilon);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio(), a/a0, epsilon);
      TS_ASSERT_DELTA(a_ratio_node->getDoubleValue(), a/a0, epsilon);

      TS_ASSERT_EQUALS(atm.GetDensityAltitude(), h);
      TS_ASSERT_EQUALS(density_altitude_node->getDoubleValue(), h);
      TS_ASSERT_EQUALS(atm.GetPressureAltitude(), h);
      TS_ASSERT_EQUALS(pressure_altitude_node->getDoubleValue(), h);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, epsilon);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity(), nu, epsilon);
    }
  }

  void testTemperatureOverride()
  {
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
    }

    // Detach the property atmosphere/override/temperature
    auto parent = t_node->getParent();
    parent->removeChild(t_node);
  }

  void testPressureOverride()
  {
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

    // Detach the property atmosphere/override/pressure
    auto parent = p_node->getParent();
    parent->removeChild(p_node);
  }

  void testDensityOverride()
  {
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

    // Detach the property atmosphere/override/density
    auto parent = rho_node->getParent();
    parent->removeChild(rho_node);
  }

  void testSetTemperatureSL()
  {
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
  }

  void testSetPressureSL()
  {
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
  }

  void testPressureConversion()
  {
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
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    atm.in.altitudeASL = 1000;
    TS_ASSERT(atm.Run(false) == false);

    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.8);
    TS_ASSERT_DELTA(atm.GetPressure()*psftopa*1e15, 1.0, 1e-5);
  }

  void testSeaLevelParametersValidation()
  {
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    atm.SetTemperatureSL(0.0, FGAtmosphere::eKelvin);
    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.8);

    atm.SetPressureSL(FGAtmosphere::ePascals, 0.0);
    TS_ASSERT_DELTA(atm.GetPressureSL()*psftopa*1e15, 1.0, 1e-5);
  }

  void testProbeAtADifferentAltitude()
  {
    auto atm = DummyAtmosphere(&fdmex, -1.0, -100.0);
    TS_ASSERT(atm.InitModel());

    TS_ASSERT_EQUALS(atm.GetTemperature(1000.), 1.8);
    TS_ASSERT_DELTA(atm.GetPressure(1000.)*psftopa*1e15, 1.0, 1e-5);
  }
};
