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
import xml.etree.ElementTree as et
from JSBSim_utils import SandBox, CreateFDM


class CheckScripts(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def testScripts(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts')
        for f in os.listdir(self.sandbox.elude(script_path)):
            fullpath = os.path.join(self.sandbox.elude(script_path), f)

            # Is f a file ?
            if not os.path.isfile(fullpath):
                continue

            # Is f an XML file ?
            try:
                tree = et.parse(fullpath)
            except et.ParseError:
                continue

            # Does f contains a JSBSim script ?
            if tree.getroot().tag != 'runscript':
                continue

            # * Aircrafts WK450 and StellarJ do not exist
            # * blank is using version 1.0 of JSBSim format which is no longer
            #   supported.
            if f in ('WK450.xml', 'StellarJ_Fly.xml', 'blank.xml'):
                continue

            fdm = CreateFDM(self.sandbox)
            self.assertTrue(fdm.load_script(os.path.join(script_path, f)),
                            msg="Failed to load script %s" % (fullpath,))
            fdm.run_ic()
            del fdm

suite = unittest.TestLoader().loadTestsFromTestCase(CheckScripts)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
