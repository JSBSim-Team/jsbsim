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
        tree = et.parse(self.sandbox.elude(self.script_path))
        self.root = tree.getroot()
        use_tag = self.root.find("./use")
        aircraft_name = use_tag.attrib['aircraft']
        run_tag = self.root.find("./run")
        self.dt = float(run_tag.attrib['dt'])

        # Read the output rate and the output file from the aircraft file
        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft', aircraft_name,
                                                         append_xml(aircraft_name))
        tree = et.parse(self.sandbox.elude(aircraft_path))
        output_tag = tree.getroot().find("./output")
        self.output_file = output_tag.attrib['name']
        self.rate = int(0.5 + 1.0/(float(output_tag.attrib['rate']) * self.dt))

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
        self.assertEqual(self.fdm.get_property_value("simulation/output/log_rate_hz"),
                         self.rate)

        self.fdm.run_ic()

        for i in xrange(self.rate):
            self.fdm.run()

        output = Table()
        output.ReadCSV(self.sandbox(self.output_file))

        # According to the settings, the output file must contain 2 lines in
        # addition to the headers :
        # 1. The initial conditions
        # 2. The output after 'rate' iterations
        self.assertEqual(output.get_column(0)[1], 0.0)
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
        output.ReadCSV(self.sandbox(self.output_file))

        # According to the settings, the output file must contain 1 line in
        # addition to the headers :
        # 1. The output after 'rate' iterations
        self.assertEqual(output.get_column(0)[1],
                         self.fdm.get_property_value("simulation/sim-time-sec"))

    def testTrimRestoresOutputSettings(self):
        self.fdm.load_script(self.script_path)

        event_tags = self.root.findall('./run/event')
        for event in event_tags:
            if event.attrib['name'] == 'Trim':
                cond_tag = event.find('./condition')
                trim_date = float(string.split(cond_tag.text)[-1])
                break

        # Disables the output during the initialization
        self.fdm.set_property_value("simulation/output/enabled", 0.0)
        self.fdm.run_ic()

        while self.fdm.get_property_value("simulation/sim-time-sec") < trim_date + 2.0*self.dt:
            self.fdm.run()
            self.assertEqual(self.fdm.get_property_value("simulation/output/enabled"),
                             0.0)

suite = unittest.TestLoader().loadTestsFromTestCase(CheckOutputRate)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
