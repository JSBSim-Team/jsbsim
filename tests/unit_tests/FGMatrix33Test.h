#include <iomanip>
#include <cxxtest/TestSuite.h>
#include <math/FGMatrix33.h>
#include <math/FGQuaternion.h>

class FGMatrix33Test : public CxxTest::TestSuite
{
public:
  void testConstructors() {
    const JSBSim::FGMatrix33 m0;
    TS_ASSERT_EQUALS(m0.Rows(), 3);
    TS_ASSERT_EQUALS(m0.Cols(), 3);
    TS_ASSERT_EQUALS(m0.Entry(1,1), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(1,2), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(1,3), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(2,1), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(2,2), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(2,3), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(3,1), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(3,2), 0.0);
    TS_ASSERT_EQUALS(m0.Entry(3,3), 0.0);
    TS_ASSERT_EQUALS(m0(1,1), 0.0);
    TS_ASSERT_EQUALS(m0(1,2), 0.0);
    TS_ASSERT_EQUALS(m0(1,3), 0.0);
    TS_ASSERT_EQUALS(m0(2,1), 0.0);
    TS_ASSERT_EQUALS(m0(2,2), 0.0);
    TS_ASSERT_EQUALS(m0(2,3), 0.0);
    TS_ASSERT_EQUALS(m0(3,1), 0.0);
    TS_ASSERT_EQUALS(m0(3,2), 0.0);
    TS_ASSERT_EQUALS(m0(3,3), 0.0);
    JSBSim::FGMatrix33 m = m0;
    TS_ASSERT_EQUALS(m.Entry(1,1), 0.0);
    TS_ASSERT_EQUALS(m.Entry(1,2), 0.0);
    TS_ASSERT_EQUALS(m.Entry(1,3), 0.0);
    TS_ASSERT_EQUALS(m.Entry(2,1), 0.0);
    TS_ASSERT_EQUALS(m.Entry(2,2), 0.0);
    TS_ASSERT_EQUALS(m.Entry(2,3), 0.0);
    TS_ASSERT_EQUALS(m.Entry(3,1), 0.0);
    TS_ASSERT_EQUALS(m.Entry(3,2), 0.0);
    TS_ASSERT_EQUALS(m.Entry(3,3), 0.0);
    TS_ASSERT_EQUALS(m(1,1), 0.0);
    TS_ASSERT_EQUALS(m(1,2), 0.0);
    TS_ASSERT_EQUALS(m(1,3), 0.0);
    TS_ASSERT_EQUALS(m(2,1), 0.0);
    TS_ASSERT_EQUALS(m(2,2), 0.0);
    TS_ASSERT_EQUALS(m(2,3), 0.0);
    TS_ASSERT_EQUALS(m(3,1), 0.0);
    TS_ASSERT_EQUALS(m(3,2), 0.0);
    TS_ASSERT_EQUALS(m(3,3), 0.0);
    m(2,2) = 1.0;
    TS_ASSERT_EQUALS(m0(2,2), 0.0);
    JSBSim::FGMatrix33 m1(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    TS_ASSERT_EQUALS(m1(1,1), 1.0);
    TS_ASSERT_EQUALS(m1(1,2), 2.0);
    TS_ASSERT_EQUALS(m1(1,3), 3.0);
    TS_ASSERT_EQUALS(m1(2,1), 4.0);
    TS_ASSERT_EQUALS(m1(2,2), 5.0);
    TS_ASSERT_EQUALS(m1(2,3), 6.0);
    TS_ASSERT_EQUALS(m1(3,1), 7.0);
    TS_ASSERT_EQUALS(m1(3,2), 8.0);
    TS_ASSERT_EQUALS(m1(3,3), 9.0);
    m1.InitMatrix();
    TS_ASSERT_EQUALS(m1(1,1), 0.0);
    TS_ASSERT_EQUALS(m1(1,2), 0.0);
    TS_ASSERT_EQUALS(m1(1,3), 0.0);
    TS_ASSERT_EQUALS(m1(2,1), 0.0);
    TS_ASSERT_EQUALS(m1(2,2), 0.0);
    TS_ASSERT_EQUALS(m1(2,3), 0.0);
    TS_ASSERT_EQUALS(m1(3,1), 0.0);
    TS_ASSERT_EQUALS(m1(3,2), 0.0);
    TS_ASSERT_EQUALS(m1(3,3), 0.0);
  }

  void testTransposed() {
    const JSBSim::FGMatrix33 m(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    JSBSim::FGMatrix33 mT = m.Transposed();
    TS_ASSERT_EQUALS(mT(1,1), 1.0);
    TS_ASSERT_EQUALS(mT(1,2), 4.0);
    TS_ASSERT_EQUALS(mT(1,3), 7.0);
    TS_ASSERT_EQUALS(mT(2,1), 2.0);
    TS_ASSERT_EQUALS(mT(2,2), 5.0);
    TS_ASSERT_EQUALS(mT(2,3), 8.0);
    TS_ASSERT_EQUALS(mT(3,1), 3.0);
    TS_ASSERT_EQUALS(mT(3,2), 6.0);
    TS_ASSERT_EQUALS(mT(3,3), 9.0);
    mT.InitMatrix(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    TS_ASSERT_EQUALS(mT(1,1), 1.0);
    TS_ASSERT_EQUALS(mT(1,2), 2.0);
    TS_ASSERT_EQUALS(mT(1,3), 3.0);
    TS_ASSERT_EQUALS(mT(2,1), 4.0);
    TS_ASSERT_EQUALS(mT(2,2), 5.0);
    TS_ASSERT_EQUALS(mT(2,3), 6.0);
    TS_ASSERT_EQUALS(mT(3,1), 7.0);
    TS_ASSERT_EQUALS(mT(3,2), 8.0);
    TS_ASSERT_EQUALS(mT(3,3), 9.0);
    mT.T();
    TS_ASSERT_EQUALS(mT(1,1), 1.0);
    TS_ASSERT_EQUALS(mT(1,2), 4.0);
    TS_ASSERT_EQUALS(mT(1,3), 7.0);
    TS_ASSERT_EQUALS(mT(2,1), 2.0);
    TS_ASSERT_EQUALS(mT(2,2), 5.0);
    TS_ASSERT_EQUALS(mT(2,3), 8.0);
    TS_ASSERT_EQUALS(mT(3,1), 3.0);
    TS_ASSERT_EQUALS(mT(3,2), 6.0);
    TS_ASSERT_EQUALS(mT(3,3), 9.0);
  }

  void testOperations() {
    JSBSim::FGMatrix33 m0;
    const JSBSim::FGMatrix33 m(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    m0 = m;
    TS_ASSERT_EQUALS(m0(1,1), 1.0);
    TS_ASSERT_EQUALS(m0(1,2), 2.0);
    TS_ASSERT_EQUALS(m0(1,3), 3.0);
    TS_ASSERT_EQUALS(m0(2,1), 4.0);
    TS_ASSERT_EQUALS(m0(2,2), 5.0);
    TS_ASSERT_EQUALS(m0(2,3), 6.0);
    TS_ASSERT_EQUALS(m0(3,1), 7.0);
    TS_ASSERT_EQUALS(m0(3,2), 8.0);
    TS_ASSERT_EQUALS(m0(3,3), 9.0);
    m0(2,2) = -10.0;
    TS_ASSERT_EQUALS(m(2,2), 5.0);
    m0 = -1.0 * m;
    TS_ASSERT_EQUALS(m0(1,1), -1.0);
    TS_ASSERT_EQUALS(m0(1,2), -2.0);
    TS_ASSERT_EQUALS(m0(1,3), -3.0);
    TS_ASSERT_EQUALS(m0(2,1), -4.0);
    TS_ASSERT_EQUALS(m0(2,2), -5.0);
    TS_ASSERT_EQUALS(m0(2,3), -6.0);
    TS_ASSERT_EQUALS(m0(3,1), -7.0);
    TS_ASSERT_EQUALS(m0(3,2), -8.0);
    TS_ASSERT_EQUALS(m0(3,3), -9.0);
    const JSBSim::FGMatrix33 m_twice = m * 2.0;
    TS_ASSERT_EQUALS(m_twice(1,1), 2.0);
    TS_ASSERT_EQUALS(m_twice(1,2), 4.0);
    TS_ASSERT_EQUALS(m_twice(1,3), 6.0);
    TS_ASSERT_EQUALS(m_twice(2,1), 8.0);
    TS_ASSERT_EQUALS(m_twice(2,2), 10.0);
    TS_ASSERT_EQUALS(m_twice(2,3), 12.0);
    TS_ASSERT_EQUALS(m_twice(3,1), 14.0);
    TS_ASSERT_EQUALS(m_twice(3,2), 16.0);
    TS_ASSERT_EQUALS(m_twice(3,3), 18.0);
    JSBSim::FGMatrix33 m_res = m_twice - m;
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 2.0);
    TS_ASSERT_EQUALS(m_res(1,3), 3.0);
    TS_ASSERT_EQUALS(m_res(2,1), 4.0);
    TS_ASSERT_EQUALS(m_res(2,2), 5.0);
    TS_ASSERT_EQUALS(m_res(2,3), 6.0);
    TS_ASSERT_EQUALS(m_res(3,1), 7.0);
    TS_ASSERT_EQUALS(m_res(3,2), 8.0);
    TS_ASSERT_EQUALS(m_res(3,3), 9.0);
    m_res = m_twice;
    m_res -= m;
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 2.0);
    TS_ASSERT_EQUALS(m_res(1,3), 3.0);
    TS_ASSERT_EQUALS(m_res(2,1), 4.0);
    TS_ASSERT_EQUALS(m_res(2,2), 5.0);
    TS_ASSERT_EQUALS(m_res(2,3), 6.0);
    TS_ASSERT_EQUALS(m_res(3,1), 7.0);
    TS_ASSERT_EQUALS(m_res(3,2), 8.0);
    TS_ASSERT_EQUALS(m_res(3,3), 9.0);
    m_res = m_twice + m;
    TS_ASSERT_EQUALS(m_res(1,1), 3.0);
    TS_ASSERT_EQUALS(m_res(1,2), 6.0);
    TS_ASSERT_EQUALS(m_res(1,3), 9.0);
    TS_ASSERT_EQUALS(m_res(2,1), 12.0);
    TS_ASSERT_EQUALS(m_res(2,2), 15.0);
    TS_ASSERT_EQUALS(m_res(2,3), 18.0);
    TS_ASSERT_EQUALS(m_res(3,1), 21.0);
    TS_ASSERT_EQUALS(m_res(3,2), 24.0);
    TS_ASSERT_EQUALS(m_res(3,3), 27.0);
    m_res += m;
    TS_ASSERT_EQUALS(m_res(1,1), 4.0);
    TS_ASSERT_EQUALS(m_res(1,2), 8.0);
    TS_ASSERT_EQUALS(m_res(1,3), 12.0);
    TS_ASSERT_EQUALS(m_res(2,1), 16.0);
    TS_ASSERT_EQUALS(m_res(2,2), 20.0);
    TS_ASSERT_EQUALS(m_res(2,3), 24.0);
    TS_ASSERT_EQUALS(m_res(3,1), 28.0);
    TS_ASSERT_EQUALS(m_res(3,2), 32.0);
    TS_ASSERT_EQUALS(m_res(3,3), 36.0);
    m_res *= 0.25;
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 2.0);
    TS_ASSERT_EQUALS(m_res(1,3), 3.0);
    TS_ASSERT_EQUALS(m_res(2,1), 4.0);
    TS_ASSERT_EQUALS(m_res(2,2), 5.0);
    TS_ASSERT_EQUALS(m_res(2,3), 6.0);
    TS_ASSERT_EQUALS(m_res(3,1), 7.0);
    TS_ASSERT_EQUALS(m_res(3,2), 8.0);
    TS_ASSERT_EQUALS(m_res(3,3), 9.0);
    m_res = m_twice / 2.0;
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 2.0);
    TS_ASSERT_EQUALS(m_res(1,3), 3.0);
    TS_ASSERT_EQUALS(m_res(2,1), 4.0);
    TS_ASSERT_EQUALS(m_res(2,2), 5.0);
    TS_ASSERT_EQUALS(m_res(2,3), 6.0);
    TS_ASSERT_EQUALS(m_res(3,1), 7.0);
    TS_ASSERT_EQUALS(m_res(3,2), 8.0);
    TS_ASSERT_EQUALS(m_res(3,3), 9.0);
    m_res = m_twice;
    m_res /= 2.0;
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 2.0);
    TS_ASSERT_EQUALS(m_res(1,3), 3.0);
    TS_ASSERT_EQUALS(m_res(2,1), 4.0);
    TS_ASSERT_EQUALS(m_res(2,2), 5.0);
    TS_ASSERT_EQUALS(m_res(2,3), 6.0);
    TS_ASSERT_EQUALS(m_res(3,1), 7.0);
    TS_ASSERT_EQUALS(m_res(3,2), 8.0);
    TS_ASSERT_EQUALS(m_res(3,3), 9.0);
    const JSBSim::FGMatrix33 eye(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    const JSBSim::FGColumnVector3 v0(1.0, -2.0, 3.0);
    JSBSim::FGColumnVector3 v = eye * v0;
    TS_ASSERT_EQUALS(v, v0);
    m_res = m_twice * eye;
    TS_ASSERT_EQUALS(m_res(1,1), 2.0);
    TS_ASSERT_EQUALS(m_res(1,2), 4.0);
    TS_ASSERT_EQUALS(m_res(1,3), 6.0);
    TS_ASSERT_EQUALS(m_res(2,1), 8.0);
    TS_ASSERT_EQUALS(m_res(2,2), 10.0);
    TS_ASSERT_EQUALS(m_res(2,3), 12.0);
    TS_ASSERT_EQUALS(m_res(3,1), 14.0);
    TS_ASSERT_EQUALS(m_res(3,2), 16.0);
    TS_ASSERT_EQUALS(m_res(3,3), 18.0);
    m_res = eye * m_twice;
    TS_ASSERT_EQUALS(m_res(1,1), 2.0);
    TS_ASSERT_EQUALS(m_res(1,2), 4.0);
    TS_ASSERT_EQUALS(m_res(1,3), 6.0);
    TS_ASSERT_EQUALS(m_res(2,1), 8.0);
    TS_ASSERT_EQUALS(m_res(2,2), 10.0);
    TS_ASSERT_EQUALS(m_res(2,3), 12.0);
    TS_ASSERT_EQUALS(m_res(3,1), 14.0);
    TS_ASSERT_EQUALS(m_res(3,2), 16.0);
    TS_ASSERT_EQUALS(m_res(3,3), 18.0);
    m_res *= eye;
    TS_ASSERT_EQUALS(m_res(1,1), 2.0);
    TS_ASSERT_EQUALS(m_res(1,2), 4.0);
    TS_ASSERT_EQUALS(m_res(1,3), 6.0);
    TS_ASSERT_EQUALS(m_res(2,1), 8.0);
    TS_ASSERT_EQUALS(m_res(2,2), 10.0);
    TS_ASSERT_EQUALS(m_res(2,3), 12.0);
    TS_ASSERT_EQUALS(m_res(3,1), 14.0);
    TS_ASSERT_EQUALS(m_res(3,2), 16.0);
    TS_ASSERT_EQUALS(m_res(3,3), 18.0);
  }

