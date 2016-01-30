# TestCosineGust.py
#
# Check the cosine gust feature.
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

import math
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestCosineGust(JSBSimTestCase):
    def testMagnitude(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'c172_cruise_8K.xml'))

        fdm.run_ic()
        t = 0.0

        startup_duration = 5.0
        steady_duration = 1.0
        end_duration = 5.0
        start_time = 10.0
        magnitude = 30.0

        end_time = start_time + startup_duration + steady_duration + end_duration

        while fdm['simulation/run_id'] == 0:
            fdm.run()
            wn = fdm['atmosphere/total-wind-north-fps']
            we = fdm['atmosphere/total-wind-east-fps']
            wd = fdm['atmosphere/total-wind-down-fps']

            if t >= start_time and t <= end_time:
                wmag = math.sqrt(wn*wn + we*we + wd*wd)
                t -= start_time
                if t <= startup_duration:
                    self.assertAlmostEqual(0.5 * magnitude * (1.0 - math.cos(math.pi*t/startup_duration)),
                                           wmag, delta=1E-3)
                else:
                    t -= startup_duration
                    if t <= steady_duration:
                        self.assertAlmostEqual(magnitude, wmag, delta=1E-8)
                    else:
                        t -= steady_duration
                        if t <= end_duration:
                            self.assertAlmostEqual(0.5 * magnitude * (1.0 + math.cos(math.pi*t/end_duration)),
                                                   wmag, delta=1E-3)

            t = fdm['simulation/sim-time-sec']

RunTest(TestCosineGust)
