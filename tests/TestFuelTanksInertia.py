# TestFuelTanksInertia.py
#
# A regression test that checks that the fuel tanks inertias are updated.
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
from JSBSim_utils import CreateFDM, ExecuteUntil, SandBox, CopyAircraftDef

class TestFuelTanksInertia(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_fuel_tanks_inertia(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')

        # The aircraft c172x does not contain an <inertia_factor> tag so we need
        # to add one.
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        tank_tag = tree.getroot().find('./propulsion/tank')
        inertia_factor = et.SubElement(tank_tag, 'inertia_factor')
        inertia_factor.text = '1.0'
        tree.write(self.sandbox('aircraft', aircraft_name,  aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        contents0 = fdm.get_property_value('propulsion/tank/contents-lbs')
        ixx0 = fdm.get_property_value('propulsion/tank/local-ixx-slug_ft2')
        iyy0 = fdm.get_property_value('propulsion/tank/local-iyy-slug_ft2')
        izz0 = fdm.get_property_value('propulsion/tank/local-izz-slug_ft2')

        ExecuteUntil(fdm, 200.0)

        contents = fdm.get_property_value('propulsion/tank/contents-lbs')
        ixx = fdm.get_property_value('propulsion/tank/local-ixx-slug_ft2')
        iyy = fdm.get_property_value('propulsion/tank/local-iyy-slug_ft2')
        izz = fdm.get_property_value('propulsion/tank/local-izz-slug_ft2')

        contents_ratio = contents / contents0
        ixx_ratio = ixx / ixx0
        iyy_ratio = iyy / iyy0
        izz_ratio = izz / izz0

        self.assertTrue(abs(contents_ratio - ixx_ratio) < 1E-7,
                        msg="Ixx does not vary as the tank content does\nIxx ratio=%f\nContents ratio=%f" % (ixx_ratio, contents_ratio))
        self.assertTrue(abs(contents_ratio - iyy_ratio) < 1E-7,
                        msg="Iyy does not vary as the tank content does\nIyy ratio=%f\nContents ratio=%f" % (iyy_ratio, contents_ratio))
        self.assertTrue(abs(contents_ratio - izz_ratio) < 1E-7,
                        msg="Izz does not vary as the tank content does\nIzz ratio=%f\nContents ratio=%f" % (izz_ratio, contents_ratio))

suite = unittest.TestLoader().loadTestsFromTestCase(TestFuelTanksInertia)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
