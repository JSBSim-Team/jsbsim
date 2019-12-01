# TestTurboProp.py
#
# Regression tests for the turboprop engine model.
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

import shutil
import xml.etree.ElementTree as et
import pandas as pd
from JSBSim_utils import JSBSimTestCase, RunTest, isDataMatching, FindDifferences


class TestTurboProp(JSBSimTestCase):
    def testEnginePowerVC(self):
        # Check that the same results are obtained whether the engine power
        # velocity correction is given in a <table> or <function>
        fdm = self.create_fdm()
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'L4102.xml'))
        fdm.run_ic()

        while fdm.run():
            pass

        # Kill the fdm so that Windows do not block further access to L410.csv.
        fdm = None
        self.delete_fdm()

        ref = pd.read_csv('L410.csv', index_col=0)

        tree = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         'engtm601.xml'))
        # Modify the engine definition to use a <function> rather than a
        # <table> component.
        root = tree.getroot()
        engPowVC_tag = root.find("table/[@name='EnginePowerVC']")
        root.remove(engPowVC_tag)
        del engPowVC_tag.attrib['name']
        func_engPowVC = et.SubElement(root, 'function')
        func_engPowVC.attrib['name'] = 'EnginePowerVC'
        func_engPowVC.append(engPowVC_tag)
        tree.write('engtm601.xml')

        # Copy the propeller file.
        shutil.copy(self.sandbox.path_to_jsbsim_file('engine', 'vrtule2.xml'),
                    '.')
        self.sandbox.delete_csv_files()

        fdm = self.create_fdm()
        fdm.set_engine_path('.')
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'L4102.xml'))
        fdm.run_ic()

        while fdm.run():
            pass

        current = pd.read_csv('L410.csv', index_col=0)

        # Check the data are matching i.e. the time steps are the same between
        # the two data sets and that the output data are also the same.
        self.assertTrue(isDataMatching(ref, current))

        # Find all the data that are differing by more than 1E-5 between the
        # two data sets.
        diff = FindDifferences(ref, current, 0.0)
        self.longMessage = True
        self.assertEqual(len(diff), 0, msg='\n'+diff.to_string())

RunTest(TestTurboProp)
