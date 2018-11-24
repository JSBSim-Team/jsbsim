# TestDeadBand.py
#
# Test that the <deadband> component is functional.
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

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel


class TestDeadBand(JSBSimTestCase):
    def dead_band(self, fdm, v, width, prop):
        half = abs(0.5*width)
        if v < -half:
            self.assertAlmostEqual(fdm[prop], v+half)
        elif v > half:
            self.assertAlmostEqual(fdm[prop], v-half)
        else:
            self.assertAlmostEqual(fdm[prop], 0.0)
        
    def test_conditions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('deadband.xml')
        fdm = tripod.start()

        fdm['test/reference'] = 1.5

        for i in range(10):
            v = 0.5*i-2.0
            fdm['test/input'] = v
            fdm.run()
            self.dead_band(fdm, v, 1.0, 'test/db-value')
            self.dead_band(fdm, v, 1.5, 'test/db-property')
            self.dead_band(fdm, -v, 1.5, 'test/db-property-inverted-input-sign')

        fdm['test/reference'] = 0.707

        for i in range(10):
            v = 0.5*i-2.0
            fdm['test/input'] = v
            fdm.run()
            self.dead_band(fdm, v, 1.0, 'test/db-value')
            self.dead_band(fdm, v, 0.707, 'test/db-property')
            self.dead_band(fdm, -v, 0.707,
                           'test/db-property-inverted-input-sign')

        fdm['test/reference'] = -1.5

        for i in range(10):
            v = 0.5*i-2.0
            fdm['test/input'] = v
            fdm.run()
            self.dead_band(fdm, v, 1.0, 'test/db-value')
            self.dead_band(fdm, v, 1.5, 'test/db-property-inverted-sign')
            
        fdm.run()


RunTest(TestDeadBand)