  void testInversion() {
    JSBSim::FGMatrix33 m(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
    JSBSim::FGMatrix33 m_res;
    TS_ASSERT_EQUALS(m.Determinant(), 1.0);
    TS_ASSERT(m.Invertible());
    m_res = m.Inverse();
    TS_ASSERT_EQUALS(m_res(1,1), 1.0);
    TS_ASSERT_EQUALS(m_res(1,2), 0.0);
    TS_ASSERT_EQUALS(m_res(1,3), 0.0);
    TS_ASSERT_EQUALS(m_res(2,1), 0.0);
    TS_ASSERT_EQUALS(m_res(2,2), 1.0);
    TS_ASSERT_EQUALS(m_res(2,3), 0.0);
    TS_ASSERT_EQUALS(m_res(3,1), 0.0);
    TS_ASSERT_EQUALS(m_res(3,2), 0.0);
    TS_ASSERT_EQUALS(m_res(3,3), 1.0);
    m.InitMatrix();
    m(1,1) = 1.0;
    TS_ASSERT_EQUALS(m.Determinant(), 0.0);
    TS_ASSERT(!m.Invertible());
    m_res = m.Inverse();
    TS_ASSERT_EQUALS(m_res(1,1), 0.0);
    TS_ASSERT_EQUALS(m_res(1,2), 0.0);
    TS_ASSERT_EQUALS(m_res(1,3), 0.0);
    TS_ASSERT_EQUALS(m_res(2,1), 0.0);
    TS_ASSERT_EQUALS(m_res(2,2), 0.0);
    TS_ASSERT_EQUALS(m_res(2,3), 0.0);
    TS_ASSERT_EQUALS(m_res(3,1), 0.0);
    TS_ASSERT_EQUALS(m_res(3,2), 0.0);
    TS_ASSERT_EQUALS(m_res(3,3), 0.0);
  }

  // Check the assignment via an initializer list
  void testAssignmentInitializerList(void) {
    JSBSim::FGMatrix33 m;
    TS_ASSERT_EQUALS(m(1,1), 0.0);
    TS_ASSERT_EQUALS(m(1,2), 0.0);
    TS_ASSERT_EQUALS(m(1,3), 0.0);
    TS_ASSERT_EQUALS(m(2,1), 0.0);
    TS_ASSERT_EQUALS(m(2,2), 0.0);
    TS_ASSERT_EQUALS(m(2,3), 0.0);
    TS_ASSERT_EQUALS(m(3,1), 0.0);
    TS_ASSERT_EQUALS(m(3,2), 0.0);
    TS_ASSERT_EQUALS(m(3,3), 0.0);

    m = { 1.0, 2.0, -3.0,
          4.0, -5.0, 6.0,
          -7.0, 8.0, 9.0};

    TS_ASSERT_EQUALS(m(1,1), 1.0);
    TS_ASSERT_EQUALS(m(1,2), 2.0);
    TS_ASSERT_EQUALS(m(1,3), -3.0);
    TS_ASSERT_EQUALS(m(2,1), 4.0);
    TS_ASSERT_EQUALS(m(2,2), -5.0);
    TS_ASSERT_EQUALS(m(2,3), 6.0);
    TS_ASSERT_EQUALS(m(3,1), -7.0);
    TS_ASSERT_EQUALS(m(3,2), 8.0);
    TS_ASSERT_EQUALS(m(3,3), 9.0);
  }

  void testInputOutput() {
    std::ostringstream os, os_ref;
    JSBSim::FGMatrix33 m;
    std::istringstream values("1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0");

    values >> m;
    TS_ASSERT_EQUALS(m(1,1), 1.0);
    TS_ASSERT_EQUALS(m(1,2), 2.0);
    TS_ASSERT_EQUALS(m(1,3), 3.0);
    TS_ASSERT_EQUALS(m(2,1), 4.0);
    TS_ASSERT_EQUALS(m(2,2), 5.0);
    TS_ASSERT_EQUALS(m(2,3), 6.0);
    TS_ASSERT_EQUALS(m(3,1), 7.0);
    TS_ASSERT_EQUALS(m(3,2), 8.0);
    TS_ASSERT_EQUALS(m(3,3), 9.0);

    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++) {
        os << m(i,j);
        if (i!=3 || j!=3)
          os << ", ";
      }
    os_ref << m;
    TS_ASSERT_EQUALS(os_ref.str(), os.str());

    os.clear();
    os.str("");
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++) {
        os << std::setw(12) << std::setprecision(10) << m(i,j);
        if (i!=3 || j!=3)
          os << ", ";
      }
    TS_ASSERT_EQUALS(m.Dump(", "), os.str());

