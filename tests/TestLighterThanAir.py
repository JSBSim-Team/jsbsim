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
from JSBSim_utils import JSBSimTestCase, RunTest, ExecuteUntil

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


RunTest(TestLighterThanAir)
