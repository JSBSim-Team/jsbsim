# CheckTrim.py
#
# Regression tests of the trim feature
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

import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest, CopyAircraftDef


class CheckTrim(JSBSimTestCase):
    def test_trim_doesnt_ignite_rockets(self):
        # Run a longitudinal trim with a rocket equipped with solid propellant
        # boosters (aka SRBs). The trim algorithm will try to reach a vertical
        # equilibrium by tweaking the throttle but since the rocket is nose up,
        # the trim cannot converge. As a result the algorithm will set full
        # throttle which will result in the SRBs ignition if the integration is
        # not suspended. This bug has been reported in FlightGear and this test
        # is checking that there is no regression.

        fdm = self.create_fdm()
        fdm.load_model('J246')
        fdm.load_ic(self.sandbox.path_to_jsbsim_file('aircraft', 'J246',
                                                     'LC39'), False)
        fdm.run_ic()

        # Check that the SRBs are not ignited
        self.assertEqual(fdm['propulsion/engine[0]/thrust-lbs'], 0.0)
        self.assertEqual(fdm['propulsion/engine[1]/thrust-lbs'], 0.0)

        try:
            fdm['simulation/do_simple_trim'] = 1
        except RuntimeError as e:
            # The trim cannot succeed. Just make sure that the raised exception
            # is due to the trim failure otherwise rethrow.
            if e.args[0] != 'Trim Failed':
                raise

        # Check that the trim did not ignite the SRBs
        self.assertEqual(fdm['propulsion/engine[0]/thrust-lbs'], 0.0)
        self.assertEqual(fdm['propulsion/engine[1]/thrust-lbs'], 0.0)

    def test_trim_on_ground(self):
        # Check that the trim is made with up to date initial conditions
        fdm = self.create_fdm()
        fdm.load_model('c172x')
        fdm['ic/theta-deg'] = 90.0
        fdm.run_ic()
        fdm['ic/theta-deg'] = 0.0
        # If the trim fails, it will raise an exception
        fdm['simulation/do_simple_trim'] = 2  # Ground trim

        # Check that the aircraft has been trimmed successfully (velocities
        # should be zero i.e. the aircraft must be motionless once trimmed).
        while fdm['simulation/sim-time-sec'] <= 1.0:
            fdm.run()
            self.assertAlmostEqual(fdm['velocities/p-rad_sec'], 0., delta=1E-4)
            self.assertAlmostEqual(fdm['velocities/q-rad_sec'], 0., delta=1E-4)
            self.assertAlmostEqual(fdm['velocities/r-rad_sec'], 0., delta=1E-4)
            self.assertAlmostEqual(fdm['velocities/u-fps'], 0.0, delta=1E-4)
            self.assertAlmostEqual(fdm['velocities/v-fps'], 0.0, delta=1E-4)
            self.assertAlmostEqual(fdm['velocities/w-fps'], 0.0, delta=1E-4)

    def test_trim_westward(self):
        # This is a regression test after the bug reported in GitHub issue #163
        # which reports a trim failure when the heading is set to 270 degrees or
        # -90 degrees i.e. westward.
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       '737_cruise.xml')
        aircraft_tree, aircraft_name, b = CopyAircraftDef(script_path,
                                                          self.sandbox)
        aircraft_tree.write(self.sandbox('aircraft', aircraft_name,
                                         aircraft_name+'.xml'))

        IC_file = self.sandbox('aircraft', aircraft_name, 'cruise_init.xml')
        tree = et.parse(IC_file)
        heading_el = tree.find('psi')
        heading_el.text = '270.0'
        tree.write(IC_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox('aircraft'))
        fdm.load_script(script_path)
        fdm.run_ic()

        while fdm['simulation/sim-time-sec'] < 6.0:
            fdm.run()

    def test_trim_with_actuator_delay(self):
        # This is a regression test that checks that actuators delays are
        # disabled when the trim takes place (GitHub issue #293).
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       'c1722.xml')
        aircraft_tree, aircraft_name, _ = CopyAircraftDef(script_path,
                                                          self.sandbox)
        root = aircraft_tree.getroot()
        elevator_actuator = root.find("flight_control/channel/actuator[@name='fcs/elevator-actuator']")
        delay = et.SubElement(elevator_actuator, 'delay')
        delay.text = '0.1'
        aircraft_tree.write(self.sandbox('aircraft', aircraft_name,
                                         aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox('aircraft'))
        fdm.load_script(script_path)
        fdm.run_ic()

        while fdm.run():
            if fdm['simulation/trim-completed'] == 1:
                break


RunTest(CheckTrim)
