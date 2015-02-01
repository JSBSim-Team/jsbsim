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
from JSBSim_utils import SandBox, CreateFDM, Table, ExecuteUntil

class CheckMomentsUpdate(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_moments_update(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'weather-balloon.xml')
        fdm = CreateFDM(self.sandbox)

        fdm.load_script(script_path)
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests', 'output.xml'))
        fdm.run_ic()

        ExecuteUntil(fdm, 1.0-2.0/120.)

        # Moves the radio sonde to modify the CG location
        fdm.set_property_value('inertia/pointmass-location-X-inches', 5.0)

        # Check that the moment is immediately updated accordingly
        fdm.run()
        Fbx = fdm.get_property_value('forces/fbx-buoyancy-lbs')
        Fbz = fdm.get_property_value('forces/fbz-buoyancy-lbs')
        CGx = fdm.get_property_value('inertia/cg-x-in') / 12.0 # Converts from in to ft
        CGz = fdm.get_property_value('inertia/cg-z-in') / 12.0
        Mby = fdm.get_property_value('moments/m-buoyancy-lbsft')

        self.assertTrue(abs(Fbz * CGx - Fbx * CGz + Mby) < 1E-7,
                        msg="Fbz*CGx-Fbx*CGz = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

        # One further step to log the same results in the output file
        fdm.run()
        csv = Table()
        csv.ReadCSV(self.sandbox('output.csv'))
        Mby = csv.get_column('M_{Buoyant} (ft-lbs)')[-1]
        Fbx = csv.get_column('F_{Buoyant x} (lbs)')[-1]
        Fbz = csv.get_column('F_{Buoyant z} (lbs)')[-1]

        self.assertTrue(abs(Fbz * CGx - Fbx * CGz + Mby) < 1E-7,
                        msg="Fbz*CGx-Fbx*CGz = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

suite = unittest.TestLoader().loadTestsFromTestCase(CheckMomentsUpdate)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
