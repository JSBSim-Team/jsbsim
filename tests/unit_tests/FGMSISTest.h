#include <fstream>
#include <string>
#include <vector>

#include <cxxtest/TestSuite.h>
#include <FGFDMExec.h>
#include <models/atmosphere/FGMSIS.h>
#include "TestUtilities.h"

using namespace JSBSim;

class DummyMSIS : public FGMSIS
{
public:
  DummyMSIS(FGFDMExec* fdmex) : FGMSIS(fdmex) {
    in.altitudeASL = 0.0;
    in.GeodLatitudeDeg = 0.0;
    in.LongitudeDeg = 0.0;
  }

  ~DummyMSIS() { PropertyManager->Unbind(this); }

  // Getters for the protected members
  double GetR(void) { return Reng; }
  static constexpr double GetRstar(void) { return Rstar; }
  static constexpr double GetBeta(void) { return Beta; }
  static constexpr double GetSutherlandConstant(void) { return SutherlandConstant; }
  static constexpr double GetPSFtoPa(void) { return psftopa; }
  static constexpr double GetPSFtoInHg(void) { return psftoinhg; }
  // Setters for the protected members
  void SetDay(double day) { day_of_year = day; }
  void SetSeconds(double seconds) { seconds_in_day = seconds; }
  void SetF107A(double value) { input.f107A = value; }
  void SetF107(double value) { input.f107 = value; }
  void SetAP(double value) { input.ap = value; }
};

constexpr double Rstar = DummyMSIS::GetRstar();
constexpr double gama = FGAtmosphere::SHRatio;
constexpr double beta = DummyMSIS::GetBeta();
constexpr double k = DummyMSIS::GetSutherlandConstant();
constexpr double psftopa = DummyMSIS::GetPSFtoPa();
constexpr double psftombar = psftopa/100.;
constexpr double psftoinhg = DummyMSIS::GetPSFtoInHg();

class FGMSISTest : public CxxTest::TestSuite, FGJSBBase
{
public:
  static constexpr double kmtoft = 1000. / fttom;
  static constexpr double gcm3_to_slugft3 = 1000. * kgtoslug / m3toft3;
  static constexpr double gtoslug = kgtoslug / 1000.;

  FGFDMExec fdmex;
  std::shared_ptr<FGAtmosphere> std_atm;
  std::vector<unsigned int> MSIS_iyd, MSIS_sec;
  std::vector<double> MSIS_alt, MSIS_glat, MSIS_glon, MSIS_f107a, MSIS_f107, MSIS_ap,
                      MSIS_T, MSIS_rho, MSIS_mair;

  FGMSISTest() {
    std_atm = fdmex.GetAtmosphere();
    fdmex.GetPropertyManager()->Unbind(std_atm);

    const double species_mmol[8] {28.0134, 31.9988, 31.9988/2.0, 4.0, 1.0, 39.948,
                                  28.0134/2.0, 31.9988/2.0};
    double n[8];
    enum {N2=0, O2, O, He, H, Ar, N, OA};
    struct nrlmsise_output output;
	  struct nrlmsise_input input[15];
  	struct nrlmsise_flags flags;
    int i;
    /* input values */
  	flags.switches[0]=0;
  	for (i=1;i<24;i++)
  		flags.switches[i]=1;
    for (i=0;i<15;i++) {
      input[i].doy=172;
      input[i].year=0; /* without effect */
      input[i].sec=29000;
      input[i].alt=400;
      input[i].g_lat=60;
      input[i].g_long=-70;
      input[i].lst=16;
      input[i].f107A=150;
      input[i].f107=150;
      input[i].ap=4;
    }
    input[1].doy=81;
    input[2].sec=75000;
    input[2].alt=1000;
    input[3].alt=100;
    input[10].alt=0;
    input[11].alt=10;
    input[12].alt=30;
    input[13].alt=50;
    input[14].alt=70;
    input[6].alt=100;
    input[4].g_lat=0;
    input[5].g_long=0;
    // input[6].lst=4;
    input[7].f107A=70;
    input[8].f107=180;
    input[9].ap=40;
    /* evaluate 0 to 14 */
    for (i=0;i<15;i++) {
      double mol = 0.0;
      double mmol = 0.0;

      input[i].lst = input[i].sec/3600.+input[i].g_long/15.;
      MSIS_iyd.push_back(input[i].doy);
      MSIS_sec.push_back(input[i].sec);
      MSIS_alt.push_back(input[i].alt);
      MSIS_glat.push_back(input[i].g_lat);
      MSIS_glon.push_back(input[i].g_long);
      MSIS_f107a.push_back(input[i].f107A);
      MSIS_f107.push_back(input[i].f107);
      MSIS_ap.push_back(input[i].ap);
      gtd7(&input[i], &flags, &output);
      MSIS_T.push_back(output.t[1]);
      MSIS_rho.push_back(output.d[5]);
      n[He] = output.d[0];
      n[O] = output.d[1];
      n[N2] = output.d[2];
      n[O2] = output.d[3];
      n[Ar] = output.d[4];
      n[H] = output.d[6];
      n[N] = output.d[7];
      n[OA] = 0.0;

      for(unsigned j=N2; j<=OA; ++j) {
        mmol += n[j]*species_mmol[j];
        mol += n[j];
      }
      MSIS_mair.push_back(mmol/mol);
    }
  }

