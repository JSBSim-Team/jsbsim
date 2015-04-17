# CheckAircrafts.py
#
# A regression test that checks that all the aircrafts can be read by JSBSim
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

import os, unittest, sys, string
import xml.etree.ElementTree as et
from JSBSim_utils import SandBox, append_xml, CreateFDM


class CheckAircrafts(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def testAircrafts(self):
        aircraft_path = self.sandbox.elude(self.sandbox.path_to_jsbsim_file('aircraft'))
        for d in os.listdir(aircraft_path):
            fullpath = os.path.join(aircraft_path, d)

            # Is d a directory ?
            if not os.path.isdir(fullpath):
                continue

            f = os.path.join(aircraft_path, d, append_xml(d))

            # Is f a file ?
            if not os.path.isfile(f):
                continue

            # Is f an XML file ?
            try:
                tree = et.parse(f)
            except et.ParseError:
                continue

            # Is f an aircraft definition file ?
            if string.upper(tree.getroot().tag) != 'FDM_CONFIG':
                continue

            if d in ('blank'):
                continue

            fdm = CreateFDM(self.sandbox)
            self.assertTrue(fdm.load_model(d),
                            msg='Failed to load aircraft %s' % (d,))
            del fdm


suite = unittest.TestLoader().loadTestsFromTestCase(CheckAircrafts)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
