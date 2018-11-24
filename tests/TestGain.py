# TestGain.py
#
# Test that the <gain> component is functional.
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


class TestGain(JSBSimTestCase):
    def test_conditions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('gain.xml')
        fdm = tripod.start()

        fdm['test/reference'] = 1.5

        for i in range(10):
            v = 0.5*i-2.0
            fdm['test/input'] = v
            fdm.run()
            self.assertAlmostEqual(fdm['test/gain-value'], 2.0*v)
            self.assertAlmostEqual(fdm['test/gain-property'], 1.5*v)
            self.assertAlmostEqual(fdm['test/gain-property-inverted-sign'],
                                   -1.5*v)
            self.assertAlmostEqual(fdm['test/gain-property-inverted-input-sign'],
                                   -1.5*v)

        fdm['test/reference'] = -0.707

        for i in range(10):
            v = 0.5*i-2.0
            fdm['test/input'] = v
            fdm.run()
            self.assertAlmostEqual(fdm['test/gain-value'], 2.0*v)
            self.assertAlmostEqual(fdm['test/gain-property'], -0.707*v)
            self.assertAlmostEqual(fdm['test/gain-property-inverted-sign'],
                                   0.707*v)
            self.assertAlmostEqual(fdm['test/gain-property-inverted-input-sign'],
                                   0.707*v)
            
        fdm.run()


RunTest(TestGain)
