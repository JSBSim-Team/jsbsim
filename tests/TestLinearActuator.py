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
    def test_bug_GH485(self):
        """Regression test for issue GH#485"""
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('linear_actuator.xml')
        fdm = tripod.start()

        for v0, v in zip((2, 1, 358, 359, 1, 2, 359), (2, 1, -2, -1, 1, 2, -1)):
            fdm['test/input'] = v0
            fdm.run()
            self.assertAlmostEqual(fdm['test/output'], v)


RunTest(TestLinearActuator)