  void testConstructor()
  {
    auto atm = DummyMSIS(&fdmex);

    double h = MSIS_alt[0]*kmtoft;

    atm.SetDay(MSIS_iyd[0]);
    atm.SetSeconds(MSIS_sec[0]);
    atm.in.altitudeASL = h;
    atm.in.GeodLatitudeDeg = MSIS_glat[0];
    atm.in.LongitudeDeg = MSIS_glon[0];
    atm.SetF107A(MSIS_f107a[0]);
    atm.SetF107(MSIS_f107[0]);
    atm.SetAP(MSIS_ap[0]);

    double T = KelvinToRankine(MSIS_T[0]);
    TS_ASSERT_EQUALS(atm.GetTemperatureSL(), 1.8);
    TS_ASSERT_EQUALS(atm.GetTemperature(), 1.8);
    TS_ASSERT_DELTA(atm.GetTemperature(h)/T, 1.0, 1E-5);
    TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
    TS_ASSERT_DELTA(atm.GetTemperatureRatio(h)*1.8/T, 1.0, 1E-5);

    double rho = MSIS_rho[0]*gcm3_to_slugft3;
    TS_ASSERT_EQUALS(atm.GetDensitySL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetDensity(), 0.0);
    TS_ASSERT_DELTA(atm.GetDensity(h)/rho, 1.0, 2E-4);
    TS_ASSERT_EQUALS(atm.GetDensityRatio(), 0.0);

    double R = Rstar / (MSIS_mair[0]*gtoslug);
    double P = rho*R*T;
    TS_ASSERT_EQUALS(atm.GetPressureSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetPressure(), 0.0);
    TS_ASSERT_DELTA(atm.GetPressure(h)/P, 1.0, 2E-4);
    TS_ASSERT_EQUALS(atm.GetPressureRatio(), 0.0);

    double a = sqrt(gama*R*T);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedSL(), 1.0);
    TS_ASSERT_EQUALS(atm.GetSoundSpeed(), 0.0);
    TS_ASSERT_DELTA(atm.GetSoundSpeed(h)/a, 1.0, 1E-4);
    TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 0.0);

    TS_ASSERT_EQUALS(atm.GetDensityAltitude(), 0.0);
    TS_ASSERT_EQUALS(atm.GetPressureAltitude(), 0.0);

    TS_ASSERT_EQUALS(atm.GetAbsoluteViscosity(), 0.0);
    TS_ASSERT_EQUALS(atm.GetKinematicViscosity(), 0.0);
  }

  void testInitModel()
  {
    auto pm = fdmex.GetPropertyManager();
    auto theta_node = pm->GetNode("atmosphere/theta");
    auto sigma_node = pm->GetNode("atmosphere/sigma");
    auto delta_node = pm->GetNode("atmosphere/delta");
    auto a_ratio_node = pm->GetNode("atmosphere/a-ratio");

    auto atm = DummyMSIS(&fdmex);
    TS_ASSERT(atm.InitModel());

    for (unsigned int i=0; i<MSIS_iyd.size(); ++i) {
      double h = MSIS_alt[i]*kmtoft;

      atm.SetDay(MSIS_iyd[i]);
      atm.SetSeconds(MSIS_sec[i]);
      atm.in.altitudeASL = h;
      atm.in.GeodLatitudeDeg = MSIS_glat[i];
      atm.in.LongitudeDeg = MSIS_glon[i];
      atm.SetF107A(MSIS_f107a[i]);
      atm.SetF107(MSIS_f107[i]);
      atm.SetAP(MSIS_ap[i]);

      double T = KelvinToRankine(MSIS_T[i]);
      TS_ASSERT_DELTA(atm.GetTemperature(h)/T, 1.0, 1E-4);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
      TS_ASSERT_EQUALS(theta_node->getDoubleValue(), 1.0);

      double rho = MSIS_rho[i]*gcm3_to_slugft3;
      TS_ASSERT_DELTA(atm.GetDensity(h)/rho, 1.0, 5E-4);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), 1.0);
      TS_ASSERT_EQUALS(sigma_node->getDoubleValue(), 1.0);

      double R = Rstar / (MSIS_mair[i]*gtoslug);
      double P = rho*R*T;
      TS_ASSERT_DELTA(atm.GetPressure(h)/P, 1.0, 5E-4);
      TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);
      TS_ASSERT_EQUALS(delta_node->getDoubleValue(), 1.0);

      double a = sqrt(gama*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h)/a, 1.0, 1E-4);
      TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 1.0);
      TS_ASSERT_EQUALS(a_ratio_node->getDoubleValue(), 1.0);

      double p_alt = atm.GetPressureAltitude();
      double P_SL = atm.GetPressureSL();
      TS_ASSERT_DELTA(std_atm->GetPressure(p_alt), P_SL, 1E-8);

      double rho_alt = atm.GetDensityAltitude();
      double rho_SL = atm.GetDensitySL();
      TS_ASSERT_DELTA(std_atm->GetDensity(rho_alt)/rho_SL, 1.0, 1E-8);
    }
  }

  void testLoadModel()
  {
    auto pm = fdmex.GetPropertyManager();
    auto theta_node = pm->GetNode("atmosphere/theta");
    auto sigma_node = pm->GetNode("atmosphere/sigma");
    auto delta_node = pm->GetNode("atmosphere/delta");
    auto a_ratio_node = pm->GetNode("atmosphere/a-ratio");

    auto atm = DummyMSIS(&fdmex);
    TS_ASSERT(atm.InitModel());

    for (unsigned int i=0; i<MSIS_iyd.size(); ++i) {
      double h = MSIS_alt[i]*kmtoft;
      std::stringstream s;

      s << "<dummy>"
        << "  <day>" << MSIS_iyd[i] << "</day>"
        << "  <utc>" << MSIS_sec[i] << "</utc>"
        << "</dummy>" << std::endl;

      Element_ptr elm = readFromXML(s.str());
      TS_ASSERT(atm.Load(elm));

      atm.in.altitudeASL = h;
      atm.in.GeodLatitudeDeg = MSIS_glat[i];
      atm.in.LongitudeDeg = MSIS_glon[i];
      atm.SetF107A(MSIS_f107a[i]);
      atm.SetF107(MSIS_f107[i]);
      atm.SetAP(MSIS_ap[i]);

      double T = KelvinToRankine(MSIS_T[i]);
      TS_ASSERT_DELTA(atm.GetTemperature(h)/T, 1.0, 1E-4);
      TS_ASSERT_EQUALS(atm.GetTemperatureRatio(), 1.0);
      TS_ASSERT_EQUALS(theta_node->getDoubleValue(), 1.0);

      double rho = MSIS_rho[i]*gcm3_to_slugft3;
      TS_ASSERT_DELTA(atm.GetDensity(h)/rho, 1.0, 5E-4);
      TS_ASSERT_EQUALS(atm.GetDensityRatio(), 1.0);
      TS_ASSERT_EQUALS(sigma_node->getDoubleValue(), 1.0);

      double R = Rstar / (MSIS_mair[i]*gtoslug);
      double P = rho*R*T;
      TS_ASSERT_DELTA(atm.GetPressure(h)/P, 1.0, 5E-4);
      TS_ASSERT_EQUALS(atm.GetPressureRatio(), 1.0);
      TS_ASSERT_EQUALS(delta_node->getDoubleValue(), 1.0);

      double a = sqrt(gama*R*T);
      TS_ASSERT_DELTA(atm.GetSoundSpeed(h)/a, 1.0, 1E-4);
      TS_ASSERT_EQUALS(atm.GetSoundSpeedRatio(), 1.0);
      TS_ASSERT_EQUALS(a_ratio_node->getDoubleValue(), 1.0);

      double p_alt = atm.GetPressureAltitude();
      double P_SL = atm.GetPressureSL();
      TS_ASSERT_DELTA(std_atm->GetPressure(p_alt), P_SL, 1E-8);

      double rho_alt = atm.GetDensityAltitude();
      double rho_SL = atm.GetDensitySL();
      TS_ASSERT_DELTA(std_atm->GetDensity(rho_alt)/rho_SL, 1.0, 1E-8);
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

    auto atm = DummyMSIS(&fdmex);
    TS_ASSERT(atm.InitModel());

    for (unsigned int i=0; i<MSIS_iyd.size(); ++i) {
      double h = MSIS_alt[i]*kmtoft;

      atm.SetDay(MSIS_iyd[i]);
      atm.SetSeconds(MSIS_sec[i]);
      atm.in.altitudeASL = h;
      atm.in.GeodLatitudeDeg = MSIS_glat[i];
      atm.in.LongitudeDeg = MSIS_glon[i];
      atm.SetF107A(MSIS_f107a[i]);
      atm.SetF107(MSIS_f107[i]);
      atm.SetAP(MSIS_ap[i]);

      TS_ASSERT(atm.Run(false) == false);

      double T = KelvinToRankine(MSIS_T[i]);
      double T_SL = atm.GetTemperatureSL();
      double T0 = atm.GetTemperature(0.0);
      TS_ASSERT_DELTA(atm.GetTemperature()/T, 1.0, 1E-4);
      TS_ASSERT_DELTA(T_node->getDoubleValue()/T, 1.0, 1E-4);
      TS_ASSERT_EQUALS(T_SL, T0);
      TS_ASSERT_EQUALS(T0_node->getDoubleValue(), T_SL);
      TS_ASSERT_DELTA(atm.GetTemperatureRatio()*T_SL/T, 1.0, 1E-4);
      TS_ASSERT_DELTA(theta_node->getDoubleValue()*T_SL/T, 1.0, 1E-4);

      double rho = MSIS_rho[i]*gcm3_to_slugft3;
      double rho_SL = atm.GetDensitySL();
      double rho0= atm.GetDensity(0.0);
      TS_ASSERT_DELTA(atm.GetDensity()/rho, 1.0, 5E-4);
      TS_ASSERT_DELTA(rho_node->getDoubleValue()/rho, 1.0, 5E-4);
      TS_ASSERT_EQUALS(rho_SL, rho0);
      TS_ASSERT_EQUALS(rho0_node->getDoubleValue(), rho_SL);
      TS_ASSERT_DELTA(atm.GetDensityRatio()*rho_SL/rho, 1.0, 5E-4);
      TS_ASSERT_DELTA(sigma_node->getDoubleValue()*rho_SL/rho, 1.0, 5E-4);

      double R = Rstar / (MSIS_mair[i]*gtoslug);
      double P = rho*R*T;
      double P_SL = atm.GetPressureSL();
      double P0 = atm.GetPressure(0.0);
      TS_ASSERT_DELTA(atm.GetPressure()/P, 1.0, 5E-4);
      TS_ASSERT_DELTA(P_node->getDoubleValue()/P, 1.0, 5E-4);
      TS_ASSERT_EQUALS(P_SL, P0);
      TS_ASSERT_DELTA(atm.GetPressureRatio()*P_SL/P, 1.0, 5E-4);
      TS_ASSERT_DELTA(delta_node->getDoubleValue()*P_SL/P, 1.0, 5E-4);

      double a = sqrt(gama*R*T);
      double a_SL = atm.GetSoundSpeedSL();
      double a0 = atm.GetSoundSpeed(0.0);
      TS_ASSERT_DELTA(atm.GetSoundSpeed()/a, 1.0, 1E-4);
      TS_ASSERT_DELTA(a_node->getDoubleValue()/a, 1.0, 1E-4);
      TS_ASSERT_EQUALS(a_SL, a0);
      TS_ASSERT_EQUALS(a0_node->getDoubleValue(), a_SL);
      TS_ASSERT_DELTA(atm.GetSoundSpeedRatio()*a_SL/a, 1.0, 1E-4);
      TS_ASSERT_DELTA(a_ratio_node->getDoubleValue()*a_SL/a, 1.0, 1E-4);

      double mu = beta*T*sqrt(T)/(k+T);
      double nu = mu/rho;
      TS_ASSERT_DELTA(atm.GetAbsoluteViscosity(), mu, 1E-4);
      TS_ASSERT_DELTA(atm.GetKinematicViscosity()/nu, 1.0, 5E-4);


      double p_alt = atm.GetPressureAltitude();
      TS_ASSERT_DELTA(std_atm->GetPressure(p_alt), P, 1E-8);
      TS_ASSERT_EQUALS(pressure_altitude_node->getDoubleValue(), p_alt);
      double rho_alt = atm.GetDensityAltitude();
      TS_ASSERT_DELTA(std_atm->GetDensity(rho_alt)/rho, 1.0, 1E-8);
      TS_ASSERT_EQUALS(density_altitude_node->getDoubleValue(), rho_alt);
    }
  }
};
