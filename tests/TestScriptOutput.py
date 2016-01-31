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

import os
import xml.etree.ElementTree as et
import pandas as pd
import numpy as np
from JSBSim_utils import (JSBSimTestCase, CreateFDM, ExecuteUntil,
                          isDataMatching, RunTest)


class TestScriptOutput(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1722.xml')

    def test_no_output(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.script_path)
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertFalse(self.sandbox.exists('output.csv'),
                         msg="Results have unexpectedly been written to 'output.csv'")

    def test_output_from_file(self):
        tree = et.parse(self.script_path)
        output_tag = et.SubElement(tree.getroot(), 'output')
        # Relative path from the aircraft directory to the output directive
        # file
        output_tag.attrib['file'] = os.path.join('..', '..', 'tests',
                                                 'output.xml')
        tree.write('c1722_0.xml')

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1722_0.xml')
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertTrue(self.sandbox.exists('output.csv'),
                        msg="The file 'output.csv' has not been created")

    def test_output(self):
        tree = et.parse(self.script_path)
        output_tag = et.SubElement(tree.getroot(), 'output')
        output_tag.attrib['name'] = 'test.csv'
        output_tag.attrib['type'] = 'CSV'
        output_tag.attrib['rate'] = '10'
        property_tag = et.SubElement(output_tag, 'property')
        property_tag.text = 'position/vrp-radius-ft'
        tree.write('c1722_0.xml')

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1722_0.xml')
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        self.assertTrue(self.sandbox.exists(output_tag.attrib['name']),
                        msg="The file 'output.csv' has not been created")
        orig = pd.read_csv('JSBout172B.csv', index_col=0)
        test = pd.read_csv('test.csv', index_col=0)
        pname = '/fdm/jsbsim/' + property_tag.text
        ref = orig[pname]
        mod = test[pname]

        # Check the data are matching i.e. the time steps are the same between
        # the two data sets.
        self.assertTrue(isDataMatching(ref, mod))

        # Find all the data that are differing by more than 1E-8 between the
        # two data sets.
        delta = pd.concat([np.abs(ref - mod), ref, mod], axis=1)
        delta.columns = ['delta', 'ref value', 'value']
        diff = delta[delta['delta'] > 1E-8]
        self.longMessage = True
        self.assertEqual(len(diff), 0, msg='\n'+diff.to_string())

RunTest(TestScriptOutput)
