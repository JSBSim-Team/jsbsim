# TestRotorZeroThrust.py
#
# Check that the induced inflow of a rotor settles near zero thrust.
#
# Copyright (c) 2026 Alexander Kalmykov
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>
#

import math

from JSBSim_utils import JSBSimTestCase, RunTest

ROTOR = 'propulsion/engine[0]/'
OGE_HEIGHT_FT = 500.0  # the ah1s ground effect vanishes around 50 ft
TWIST_RAD = -0.175  # ah1s_rotor.xml, with a tip loss factor of 1
# At rest (mu = lambda = 0) the thrust coefficient of /SH79/ eqn(27) is
# a/2*sigma*(theta_0/3 + twist/4), zero at this collective pitch.
ZERO_THRUST_PITCH_RAD = -0.75 * TWIST_RAD
PITCH_OFFSET_RAD = math.radians(0.01)
SETTLE_TIME_S = 10.0  # the inflow lag of the ah1s rotor is 0.09 s
WINDOW_S = 1.0
RTOL = 1e-4


class TestRotorZeroThrust(JSBSimTestCase):
    def hold_collective(self, dt, pitch_rad):
        fdm = self.create_fdm()
        # FGRotor reads the time step when the model is loaded.
        fdm.set_dt(dt)
        fdm.load_model('ah1s')
        fdm.load_ic('reset00', True)
        fdm['ic/h-agl-ft'] = OGE_HEIGHT_FT
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()
        fdm['fcs/rpm-governor-active-norm'] = 1.0
        # The ah1s collective is gain * stick + bias, in radians.
        fdm['fcs/adj/collective-gain'] = 0.0
        fdm['fcs/adj/collective-bias'] = pitch_rad
        while fdm.get_sim_time() < SETTLE_TIME_S:
            fdm.run()

        samples = []
        while fdm.get_sim_time() < SETTLE_TIME_S + WINDOW_S:
            fdm.run()
            samples.append((fdm[ROTOR + 'induced-inflow-ratio'],
                            fdm[ROTOR + 'thrust-coefficient'],
                            fdm[ROTOR + 'advance-ratio'],
                            fdm[ROTOR + 'inflow-ratio']))
        self.assertAlmostEqual(fdm[ROTOR + 'collective-ctrl-rad'], pitch_rad,
                               delta=1e-9)
        return samples

    def check_settled(self, dt, pitch_rad):
        samples = self.hold_collective(dt, pitch_rad)
        nu_end = samples[-1][0]
        for nu, ct, mu, lam in samples:
            # The inflow follows the sign of the thrust and holds still.
            self.assertGreater(nu * ct, 0.0)
            self.assertAlmostEqual(nu / nu_end, 1.0, delta=RTOL)
            # Momentum theory: nu = Ct/(2*sqrt(mu^2+lambda^2))
            self.assertAlmostEqual(nu * 2.0 * math.sqrt(mu*mu + lam*lam) / ct,
                                   1.0, delta=RTOL)
        return nu_end

    def check_time_step_independence(self, pitch_rad):
        nu_ref = self.check_settled(1/120, pitch_rad)
        nu = self.check_settled(1/1000, pitch_rad)
        self.assertAlmostEqual(nu / nu_ref, 1.0, delta=RTOL)
        return nu

    def test_just_above_zero_thrust(self):
        nu = self.check_time_step_independence(ZERO_THRUST_PITCH_RAD
                                               + PITCH_OFFSET_RAD)
        self.assertGreater(nu, 0.0)

    def test_just_below_zero_thrust(self):
        nu = self.check_time_step_independence(ZERO_THRUST_PITCH_RAD
                                               - PITCH_OFFSET_RAD)
        self.assertLess(nu, 0.0)

RunTest(TestRotorZeroThrust)
