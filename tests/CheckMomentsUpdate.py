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

import pandas as pd
from JSBSim_utils import JSBSimTestCase, CreateFDM, ExecuteUntil, RunTest

mol2lbs = 0.00013841 * 32.174049


class CheckMomentsUpdate(JSBSimTestCase):
    def CheckCGPosition(self):
        weight = self.fdm['inertia/weight-lbs']
        empty_weight = self.fdm['inertia/empty-weight-lbs']
        contents = self.fdm['buoyant_forces/gas-cell/contents-mol']
        radiosonde_weight = weight - empty_weight - contents * mol2lbs

        CGx = self.fdm['inertia/cg-x-in']
        CGy = self.fdm['inertia/cg-y-in']
        CGz = self.fdm['inertia/cg-z-in']
        X = self.fdm['inertia/pointmass-location-X-inches']
        Y = self.fdm['inertia/pointmass-location-Y-inches']
        Z = self.fdm['inertia/pointmass-location-Z-inches']

        self.assertAlmostEqual(CGx, X * radiosonde_weight / weight, delta=1E-7)
        self.assertAlmostEqual(CGy, Y * radiosonde_weight / weight, delta=1E-7)
        self.assertAlmostEqual(CGz, Z * radiosonde_weight / weight, delta=1E-7)

    def test_moments_update(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       'weather-balloon.xml')
        self.fdm = CreateFDM(self.sandbox)

        self.fdm.load_script(script_path)
        self.fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests', 'output.xml'))
        self.fdm.run_ic()

        self.CheckCGPosition()

        dt = self.fdm['simulation/dt']
        ExecuteUntil(self.fdm, 1.0-2.0*dt)

        self.CheckCGPosition()

        # Moves the radio sonde to modify the CG location
        self.fdm['inertia/pointmass-location-X-inches'] = 5.0

        # Check that the moment is immediately updated accordingly
        self.fdm.run()
        self.CheckCGPosition()

        Fbx = self.fdm['forces/fbx-buoyancy-lbs']
        Fbz = self.fdm['forces/fbz-buoyancy-lbs']
        CGx = self.fdm['inertia/cg-x-in'] / 12.0  # Converts from in to ft
        CGz = self.fdm['inertia/cg-z-in'] / 12.0
        Mby = self.fdm['moments/m-buoyancy-lbsft']

        self.assertAlmostEqual(Fbx * CGz - Fbz * CGx, Mby, delta=1E-7,
                               msg="Fbx*CGz-Fbz*CGx = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

        # One further step to log the same results in the output file
        self.fdm.run()
        self.CheckCGPosition()

        csv = pd.read_csv('output.csv')
        Mby = csv['M_{Buoyant} (ft-lbs)'].iloc[-1]
        Fbx = csv['F_{Buoyant x} (lbs)'].iloc[-1]
        Fbz = csv['F_{Buoyant z} (lbs)'].iloc[-1]

        self.assertAlmostEqual(Fbx * CGz - Fbz * CGx, Mby, delta=1E-7,
                               msg="Fbx*CGz-Fbz*CGx = %f and Mby = %f do not match" % (Fbx*CGz-Fbz*CGx, Mby))

        del self.fdm

RunTest(CheckMomentsUpdate)
