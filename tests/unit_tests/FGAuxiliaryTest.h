#include <limits>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGAtmosphere.h>
#include <models/FGAuxiliary.h>

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

using namespace JSBSim;

class DummyAtmosphere : public FGAtmosphere
{
public:
  DummyAtmosphere(FGFDMExec* fdm) : FGAtmosphere(fdm) {}

  // Getters for the protected members
  static constexpr double GetR(void) { return Reng0; }
};

constexpr double R = DummyAtmosphere::GetR();

class FGAuxiliaryTest : public CxxTest::TestSuite
{
public:
  static constexpr double gama = FGAtmosphere::SHRatio;

  FGFDMExec fdmex;
  std::shared_ptr<FGAtmosphere> atm;

  FGAuxiliaryTest() {
    auto aux = fdmex.GetAuxiliary();
    atm = fdmex.GetAtmosphere();
    atm->InitModel();
    fdmex.GetPropertyManager()->Unbind(aux);
  }

  void testPitotTotalPressure() {
    auto aux = FGAuxiliary(&fdmex);
    aux.in.vLocation = fdmex.GetAuxiliary()->in.vLocation;

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double p1 = atm->GetPressureSL();
    double t1 = atm->GetTemperatureSL();
    double rho1 = atm->GetDensitySL();
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
      double P2 = aux.PitotTotalPressure(M1, p1);
      double p2 = P2*pow(t2/T0, gama/(gama-1.0));
      double rho2 = p2/(R*t2);

      // mass conservation
      TS_ASSERT_DELTA(rho1*u1, rho2*u2, epsilon);
      // momentum conservation
      TS_ASSERT_DELTA(p1+rho1*u1*u1, p2+rho2*u2*u2, 1000.*epsilon);
      // energy conservation
#ifdef __arm64__
      TS_ASSERT_DELTA((Cp*t1+0.5*u1*u1)/(Cp*t2+0.5*u2*u2), 1.0, epsilon);
#else
      TS_ASSERT_DELTA(Cp*t1+0.5*u1*u1, Cp*t2+0.5*u2*u2, epsilon);
#endif
    }

    fdmex.GetPropertyManager()->Unbind(&aux);
  }

  void testMachFromImpactPressure() {
    auto aux = FGAuxiliary(&fdmex);
    aux.in.vLocation = fdmex.GetAuxiliary()->in.vLocation;

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double p1 = atm->GetPressureSL();
    double t1 = atm->GetTemperatureSL();
    double rho1 = atm->GetDensitySL();
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
      double mach1 = aux.MachFromImpactPressure(P2-p1, p1);
      double a2 = sqrt(gama*R*t2);
      double M2 = u2/a2;
      double mach2 = aux.MachFromImpactPressure(P2-p2, p2);

      // mass conservation
      TS_ASSERT_DELTA(rho1*u1, rho2*u2, epsilon);
      // momentum conservation
      TS_ASSERT_DELTA(p1+rho1*u1*u1, p2+rho2*u2*u2, 1000.*epsilon);
      // energy conservation
#ifdef __arm64__
      TS_ASSERT_DELTA((Cp*t1+0.5*u1*u1)/(Cp*t2+0.5*u2*u2), 1.0, epsilon);
#else
      TS_ASSERT_DELTA(Cp*t1+0.5*u1*u1, Cp*t2+0.5*u2*u2, epsilon);
#endif
      // Check the Mach computations
      TS_ASSERT_DELTA(mach1, M1, 1e-7);
      TS_ASSERT_DELTA(mach2, M2, 1e-7);
    }

    fdmex.GetPropertyManager()->Unbind(&aux);
  }

  void testCASConversion() {
    auto aux = FGAuxiliary(&fdmex);
    aux.in.StdDaySLsoundspeed = atm->StdDaySLsoundspeed;
    aux.in.vLocation = fdmex.GetAuxiliary()->in.vLocation;

    // Ambient conditions far upstream (i.e. upstream the normal schock
    // in supersonic flight)
    double t1 = atm->GetTemperatureSL();
    double p1 = atm->GetPressureSL();

    TS_ASSERT_DELTA(aux.VcalibratedFromMach(0.0, p1), 0.0, epsilon);
    TS_ASSERT_DELTA(aux.MachFromVcalibrated(0.0, p1), 0.0, epsilon);

    // Check that VCAS match the true airspeed at sea level
    for(double M1=0.1; M1<3.0; M1+=0.25) {
      double u1 = M1*sqrt(gama*R*t1);
      TS_ASSERT_DELTA(aux.VcalibratedFromMach(M1, p1)/u1, 1.0, 1e-7);
      TS_ASSERT_DELTA(aux.MachFromVcalibrated(u1, p1)/M1, 1.0, 1e-7);
    }

    // Check the VCAS computation at an altitude of 1000 ft
    double asl = atm->GetSoundSpeedSL();
    p1 = atm->GetPressure(1000.);

    TS_ASSERT_DELTA(aux.VcalibratedFromMach(0.0, p1), 0.0, epsilon);
    TS_ASSERT_DELTA(aux.MachFromVcalibrated(0.0, p1), 0.0, epsilon);

    for(double M=0.1; M<3.0; M+=0.25) {
      double vcas = M*asl;
      double M1 = aux.MachFromVcalibrated(vcas, p1);
      TS_ASSERT_DELTA(aux.VcalibratedFromMach(M1, p1)/vcas, 1.0, 1e-7);
    }

    double psl = atm->GetPressureSL();
    t1 = atm->GetTemperature(1000.);
    double rho1 = atm->GetDensity(1000.);
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
      double mach = aux.MachFromImpactPressure(P2-p1, psl);

      TS_ASSERT_DELTA(aux.VcalibratedFromMach(M1, p1)/(mach*asl), 1.0, 1e-8);
    }

    fdmex.GetPropertyManager()->Unbind(&aux);
  }
};
