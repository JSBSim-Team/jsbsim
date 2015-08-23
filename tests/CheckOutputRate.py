# CheckOutputRate.py
#
# A regression test on the output features that allow to set the output rate
# including enabling/disabling the output.
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

import sys, unittest, string
import xml.etree.ElementTree as et
from JSBSim_utils import CreateFDM, SandBox, append_xml, Table


class CheckOutputRate(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

        self.fdm = CreateFDM(self.sandbox)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')

        # Read the time step 'dt' from the script file
        self.tree = et.parse(self.sandbox.elude(self.script_path))
        root = self.tree.getroot()
        use_tag = root.find("./use")
        aircraft_name = use_tag.attrib['aircraft']
        self.run_tag = root.find("./run")
        self.dt = float(self.run_tag.attrib['dt'])

        # Read the date at which the trim will be run
        event_tags = root.findall('./run/event')
        for event in event_tags:
            if event.attrib['name'] == 'Trim':
                cond_tag = event.find('./condition')
                self.trim_date = float(string.split(cond_tag.text)[-1])
                break

        # Read the output rate and the output file from the aircraft file
        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft', aircraft_name,
                                                         append_xml(aircraft_name))
        tree = et.parse(self.sandbox.elude(aircraft_path))
        output_tag = tree.getroot().find("./output")
        self.output_file = self.sandbox(output_tag.attrib['name'])
        self.rateHz = float(output_tag.attrib['rate'])
        self.rate = int(1.0 / (self.rateHz * self.dt))

    def tearDown(self):
        del self.fdm
        self.sandbox.erase()

    def testOutputRate(self):
        self.fdm.load_script(self.script_path)

        # Check that the output is enabled by default
        self.assertEqual(self.fdm.get_property_value("simulation/output/enabled"),
                         1.0)

        # Check that the rate is consistent with the values extracted from the
        # script and the aircraft definition
        self.assertAlmostEqual(self.fdm.get_property_value("simulation/output/log_rate_hz"),
                               self.rateHz, delta=1E-5)

        self.fdm.run_ic()

        for i in xrange(self.rate):
            self.fdm.run()

        output = Table()
        output.ReadCSV(self.output_file)

        # According to the settings, the output file must contain 2 lines in
        # addition to the headers :
        # 1. The initial conditions
        # 2. The output after 'rate' iterations
        self.assertEqual(output.get_column(0)[1], 0.0)
        self.assertEqual(output.get_column(0)[2], self.rate * self.dt)
        self.assertEqual(output.get_column(0)[2],
                         self.fdm.get_property_value("simulation/sim-time-sec"))

    def testDisablingOutput(self):
        self.fdm.load_script(self.script_path)

        # Disables the output during the initialization
        self.fdm.set_property_value("simulation/output/enabled", 0.0)
        self.fdm.run_ic()
        self.fdm.set_property_value("simulation/output/enabled", 1.0)

        for i in xrange(self.rate):
            self.fdm.run()

        output = Table()
        output.ReadCSV(self.output_file)

        # According to the settings, the output file must contain 1 line in
        # addition to the headers :
        # 1. The output after 'rate' iterations
        self.assertEqual(output.get_column(0)[1],
                         self.fdm.get_property_value("simulation/sim-time-sec"))

    def testTrimRestoresOutputSettings(self):
        self.fdm.load_script(self.script_path)

        # Disables the output during the initialization
        self.fdm.set_property_value("simulation/output/enabled", 0.0)
        self.fdm.run_ic()

        # Check that the output remains disabled even after the trim is
        # executed
        while self.fdm.get_property_value("simulation/sim-time-sec") < self.trim_date + 2.0*self.dt:
            self.fdm.run()
            self.assertEqual(self.fdm.get_property_value("simulation/output/enabled"),
                             0.0)

        # Re-enable the output and check that the output rate is unaffected by
        # the previous operations
        self.fdm.set_property_value("simulation/output/enabled", 1.0)
        frame = int(self.fdm.get_property_value("simulation/frame"))

        for i in xrange(self.rate):
            self.fdm.run()

        output = Table()
        output.ReadCSV(self.output_file)

        # The frame at which the data is logged must be the next multiple of
        # the output rate
        self.assertEqual(int(output.get_column(0)[1]/self.dt),
                         (1 + frame/self.rate)*self.rate)

    def testDisablingOutputInScript(self):
        property = et.SubElement(self.run_tag, 'property')
        property.text = 'simulation/output/enabled'
        property.attrib['value'] = "0.0"
        self.tree.write(self.sandbox('c1722_0.xml'))

        self.fdm.load_script('c1722_0.xml')

        # Check that the output is disabled
        self.assertEqual(self.fdm.get_property_value("simulation/output/enabled"),
                         0.0)

        self.fdm.run_ic()
        self.fdm.set_property_value("simulation/output/enabled", 1.0)

        for i in xrange(self.rate):
            self.fdm.run()

        output = Table()
        output.ReadCSV(self.output_file)

        # According to the settings, the output file must contain 1 line in
        # addition to the headers :
        # 1. The output after 'rate' iterations
        self.assertEqual(output.get_column(0)[1],
                         self.fdm.get_property_value("simulation/sim-time-sec"))

suite = unittest.TestLoader().loadTestsFromTestCase(CheckOutputRate)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
