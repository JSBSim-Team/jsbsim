# TestThrusterReset.py
#
# Check that a reset brings propellers and rotors back to rest so that the same
# inputs give the same trajectory before and after a reset.
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

from JSBSim_utils import JSBSimTestCase, RunTest

STATE = ['position/lat-geod-rad', 'position/long-gc-rad', 'position/h-sl-ft',
         'attitude/phi-rad', 'attitude/theta-rad', 'attitude/psi-rad',
         'velocities/u-fps', 'velocities/v-fps', 'velocities/w-fps',
         'velocities/p-rad_sec', 'velocities/q-rad_sec', 'velocities/r-rad_sec']


class TestThrusterReset(JSBSimTestCase):
    def run_steps(self, fdm, inputs, outputs, steps):
        trajectory = []
        for _ in range(steps):
            # The commands are zeroed by the reset, so they are set at each step.
            for name, value in inputs.items():
                fdm[name] = value
            fdm.run()
            trajectory.append([fdm[p] for p in outputs])
        return trajectory

    def check_reset(self, aircraft, ic, inputs, thruster_props, steps):
        fdm = self.create_fdm()
        fdm.load_model(aircraft)
        fdm.load_ic(ic, True)
        fdm.run_ic()
        outputs = thruster_props + STATE
        initial = [fdm[p] for p in outputs]

        ref = self.run_steps(fdm, inputs, outputs, steps)
        # The thrusters must have left their initial state before the reset.
        for name, value in zip(thruster_props, initial):
            self.assertNotEqual(fdm[name], value)

        fdm.reset_to_initial_conditions(0)
        at_reset = [fdm[p] for p in outputs]
        self.assertEqual(at_reset, initial)
        trajectory = self.run_steps(fdm, inputs, outputs, steps)
        for step, (state, ref_state) in enumerate(zip(trajectory, ref)):
            self.assertEqual(state, ref_state, msg='step %d' % step)
        return at_reset

    def test_propeller(self):
        # Brushless DC motors: their engine model has no ResetToIC() of its own.
        rpm_props = ['propulsion/engine[%d]/propeller-rpm' % i for i in range(4)]
        at_reset = self.check_reset('F450', 'initGrnd',
                                    {'fcs/throttle-cmd-norm': 0.6}, rpm_props,
                                    240)
        self.assertEqual(at_reset[:4], [0.0]*4)

    def test_rotor(self):
        # Electric engine driving the main rotor through a transmission, the
        # tail rotor RPM is slaved to the main rotor.
        self.check_reset('ah1s', 'reset00', {'fcs/rpm-governor-active-norm': 1.0},
                         ['propulsion/engine[0]/rotor-rpm',
                          'propulsion/engine[0]/engine-rpm',
                          'propulsion/engine[1]/rotor-rpm'], 600)

    def test_constant_speed_propeller(self):
        # The piston engine already stops its propeller on reset, this checks
        # that the blade angle goes back to its initial value too.
        self.check_reset('c310', 'reset00',
                         {'propulsion/magneto_cmd': 3,
                          'propulsion/starter_cmd': 1,
                          'fcs/mixture-cmd-norm[0]': 1.0,
                          'fcs/throttle-cmd-norm[0]': 1.0,
                          'fcs/advance-cmd-norm[0]': 0.5},
                         ['propulsion/engine[0]/blade-angle'], 600)

RunTest(TestThrusterReset)
