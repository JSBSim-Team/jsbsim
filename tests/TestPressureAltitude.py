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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest

class TestPressureAltitude(JSBSimTestCase):

    def test_pressurealtitude(self):
        fdm = CreateFDM(self.sandbox)
        
        script_path = self.sandbox.path_to_jsbsim_file('scripts', '737_cruise.xml')
        fdm.load_script(script_path)

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
            (30000,   27,   28513.556862000503)
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

        del fdm


RunTest(TestPressureAltitude)


