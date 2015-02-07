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

mol2lbs = 0.00013841 * 32.174049

class CheckMomentsUpdate(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def CheckCGPosition(self):
        weight = self.fdm.get_property_value('inertia/weight-lbs')
        empty_weight = self.fdm.get_property_value('inertia/empty-weight-lbs')
        contents = self.fdm.get_property_value('buoyant_forces/gas-cell/contents-mol')
        radiosonde_weight = weight - empty_weight - contents * mol2lbs

        CGx = self.fdm.get_property_value('inertia/cg-x-in')
        CGy = self.fdm.get_property_value('inertia/cg-y-in')
        CGz = self.fdm.get_property_value('inertia/cg-z-in')
        X = self.fdm.get_property_value('inertia/pointmass-location-X-inches')
        Y = self.fdm.get_property_value('inertia/pointmass-location-Y-inches')
        Z = self.fdm.get_property_value('inertia/pointmass-location-Z-inches')

        self.assertAlmostEqual(CGx, X * radiosonde_weight / weight, delta = 1E-7)
        self.assertAlmostEqual(CGy, Y * radiosonde_weight / weight, delta = 1E-7)
        self.assertAlmostEqual(CGz, Z * radiosonde_weight / weight, delta = 1E-7)

    def test_moments_update(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'weather-balloon.xml')
        self.fdm = CreateFDM(self.sandbox)

        self.fdm.load_script(script_path)
        self.fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests', 'output.xml'))
        self.fdm.run_ic()

        self.CheckCGPosition()

        dt = self.fdm.get_property_value('simulation/dt')
        ExecuteUntil(self.fdm, 1.0-2.0*dt)

        self.CheckCGPosition()

        # Moves the radio sonde to modify the CG location
        self.fdm.set_property_value('inertia/pointmass-location-X-inches', 5.0)

        # Check that the moment is immediately updated accordingly
        self.fdm.run()
        self.CheckCGPosition()

        Fbx = self.fdm.get_property_value('forces/fbx-buoyancy-lbs')
        Fbz = self.fdm.get_property_value('forces/fbz-buoyancy-lbs')
        CGx = self.fdm.get_property_value('inertia/cg-x-in') / 12.0 # Converts from in to ft
        CGz = self.fdm.get_property_value('inertia/cg-z-in') / 12.0
        Mby = self.fdm.get_property_value('moments/m-buoyancy-lbsft')

        self.assertAlmostEqual(Fbx * CGz - Fbz * CGx, Mby, delta=1E-7,
                               msg="Fbx*CGz-Fbz*CGx = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

        # One further step to log the same results in the output file
        self.fdm.run()
        self.CheckCGPosition()

        csv = Table()
        csv.ReadCSV(self.sandbox('output.csv'))
        Mby = csv.get_column('M_{Buoyant} (ft-lbs)')[-1]
        Fbx = csv.get_column('F_{Buoyant x} (lbs)')[-1]
        Fbz = csv.get_column('F_{Buoyant z} (lbs)')[-1]

        self.assertAlmostEqual(Fbx * CGz - Fbz * CGx, Mby, delta=1E-7,
                               msg="Fbx*CGz-Fbz*CGx = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

suite = unittest.TestLoader().loadTestsFromTestCase(CheckMomentsUpdate)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
