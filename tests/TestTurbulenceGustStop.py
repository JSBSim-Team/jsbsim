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

    def setUp(self, *args): 
        super().setUp(*args)

        self.fdm = self.create_fdm()
        self.fdm.load_model('A4') 

        # Set engine running
        self.fdm['propulsion/engine[0]/set-running'] = 1

        self.fdm['ic/h-sl-ft'] = 20000
        self.fdm['ic/vc-kts'] = 250
        self.fdm['ic/gamma-deg'] = 0

        self.fdm.run_ic()

        self.fdm['simulation/do_simple_trim'] = 1
        
        # Set baseline NED wind
        self.fdm["atmosphere/wind-north-fps"] = self.BASELINE_WIND_NORTH
        self.fdm["atmosphere/wind-east-fps"] = self.BASELINE_WIND_EAST
        self.fdm["atmosphere/wind-down-fps"] = self.BASELINE_WIND_DOWN

    def testCosineGust(self):
        # Setup cosine gust
        self.fdm["atmosphere/cosine-gust/startup-duration-sec"] = 5
        self.fdm["atmosphere/cosine-gust/steady-duration-sec"] = 1
        self.fdm["atmosphere/cosine-gust/end-duration-sec"] = 5
        self.fdm["atmosphere/cosine-gust/magnitude-ft_sec"] = 30
        self.fdm["atmosphere/cosine-gust/frame"] = 2
        self.fdm["atmosphere/cosine-gust/X-velocity-ft_sec"] = -1
        self.fdm["atmosphere/cosine-gust/Y-velocity-ft_sec"] = -0.5
        self.fdm["atmosphere/cosine-gust/Z-velocity-ft_sec"] = 0
        self.fdm["atmosphere/cosine-gust/start"] =1

        while self.fdm.get_sim_time() < 15:
            self.fdm.run()

        self.checkBaselineWind()

    def testGust(self):
        # Setup manual gust
        self.fdm["atmosphere/gust-north-fps"] = 7
        self.fdm["atmosphere/gust-east-fps"] = 8
        self.fdm["atmosphere/gust-down-fps"] = 9

        while self.fdm.get_sim_time() < 15:
            self.fdm.run()

        # Reset manual gust
        self.fdm["atmosphere/gust-north-fps"] = 0
        self.fdm["atmosphere/gust-east-fps"] = 0
        self.fdm["atmosphere/gust-down-fps"] = 0

        self.fdm.run()

        self.checkBaselineWind()

    def testTurbulence(self):
        # Setup turbulence
        self.fdm["atmosphere/turb-type"] = 3
        self.fdm["atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps"] = 75
        self.fdm["atmosphere/turbulence/milspec/severity"] = 6

        while self.fdm.get_sim_time() < 15:
            self.fdm.run()

        # Reset turbulence
        self.fdm["atmosphere/turb-type"] = 0

        self.fdm.run()

        self.checkBaselineWind()

    def checkBaselineWind(self):
        # Confirm that winds have reset to baseline values before gusts/turbulence
        wn = self.fdm['atmosphere/total-wind-north-fps']
        we = self.fdm['atmosphere/total-wind-east-fps']
        wd = self.fdm['atmosphere/total-wind-down-fps']

        self.assertAlmostEqual(wn, self.BASELINE_WIND_NORTH, delta=1E-8)
        self.assertAlmostEqual(we, self.BASELINE_WIND_EAST, delta=1E-8)
        self.assertAlmostEqual(wd, self.BASELINE_WIND_DOWN, delta=1E-8)

RunTest(TestTurbulenceGustStop)
