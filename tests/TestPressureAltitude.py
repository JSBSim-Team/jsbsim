# TestPressureAltitude.py
#
# Test that pressure altitude works 
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

class TestPressureAltitude(JSBSimTestCase):

    def test_pressurealtitude(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        # Reference data (Geometric Altitude, Temp Delta (Rankine), Pressure Altitude)
        reference_data = [
            (0,        0,   0),
            (0,      -27,   0),
            (0,       27,   0),
            (10000,    0,   10000),
            (10000,  -27,   10549.426597202142),
            (10000,   27,   9504.969939165301),
            (20000,    0,   20000),
            (20000,  -27,   21099.40877940678),
            (20000,   27,   19009.488882465),
            (30000,    0,   30000),
            (30000,  -27,   31649.946590500946),
            (30000,   27,   28513.556862000503),
            (40000,    0,   40000),
            (40000,  -27,   42294.25242340247),
            (40000,   27,   37972.879013500584),
            (50000,    0,   50000),
            (50000,  -27,   53040.858132036126),
            (50000,   27,   47323.24573196882),
            (60000,    0,   60000),
            (60000,  -27,   63788.23024872676),
            (60000,   27,   56673.032160129085),
            (100000,   0,   100000),
            (100000, -27,   107018.51146890492),
            (100000,  27,   93910.6118895332),
            (150000,   0,   150000),
            (150000, -27,   161956.60354430682),
            (150000,  27,   139810.93668842476),
            (160000,   0,   160000),
            (160000, -27,   172582.32995327076),
            (160000,  27,   148992.4108097521),
            (220000,   0,   220000),
            (220000, -27,   233772.88181515134),
            (220000,  27,   207274.89794422916),
            (260000,   0,   260000),
            (260000, -27,   275000.20893894637),
            (260000,  27,   246263.63421221747),
            (290000,   0,   290000),
            (290000, -27,   306867.8082342206),
            (290000,  27,   275185.69941262825),
            (320000,   0,   320000),
            (320000, -27,   339541.05112835445),
            (320000,  27,   302991.642663158)
            ]

        # Run through refernce data and compare JSBSim calculated pressure altitude to expected
        for geometric_alt, delta_T, pressure_alt in reference_data:

            fdm['ic/h-sl-ft'] = geometric_alt
            fdm['atmosphere/delta-T'] = delta_T
            fdm.run_ic()

            jsbSim_pressure_alt = fdm['atmosphere/pressure-altitude']

            self.assertAlmostEqual(pressure_alt, jsbSim_pressure_alt, delta=1e-7)

            pressure_ref = fdm['atmosphere/P-psf']

            fdm['ic/h-sl-ft'] = jsbSim_pressure_alt
            fdm['atmosphere/delta-T'] = 0.0
            fdm.run_ic()

            self.assertAlmostEqual(pressure_ref, fdm['atmosphere/P-psf'])

RunTest(TestPressureAltitude)


