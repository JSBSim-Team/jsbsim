/* FGAccelerationsTest.h
 *
 * Checks the ground friction and wheel spin constraints solved by
 * FGAccelerations against values derived by hand.
 *
 * Copyright (c) 2026 Felipegalind0
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>
 */

#include <cmath>
#include <memory>
#include <vector>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGAccelerations.h>
#include <math/LagrangeMultiplier.h>
#include "TestAssertions.h"

using namespace JSBSim;

// The projected Gauss-Seidel iterations stop once the multipliers change by
// less than 1E-5 in total, so the results are only that accurate.
const double epsilon = 1e-5;
// Constraint velocities after one step of the solved accelerations.
const double residual_tol = 1e-6;
// Quantities that no multiplier acts on must remain exactly zero.
const double zero_tol = 1e-12;

#define TS_ASSERT_VECTOR_IS_ZERO(x) \
  assertVectorEqual(__FILE__, __LINE__, x, FGColumnVector3(), zero_tol)

constexpr double dt = 0.1;
constexpr double mass = 2.0;
constexpr double Jyy = 8.0;

// Body axes: x forward, y right, z down. The wheel axle is 1 ft below the CG
// and the wheel has a radius of 1 ft and a spin inertia of 1 slug*ft^2. Its
// positive spin (forward rolling) is about -y.
//
// Rolling friction acts along x at the contact point, 1 ft below the axle. It
// reaches the airframe through the axle, where it applies the moment
// (0,0,1) x (1,0,0) = (0,1,0), and applies the torque -R = -1 to the wheel.
// Its constraint velocity is u + q - Rate.
//
// A brake torque acts between the wheel and the airframe: +1 on the wheel and
// (0,1,0) on the airframe. Its constraint velocity is the spin relative to the
// airframe, Rate + q.

LagrangeMultiplier MakeRow(const FGColumnVector3& force,
                           const FGColumnVector3& moment,
                           WheelSpinDOF* wheel, double wheelJacobian)
{
  LagrangeMultiplier row;
  row.ForceJacobian = force;
  row.MomentJacobian = moment;
  row.Min = -100.;
  row.Max = 100.;
  row.value = 0.;
  row.Wheel = wheel;
  row.WheelJacobian = wheelJacobian;
  return row;
}

LagrangeMultiplier RollRow(WheelSpinDOF* wheel)
{
  return MakeRow(FGColumnVector3(1., 0., 0.), FGColumnVector3(0., 1., 0.),
                 wheel, wheel ? -1. : 0.);
}

LagrangeMultiplier BrakeRow(WheelSpinDOF* wheel)
{
  return MakeRow(FGColumnVector3(), FGColumnVector3(0., 1., 0.), wheel, 1.);
}

WheelSpinDOF MakeWheel(double rate)
{
  WheelSpinDOF wheel;
  wheel.Jinv = 1.;
  wheel.Rate = rate;
  wheel.Accel = 0.;
  return wheel;
}

// Sets every input of the acceleration model so that only the friction rows
// act: no applied force or moment, no gravity, no planet or terrain motion and
// identity frame transformations.
std::shared_ptr<FGAccelerations> SetInputs(FGFDMExec& fdmex,
                                           std::vector<LagrangeMultiplier*>& rows,
                                           const FGColumnVector3& vUVW,
                                           const FGColumnVector3& vPQR,
                                           double deltaT = dt)
{
  fdmex.SetHoldDown(false);
  fdmex.SetPropertyValue("simulation/gravitational-torque", 0.0);

  auto accel = fdmex.GetAccelerations();
  TS_ASSERT(accel->InitModel());

  const FGMatrix33 identity(1., 0., 0.,
                            0., 1., 0.,
                            0., 0., 1.);
  auto& in = accel->in;
  in.J = FGMatrix33(4., 0., 0.,
                    0., Jyy, 0.,
                    0., 0., 16.);
  in.Jinv = FGMatrix33(1./4., 0., 0.,
                       0., 1./Jyy, 0.,
                       0., 0., 1./16.);
  in.Ti2b = identity;
  in.Tb2i = identity;
  in.Tec2b = identity;
  in.Tec2i = identity;
  in.Moment = FGColumnVector3();
  in.GroundMoment = FGColumnVector3();
  in.Force = FGColumnVector3();
  in.GroundForce = FGColumnVector3();
  in.vGravAccel = FGColumnVector3();
  in.vPQRi = vPQR; // The planet does not rotate.
  in.vPQR = vPQR;
  in.vUVW = vUVW;
  in.vInertialPosition = FGColumnVector3();
  in.vOmegaPlanet = FGColumnVector3();
  in.TerrainVelocity = FGColumnVector3();
  in.TerrainAngularVel = FGColumnVector3();
  in.DeltaT = deltaT;
  in.Mass = mass;
  in.MultipliersList = &rows;

  return accel;
}

