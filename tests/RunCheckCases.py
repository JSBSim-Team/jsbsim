# RunCheckCases.py
#
# Regression tests based on the check cases that are existing in JSBSim for
# years now (see directory check_cases/)
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

import sys, unittest, os
from JSBSim_utils import CreateFDM, Table, SandBox


class TestOrbitCheckCase(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox('check_cases', 'orbit')

    def tearDown(self):
        self.sandbox.erase()

    def testOrbitCheckCase(self):
        os.environ['JSBSIM_DEBUG'] = str(0)
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'ball_orbit.xml'))
        fdm.run_ic()

        while fdm.run():
            pass

        ref, current = Table(), Table()
        ref.ReadCSV(self.sandbox.elude(self.sandbox.path_to_jsbsim_file('logged_data', 'BallOut.csv')))
        current.ReadCSV(self.sandbox('BallOut.csv'))

        diff = ref.compare(current)
        self.longMessage = True
        self.assertTrue(diff.empty(), msg='\n'+repr(diff))

suite = unittest.TestLoader().loadTestsFromTestCase(TestOrbitCheckCase)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
