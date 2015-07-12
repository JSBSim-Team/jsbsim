# CheckScripts.py
#
# A regression test that checks that all the scripts can be read by JSBSim
# without issuing errors.
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

import os, unittest, sys
from JSBSim_utils import SandBox, CreateFDM, CheckXMLFile


class CheckScripts(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()
        self.scripts = 0

    def tearDown(self):
        print "Tested %g scripts" % (self.scripts,)
        self.sandbox.erase()

    def testScripts(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts')
        for f in os.listdir(self.sandbox.elude(script_path)):
            fullpath = os.path.join(self.sandbox.elude(script_path), f)

            # Does f contains a JSBSim script ?
            if not CheckXMLFile(fullpath, 'runscript'):
                continue

            fdm = CreateFDM(self.sandbox)
            self.assertTrue(fdm.load_script(os.path.join(script_path, f)),
                            msg="Failed to load script %s" % (fullpath,))
            fdm.run_ic()

            self.scripts += 1
            del fdm

suite = unittest.TestLoader().loadTestsFromTestCase(CheckScripts)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