class FGAccelerationsTest : public CxxTest::TestSuite
{
public:
  void testFrictionWithoutWheel() {
    LagrangeMultiplier roll = RollRow(nullptr);
    std::vector<LagrangeMultiplier*> rows {&roll};
    FGFDMExec fdmex;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(2., 0., 0.),
                           FGColumnVector3());

    TS_ASSERT(!accel->Run(false));

    // A = 1/2 + 1/8 = 5/8, rhs = -u/dt = -20
    TS_ASSERT_DELTA(roll.value, -32., epsilon);
    TS_ASSERT_VECTOR_EQUALS(accel->GetForces(), FGColumnVector3(-32., 0., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetMoments(), FGColumnVector3(0., -32., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetUVWdot(), FGColumnVector3(-16., 0., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetPQRdot(), FGColumnVector3(0., -4., 0.));
  }

  void testFreeWheel() {
    WheelSpinDOF wheel = MakeWheel(0.);
    LagrangeMultiplier roll = RollRow(&wheel);
    std::vector<LagrangeMultiplier*> rows {&roll};
    FGFDMExec fdmex;
    const double u = 2.;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(u, 0., 0.),
                           FGColumnVector3());

    TS_ASSERT(!accel->Run(false));

    // A = 1/2 + 1/8 + 1 = 13/8, rhs = -20
    const double lambda = -160./13.;
    TS_ASSERT_DELTA(roll.value, lambda, epsilon);
    TS_ASSERT_DELTA(wheel.Accel, 160./13., epsilon);
    TS_ASSERT_DELTA(wheel.Rate, 0., zero_tol);
    TS_ASSERT_VECTOR_EQUALS(accel->GetForces(), FGColumnVector3(lambda, 0., 0.));
    // The airframe only gets the moment of the force at the axle. The moment
    // about the axle spins the wheel and must not also act on the airframe.
    TS_ASSERT_VECTOR_EQUALS(accel->GetMoments(), FGColumnVector3(0., lambda, 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetUVWdot(),
                            FGColumnVector3(lambda/mass, 0., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetPQRdot(),
                            FGColumnVector3(0., lambda/Jyy, 0.));

    // The wheel rolls without slipping after the step.
    double u1 = u + dt * accel->GetUVWdot(FGJSBBase::eU);
    double q1 = dt * accel->GetPQRdot(FGJSBBase::eQ);
    double rate1 = wheel.Rate + dt * wheel.Accel;
    TS_ASSERT_DELTA(u1 + q1 - rate1, 0., residual_tol);
  }

  void testBrakeBetweenWheelAndAirframe() {
    WheelSpinDOF wheel = MakeWheel(3.);
    LagrangeMultiplier brake = BrakeRow(&wheel);
    std::vector<LagrangeMultiplier*> rows {&brake};
    FGFDMExec fdmex;
    const double q = 2.;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(),
                           FGColumnVector3(0., q, 0.));

    TS_ASSERT(!accel->Run(false));

    // Relative spin Rate + q = 5, A = 1/8 + 1 = 9/8, rhs = -50
    TS_ASSERT_DELTA(brake.value, -400./9., epsilon);
    TS_ASSERT_DELTA(wheel.Accel, -400./9., epsilon);
    TS_ASSERT_DELTA(wheel.Rate, 3., zero_tol);
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetForces());
    TS_ASSERT_VECTOR_EQUALS(accel->GetMoments(),
                            FGColumnVector3(0., -400./9., 0.));
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetUVWdot());
    TS_ASSERT_VECTOR_EQUALS(accel->GetPQRdot(), FGColumnVector3(0., -50./9., 0.));

    // The brake stops the relative spin within the step, and the angular
    // momentum about y of the airframe and the wheel (spinning about -y) is
    // unchanged.
    double q1 = q + dt * accel->GetPQRdot(FGJSBBase::eQ);
    double rate1 = wheel.Rate + dt * wheel.Accel;
    TS_ASSERT_DELTA(rate1 + q1, 0., residual_tol);
    TS_ASSERT_DELTA(Jyy*q1 - rate1, 13., residual_tol);
  }

