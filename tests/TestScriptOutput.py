# TestScriptInputOutput.py
#
# Check that <output> tags specified in a script are properly handled
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

import sys, unittest
import xml.etree.ElementTree as et
import pandas as pd
import numpy as np
from JSBSim_utils import CreateFDM, SandBox, ExecuteUntil


class TestScriptOutput(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1722.xml')

    def tearDown(self):
        self.sandbox.erase()

    def test_no_output(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.script_path)
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertFalse(self.sandbox.exists('output.csv'),
                         msg="Results have unexpectedly been written to 'output.csv'")

    def test_output_from_file(self):
        tree = et.parse(self.sandbox.elude(self.script_path))
        output_tag = et.SubElement(tree.getroot(), 'output')
        output_tag.attrib['file'] = self.sandbox.elude(self.sandbox.path_to_jsbsim_file('tests', 'output.xml'))
        tree.write(self.sandbox('c1722_0.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1722_0.xml')
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertTrue(self.sandbox.exists('output.csv'),
                        msg="The file 'output.csv' has not been created")

    def test_output(self):
        tree = et.parse(self.sandbox.elude(self.script_path))
        output_tag = et.SubElement(tree.getroot(), 'output')
        output_tag.attrib['name'] = 'test.csv'
        output_tag.attrib['type'] = 'CSV'
        output_tag.attrib['rate'] = '10'
        property_tag = et.SubElement(output_tag, 'property')
        property_tag.text = 'position/vrp-radius-ft'
        tree.write(self.sandbox('c1722_0.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1722_0.xml')
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertTrue(self.sandbox.exists(output_tag.attrib['name']),
                        msg="The file 'output.csv' has not been created")
        orig = pd.read_csv(self.sandbox('JSBout172B.csv'))
        test = pd.read_csv(self.sandbox('test.csv'))
        self.assertEqual(np.max(orig['Time']-test['Time']), 0.0)
        pname = '/fdm/jsbsim/' + property_tag.text
        self.assertEqual(np.max(orig[pname]-test[pname]), 0.0)

suite = unittest.TestLoader().loadTestsFromTestCase(TestScriptOutput)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
