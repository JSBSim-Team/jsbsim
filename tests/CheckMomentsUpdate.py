# CheckMomentsUpdate.py
#
# Regression test to check the moments are computed according to the last
# update of the CG location (issue reported by Marta Marimon)
#
# Copyright (c) 2015 Bertrand Coconnier
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

import unittest, sys
from JSBSim_utils import SandBox, CreateFDM

class CheckMomentsUpdate(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_moments_update(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'weather-balloon.xml')
        fdm = CreateFDM(self.sandbox)

        fdm.load_script(script_path)
        fdm.run_ic()

        # Moves the radio sonde to modify the CG location
        fdm.set_property_value('inertia/pointmass-location-X-inches', 5.0)

        # Check that the moment is immediately updated accordingly
        fdm.run()
        Fbz = fdm.get_property_value('forces/fbz-buoyancy-lbs')
        CGx = fdm.get_property_value('inertia/cg-x-in') / 12.0 # Converts from in to ft
        Mby = fdm.get_property_value('moments/m-buoyancy-lbsft')
        self.assertTrue(abs(Fbz * CGx + Mby) < 1E-7,
                        msg="Fbz*CGx = %f and Mby = %f do not match" % (-Fbz*CGx, Mby))

suite = unittest.TestLoader().loadTestsFromTestCase(CheckMomentsUpdate)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
