# TestSuspend.py
#
# A regression test that checks that suspending JSBSim is not modifying the
# results compared to the same script that was run without being suspended.
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

import pandas as pd
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, RunTest, ExecuteUntil, FindDifferences


class TestSuspend(JSBSimTestCase):
    def setUp(self, *args):
        JSBSimTestCase.setUp(self, *args)

        fdm = self.initFDM()

        while fdm.run():
            pass

        self.ref = pd.read_csv('BallOut.csv', index_col=0)

    def initFDM(self):
        script_name = 'cannonball.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        # To prevent running into issues with floating point exceptions
        tree = et.parse(script_path)
        run_tag = tree.getroot().find('./run')
        run_tag.attrib['end'] = '36'
        tree.write(script_name)

        fdm = self.create_fdm()
        fdm.load_script(script_name)

        fdm['simulation/integrator/rate/rotational'] = 5
        fdm['simulation/integrator/rate/translational'] = 5
        fdm['simulation/integrator/position/rotational'] = 5
        fdm['simulation/integrator/position/translational'] = 5
        fdm.run_ic()
        fdm.set_dt(0.05)
        return fdm

    def testSuspend(self):
        fdm = self.initFDM()
        ExecuteUntil(fdm, 1.0)

        fdm.suspend_integration()
        fdm.disable_output()
        for i in range(5):
            fdm.run()
        fdm.resume_integration()
        fdm.enable_output()

        while fdm.run():
            pass

        out = pd.read_csv('BallOut.csv', index_col=0)

        # Find all the data that are differing by more than 1E-8 between the
        # two data sets.
        diff = FindDifferences(self.ref, out, 1E-8)
        self.longMessage = True
        self.assertEqual(len(diff), 0, msg='\n'+diff.to_string())

    def testHold(self):
        fdm = self.initFDM()
        ExecuteUntil(fdm, 1.0)

        fdm.hold()
        for i in range(5):
            fdm.run()
        fdm.resume()

        while fdm.run():
            pass

        out = pd.read_csv('BallOut.csv', index_col=0)

        # Find all the data that are differing by more than 1E-8 between the
        # two data sets.
        diff = FindDifferences(self.ref, out, 1E-8)
        self.longMessage = True
        self.assertEqual(len(diff), 0, msg='\n'+diff.to_string())

RunTest(TestSuspend)
