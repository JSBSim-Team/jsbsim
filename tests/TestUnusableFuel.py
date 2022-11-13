# TestUnusableFuel.py
#
# Checks that the unusable fuel feature work (see issue GH#759)
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

from JSBSim_utils import JSBSimTestCase


class TestUnusableFuel(JSBSimTestCase):
    def test_unusable_fuel(self):
        fdm = self.create_fdm()
        fdm.load_model("c172x")
        fdm.load_ic("reset01", True)

        fdm["propulsion/tank[0]/unusable-volume-gal"] = 1.5
        fdm["propulsion/tank[1]/unusable-volume-gal"] = 1.5
        fdm["propulsion/tank[0]/contents-lbs"] = 10
        fdm["propulsion/tank[1]/contents-lbs"] = 10
        fdm.run_ic()

        # Check the engine is running
        self.assertNotEqual(fdm["propulsion/engine/set-running"], 0.0)
        # Check that tanks are not yet emptied.
        self.assertLess(
            fdm["propulsion/tank[0]/unusable-volume-gal"],
            fdm["propulsion/tank[0]/contents-volume-gal"],
        )
        self.assertLess(
            fdm["propulsion/tank[1]/unusable-volume-gal"],
            fdm["propulsion/tank[1]/contents-volume-gal"],
        )

        # Run until the engine starves.
        while fdm["propulsion/engine/set-running"] != 0.0:
            fdm.run()

        # Check that the fuel content in gallons is lower or equal to the unusable volume.
        self.assertGreaterEqual(
            fdm["propulsion/tank[0]/unusable-volume-gal"],
            fdm["propulsion/tank[0]/contents-volume-gal"],
        )
        self.assertGreaterEqual(
            fdm["propulsion/tank[1]/unusable-volume-gal"],
            fdm["propulsion/tank[1]/contents-volume-gal"],
        )
