# TestTurbulenceGustStop.py
#
# Check that turbulence and gusts end their effect when completed or cancelled.
#
# Copyright (c) 2025 Sean McLeod
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

import math
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestTurbulenceGustStop(JSBSimTestCase):
    # Initial wind
    BASELINE_WIND_NORTH = 5
    BASELINE_WIND_EAST  = 6
    BASELINE_WIND_DOWN  = 7

    def testCosineGust(self):
        fdm = self.setup()

        # Setup cosine gust
        fdm["atmosphere/cosine-gust/startup-duration-sec"] = 5
        fdm["atmosphere/cosine-gust/steady-duration-sec"] = 1
        fdm["atmosphere/cosine-gust/end-duration-sec"] = 5
        fdm["atmosphere/cosine-gust/magnitude-ft_sec"] = 30
        fdm["atmosphere/cosine-gust/frame"] = 2
        fdm["atmosphere/cosine-gust/X-velocity-ft_sec"] = -1
        fdm["atmosphere/cosine-gust/Y-velocity-ft_sec"] = -0.5
        fdm["atmosphere/cosine-gust/Z-velocity-ft_sec"] = 0
        fdm["atmosphere/cosine-gust/start"] =1

        while fdm.get_sim_time() < 15:
            fdm.run()

        self.checkBaselineWind(fdm)

    def testGust(self):
        fdm = self.setup()

        # Setup manual gust
        fdm["atmosphere/gust-north-fps"] = 7
        fdm["atmosphere/gust-east-fps"] = 8
        fdm["atmosphere/gust-down-fps"] = 9

        while fdm.get_sim_time() < 15:
            fdm.run()

        # Reset manual gust
        fdm["atmosphere/gust-north-fps"] = 0
        fdm["atmosphere/gust-east-fps"] = 0
        fdm["atmosphere/gust-down-fps"] = 0

        fdm.run()

        self.checkBaselineWind(fdm)

    def testTurbulence(self):
        fdm = self.setup()

        # Setup turbulence
        fdm["atmosphere/turb-type"] = 3
        fdm["atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps"] = 75
        fdm["atmosphere/turbulence/milspec/severity"] = 6

        while fdm.get_sim_time() < 15:
            fdm.run()

        # Reset turbulence
        fdm["atmosphere/turb-type"] = 0

        fdm.run()

        self.checkBaselineWind(fdm)

    def setup(self):
        fdm = self.create_fdm()
        fdm.load_model('A4') 

        # Set engine running
        fdm['propulsion/engine[0]/set-running'] = 1

        fdm['ic/h-sl-ft'] = 20000
        fdm['ic/vc-kts'] = 250
        fdm['ic/gamma-deg'] = 0

        fdm.run_ic()

        fdm['simulation/do_simple_trim'] = 1
        
        # Set baseline NED wind
        fdm["atmosphere/wind-north-fps"] = self.BASELINE_WIND_NORTH
        fdm["atmosphere/wind-east-fps"] = self.BASELINE_WIND_EAST
        fdm["atmosphere/wind-down-fps"] = self.BASELINE_WIND_DOWN

        return fdm

    def checkBaselineWind(self, fdm):
        # Confirm that winds have reset to baseline values before gusts/turbulence
        wn = fdm['atmosphere/total-wind-north-fps']
        we = fdm['atmosphere/total-wind-east-fps']
        wd = fdm['atmosphere/total-wind-down-fps']

        self.assertAlmostEqual(wn, self.BASELINE_WIND_NORTH, delta=1E-8)
        self.assertAlmostEqual(we, self.BASELINE_WIND_EAST, delta=1E-8)
        self.assertAlmostEqual(wd, self.BASELINE_WIND_DOWN, delta=1E-8)

RunTest(TestTurbulenceGustStop)
