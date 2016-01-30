# TestHoldDown.py
#
# Test the hold down feature (property forces/hold-down)
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

import os
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestHoldDown(JSBSimTestCase):
    def test_static_hold_down(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('J246')
        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft')
        fdm.load_ic(os.path.join(aircraft_path, 'J246', 'LC39'), False)
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()
        h0 = fdm['position/h-sl-ft']
        t = 0.0

        while t < 420.0:
            fdm.run()
            t = fdm['simulation/sim-time-sec']
            self.assertAlmostEqual(fdm['position/h-sl-ft'], h0, delta=1E-5)

RunTest(TestHoldDown)
