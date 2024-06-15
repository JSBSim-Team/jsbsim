#include <limits>
#include <cxxtest/TestSuite.h>
#include "TestAssertions.h"
#include <math/FGQuaternion.h>

const double epsilon = 100. * std::numeric_limits<double>::epsilon();

class FGQuaternionTest : public CxxTest::TestSuite
{
public:
  void testConstructors() {
    double angle = 10. * M_PI / 180.;
    double ca = cos(angle), sa = sin(angle);
    double ca_half = cos(0.5*angle), sa_half = sin(0.5*angle);
    // Default constructor
    JSBSim::FGQuaternion q;
    TS_ASSERT_EQUALS(1.0, q(1));
    TS_ASSERT_EQUALS(0.0, q(2));
    TS_ASSERT_EQUALS(0.0, q(3));
    TS_ASSERT_EQUALS(0.0, q(4));

    // Several ways to build a quaternion representing a rotation of an angle
    // 'angle' around the X axis
    q = JSBSim::FGQuaternion(1, angle);
    TS_ASSERT_DELTA(ca_half, q(1), epsilon);
    TS_ASSERT_DELTA(sa_half ,q(2), epsilon);
    TS_ASSERT_EQUALS(0.0, q(3));
    TS_ASSERT_EQUALS(0.0, q(4));
    JSBSim::FGQuaternion q2(angle, 0.0, 0.0);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    JSBSim::FGColumnVector3 v(angle, 0.0, 0.0);
    q2 = JSBSim::FGQuaternion(v);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    JSBSim::FGMatrix33 m(1.0, 0.0, 0.0, 0.0, ca, sa, 0.0, -sa, ca);
    q2 = JSBSim::FGQuaternion(m);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    JSBSim::FGColumnVector3 euler = q2.GetEuler();
    TS_ASSERT_DELTA(v(1), euler(1), epsilon);
    TS_ASSERT_DELTA(v(2), euler(2), epsilon);
    TS_ASSERT_DELTA(v(3), euler(3), epsilon);
    JSBSim::FGMatrix33 mT = q2.GetT();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = JSBSim::FGMatrix33(q2);
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = q2.GetTInv();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(j,i), epsilon);

