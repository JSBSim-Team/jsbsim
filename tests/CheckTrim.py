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
from jsbsim import TrimFailureError


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
        fdm.load_ic('LC39', True)
        fdm.run_ic()

        # Check that the SRBs are not ignited
        self.assertEqual(fdm['propulsion/engine[0]/thrust-lbs'], 0.0)
        self.assertEqual(fdm['propulsion/engine[1]/thrust-lbs'], 0.0)

        # Trigger the trimming and check that it fails (i.e. it raises an
        # exception TrimFailureError)
        with self.assertRaises(TrimFailureError):
            fdm['simulation/do_simple_trim'] = 1

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

    def test_piston_steady_state_refreshes_fcs_engine_feedback(self):
        # The FCS sets BSFC from MAP, which changes while the piston engine
        # settles. A frozen BSFC is a false steady state (issue #1440).
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')
        aircraft_tree, aircraft_name, _ = CopyAircraftDef(script_path,
                                                          self.sandbox)
        system = et.SubElement(aircraft_tree.getroot(), 'system',
                               name='Piston engine feedback')
        channel = et.SubElement(system, 'channel', name='Engine BSFC')
        channel.append(et.fromstring('''
            <fcs_function name="systems/map-bsfc">
              <function>
                <sum>
                  <value>0.3</value>
                  <product>
                    <value>0.005</value>
                    <property>propulsion/engine/map-inhg</property>
                  </product>
                </sum>
              </function>
              <output>propulsion/engine/bsfc-lbs_hphr</output>
            </fcs_function>'''))
        aircraft_tree.write(self.sandbox('aircraft', aircraft_name,
                                         aircraft_name + '.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox('aircraft'))
        self.assertTrue(fdm.load_model(aircraft_name))
        self.assertTrue(fdm.load_ic('reset01', True))
        fdm['ic/h-sl-ft'] = 10000.0
        fdm['ic/vc-kts'] = 90.0
        self.assertTrue(fdm.run_ic())
        self.assertEqual(fdm['propulsion/engine/set-running'], 1.0)

        fdm['fcs/throttle-cmd-norm'] = 0.2
        fdm.run()
        fdm.set_trim_status(True)
        fdm.suspend_integration()
        try:
            fdm.get_propulsion().get_steady_state()
        finally:
            fdm.resume_integration()
            fdm.set_trim_status(False)

        map_inhg = fdm['propulsion/engine/map-inhg']
        bsfc = fdm['propulsion/engine/bsfc-lbs_hphr']
        self.assertAlmostEqual(bsfc, 0.3 + 0.005 * map_inhg, delta=1e-3)

    def test_steady_state_budget_is_independent_for_each_engine(self):
        # Engine 0 cannot settle in 6,000 calculations; it must not use up
        # engine 1's budget before engine 1 is calculated.
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       '737_cruise.xml')
        aircraft_tree, aircraft_name, _ = CopyAircraftDef(script_path,
                                                          self.sandbox)
        first_engine = aircraft_tree.getroot().find('propulsion/engine')
        del first_engine.attrib['file']
        slow_turbine = et.parse(self.sandbox.path_to_jsbsim_file(
            'engine', 'CFM56.xml')).getroot()
        spool_down = et.SubElement(slow_turbine, 'function',
                                   name='N2SpoolDown')
        et.SubElement(spool_down, 'value').text = '0.001'
        first_engine.insert(0, slow_turbine)
        aircraft_tree.write(self.sandbox('aircraft', aircraft_name,
                                         aircraft_name + '.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox('aircraft'))
        self.assertTrue(fdm.load_model(aircraft_name))
        self.assertTrue(fdm.load_ic('cruise_init', True))
        self.assertTrue(fdm.run_ic())
        propulsion = fdm.get_propulsion()
        propulsion.init_running(-1)
        fdm['fcs/throttle-cmd-norm[0]'] = 0.0
        fdm['fcs/throttle-cmd-norm[1]'] = 0.0
        fdm.run()
        self.assertGreater(fdm['propulsion/engine[1]/n2'], 90.0)

        fdm.set_trim_status(True)
        fdm.suspend_integration()
        try:
            propulsion.get_steady_state()
        finally:
            fdm.resume_integration()
            fdm.set_trim_status(False)

        self.assertGreater(fdm['propulsion/engine[0]/n2'], 60.0)
        self.assertAlmostEqual(fdm['propulsion/engine[1]/n2'], 60.0,
                               delta=0.01)


RunTest(CheckTrim)
