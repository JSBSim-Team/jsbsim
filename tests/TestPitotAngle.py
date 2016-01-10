# TestPitotAngle.py
#
# Test the Pitot angle feature
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

import unittest, sys, os
import xml.etree.ElementTree as et
from JSBSim_utils import SandBox, CreateFDM, CopyAircraftDef, append_xml


class TestPitotAngle(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_CAS_ic(self):
        script_name = 'Short_S23_3.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        tree, aircraft_name, path_to_jsbsim_aircrafts = CopyAircraftDef(script_path, self.sandbox)
        metrics_tag = tree.getroot().find('./metrics')
        pitot_tag = et.SubElement(metrics_tag, 'pitot_angle')
        pitot_tag.attrib['unit'] = 'DEG'
        pitot_tag.text = '5.0'
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        tree = et.parse(self.sandbox.elude(script_path))
        use_element = tree.getroot().find('use')
        IC_file = use_element.attrib['initialize']
        tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, append_xml(IC_file)))
        vc_tag = tree.getroot().find('./vc')
        VCAS = float(vc_tag.text)
        if 'unit' in vc_tag.attrib and vc_tag.attrib['unit'] == 'FT/SEC':
            VCAS /= 1.68781

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        self.assertAlmostEqual(fdm.get_property_value('velocities/vc-kts'),
                               VCAS, delta=1E-7)

suite = unittest.TestLoader().loadTestsFromTestCase(TestPitotAngle)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
