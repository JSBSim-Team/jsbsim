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

import unittest, sys, os
from JSBSim_utils import SandBox, CreateFDM


class TestHoldDown(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_static_hold_down(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('J246')
        aircraft_path = self.sandbox.elude(self.sandbox.path_to_jsbsim_file('aircraft'))
        fdm.load_ic(os.path.join(aircraft_path, 'J246', 'LC39'), False)
        fdm.set_property_value('forces/hold-down', 1.0)
        fdm.run_ic()
        h0 = fdm.get_property_value('position/h-sl-ft')
        t = 0.0

        while t < 420.0:
            fdm.run()
            t = fdm.get_property_value('simulation/sim-time-sec')
            self.assertAlmostEqual(fdm.get_property_value('position/h-sl-ft'),
                                   h0, delta=1E-5)

suite = unittest.TestLoader().loadTestsFromTestCase(TestHoldDown)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
