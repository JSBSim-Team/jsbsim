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

import xml.etree.ElementTree as et
import pandas as pd
from JSBSim_utils import ExecuteUntil, JSBSimTestCase, RunTest

fpstokts = 0.592484


class TestICOverride(JSBSimTestCase):
    def test_IC_override(self):
        # Run the script c1724.xml
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1724.xml')

        fdm = self.create_fdm()
        fdm.load_script(script_path)

        vt0 = fdm['ic/vt-kts']

        fdm.run_ic()
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)
        self.assertAlmostEqual(fdm['velocities/vt-fps'], vt0 / fpstokts,
                               delta=1E-7)

        ExecuteUntil(fdm, 1.0)

        # Check that the total velocity exported in the output file matches the
        # IC defined in the initialization file
        ref = pd.read_csv('JSBout172B.csv')
        self.assertEqual(ref['Time'][0], 0.0)
        self.assertAlmostEqual(ref['V_{Total} (ft/s)'][0], vt0 / fpstokts,
                               delta=1E-7)

        # Now, we will re-run the same test but the IC will be overridden in
        # the script. The initial total velocity is increased by 1 ft/s
        vt0 += 1.0

        # The script c1724.xml is loaded and the following line is added in it:
        #    <property value="..."> ic/vt-kts </property>
        # The modified script is then saved with the named 'c1724_0.xml'
        tree = et.parse(script_path)
        run_tag = tree.getroot().find("run")
        property = et.SubElement(run_tag, 'property')
        property.text = 'ic/vt-kts'
        property.attrib['value'] = str(vt0)
        tree.write('c1724_0.xml')

        # Kill the fdm so that Windows do not block further access to
        # JSBout172B.csv
        fdm = None
        self.delete_fdm()

        # Re-run the same check than above. This time we are making sure than
        # the total initial velocity is increased by 1 ft/s
        self.sandbox.delete_csv_files()

        fdm = self.create_fdm()
        fdm.load_script('c1724_0.xml')

        self.assertAlmostEqual(fdm['ic/vt-kts'], vt0, delta=1E-6)

        fdm.run_ic()
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)
        self.assertAlmostEqual(fdm['velocities/vt-fps'], vt0 / fpstokts,
                               delta=1E-6)

        ExecuteUntil(fdm, 1.0)

        mod = pd.read_csv('JSBout172B.csv')
        self.assertAlmostEqual(mod['V_{Total} (ft/s)'][0], vt0 / fpstokts,
                               delta=1E-6)

    def test_asl_override_vs_geod_lat(self):
        fdm = self.create_fdm()
        fdm.load_model('c310')
        fdm.load_ic('ellington.xml', True)

        geod_lat = fdm['ic/lat-geod-deg']
        fdm['ic/h-sl-ft'] = 35000.
        self.assertAlmostEqual(fdm['ic/lat-geod-deg'], geod_lat)

RunTest(TestICOverride)
