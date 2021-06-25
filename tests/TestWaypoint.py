# TestWaypoint.py
#
# Test the waypoint system
#
# Copyright (c) 2016 Bertrand Coconnier
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
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestWaypoint(JSBSimTestCase):
    def test_waypoint(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('c310')
        fdm.load_ic('reset00', True)

        slr = 20925646.32546  # Sea Level Radius
        TestCases = ((0.25*math.pi, 0.5*math.pi, 0.0, 0.0),
                     (0.0, 0.5*math.pi, math.pi, slr*0.25*math.pi),
                     # North pole
                     (0.5*math.pi, 0.5*math.pi, 0.0, slr*0.25*math.pi),
                     # South pole
                     (-0.5*math.pi, 0.5*math.pi, math.pi, slr*0.75*math.pi),
                     (0.0, 0.0, 1.5*math.pi, slr*0.5*math.pi),
                     (0.0, math.pi, 0.5*math.pi, slr*0.5*math.pi))

        fdm['ic/lat-gc-rad'] = TestCases[0][0]
        fdm['ic/long-gc-rad'] = TestCases[0][1]

        for case in TestCases:
            fdm['guidance/target_wp_latitude_rad'] = case[0]
            fdm['guidance/target_wp_longitude_rad'] = case[1]
            fdm.run_ic()
            self.assertAlmostEqual(fdm['guidance/wp-heading-rad'], case[2])
            self.assertAlmostEqual(fdm['guidance/wp-distance'], case[3])

RunTest(TestWaypoint)
