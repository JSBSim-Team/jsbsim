# CheckTrim.py
#
# Regression tests of the trim feature
#
# Copyright (c) 2016 Bertrand Coconnier
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


class CheckTrim(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_trim_doesnt_ignite_rockets(self):
        # Run a longitudinal trim with a rocket equipped with solid propellant
        # boosters (aka SRBs). The trim algorithm will try to reach a vertical
        # equilibrium by tweaking the throttle but since the rocket is nose up,
        # the trim cannot converge. As a result the algorithm will set full
        # throttle which will result in the SRBs ignition if the integration is
        # not suspended. This bug has been reported in FlightGear and this test
        # is checking that there is no regression.

        fdm = CreateFDM(self.sandbox)
        fdm.load_model('J246')
        aircraft_path = self.sandbox.elude(self.sandbox.path_to_jsbsim_file('aircraft'))
        fdm.load_ic(os.path.join(aircraft_path, 'J246', 'LC39'), False)
        fdm.run_ic()

        # Check that the SRBs are not ignited
        self.assertEqual(fdm['propulsion/engine[0]/thrust-lbs'], 0.0)
        self.assertEqual(fdm['propulsion/engine[1]/thrust-lbs'], 0.0)

        try:
            fdm['simulation/do_simple_trim'] = 1
        except RuntimeError as e:
            # The trim cannot succeed. Just make sure that the raised exception
            # is due to the trim failure otherwise rethrow.
            if e.args[0] != 'Trim Failed':
                raise

        # Check that the trim did not ignite the SRBs
        self.assertEqual(fdm['propulsion/engine[0]/thrust-lbs'], 0.0)
        self.assertEqual(fdm['propulsion/engine[1]/thrust-lbs'], 0.0)

    def test_trim_on_ground(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('c172x')
        fdm['ic/theta-deg'] = 10.0
        fdm.run_ic()
        fdm['ic/theta-deg'] = 0.0
        fdm['simulation/do_simple_trim'] = 2

suite = unittest.TestLoader().loadTestsFromTestCase(CheckTrim)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
