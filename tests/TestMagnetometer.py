# TestMagnetometer.py
#
# Test that the <magnetometer> component is functional.
#
# Copyright (c) 2020 Bertrand Coconnier
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


class TestMagnetometer(JSBSimTestCase):
    # Regression test for the bug reported in issue 332
    def test(self):
        tripod = FlightModel(self, "tripod")
        tripod.include_system_test_file("magnetometer.xml")
        fdm = tripod.start()
        self.assertAlmostEqual(fdm['test/magnetic-field']/27661.1, 1.0, 5)


RunTest(TestMagnetometer)
