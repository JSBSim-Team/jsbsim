# CheckDebugLvl.py
#
# Regression test that check that the same results are obtained whether the
# environment variable JSBSIM_DEBUG is set ot zero or not.
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
from JSBSim_utils import JSBSimTestCase, CreateFDM, Table, ExecuteUntil, RunTest


class TestDebugLvl(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self, 'check_cases', 'orbit')

    def testDebugLvl(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'ball_orbit.xml'))
        fdm.run_ic()

        ExecuteUntil(fdm, 1000.)

        ref, current = Table(), Table()
        ref.ReadCSV('BallOut.csv')
        del fdm

        os.environ["JSBSIM_DEBUG"] = str(0)
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'ball_orbit.xml'))
        fdm.run_ic()

        ExecuteUntil(fdm, 1000.)

        current.ReadCSV('BallOut.csv')

        diff = ref.compare(current)
        self.longMessage = True
        self.assertTrue(diff.empty(), msg='\n'+repr(diff))

RunTest(TestDebugLvl)
