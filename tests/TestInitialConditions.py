# TestICOutput.py
#
# A regression test that checks that IC are correctly read from the IC file then
# loaded in the ic/ properties. It also checks that the correct ICs are reported
# in the data written in CSV files.
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

import unittest, sys, os
import xml.etree.ElementTree as et
from JSBSim_utils import CreateFDM, SandBox, append_xml, ExecuteUntil, Table

# Values copied from FGJSBBase.cpp
ktstofps = 1.68781
radtodeg = 57.295779513082320876798154814105
mtoft = 1.0 / 0.3048

class TestInitialConditions(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_initial_conditions(self):
        # A dictionary that contains the XML tags to extract from the IC file
        # along with the name of the properties that contain the values
        # extracted from the IC file.
        vars = [{'tag': 'vt', 'unit': 'KTS', 'conv': ktstofps,
                 'ic_prop': 'ic/vt-fps', 'prop': 'velocities/vt-fps',
                 'CSV_header': 'V_{Total} (ft/s)'},
                {'tag': 'latitude', 'unit': 'RAD', 'conv': radtodeg,
                 'ic_prop': 'ic/lat-gc-deg', 'prop': 'position/lat-gc-deg',
                 'CSV_header': 'Latitude (deg)'},
                {'tag': 'longitude', 'unit': 'RAD', 'conv': radtodeg,
                 'ic_prop': 'ic/long-gc-deg', 'prop': 'position/long-gc-deg',
                 'CSV_header': 'Longitude (deg)'},
                {'tag': 'altitude', 'unit': 'M', 'conv': mtoft,
                 'ic_prop': 'ic/h-sl-ft', 'prop': 'position/h-sl-ft',
                 'CSV_header': 'Altitude ASL (ft)'},
                {'tag': 'phi', 'unit': 'RAD', 'conv': radtodeg,
                 'ic_prop': 'ic/phi-deg', 'prop': 'attitude/phi-deg',
                 'CSV_header': 'Phi (deg)'},
                {'tag': 'theta', 'unit': 'RAD', 'conv': radtodeg,
                 'ic_prop': 'ic/theta-deg', 'prop': 'attitude/theta-deg',
                 'CSV_header': 'Theta (deg)'},
                {'tag': 'psi', 'unit': 'RAD', 'conv': radtodeg,
                 'ic_prop': 'ic/psi-true-deg', 'prop': 'attitude/psi-deg',
                 'CSV_header': 'Psi (deg)'}]

        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')

        # Read the IC file name from the script
        tree = et.parse(self.sandbox.elude(script_path))
        use_tag = tree.getroot().find('use')

        aircraft_name = use_tag.attrib['aircraft']
        aircraft_path = os.path.join('aircraft', aircraft_name)
        path_to_jsbsim_aircrafts = self.sandbox.elude(self.sandbox.path_to_jsbsim_file(aircraft_path))

        IC_file = append_xml(use_tag.attrib['initialize'])
        IC_tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, IC_file))
        IC_root = IC_tree.getroot()

        # Extract the IC values from XML
        for var in vars:
            var_tag = IC_root.find('./'+var['tag'])
            if var_tag is None:
                var['value'] = 0.0
                continue

            var['value'] = float(var_tag.text)
            if 'unit' in var_tag.attrib and var_tag.attrib['unit'] == var['unit']:
                var['value'] *= var['conv']

        # Initialize the script
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(script_path)
        fdm.run_ic()

        # Sanity check, we just initialized JSBSim with the ICs, the time must
        # be set to 0.0
        self.assertEqual(fdm.get_property_value('simulation/sim-time-sec'), 0.0)

        # Check that the properties (including in 'ic/') have been correctly
        # intialized (i.e. that they contain the value read from the XML file).
        for var in vars:
            value = var['value']
            prop = fdm.get_property_value(var['ic_prop'])
            self.assertAlmostEqual(value, prop, delta=1E-7,
                                   msg="%s is %f != %f" % (var['tag'], value, prop))
            prop = fdm.get_property_value(var['prop'])
            self.assertAlmostEqual(value, prop, delta=1E-7,
                                   msg="%s is %f != %f" % (var['tag'], value, prop))

        # Execute the first second of the script. This is to make sure that
        # the CSV file is open and the ICs have been written in it.
        ExecuteUntil(fdm, 1.0)

        # Copies the CSV file content in a table
        ref = Table()
        ref.ReadCSV(self.sandbox('JSBout172B.csv'))

        # Sanity check: make sure that the time step 0.0 has been copied in the
        # CSV file.
        self.assertEqual(ref.get_column('Time')[1], 0.0)

        # Check that the value in the CSV file equals the value read from the
        # IC file.
        for var in vars:
            value = var['value']
            csv_value = ref.get_column(var['CSV_header'])[1]
            self.assertAlmostEqual(value, csv_value, delta=1E-7,
                                   msg="%s is %f != %f" % (var['tag'], value, csv_value))

suite = unittest.TestLoader().loadTestsFromTestCase(TestInitialConditions)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
