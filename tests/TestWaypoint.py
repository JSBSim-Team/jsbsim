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

        a = 20925646.32546  # WGS84 semimajor axis length in feet
        b = 20855486.5951   # WGS84 semiminor axis length in feet
        h = (a - b) / (a + b)
        sq_h = h * h
        p = math.pi * (a + b) * (1. + 3. * sq_h/(10. + math.sqrt(4. - 3. * sq_h)))

        fdm['ic/lat-geod-rad'] = 0.0
        fdm['ic/long-gc-rad'] = 0.0
        fdm['guidance/target_wp_latitude_rad'] = 0.0

        # Check the distance and heading to other points on the equator.
        for ilon in range(-5, 6):
            lon = ilon * math.pi/6.0
            fdm['guidance/target_wp_longitude_rad'] = lon
            fdm.run_ic()
            distance = abs(lon * a)
            self.assertAlmostEqual(fdm['guidance/wp-distance'], distance,
                                   delta=1.)
            if abs(distance > 1E-9):
                self.assertAlmostEqual(fdm['guidance/wp-heading-rad'],
                                       lon * 0.5 * math.pi / abs(lon))

        # Check the distance and heading to the North pole
        fdm['guidance/target_wp_latitude_rad'] = 0.5 * math.pi
        fdm['guidance/target_wp_longitude_rad'] = 0.0
        for ilon in range(-5, 7):
            lon = ilon * math.pi / 6.0
            fdm['ic/long-gc-rad'] = lon
            fdm.run_ic()
            self.assertAlmostEqual(fdm['guidance/wp-distance'], 0.25 * p,
                                   delta=1.)
            self.assertAlmostEqual(fdm['guidance/wp-heading-rad'], 0.0)

        # Check the distance and heading to the South pole
        fdm['guidance/target_wp_latitude_rad'] = -0.5 * math.pi
        fdm['guidance/target_wp_longitude_rad'] = 0.0
        for ilon in range(-5, 7):
            lon = ilon * math.pi / 6.0
            fdm['ic/long-gc-rad'] = lon
            fdm.run_ic()
            self.assertAlmostEqual(fdm['guidance/wp-distance'], 0.25 * p,
                                   delta=1.)
            self.assertAlmostEqual(fdm['guidance/wp-heading-rad'], math.pi)

        # Check the distance to the antipode
        for ilat in range(-5, 6):
            glat = ilat * math.pi / 12.
            fdm['ic/lat-geod-rad'] = glat
            fdm['guidance/target_wp_latitude_rad'] = -glat
            for ilon in range(-5, 6):
                lon = ilon * math.pi / 6.
                fdm['ic/long-gc-rad'] = lon
                fdm['guidance/target_wp_longitude_rad'] = lon + math.pi
                fdm.run_ic()
                self.assertAlmostEqual(fdm['guidance/wp-distance'], 0.5 * p,
                                       delta=1.)

RunTest(TestWaypoint)
