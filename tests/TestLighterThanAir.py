# TestLighterThanAir.py
#
# Tests that check that lighter than air vehicles (balloons, blimps, etc.) are
# working as expected.
#
# Copyright (c) 2022 Bertrand Coconnier
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
from JSBSim_utils import JSBSimTestCase, RunTest, ExecuteUntil, CopyAircraftDef

import fpectl


class TestLighterThanAir(JSBSimTestCase):
    def testValve(self):
        fpectl.turnon_sigfpe()

        fdm = self.create_fdm()
        fdm.load_script(
            self.sandbox.path_to_jsbsim_file("scripts", "weather-balloon.xml")
        )

        fdm.run_ic()
        fdm["buoyant_forces/gas-cell/burst"] = 1.0

        ExecuteUntil(fdm, 10.0)

        fpectl.turnoff_sigfpe()

    def testAirCellWeighsTheAirItDisplaces(self):
        # A cell of air must weigh what the atmosphere's density says it
        # displaces, whatever its pressure and temperature.
        script_path = self.sandbox.path_to_jsbsim_file(
            "scripts", "weather-balloon.xml"
        )
        tree, aircraft_name, _ = CopyAircraftDef(script_path, self.sandbox)
        tree.getroot().find("buoyant_forces/gas_cell").attrib["type"] = "AIR"
        tree.write(self.sandbox("aircraft", aircraft_name, aircraft_name + ".xml"))

        fdm = self.create_fdm()
        fdm.set_aircraft_path("aircraft")
        fdm.load_script(script_path)
        fdm.run_ic()
        fdm.run()

        gas_weight = (
            fdm["inertia/weight-lbs"]
            - fdm["inertia/empty-weight-lbs"]
            - fdm["inertia/pointmass-weight-lbs"]
        )
        gas_mass = gas_weight * fdm["inertia/mass-slugs"] / fdm["inertia/weight-lbs"]
        density_ratio = (
            fdm["buoyant_forces/gas-cell/pressure-psf"] / fdm["atmosphere/P-psf"]
        ) * (fdm["atmosphere/T-R"] / fdm["buoyant_forces/gas-cell/temp-R"])
        displaced_mass = (
            fdm["buoyant_forces/gas-cell/volume-ft3"]
            * fdm["atmosphere/rho-slugs_ft3"]
            * density_ratio
        )

        self.assertAlmostEqual(gas_mass / displaced_mass, 1.0, delta=1e-4)

RunTest(TestLighterThanAir)
