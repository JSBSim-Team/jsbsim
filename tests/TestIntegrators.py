# TestIntegrators.py
#
# Test that integrators in <pid> and <filter> are functional.
#
# Copyright (c) 2018 Bertrand Coconnier
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
from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel


class FDMIntegrators(FlightModel):
    def before_loading(self):
        self.fdm.set_dt(0.005)

class TestIntegrators(JSBSimTestCase):
    def start_fdm(self):
        tripod = FDMIntegrators(self, 'tripod')
        tripod.include_system_test_file('integrators.xml')
        return tripod.start()

    def test_integrators(self):
        fdm = self.start_fdm()

        self.assertAlmostEqual(fdm['test/output-pid-rect'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-trap'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab2'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab3'], 0.0)
        self.assertAlmostEqual(fdm['test/output-integrator'], 0.0)

        k = 8*math.pi # Constant for a period of 1/4s

        for i in range(100):
            t = fdm['simulation/sim-time-sec']
            fdm['test/input'] = math.sin(k*t)
            # Skip the first value because Adams-Bashforth 3 is not yet
            # correctly initialized.
            if i>1:
                self.assertAlmostEqual(fdm['test/output-pid-ab3'],
                                       (1.0-math.cos(k*t))/k, delta=1E-4)

            # Check that <integrator> is giving the same output than <pid> with
            # a trapezoidal integration scheme.
            self.assertAlmostEqual(fdm['test/output-pid-trap'],
                                   fdm['test/output-integrator'], delta=1E-12)
            fdm.run()

        # Checks that a positive trigger value suspends the integration
        fdm['test/trigger'] = 1.0
        vrect = fdm['test/output-pid-rect']
        vtrap = fdm['test/output-pid-trap']
        vab2 = fdm['test/output-pid-ab2']
        vab3 = fdm['test/output-pid-ab3']
        vinteg = fdm['test/output-integrator']
        for i in range(49):
            fdm.run()
            self.assertAlmostEqual(fdm['test/output-pid-rect'], vrect)
            self.assertAlmostEqual(fdm['test/output-pid-trap'], vtrap)
            self.assertAlmostEqual(fdm['test/output-pid-ab2'], vab2)
            self.assertAlmostEqual(fdm['test/output-pid-ab3'], vab3)
            self.assertAlmostEqual(fdm['test/output-integrator'], vinteg)

        # Checks that a negative trigger value resets the integration
        fdm['test/trigger'] = -1.0
        # The input needs to be set to 0.0 for a correct initialization of the
        # Adams-Bashforth schemes.
        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/output-pid-rect'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-trap'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab2'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab3'], 0.0)
        self.assertAlmostEqual(fdm['test/output-integrator'], 0.0)

        # Checks that resetting the trigger to zero restarts the integration
        fdm['test/trigger'] = 0.0
        t0 = fdm['simulation/sim-time-sec']
        for i in range(50):
            t = fdm['simulation/sim-time-sec']
            fdm['test/input'] = math.sin(k*(t-t0))
            # Skip the first value because Adams-Bashforth 3 is not yet
            # correctly initialized.
            if i>1:
                self.assertAlmostEqual(fdm['test/output-pid-ab3'],
                                       (1.0-math.cos(k*(t-t0)))/k, delta=1E-4)
            # Check that <integrator> is giving the same output than <pid> with
            # a trapezoidal integration scheme.
            self.assertAlmostEqual(fdm['test/output-pid-trap'],
                                   fdm['test/output-integrator'], delta=1E-12)
            fdm.run()

    def test_pid(self):
        fdm = self.start_fdm()

        self.assertAlmostEqual(fdm['pid/negative-combined'], 0.0)
        self.assertAlmostEqual(fdm['pid/kp-alone'], 0.0)
        self.assertAlmostEqual(fdm['pid/ki-alone'], 0.0)
        self.assertAlmostEqual(fdm['pid/kd-alone'], 0.0)
        self.assertAlmostEqual(fdm['pid/kp-alone-inverted-input-sign'], 0.0)
        self.assertAlmostEqual(fdm['pid/ki-alone-inverted-input-sign'], 0.0)
        self.assertAlmostEqual(fdm['pid/kd-alone-inverted-input-sign'], 0.0)

        k = 2*math.pi # Constant for a period of 1s
        kp = 2.0
        ki = 0.5
        kd = -1.5
        fdm['test/kp'] = kp
        fdm['test/ki'] = ki
        fdm['test/kd'] = kd

        for i in range(100):
            t = fdm['simulation/sim-time-sec']
            fdm['test/input'] = math.sin(k*t)

            self.assertAlmostEqual(fdm['pid/ki-alone'],
                                   ki*(1.0-math.cos(k*t))/k, delta=1E-4)
            self.assertAlmostEqual(fdm['pid/ki-alone-inverted-input-sign'],
                                   -ki*(1.0-math.cos(k*t))/k, delta=1E-4)

            self.assertAlmostEqual(-fdm['pid/negative-combined'],
                                   fdm['pid/kp-alone']+fdm['pid/ki-alone']+fdm['pid/kd-alone'])
            fdm.run()

            if i>1:
                self.assertAlmostEqual(fdm['pid/kd-alone'], kd*k*math.cos(k*t),
                                       delta=0.15)
                self.assertAlmostEqual(fdm['pid/kd-alone-inverted-input-sign'],
                                       -kd*k*math.cos(k*t), delta=0.15)
            self.assertAlmostEqual(fdm['pid/kp-alone'],
                                   kp*math.sin(k*t), delta=1E-4)
            self.assertAlmostEqual(fdm['pid/kp-alone-inverted-input-sign'],
                                   -kp*math.sin(k*t), delta=1E-4)


RunTest(TestIntegrators)
