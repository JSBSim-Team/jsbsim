# CheckScripts.py
#
# A regression test that checks that all the scripts can be read by JSBSim
# without issuing errors.
#
# Copyright (c) 2015-2019 Bertrand Coconnier
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
from JSBSim_utils import JSBSimTestCase, RunTest, ExecuteUntil

import fpectl
import xml.etree.ElementTree as et
import pandas as pd


class CheckScripts(JSBSimTestCase):
    def testScripts(self):
        fpectl.turnon_sigfpe()

        for s in self.script_list(['737_cruise_steady_turn_simplex.xml']):
            fdm = self.create_fdm()
            try:
                self.assertTrue(fdm.load_script(s),
                                msg="Failed to load script %s" % (s,))

                fdm.run_ic()
                ExecuteUntil(fdm, 30.)

            except Exception as e:
                fpectl.turnoff_sigfpe()
                self.fail("Script %s failed:\n%s" % (s, e.args[0]))

        fpectl.turnoff_sigfpe()

    def testScriptEndTime(self):
        # Regression test: using a time step different than 120Hz in a script
        # could result in executing an extra time step in certain conditions
        # reproduced in this test.
        # Here, we are checking that the last step logged in the CSV file
        # corresponds to the end time specified in the script.
        script_name = 'c1722.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        tree = et.parse(script_path)
        run_tag = tree.getroot().find('./run')
        run_tag.attrib['dt'] = '0.001'
        end_time = float(run_tag.attrib['end'])
        tree.write(script_name)

        fdm = self.create_fdm()
        fdm.load_script(script_name)
        fdm['simulation/output/log_rate_hz'] = 200
        fdm.run_ic()

        while fdm.run():
            pass

        out = pd.read_csv('JSBout172B.csv', index_col=0)
        self.assertAlmostEqual(out.index[-1], end_time)


RunTest(CheckScripts)
