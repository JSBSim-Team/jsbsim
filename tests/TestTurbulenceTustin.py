# TestTurbulenceTustin.py
#
# Check that the Tustin turbulence model keeps the intensity requested by
# MIL-F-8785C in free flight, whatever the time step.
#
# Copyright (c) 2026 Alexander Kalmykov
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
from JSBSim_utils import JSBSimTestCase, RunTest


class TestTurbulenceTustin(JSBSimTestCase):
    # sigma_w of severity 4 (the 1e-3 curve of MIL-F-8785C Fig. 7) at the initial
    # altitude, interpolated as in FGWinds. The aircraft only descends from there,
    # so 2*SIGMA_W bounds the whole run.
    SIGMA_W = 10.125  # ft/s

    def testFreeFlightIntensity(self):
        for turb_type in (3, 4):  # Milspec, Tustin
            for dt in (1/120, 1/1000):
                with self.subTest(turb_type=turb_type, dt=dt):
                    w_rms = self.runFreeFlight(turb_type, dt)
                    self.assertLess(w_rms, 2.0*self.SIGMA_W)

    def runFreeFlight(self, turb_type, dt):
        fdm = self.create_fdm()
        fdm['simulation/randomseed'] = 1
        fdm.load_model('c172p')
        fdm.set_dt(dt)

        # No trim and no autopilot: the aircraft descends by about 1000 ft/min.
        fdm['ic/h-sl-ft'] = 2800
        fdm['ic/vc-kts'] = 74
        fdm['ic/gamma-deg'] = 0
        fdm['propulsion/engine[0]/set-running'] = 1
        fdm.run_ic()
        fdm['fcs/throttle-cmd-norm'] = 0.7
        fdm['fcs/mixture-cmd-norm'] = 1.0

        fdm['atmosphere/turbulence/milspec/severity'] = 4
        fdm['atmosphere/turb-type'] = turb_type

        sum_w2 = 0.0
        n = 0
        while fdm.get_sim_time() < 60:
            fdm.run()
            sum_w2 += fdm['atmosphere/turb-down-fps']**2
            n += 1

        return math.sqrt(sum_w2/n)

RunTest(TestTurbulenceTustin)
