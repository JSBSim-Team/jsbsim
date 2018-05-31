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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest

class TestDensityAltitude(JSBSimTestCase):

    def test_densityaltitude(self):
        fdm = CreateFDM(self.sandbox)
        
        script_path = self.sandbox.path_to_jsbsim_file('scripts', '737_cruise.xml')
        fdm.load_script(script_path)

        # Reference data (Pressure Altitude, Temp Delta (Rankine), Density Altitude)
        reference_data = [
            (0,        0,   0),
            (0,      -27,  -1838.7331194627707),
            (0,       27,   1724.1606364208037),
            (10000,    0,   10000),
            (10000,  -27,   8839.82645080842),
            (10000,   27,   11113.201852749373),
            (20000,    0,   20000),
            (20000,  -27,   19508.060846598906),
            (20000,   27,   20493.173549213785)
            ]

        # Run through refernce data and compare JSBSim calculated density altitude to expected
        for pressure_alt, delta_T, density_alt in reference_data:

            fdm['ic/h-agl-ft'] = pressure_alt
            fdm.run_ic()
            fdm['atmosphere/delta-T'] = delta_T

            jsbSim_density_alt = fdm['atmosphere/density-altitude']

            self.assertAlmostEqual(density_alt, jsbSim_density_alt, delta=1e-7)

        del fdm


RunTest(TestDensityAltitude)


