# TestIntegrators.py
#
# Test that integrators in <pid> and <filter> are functional.
#
# Copyright (c) 2018 Bertrand Coconnier
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

import os, math
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, RunTest


class TestIntegrators(JSBSimTestCase):
    def test_pid(self):
        fdm = self.create_fdm()
        path = self.sandbox.path_to_jsbsim_file('tests')
        tree = et.parse(os.path.join(path, 'tripod.xml'))
        root = tree.getroot()
        system_tag = et.SubElement(root, 'system')
        system_tag.attrib['file'] = 'integrators.xml'
        tree.write('tripod.xml')
        
        fdm.set_aircraft_path('.')
        fdm.set_systems_path(path)
        fdm.set_dt(0.005)
        fdm.load_model('tripod', False)

        fdm['test/input'] = 0.0
        fdm.run_ic()
        self.assertAlmostEqual(fdm['test/output-pid-rect'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-trap'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab2'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab3'], 0.0)

        k = 8*math.pi # Constant for a period of 1/4s

        for i in range(100):
            t = fdm['simulation/sim-time-sec']
            fdm['test/input'] = math.sin(k*t)
            # Skip the first value because Adams-Bashforth 3 is not yet
            # correctly initialized.
            if i>1:
                self.assertAlmostEqual(fdm['test/output-pid-ab3'],
                                       (1.0-math.cos(k*t))/k, delta=1E-4)
            fdm.run()

        # Checks that a positive trigger value suspends the integration
        fdm['test/trigger'] = 1.0
        vrect = fdm['test/output-pid-rect']
        vtrap = fdm['test/output-pid-trap']
        vab2 = fdm['test/output-pid-ab2']
        vab3 = fdm['test/output-pid-ab3']
        for i in range(49):
            fdm.run()
            self.assertAlmostEqual(fdm['test/output-pid-rect'], vrect)
            self.assertAlmostEqual(fdm['test/output-pid-trap'], vtrap)
            self.assertAlmostEqual(fdm['test/output-pid-ab2'], vab2)
            self.assertAlmostEqual(fdm['test/output-pid-ab3'], vab3)

        # Checks that a negative trigger value resets the integration
        fdm['test/trigger'] = -1.0
        # The input needs to be set to 0.0 for a correct initialization of the
        # Adams-Bashforth schemes.
        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/output-pid-rect'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-trap'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab2'], 0.0)
        self.assertAlmostEqual(fdm['test/output-pid-ab3'], 0.0)

        # Checks that resetting the trigger to zero restarts the integration
        fdm['test/trigger'] = 0.0
        t0 = fdm['simulation/sim-time-sec']
        for i in range(50):
            t = fdm['simulation/sim-time-sec']
            fdm['test/input'] = math.sin(k*(t-t0))
            # Skip the first value because Adams-Bashforth 3 is not yet
            # correctly initialized.
            if i>1:
                self.assertAlmostEqual(fdm['test/output-pid-ab3'],
                                       (1.0-math.cos(k*(t-t0)))/k, delta=1E-4)
            fdm.run()


RunTest(TestIntegrators)
