# TestEngineIndexedProps.py
#
# Check that indexed properties (where the engine number is replaced by '#) in
# engines XML definition are working.
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
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest

class TestEngineIndexedProps(JSBSimTestCase):
    def testEnginePowerVC(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'L4102.xml'))
        fdm.run_ic()
        pm = fdm.get_property_manager()
        self.assertTrue(pm.hasNode('propulsion/engine[0]/EnginePowerVC'))
        self.assertTrue(pm.hasNode('propulsion/engine[1]/EnginePowerVC'))

        while fdm.run():
            self.assertAlmostEqual(fdm['propulsion/engine[0]/EnginePowerVC'],
                                   fdm['propulsion/engine[1]/EnginePowerVC'])

    def testFunctionWithIndexedProps(self):
        tree = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         'eng_PegasusXc.xml'))
        # Define the function starter-max-power-W as a 'post' function
        root = tree.getroot()
        startPowFunc_tag = root.find("function/[@name='propulsion/engine[#]/starter-max-power-W']")
        startPowFunc_tag.attrib['type']='post'
        tree.write('eng_PegasusXc.xml')

        # Copy the propeller file.
        shutil.copy(self.sandbox.path_to_jsbsim_file('engine', 'prop_deHavilland5000.xml'),
                    '.')
        fdm = CreateFDM(self.sandbox)
        fdm.set_engine_path('.')
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'Short_S23_1.xml'))
        fdm.run_ic()
        pm = fdm.get_property_manager()
        self.assertTrue(pm.hasNode('propulsion/engine[0]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[1]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[2]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[3]/starter-max-power-W'))

        while fdm.run():
            rpm = [fdm['propulsion/engine[0]/engine-rpm'],
                   fdm['propulsion/engine[1]/engine-rpm'],
                   fdm['propulsion/engine[2]/engine-rpm'],
                   fdm['propulsion/engine[3]/engine-rpm']]
            for i in range(4):
                maxPower = max(0.0, 1.0-rpm[i]/400)*498.941*0.10471976*rpm[i]
                self.assertAlmostEqual(fdm['propulsion/engine[%d]/starter-max-power-W' % (i,)],
                                       maxPower)

    def testTableWithIndexedVars(self):
        tree = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         'eng_PegasusXc.xml'))
        # Define the function starter-max-power-W as a 'post' function
        root = tree.getroot()
        startPowFunc_tag = root.find("function/[@name='propulsion/engine[#]/starter-max-power-W']")
        startPowFunc_tag.attrib['type']='post'
        max_tag = startPowFunc_tag.find('product/max')
        diff_tag = max_tag.find('difference')
        max_tag.remove(diff_tag)
        table_tag = et.SubElement(max_tag,'table')
        table_tag.attrib['name']='propulsion/engine[#]/starter-tabular-data'
        indepVar_tag = et.SubElement(table_tag, 'independentVar')
        indepVar_tag.attrib['lookup']='row'
        indepVar_tag.text = 'propulsion/engine[#]/engine-rpm'
        tData_tag = et.SubElement(table_tag, 'tableData')
        tData_tag.text ='0.0 1.0\n400.0 0.0'
        tree.write('eng_PegasusXc.xml')

        # Copy the propeller file.
        shutil.copy(self.sandbox.path_to_jsbsim_file('engine', 'prop_deHavilland5000.xml'),
                    '.')
        fdm = CreateFDM(self.sandbox)
        fdm.set_engine_path('.')
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'Short_S23_1.xml'))
        fdm.run_ic()
        pm = fdm.get_property_manager()
        self.assertTrue(pm.hasNode('propulsion/engine[0]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[1]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[2]/starter-max-power-W'))
        self.assertTrue(pm.hasNode('propulsion/engine[3]/starter-max-power-W'))

        while fdm.run():
            rpm = [fdm['propulsion/engine[0]/engine-rpm'],
                   fdm['propulsion/engine[1]/engine-rpm'],
                   fdm['propulsion/engine[2]/engine-rpm'],
                   fdm['propulsion/engine[3]/engine-rpm']]
            for i in range(4):
                tabularData = max(0.0, 1.0-rpm[i]/400)
                maxPower = tabularData*498.941*0.10471976*rpm[i]
                self.assertAlmostEqual(fdm['propulsion/engine[%d]/starter-max-power-W' % (i,)],
                                       maxPower)
                self.assertAlmostEqual(fdm['propulsion/engine[%d]/starter-tabular-data' % (i,)],
                                       tabularData)

RunTest(TestEngineIndexedProps)
