# TestFilter.py
#
# Test that the <filter> component is functional.
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


class TestFilter(JSBSimTestCase):
    def test_conditions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('filter.xml')
        fdm = tripod.start()

        fdm['test/reference'] = 1.5
        half_dt = 0.5*fdm['simulation/dt']

        for i in range(100):
            fdm.run()
            t = fdm['simulation/sim-time-sec']
            v = 1.0-math.exp(-2.0*(t+half_dt))
            self.assertAlmostEqual(fdm['test/lag-value'], v, delta=1E-4)
            self.assertAlmostEqual(fdm['test/lag-value-inverted-sign'], -v,
                                   delta=1E-4)
            v = 1.0-math.exp(-1.5*t)
            self.assertAlmostEqual(fdm['test/lag-property'], v, delta=1E-4)
            v = math.exp(1.5*t)-1.0
            self.assertAlmostEqual(fdm['test/lag-property-inverted-sign'], v,
                                   delta=1E-4)

        fdm['test/reference'] = -0.707
        t0, v0 = t, v

        for i in range(100):
            fdm.run()
            t = fdm['simulation/sim-time-sec']
            v = 1.0-math.exp(-2.0*(t+half_dt))
            self.assertAlmostEqual(fdm['test/lag-value'], v, delta=1E-4)
            self.assertAlmostEqual(fdm['test/lag-value-inverted-sign'], -v,
                                   delta=1E-4)
            v = v0+(v0+1)*(math.exp(-0.707*(t-t0))-1.0)
            self.assertAlmostEqual(fdm['test/lag-property-inverted-sign'], v,
                                   delta=1E-4)


RunTest(TestFilter)
