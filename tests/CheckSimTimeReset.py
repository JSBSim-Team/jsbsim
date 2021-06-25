# CheckSimTimeReset.py
#
# A regression test that checks that the sim time is set to 0.0 when the
# simulation is reset
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

import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, ExecuteUntil, RunTest


class TestSimTimeReset(JSBSimTestCase):
    def test_no_script(self):
        fdm = self.create_fdm()
        fdm.load_model('c172x')
        fdm.load_ic('reset01.xml', True)
        fdm.run_ic()

        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)
        ExecuteUntil(fdm, 5.0)

        t = fdm['simulation/sim-time-sec']
        fdm['simulation/do_simple_trim'] = 1
        self.assertEqual(fdm['simulation/sim-time-sec'], t)

        fdm.reset_to_initial_conditions(1)
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)

    def test_script_start_time_0(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        fdm = self.create_fdm()
        fdm.load_script(script_path)
        fdm.run_ic()

        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)
        ExecuteUntil(fdm, 5.0)

        fdm.reset_to_initial_conditions(1)
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)

    def test_script_start_time(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        tree = et.parse(script_path)
        run_tag = tree.getroot().find('./run')
        run_tag.attrib['start'] = '1.2'
        tree.write(script_name)
        fdm = self.create_fdm()

        fdm.load_script(script_name)
        fdm.run_ic()

        self.assertEqual(fdm['simulation/sim-time-sec'], 1.2)
        ExecuteUntil(fdm, 5.0)

        fdm.reset_to_initial_conditions(1)
        self.assertEqual(fdm['simulation/sim-time-sec'], 1.2)

    def test_script_no_start_time(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        tree = et.parse(script_path)
        run_tag = tree.getroot().find('./run')
        # Remove the parameter 'start' from the tag <run>
        del run_tag.attrib['start']
        tree.write(script_name)
        fdm = self.create_fdm()

        fdm.load_script(script_name)
        fdm.run_ic()

        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)
        ExecuteUntil(fdm, 5.0)

        fdm.reset_to_initial_conditions(1)
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)

RunTest(TestSimTimeReset)