    os.clear();
    os.str("");
    for (int i=1; i<=3; i++) {
      os << "# ";
      for (int j=1; j<=3; j++) {
        os << std::right << std::fixed << std::setw(9);
        os << std::setprecision(6) << m(i,j);
        if (j != 3)
          os << ", ";
        else {
          if (i != 3)
            os << std::endl;
          else
            os << std::setw(0) << std::left;
        }
      }
    }
    TS_ASSERT_EQUALS(m.Dump(", ", "# "), os.str());
  }

  void testAngles() {
    double phi = 10. * M_PI / 180.;
    double theta = 45. * M_PI / 180.;
    double psi = 265. * M_PI / 180.;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = cos(theta), sth = sin(theta);
    double cpsi = cos(psi), spsi = sin(psi);
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    // Check that m is orthogonal
    TS_ASSERT_DELTA(m.Determinant(), 1.0, 1E-8);
    JSBSim::FGMatrix33 mInv = m.Inverse();
    JSBSim::FGMatrix33 mT = m.Transposed();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++) {
        TS_ASSERT_DELTA(m(i,j), mInv(j,i), 1E-8);
        TS_ASSERT_DELTA(mT(i,j), mInv(i,j), 1E-8);
      }
    JSBSim::FGMatrix33 eye = m * mInv;
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++) {
        if (i == j) {
          TS_ASSERT_DELTA(eye(i,j), 1.0, 1E-8);
        } else {
          TS_ASSERT_DELTA(eye(i,j), 0.0, 1E-8);
        }
      }
    m.InitMatrix(0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0);
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(2), 0.5*M_PI, 1E-8);
    m.InitMatrix(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0);
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(2), -0.5*M_PI, 1E-8);

    JSBSim::FGQuaternion q = m_phi.GetQuaternion();
    TS_ASSERT_DELTA(q(1), cos(0.5*phi), 1E-8);
    TS_ASSERT_DELTA(q(2), sin(0.5*phi), 1E-8);
    TS_ASSERT_DELTA(q(3), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(4), 0.0, 1E-8);

    q = m_th.GetQuaternion();
    TS_ASSERT_DELTA(q(1), cos(0.5*theta), 1E-8);
    TS_ASSERT_DELTA(q(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(3), sin(0.5*theta), 1E-8);
    TS_ASSERT_DELTA(q(4), 0.0, 1E-8);

    q = m_psi.GetQuaternion();
    TS_ASSERT_DELTA(q(1), cos(0.5*psi), 1E-8);
    TS_ASSERT_DELTA(q(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(3), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(4), sin(0.5*psi), 1E-8);

    // These ones are designed to activate specific branches in
    // FGMatrix33::GetQuaternion()
    phi = 100. * M_PI / 180.;
    cphi = cos(phi); sphi = sin(phi);
    m.InitMatrix(1.0, 0.0, 0.0, 0.0, cphi, sphi, 0.0, -sphi, cphi);
    q = m.GetQuaternion();
    TS_ASSERT_DELTA(q(1), cos(0.5*phi), 1E-8);
    TS_ASSERT_DELTA(q(2), sin(0.5*phi), 1E-8);
    TS_ASSERT_DELTA(q(3), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(4), 0.0, 1E-8);
    theta = 100. * M_PI / 180.;
    cth = cos(theta); sth = sin(theta);
    m.InitMatrix(cth, 0.0, -sth, 0.0, 1.0, 0.0, sth, 0.0, cth);
    q = m.GetQuaternion();
    TS_ASSERT_DELTA(q(1), cos(0.5*theta), 1E-8);
    TS_ASSERT_DELTA(q(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(q(3), sin(0.5*theta), 1E-8);
    TS_ASSERT_DELTA(q(4), 0.0, 1E-8);
  }

  void test_angles_psi_270deg()
  {
    double phi = 10. * M_PI / 180.;
    double theta = 45. * M_PI / 180.;
    double psi = 1.5*M_PI;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = cos(theta), sth = sin(theta);
    double cpsi = 0.0, spsi = -1.0;
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
  }

  void test_angles_phi_m90deg()
  {
    double phi = -0.5*M_PI;
    double theta = 10. * M_PI / 180.;
    double psi = 45. * M_PI / 180.;
    double cphi = 0.0, sphi = -1.0;
    double cth = cos(theta), sth = sin(theta);
    double cpsi = cos(psi), spsi = sin(psi);
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
  }

  void test_angles_gimbal_lock_up()
  {
    double phi = 28. * M_PI / 180.;
    double theta = 0.5*M_PI;
    double psi = 0.0;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = 0.0, sth = 1.0;
    double cpsi = 1.0, spsi = 0.0;
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
  }

  void test_angles_gimbal_lock_up2()
  {
    double phi = 28. * M_PI / 180.;
    double theta = 0.5*M_PI;
    double psi = 17. * M_PI / 180.;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = 0.0, sth = 1.0;
    double cpsi = cos(psi), spsi = sin(psi);
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi-psi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
  }

  void test_angles_gimbal_lock_down()
  {
    double phi = 28. * M_PI / 180.;
    double theta = -0.5*M_PI;
    double psi = 0.0;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = 0.0, sth = -1.0;
    double cpsi = 1.0, spsi = 0.0;
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
  }

  void test_angles_gimbal_lock_down2()
  {
    double phi = 28. * M_PI / 180.;
    double theta = -0.5*M_PI;
    double psi = 17. * M_PI / 180.;
    double cphi = cos(phi), sphi = sin(phi);
    double cth = 0.0, sth = -1.0;
    double cpsi = cos(psi), spsi = sin(psi);
    const JSBSim::FGMatrix33 m_phi(1.0,   0.0,  0.0,
                                   0.0,  cphi, sphi,
                                   0.0, -sphi, cphi);
    const JSBSim::FGMatrix33 m_th(cth, 0.0, -sth,
                                  0.0, 1.0,  0.0,
                                  sth, 0.0,  cth);
    const JSBSim::FGMatrix33 m_psi(cpsi,  spsi, 0.0,
                                   -spsi, cpsi, 0.0,
                                   0.0,    0.0, 1.0);
    JSBSim::FGColumnVector3 angles = m_phi.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_th.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
    angles = m_psi.GetEuler();
    TS_ASSERT_DELTA(angles(1), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(2), 0.0, 1E-8);
    TS_ASSERT_DELTA(angles(3), psi, 1E-8);
    JSBSim::FGMatrix33 m = m_phi * m_th * m_psi;
    angles = m.GetEuler();
    TS_ASSERT_DELTA(angles(1), phi+psi, 1E-8);
    TS_ASSERT_DELTA(angles(2), theta, 1E-8);
    TS_ASSERT_DELTA(angles(3), 0.0, 1E-8);
  }
};