  void testRollAndBrakeOnSameWheel() {
    WheelSpinDOF wheel = MakeWheel(0.);
    LagrangeMultiplier roll = RollRow(&wheel);
    LagrangeMultiplier brake = BrakeRow(&wheel);
    std::vector<LagrangeMultiplier*> rows {&roll, &brake};
    FGFDMExec fdmex;
    const double u = 2.;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(u, 0., 0.),
                           FGColumnVector3());

    TS_ASSERT(!accel->Run(false));

    // A = [[13/8, -7/8], [-7/8, 9/8]], rhs = [-20, 0]. The off-diagonal term
    // is 1/8 through the airframe pitch and -1 through the shared wheel.
    TS_ASSERT_DELTA(roll.value, -360./17., epsilon);
    TS_ASSERT_DELTA(brake.value, -280./17., epsilon);
    TS_ASSERT_DELTA(wheel.Accel, -roll.value + brake.value, epsilon);
    TS_ASSERT_DELTA(wheel.Accel, 80./17., epsilon);
    TS_ASSERT_VECTOR_EQUALS(accel->GetForces(),
                            FGColumnVector3(-360./17., 0., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetMoments(),
                            FGColumnVector3(0., -640./17., 0.));

    // The wheel rolls without slipping and without spinning relative to the
    // airframe after the step.
    double u1 = u + dt * accel->GetUVWdot(FGJSBBase::eU);
    double q1 = dt * accel->GetPQRdot(FGJSBBase::eQ);
    double rate1 = wheel.Rate + dt * wheel.Accel;
    TS_ASSERT_DELTA(u1 + q1 - rate1, 0., residual_tol);
    TS_ASSERT_DELTA(rate1 + q1, 0., residual_tol);
  }

  void testBrakeAtItsLimit() {
    WheelSpinDOF wheel = MakeWheel(3.);
    LagrangeMultiplier brake = BrakeRow(&wheel);
    brake.Min = -2.;
    brake.Max = 2.;
    brake.value = 10.; // Warm start outside the bounds
    std::vector<LagrangeMultiplier*> rows {&brake};
    FGFDMExec fdmex;
    const double q = 2.;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(),
                           FGColumnVector3(0., q, 0.));

    TS_ASSERT(!accel->Run(false));

