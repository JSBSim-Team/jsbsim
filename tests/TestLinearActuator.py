# TestDeadBand.py
#
# Test that the <linear_actuator> component is functional.
#
# Copyright (c) 2021 Bertrand Coconnier
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

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel


class TestLinearActuator(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('linear_actuator.xml')
        self.fdm = tripod.start()

    def test_bug_GH485(self):
        """Regression test for issue GH#485"""

        # Oscillations at 12o'c
        # The last sequence 1, 2, 359 triggered the bug GH#485
        for v0, v in zip((2, 1, 358, 359, 1, 2, 359), (2, 1, -2, -1, 1, 2, -1)):
            self.fdm['test/input'] = v0
            self.fdm.run()
            self.assertAlmostEqual(self.fdm['test/output'], v)

    def test_rotations(self):
        # 2.5 clockwise rotations
        for v in range(900):
            self.fdm['test/input'] = v % 360
            self.fdm.run()
            self.assertAlmostEqual(self.fdm['test/output'], v)

        # 4 anticlockwise rotations
        for v in range(900, -540, -1):
            self.fdm['test/input'] = v % 360
            self.fdm.run()
            self.assertAlmostEqual(self.fdm['test/output'], v)


RunTest(TestLinearActuator)
