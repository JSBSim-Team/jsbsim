# TestICOverride.py
#
# A regression test that checks that IC loaded from a file by a script can be
# overridden.
#
# Copyright (c) 2014 Bertrand Coconnier
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
from JSBSim_utils import Table, CreateFDM, ExecuteUntil, SandBox

fpstokts = 0.592484

class TestICOverride(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def test_IC_override(self):
        # Run the script c1724.xml
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1724.xml')

        fdm = CreateFDM(self.sandbox)
        fdm.load_script(script_path)

        vt0 = fdm.get_property_value('ic/vt-kts')

        fdm.run_ic()
        ExecuteUntil(fdm, 1.0)

        # Check that the total velocity exported in the output file matches the IC
        # defined in the initialization file
        ref = Table()
        ref.ReadCSV(self.sandbox('JSBout172B.csv'))

        for col, title in enumerate(ref._lines[0]):
            if title == 'V_{Total} (ft/s)':
                self.assertTrue(abs(ref._lines[1][col] - (vt0 / fpstokts)) < 1E-5,
                                msg="Original script %s\nThe total velocity is %f. The value %f was expected" % (script_path, ref._lines[1][col], vt0 / fpstokts))
                break
        else:
            self.fail("The total velocity is not exported in %s" % (script_path,))

        # Now, we will re-run the same test but the IC will be overridden in the scripts
        # The initial total velocity is increased by 1 ft/s
        vt0 += 1.0

        # The script c1724.xml is loaded and the following line is added in it:
        #    <property value="..."> ic/vt-kts </property>
        # The modified script is then saved with the named 'c1724_0.xml'
        tree = et.parse(self.sandbox.elude(script_path))
        run_tag = tree.getroot().find("./run")
        property = et.SubElement(run_tag, 'property')
        property.text = 'ic/vt-kts'
        property.attrib['value'] = str(vt0)
        tree.write(self.sandbox('c1724_0.xml'))

        # Re-run the same check than above. This time we are making sure than the total
        # initial velocity is increased by 1 ft/s
        self.sandbox.delete_csv_files()

        # Because JSBSim internals use static pointers, we cannot rely on Python
        # garbage collector to decide when the FDM is destroyed otherwise we can
        # get dangling pointers.
        del fdm

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1724_0.xml')

        self.assertTrue(abs(fdm.get_property_value('ic/vt-kts') - vt0) < 1E-5,
                        msg="Modified script %s\nThe total velocity in the IC (%f) is different from %f" % (self.sandbox('JSBout172B.csv'), fdm.get_property_value('ic/vt-kts'), vt0))

        fdm.run_ic()
        ExecuteUntil(fdm, 1.0)

        mod = Table()
        mod.ReadCSV(self.sandbox('JSBout172B.csv'))

        for col, title in enumerate(mod._lines[0]):
            if title == 'V_{Total} (ft/s)':
                self.assertTrue(abs(mod._lines[1][col] - (vt0 / fpstokts)) < 1E-5,
                                msg="Modified script %s\nThe total velocity is %f. The value %f was expected" % (self.sandbox('JSBout172B.csv'), mod._lines[1][col], vt0 / fpstokts))
                break
        else:
            self.fail("The total velocity is not exported in %s" % (sandbox('JSBout172B.csv'),))

suite = unittest.TestLoader().loadTestsFromTestCase(TestICOverride)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.

