# TestPlanet.py
#
# Test that the <planet> section is functional.
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


class TestPlanet(JSBSimTestCase):
    def test_moon(self):
        tripod = FlightModel(self, 'tripod')
        moon_file = self.sandbox.path_to_jsbsim_file('tests/moon.xml')
        tripod.include_planet_test_file(moon_file)
        self.fdm = tripod.start()
        self.fdm['ic/h-agl-ft'] = 0.2
        self.fdm['ic/long-gc-deg'] = 0.0
        self.fdm['ic/lat-geod-deg'] = 0.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1738100, 1.0)
        self.assertAlmostEqual(self.fdm['accelerations/gravity-ft_sec2']*0.3048, 1.62, delta=3e-3)

        self.fdm['ic/lat-geod-deg'] = 90.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1736000, 1.0)


RunTest(TestPlanet)
