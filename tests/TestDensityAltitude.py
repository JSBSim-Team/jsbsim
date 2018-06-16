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
            (30000,   27,   29903.8616766)
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

        del fdm


RunTest(TestDensityAltitude)


