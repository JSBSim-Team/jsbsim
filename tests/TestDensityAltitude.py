# TestDensityAltitude.py
#
# Test that density altitude works 
#
# Copyright (c) 2018 Sean McLeod
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

from JSBSim_utils import JSBSimTestCase, RunTest

class TestDensityAltitude(JSBSimTestCase):

    def test_densityaltitude(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')        

        # Reference data (Geometric Altitude, Temp Delta (Rankine), Density Altitude)
        reference_data = [
            (0,        0,   0),
            (0,      -27,  -1838.3210293),
            (0,       27,   1724.0715454),
            (10000,    0,   10000),
            (10000,  -27,   8842.6417730),
            (10000,   27,   11117.881412),
            (20000,    0,   20000),
            (20000,  -27,   19524.3027252),
            (20000,   27,   20511.1447732),
            (30000,    0,   30000),
            (30000,  -27,   30206.6618955),
            (30000,   27,   29903.8616766),
            (40000,    0,   40000),
            (40000,  -27,   40795.49296642545),
            (40000,   27,   39370.88017359472),
            (50000,    0,   50000),
            (50000,  -27,   51540.55687445524),
            (50000,   27,   48722.498495051164),
            (60000,    0,   60000),
            (60000,  -27,   62286.38628793139),
            (60000,   27,   58073.53700852329),
            (100000,   0,   100000),
            (100000, -27,   105340.83866126923),
            (100000,  27,   95441.10933931815),
            (150000,   0,   150000),
            (150000, -27,   159983.12837740956),
            (150000,  27,   141847.12260237316),
            (160000,   0,   160000),
            (160000, -27,   171305.317547756),
            (160000,  27,   150762.4482763878),
            (220000,   0,   220000),
            (220000, -27,   233395.46205079104),
            (220000,  27,   207735.4268030979),
            (260000,   0,   260000),
            (260000, -27,   274351.9265767301),
            (260000,  27,   246964.3481013492),
            (290000,   0,   290000),
            (290000, -27,   305321.815863847),
            (290000,  27,   276290.37419984984),
            (320000,   0,   320000),
            (320000, -27,   337990.28144264355),
            (320000,  27,   304417.9280936986)
            ]

        # Run through refernce data and compare JSBSim calculated density altitude to expected
        for geometric_alt, delta_T, density_alt in reference_data:

            fdm['ic/h-sl-ft'] = geometric_alt
            fdm['atmosphere/delta-T'] = delta_T
            fdm.run_ic()

            jsbSim_density_alt = fdm['atmosphere/density-altitude']

            if density_alt < 1E-9:
                self.assertAlmostEqual(density_alt, jsbSim_density_alt)
            else:
                self.assertAlmostEqual(1.0, jsbSim_density_alt/density_alt)

            density_ref = fdm['atmosphere/rho-slugs_ft3']

            fdm['ic/h-sl-ft'] = jsbSim_density_alt
            fdm['atmosphere/delta-T'] = 0.0
            fdm.run_ic()

            self.assertAlmostEqual(density_ref, fdm['atmosphere/rho-slugs_ft3'])

RunTest(TestDensityAltitude)