    // The unbounded solution -400/9 is projected on [-2, 2].
    TS_ASSERT_DELTA(brake.value, -2., epsilon);
    TS_ASSERT_DELTA(wheel.Accel, -2., epsilon);
    TS_ASSERT_VECTOR_EQUALS(accel->GetMoments(), FGColumnVector3(0., -2., 0.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetPQRdot(), FGColumnVector3(0., -0.25, 0.));

    // The brake slows the relative spin without stopping it.
    double q1 = q + dt * accel->GetPQRdot(FGJSBBase::eQ);
    double rate1 = wheel.Rate + dt * wheel.Accel;
    TS_ASSERT_DELTA(rate1 + q1, 4.775, epsilon);
    TS_ASSERT_LESS_THAN(rate1 + q1, 5.);
    TS_ASSERT_LESS_THAN(0., rate1 + q1);

    // The torques on the airframe and on the wheel are equal and opposite.
    // The wheel spins about -y, so its torque about y is -Accel/Jinv.
    TS_ASSERT_DELTA(accel->GetMoments(FGJSBBase::eM) - wheel.Accel/wheel.Jinv,
                    0., epsilon);
  }

  void testRowsOnDifferentWheels() {
    WheelSpinDOF wheel1 = MakeWheel(1.);
    WheelSpinDOF wheel2 = MakeWheel(-2.);
    // Orthogonal force directions without moments, chosen so that the rows
    // could only be coupled through the wheels.
    LagrangeMultiplier roll1 = MakeRow(FGColumnVector3(1., 0., 0.),
                                       FGColumnVector3(), &wheel1, -1.);
    LagrangeMultiplier roll2 = MakeRow(FGColumnVector3(0., 0., 1.),
                                       FGColumnVector3(), &wheel2, -1.);
    std::vector<LagrangeMultiplier*> rows {&roll1, &roll2};
    FGFDMExec fdmex;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(), FGColumnVector3());

    TS_ASSERT(!accel->Run(false));

    // A = diag(3/2, 3/2), rhs = [10, -20]
    TS_ASSERT_DELTA(roll1.value, 20./3., epsilon);
    TS_ASSERT_DELTA(roll2.value, -40./3., epsilon);
    TS_ASSERT_DELTA(wheel1.Accel, -20./3., epsilon);
    TS_ASSERT_DELTA(wheel2.Accel, 40./3., epsilon);
    TS_ASSERT_VECTOR_EQUALS(accel->GetForces(),
                            FGColumnVector3(20./3., 0., -40./3.));
    TS_ASSERT_VECTOR_EQUALS(accel->GetUVWdot(),
                            FGColumnVector3(10./3., 0., -20./3.));
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetMoments());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetPQRdot());
  }

  void testZeroTimeStep() {
    WheelSpinDOF wheel = MakeWheel(3.);
    wheel.Accel = 123.; // Must be reset by the solve
    LagrangeMultiplier brake = BrakeRow(&wheel);
    std::vector<LagrangeMultiplier*> rows {&brake};
    FGFDMExec fdmex;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(),
                           FGColumnVector3(0., 2., 0.), 0.);

    TS_ASSERT(!accel->Run(false));

    // Without a time step no velocity is driven to zero, so no force acts.
    TS_ASSERT(std::isfinite(brake.value));
    TS_ASSERT(std::isfinite(wheel.Accel));
    TS_ASSERT_DELTA(brake.value, 0., zero_tol);
    TS_ASSERT_DELTA(wheel.Accel, 0., zero_tol);
    TS_ASSERT_DELTA(wheel.Rate, 3., zero_tol);
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetForces());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetMoments());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetUVWdot());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetPQRdot());
  }

  void testEmptyMultiplierList() {
    LagrangeMultiplier roll = RollRow(nullptr);
    std::vector<LagrangeMultiplier*> rows {&roll};
    FGFDMExec fdmex;
    auto accel = SetInputs(fdmex, rows, FGColumnVector3(2., 0., 0.),
                           FGColumnVector3());

    TS_ASSERT(!accel->Run(false));
    TS_ASSERT_DELTA(accel->GetForces(FGJSBBase::eX), -32., epsilon);

    // The friction from the previous solve is cleared once no contact is left.
    rows.clear();
    TS_ASSERT(!accel->Run(false));
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetForces());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetMoments());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetUVWdot());
    TS_ASSERT_VECTOR_IS_ZERO(accel->GetPQRdot());
  }
};