    // around the Y axis
    q = JSBSim::FGQuaternion(2, angle);
    TS_ASSERT_DELTA(ca_half, q(1), epsilon);
    TS_ASSERT_EQUALS(0.0, q(2));
    TS_ASSERT_DELTA(sa_half, q(3), epsilon);
    TS_ASSERT_EQUALS(0.0, q(4));
    q2 = JSBSim::FGQuaternion(0.0, angle, 0.0);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    v.InitMatrix(0.0, angle, 0.0);
    q2 = JSBSim::FGQuaternion(v);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    m.InitMatrix(ca, 0.0, -sa, 0.0, 1.0, 0.0, sa, 0.0, ca);
    q2 = JSBSim::FGQuaternion(m);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    euler = q2.GetEuler();
    TS_ASSERT_DELTA(v(1), euler(1), epsilon);
    TS_ASSERT_DELTA(v(2), euler(2), epsilon);
    TS_ASSERT_DELTA(v(3), euler(3), epsilon);
    mT = q2.GetT();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = JSBSim::FGMatrix33(q2);
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = q2.GetTInv();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(j,i), epsilon);

    // around the Z axis
    q = JSBSim::FGQuaternion(3, angle);
    TS_ASSERT_DELTA(ca_half, q(1), epsilon);
    TS_ASSERT_DELTA(0.0, q(2), epsilon);
    TS_ASSERT_DELTA(0.0, q(3), epsilon);
    TS_ASSERT_DELTA(sa_half, q(4), epsilon);
    q2 = JSBSim::FGQuaternion(0.0, 0.0, angle);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    v.InitMatrix(0.0, 0.0, angle);
    q2 = JSBSim::FGQuaternion(v);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    m.InitMatrix(ca, sa, 0.0, -sa, ca, 0.0, 0.0, 0.0, 1.0);
    q2 = JSBSim::FGQuaternion(m);
    TS_ASSERT_DELTA(q(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q(4), q2(4), epsilon);
    euler = q2.GetEuler();
    TS_ASSERT_DELTA(v(1), euler(1), epsilon);
    TS_ASSERT_DELTA(v(2), euler(2), epsilon);
    TS_ASSERT_DELTA(v(3), euler(3), epsilon);
    mT = q2.GetT();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = JSBSim::FGMatrix33(q2);
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(i,j), epsilon);
    mT = q2.GetTInv();
    for (int i=1; i<=3; i++)
      for (int j=1; j<=3; j++)
        TS_ASSERT_DELTA(m(i,j), mT(j,i), epsilon);

    // Constructor with an angle and an axis of rotation.
    v.InitMatrix(1.0, 2.0, -0.5);
    q2 = JSBSim::FGQuaternion(angle, v);
    v.Normalize();
    TS_ASSERT_DELTA(q2(1), ca_half, epsilon);
    TS_ASSERT_DELTA(q2(2), sa_half*v(1), epsilon);
    TS_ASSERT_DELTA(q2(3), sa_half*v(2), epsilon);
    TS_ASSERT_DELTA(q2(4), sa_half*v(3), epsilon);

    // Initializes to zero.
    q2 = JSBSim::FGQuaternion::zero();
    TS_ASSERT_EQUALS(0.0, q2.Entry(1));
    TS_ASSERT_EQUALS(0.0, q2.Entry(2));
    TS_ASSERT_EQUALS(0.0, q2.Entry(3));
    TS_ASSERT_EQUALS(0.0, q2.Entry(4));
  }

  void testComponentWise() {
    JSBSim::FGQuaternion q(0.5, 1.0, -0.75);
    double x = q(1);
    double y = q(2);
    double z = q(3);
    double w = q(4);
    q.Entry(1) = x + 1.0;
    TS_ASSERT_EQUALS(q.Entry(1), x + 1.0);
    // Check there are no side effects on other components
    TS_ASSERT_EQUALS(q.Entry(2), y);
    TS_ASSERT_EQUALS(q.Entry(3), z);
    TS_ASSERT_EQUALS(q.Entry(4), w);
  }

  void testCopyConstructor() {
    JSBSim::FGQuaternion q0(0.5, 1.0, -0.75);
    JSBSim::FGQuaternion q1(q0); // Copy before updating the cache

    // First make sure that q0 and q1 are identical
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);

    // Second, make sure that q0 and q1 are distinct copies
    // i.e. that q0 and q1 does not point to the same memory location
    double z = q0.Entry(2);
    q1.Entry(2) = 5.0;
    TS_ASSERT_DELTA(z, q0.Entry(2), epsilon); // q0[2] must remain unchanged
    TS_ASSERT_DELTA(5.0, q1.Entry(2), epsilon); // q1[2] must now contain 5.0

    // Force the cache update
    TS_ASSERT_DELTA(0.5, q0.GetEuler(1), epsilon);

    JSBSim::FGQuaternion q2(q0);

    // First make sure that q0 and q2 are identical
    TS_ASSERT_DELTA(q0(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q2(4), epsilon);

    // Second, make sure that q0 and q2 are distinct copies
    // i.e. that q0 and q2 does not point to the same memory location
    z = q0.Entry(2);
    q2.Entry(2) = 5.0;
    TS_ASSERT_DELTA(z, q0.Entry(2), epsilon); // q0[2] must remain unchanged
    TS_ASSERT_DELTA(5.0, q2.Entry(2), epsilon); // q2[2] must now contain 5.0
  }

  void testEquality() {
    JSBSim::FGQuaternion q0(0.5, 1.0, -0.75);
    JSBSim::FGQuaternion q1(q0);
    TS_ASSERT_EQUALS(q0, q1);
    q1(1) += 0.1;
    TS_ASSERT(!(q0 == q1));
    TS_ASSERT_DIFFERS(q0, q1);
    q1(1) = q0(1);
    q1(2) += 0.1;
    TS_ASSERT(!(q0 == q1));
    TS_ASSERT_DIFFERS(q0, q1);
    q1(2) = q0(2);
    q1(3) += 0.1;
    TS_ASSERT(!(q0 == q1));
    TS_ASSERT_DIFFERS(q0, q1);
    q1(3) = q0(3);
    q1(4) += 0.1;
    TS_ASSERT(!(q0 == q1));
    TS_ASSERT_DIFFERS(q0, q1);
  }

  void testAssignment() {
    JSBSim::FGQuaternion q0(0.5, 1.0, -0.75);
    JSBSim::FGQuaternion q1 = q0; // Copy before updating the cache

    // First make sure that q0 and q1 are identical
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);

    // Second, make sure that q0 and q1 are distinct copies
    // i.e. that q0 and q1 does not point to the same memory location
    double z = q0.Entry(2);
    q1.Entry(2) = 5.0;
    TS_ASSERT_DELTA(z, q0.Entry(2), epsilon); // q0[2] must remain unchanged
    TS_ASSERT_DELTA(5.0, q1.Entry(2), epsilon); // q1[2] must now contain 5.0

    const JSBSim::FGQuaternion q2 = q0;

    // First make sure that q0 and q2 are identical
    TS_ASSERT_DELTA(q0(1), q2(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q2(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q2(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q2(4), epsilon);

    // Second, make sure that q0 and q2 are distinct copies
    // i.e. that q0 and q2 does not point to the same memory location
    z = q2.Entry(2);
    q0.Entry(2) = 5.0;
    TS_ASSERT_DELTA(z, q2.Entry(2), epsilon); // q2[2] must remain unchanged
    TS_ASSERT_DELTA(5.0, q0.Entry(2), epsilon); // q0[2] must now contain 5.0

    // Test the assignment of a quaternion with a valid cache.
    q0(3) = -1.5;
    JSBSim::FGMatrix33 m = q0.GetT();
    JSBSim::FGColumnVector3 v = q0.GetEuler();
    q1 = q0;
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);
    TS_ASSERT_VECTOR_EQUALS(v, q1.GetEuler());
    TS_ASSERT_MATRIX_EQUALS(m, q1.GetT());
    TS_ASSERT_MATRIX_EQUALS(m.Transposed(), q1.GetTInv());
  }

  void testEulerAngles() {
    JSBSim::FGQuaternion q0(0.5, 1.0, -0.75);

    // Euler angles in radians
    double x = q0.GetEuler(1);
    double y = q0.GetEuler(2);
    double z = q0.GetEuler(3);
    x = x > M_PI ? x - 2.*M_PI : x;
    x = x < -M_PI ? x + 2.*M_PI : x;
    y = y > M_PI ? y - 2.*M_PI : y;
    y = y < -M_PI ? y + 2.*M_PI : y;
    z = z > M_PI ? z - 2.*M_PI : z;
    z = z < -M_PI ? z + 2.*M_PI : z;
    TS_ASSERT_DELTA(0.5, x, epsilon);
    TS_ASSERT_DELTA(1.0, y, epsilon);
    TS_ASSERT_DELTA(-0.75, z, epsilon);

    JSBSim::FGColumnVector3 euler = q0.GetEuler();
    x = euler(1);
    y = euler(2);
    z = euler(3);
    x = x > M_PI ? x - 2.*M_PI : x;
    x = x < -M_PI ? x + 2.*M_PI : x;
    y = y > M_PI ? y - 2.*M_PI : y;
    y = y < -M_PI ? y + 2.*M_PI : y;
    z = z > M_PI ? z - 2.*M_PI : z;
    z = z < -M_PI ? z + 2.*M_PI : z;
    TS_ASSERT_DELTA(0.5, x, epsilon);
    TS_ASSERT_DELTA(1.0, y, epsilon);
    TS_ASSERT_DELTA(-0.75, z, epsilon);

    // Euler angles in degrees
    q0 = JSBSim::FGQuaternion(M_PI / 3.0, 0.25 * M_PI, -M_PI / 6.0);

    x = q0.GetEulerDeg(1);
    y = q0.GetEulerDeg(2);
    z = q0.GetEulerDeg(3);
    x = x > 180.0 ? x - 360. : x;
    x = x < -180.0 ? x + 360. : x;
    y = y > 180.0 ? y - 360. : y;
    y = y < -180.0 ? y + 360. : y;
    z = z > 180.0 ? z - 360. : z;
    z = z < -180.0 ? z + 360. : z;
    TS_ASSERT_DELTA(60., x, epsilon);
    TS_ASSERT_DELTA(45., y, epsilon);
#ifdef __arm64__
    TS_ASSERT_DELTA(-30., z, epsilon*10.);
#else
    TS_ASSERT_DELTA(-30., z, epsilon);
#endif

    euler = q0.GetEulerDeg();
    x = euler(1);
    y = euler(2);
    z = euler(3);
    x = x > 180.0 ? x - 360. : x;
    x = x < -180.0 ? x + 360. : x;
    y = y > 180.0 ? y - 360. : y;
    y = y < -180.0 ? y + 360. : y;
    z = z > 180.0 ? z - 360. : z;
    z = z < -180.0 ? z + 360. : z;
    TS_ASSERT_DELTA(60., x, epsilon);
    TS_ASSERT_DELTA(45., y, epsilon);
#ifdef __arm64__
    TS_ASSERT_DELTA(-30., z, epsilon*10.);
#else
    TS_ASSERT_DELTA(-30., z, epsilon);
#endif

    // Euler angles sin
    TS_ASSERT_DELTA(0.5*sqrt(3), q0.GetSinEuler(1), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(2), q0.GetSinEuler(2), epsilon);
    TS_ASSERT_DELTA(-0.5, q0.GetSinEuler(3), epsilon);

    // Euler angles cos
    TS_ASSERT_DELTA(0.5, q0.GetCosEuler(1), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(2), q0.GetCosEuler(2), epsilon);
    TS_ASSERT_DELTA(0.5*sqrt(3), q0.GetCosEuler(3), epsilon);

    JSBSim::FGColumnVector3 v = q0.GetEulerDeg();
    TS_ASSERT_DELTA(v(1), euler(1), epsilon);
    TS_ASSERT_DELTA(v(2), euler(2), epsilon);
    TS_ASSERT_DELTA(v(3), euler(3), epsilon);
}

  void testOperations() {
    double angle = 10. * M_PI / 180.;
    const JSBSim::FGQuaternion q0(0.5, 1.0, -0.75);
    const JSBSim::FGQuaternion unit;
    JSBSim::FGQuaternion q1 = q0, zero;

    q1 *= 2.0;
    TS_ASSERT_DELTA(q1(1), 2.0 * q0(1), epsilon);
    TS_ASSERT_DELTA(q1(2), 2.0 * q0(2), epsilon);
    TS_ASSERT_DELTA(q1(3), 2.0 * q0(3), epsilon);
    TS_ASSERT_DELTA(q1(4), 2.0 * q0(4), epsilon);

    q1 = 2.0 * q0;
    TS_ASSERT_DELTA(q1(1), 2.0 * q0(1), epsilon);
    TS_ASSERT_DELTA(q1(2), 2.0 * q0(2), epsilon);
    TS_ASSERT_DELTA(q1(3), 2.0 * q0(3), epsilon);
    TS_ASSERT_DELTA(q1(4), 2.0 * q0(4), epsilon);

    q1 /= 2.0;
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);

    q1 = q0;
    q1 += q0;
    TS_ASSERT_DELTA(q1(1), 2.0 * q0(1), epsilon);
    TS_ASSERT_DELTA(q1(2), 2.0 * q0(2), epsilon);
    TS_ASSERT_DELTA(q1(3), 2.0 * q0(3), epsilon);
    TS_ASSERT_DELTA(q1(4), 2.0 * q0(4), epsilon);

    q1 -= q0;
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);

    q1 = q0 + q0;
    TS_ASSERT_DELTA(q1(1), 2.0 * q0(1), epsilon);
    TS_ASSERT_DELTA(q1(2), 2.0 * q0(2), epsilon);
    TS_ASSERT_DELTA(q1(3), 2.0 * q0(3), epsilon);
    TS_ASSERT_DELTA(q1(4), 2.0 * q0(4), epsilon);

    q1 = q1 - q0;
    TS_ASSERT_DELTA(q0(1), q1(1), epsilon);
    TS_ASSERT_DELTA(q0(2), q1(2), epsilon);
    TS_ASSERT_DELTA(q0(3), q1(3), epsilon);
    TS_ASSERT_DELTA(q0(4), q1(4), epsilon);

    q1 = q0.Conjugate();
    TS_ASSERT_DELTA(q1(1), q0(1), epsilon);
    TS_ASSERT_DELTA(q1(2), -q0(2), epsilon);
    TS_ASSERT_DELTA(q1(3), -q0(3), epsilon);
    TS_ASSERT_DELTA(q1(4), -q0(4), epsilon);

    q1 *= q0;
    TS_ASSERT_DELTA(q0.SqrMagnitude(), q1(1), epsilon);
    TS_ASSERT_DELTA(0.0, q1(2), epsilon);
    TS_ASSERT_DELTA(0.0, q1(3), epsilon);
    TS_ASSERT_DELTA(0.0, q1(4), epsilon);

    q1 = q0.Inverse();
    q1 *= q0;
    TS_ASSERT_DELTA(unit(1), q1(1), epsilon);
    TS_ASSERT_DELTA(unit(2), q1(2), epsilon);
    TS_ASSERT_DELTA(unit(3), q1(3), epsilon);
    TS_ASSERT_DELTA(unit(4), q1(4), epsilon);

    //Check the inverse of a null quaternion
    zero = q1 - q1;
    TS_ASSERT_EQUALS(0.0, zero(1));
    TS_ASSERT_EQUALS(0.0, zero(2));
    TS_ASSERT_EQUALS(0.0, zero(3));
    TS_ASSERT_EQUALS(0.0, zero(4));
    q1 = zero.Inverse();
    TS_ASSERT_EQUALS(q1, zero);

    q1 = JSBSim::FGQuaternion(1, angle);
    q1 = q1.Conjugate();
    JSBSim::FGColumnVector3 euler = q1.GetEuler();
    TS_ASSERT_DELTA(-angle, euler(1), epsilon);
    TS_ASSERT_DELTA(0.0, euler(2), epsilon);
    TS_ASSERT_DELTA(0.0, euler(3), epsilon);

    q1 = q0 * JSBSim::FGQuaternion(1, angle);
    euler = q1.GetEuler();
    double z = euler(3);
    z = z > M_PI ? z - 2.0 * M_PI : z;
    z = z < -M_PI ? z + 2.0 * M_PI : z;
    TS_ASSERT_DELTA(0.5 + angle, euler(1), epsilon);
    TS_ASSERT_DELTA(1.0, euler(2), epsilon);
    TS_ASSERT_DELTA(-0.75, z, epsilon);

    q1 = JSBSim::FGQuaternion(3, angle) * q0;
    euler = q1.GetEuler();
    z = euler(3);
    z = z > M_PI ? z - 2.0 * M_PI : z;
    z = z < -M_PI ? z + 2.0 * M_PI : z;
    TS_ASSERT_DELTA(0.5, euler(1), epsilon);
    TS_ASSERT_DELTA(1.0, euler(2), epsilon);
    TS_ASSERT_DELTA(angle-0.75, z, epsilon);
  }

  void testFunctions() {
    JSBSim::FGColumnVector3 omega(3., 4., 0.);
    omega.Normalize();
    omega *= M_PI / 6.0;
    JSBSim::FGQuaternion q1 = QExp(omega);
    TS_ASSERT_DELTA(0.5 * sqrt(3.0), q1(1), epsilon);
    TS_ASSERT_DELTA(0.3, q1(2), epsilon);
    TS_ASSERT_DELTA(0.4, q1(3), epsilon);
    TS_ASSERT_EQUALS(0.0, q1(4));

    omega.InitMatrix();
    q1 = QExp(omega);
    TS_ASSERT_EQUALS(1.0, q1(1));
    TS_ASSERT_EQUALS(0.0, q1(2));
    TS_ASSERT_EQUALS(0.0, q1(3));
    TS_ASSERT_EQUALS(0.0, q1(4));

    omega(3) = -1.0;
    JSBSim::FGQuaternion q2 = q1.GetQDot(omega);
    JSBSim::FGQuaternion q_omega, qref;
    q_omega(1) = 0.0;
    q_omega(2) = 0.5 * omega(1);
    q_omega(3) = 0.5 * omega(2);
    q_omega(4) = 0.5 * omega(3);
    qref = q_omega * q1;
    TS_ASSERT_EQUALS(qref, q2);
  }

  void testNormalize() {
    JSBSim::FGQuaternion q0, q1, zero;
    q1.Normalize();
    TS_ASSERT_EQUALS(q0, q1);

    zero = q0 - q1;
    // Check that 'zero' is null
    TS_ASSERT_EQUALS(0.0, zero(1));
    TS_ASSERT_EQUALS(0.0, zero(2));
    TS_ASSERT_EQUALS(0.0, zero(3));
    TS_ASSERT_EQUALS(0.0, zero(4));
    // Check that Normalize is a no-op on null quaternions
    zero.Normalize();
    TS_ASSERT_EQUALS(0.0, zero(1));
    TS_ASSERT_EQUALS(0.0, zero(2));
    TS_ASSERT_EQUALS(0.0, zero(3));
    TS_ASSERT_EQUALS(0.0, zero(4));

    // Test the normalization of quaternion which magnitude is neither zero nor
    // unity.
    JSBSim::FGColumnVector3 v(1.0, 2.0, -0.5);
    q0 = JSBSim::FGQuaternion(0.4, v);
    double x = q0(1);
    double y = q0(2);
    double z = q0(3);
    double w = q0(4);
    q0 *= 2.0;
    TS_ASSERT_DELTA(q0(1), 2.0*x, epsilon);
    TS_ASSERT_DELTA(q0(2), 2.0*y, epsilon);
    TS_ASSERT_DELTA(q0(3), 2.0*z, epsilon);
    TS_ASSERT_DELTA(q0(4), 2.0*w, epsilon);
    q0.Normalize();
    TS_ASSERT_DELTA(q0(1), x, epsilon);
    TS_ASSERT_DELTA(q0(2), y, epsilon);
    TS_ASSERT_DELTA(q0(3), z, epsilon);
    TS_ASSERT_DELTA(q0(4), w, epsilon);
  }

  void testOutput() {
    JSBSim::FGQuaternion q;
    std::string s = q.Dump(" , ");
    TS_ASSERT_EQUALS(std::string("1 , 0 , 0 , 0"), s);
    std::ostringstream os;
    os << q;
    TS_ASSERT_EQUALS(std::string("1 , 0 , 0 , 0"), os.str());
  }
};
