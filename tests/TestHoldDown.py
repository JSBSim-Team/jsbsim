# TestHoldDown.py
#
# Test the hold down feature (property forces/hold-down)
#
# Copyright (c) 2015 Bertrand Coconnier
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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, ExecuteUntil


class TestHoldDown(JSBSimTestCase):
    def test_static_hold_down(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('J246')
        fdm.load_ic('LC39', True)
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()
        h0 = fdm['position/vrp-radius-ft']
        t = 0.0

        while t < 420.0:
            fdm.run()
            t = fdm['simulation/sim-time-sec']
            self.assertAlmostEqual(fdm['position/vrp-radius-ft']/h0, 1.0,
                                   delta=1E-9)

    def test_dynamic_hold_down(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('J246')
        fdm.load_ic('LC39', True)
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()
        h0 = fdm['position/vrp-radius-ft']
        # Start solid rocket boosters
        fdm['fcs/throttle-cmd-norm[0]'] = 1.0
        fdm['fcs/throttle-cmd-norm[1]'] = 1.0
        t = 0.0

        while t < 420.0:
            fdm.run()
            t = fdm['simulation/sim-time-sec']
            self.assertAlmostEqual(fdm['position/vrp-radius-ft']/h0, 1.0,
                                   delta=1E-9)

    def test_hold_down_with_gnd_reactions(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'c1721.xml'))
        fdm.run_ic()
        ExecuteUntil(fdm, 0.25)

        fdm['forces/hold-down'] = 1.0
        h0 = fdm['position/h-sl-ft']
        pitch = fdm['attitude/pitch-rad']
        roll = fdm['attitude/roll-rad']
        heading = fdm['attitude/heading-true-rad']

        while fdm['simulation/sim-time-sec'] < 2.0:
            fdm.run()
            self.assertAlmostEqual(fdm['accelerations/pdot-rad_sec2'], 0.0)
            self.assertAlmostEqual(fdm['accelerations/qdot-rad_sec2'], 0.0)
            self.assertAlmostEqual(fdm['accelerations/rdot-rad_sec2'], 0.0)
            self.assertAlmostEqual(fdm['accelerations/udot-ft_sec2'], 0.0)
            self.assertAlmostEqual(fdm['accelerations/vdot-ft_sec2'], 0.0)
            self.assertAlmostEqual(fdm['accelerations/wdot-ft_sec2'], 0.0)

        self.assertAlmostEqual(fdm['position/h-sl-ft'], h0, delta=1E-6)
        self.assertAlmostEqual(fdm['attitude/pitch-rad'], pitch)
        self.assertAlmostEqual(fdm['attitude/roll-rad'], roll)
        self.assertAlmostEqual(fdm['attitude/heading-true-rad'], heading)

RunTest(TestHoldDown)
