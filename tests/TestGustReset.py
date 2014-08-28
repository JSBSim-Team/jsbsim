# TestGustReset.py
#
# Check that reinitialization is correctly handled by JSBSim when the reset
# takes place in the middle of a gust sequence.
#
# Copyright (c) 2014 Bertrand Coconnier
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

import sys, unittest
from JSBSim_utils import CreateFDM, Table, SandBox, ExecuteUntil

class TestGustReset(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_gust_reset(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'c172_cruise_8K.xml'))
        fdm.set_property_value('simulation/randomseed', 0.0)
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests', 'output.xml'))

        fdm.run_ic()
        ExecuteUntil(fdm, 15.5)

        ref, current = Table(), Table()
        ref.ReadCSV(self.sandbox('output.csv'))

        fdm.set_property_value('simulation/randomseed', 0.0)
        fdm.reset_to_initial_conditions(1)
        ExecuteUntil(fdm, 15.5)

        current.ReadCSV(self.sandbox('output_0.csv'))

        diff = ref.compare(current)
        self.longMessage = True
        self.assertTrue(diff.empty(), msg='\n'+repr(diff))

suite = unittest.TestLoader().loadTestsFromTestCase(TestGustReset)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures:
    sys.exit(-1) # 'make test' will report the test failed.
