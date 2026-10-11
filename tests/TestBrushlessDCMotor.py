# TestBrushlessDCMotor.py
#
# Check that the friction of a brushless DC motor resists its rotation: a motor
# whose throttle is cut stops its propeller, and while it brakes its friction
# adds to the braking torque.
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

import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest

ENGINES = range(4)
SPIN_UP_THROTTLE = 0.3  # Below the F450 hover throttle: it stays on the ground.
SPIN_UP_STEPS = 480  # 4 seconds at default 120Hz
STOP_STEPS = 120  # 1 second at 120Hz


class TestBrushlessDCMotor(JSBSimTestCase):
    def setUp(self, *args):
        JSBSimTestCase.setUp(self, *args)
        motor = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         'DJI_E305.xml'))
        self.max_volts = float(motor.find('maxvolts').text)
        self.kv = float(motor.find('velocityconstant').text)
        self.resistance = float(motor.find('coilresistance').text)
        self.i0 = float(motor.find('noloadcurrent').text)

        self.fdm = self.create_fdm()
        self.fdm.load_model('F450')
        self.fdm.load_ic('initGrnd', True)
        self.fdm.run_ic()

    def run_steps(self, throttle, steps):
        self.fdm['fcs/throttle-cmd-norm'] = throttle
        for _ in range(steps):
            self.fdm.run()

    def torque_per_friction_free_current(self, n):
        # Motor torque, up to a constant factor, over the current that remains
        # once the friction is overcome by a turning shaft: (5) in Drela's
        # "First-Order DC Electric Motor Model".
        engine = f'propulsion/engine[{n}]/'
        current = self.fdm[engine + 'current-amperes']
        volts = self.max_volts * self.fdm[f'fcs/throttle-pos-norm[{n}]']
        rpm = (volts - current * self.resistance) * self.kv
        return self.fdm[engine + 'power-hp'] / rpm / (current - self.i0)

    def test_cut_throttle_stops_propellers(self):
        self.run_steps(SPIN_UP_THROTTLE, SPIN_UP_STEPS)
        for n in ENGINES:
            self.assertGreater(
                self.fdm[f'propulsion/engine[{n}]/propeller-rpm'], 1000.0)

        self.run_steps(0.0, STOP_STEPS)
        stall_rpm = self.i0*self.resistance*self.kv
        for n in ENGINES:
            self.assertEqual(self.fdm[f'fcs/throttle-pos-norm[{n}]'], 0.0)
            # The friction used to vanish below I0*R*Kv, where a motor whose
            # throttle is cut no longer draws I0 from its back EMF.
            self.assertEqual(self.fdm[f'propulsion/engine[{n}]/propeller-rpm'],
                             0.0, msg=f'engine {n}, the friction vanishes '
                             f'below {stall_rpm:.1f} RPM')

    def test_friction_resists_rotation(self):
        self.run_steps(SPIN_UP_THROTTLE, SPIN_UP_STEPS)
        driving = [self.torque_per_friction_free_current(n) for n in ENGINES]

        self.fdm['fcs/throttle-cmd-norm'] = 0.0
        # Effectors.xml runs before FlightControl.xml: the cut needs 2 frames.
        self.fdm.run()
        self.fdm.run()

        for n in ENGINES:
            current = self.fdm[f'propulsion/engine[{n}]/current-amperes']
            self.assertLess(current, -self.i0)
            braking = self.torque_per_friction_free_current(n)
            self.assertAlmostEqual(braking / driving[n], 1.0, places=9)


RunTest(TestBrushlessDCMotor)
