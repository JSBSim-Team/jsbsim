# TestAccelerometer.py
#
# A regression test that checks that accelerometers are measuring 0g in orbit
# and 1g in steady flight or on ground.
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

import sys, unittest, math
import xml.etree.ElementTree as et
from JSBSim_utils import CreateFDM, SandBox, CopyAircraftDef


class TestAccelerometer(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()

    def tearDown(self):
        self.sandbox.erase()

    def AddAccelerometersToAircraft(self, script_path):
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        system_tag = et.SubElement(tree.getroot(), 'system')
        system_tag.attrib['file'] = 'accelerometers'
        tree.write(self.sandbox('aircraft', aircraft_name, aircraft_name+'.xml'))
        
    def testOrbit(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        # The time step is too small in ball_orbit so let's increase it to 0.1s
        # for a quicker run
        tree = et.parse(self.sandbox.elude(script_path))
        run_tag = tree.getroot().find('./run')
        run_tag.attrib['dt'] = '0.1'
        tree.write(self.sandbox(script_name))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_name)
        fdm.set_property_value('fcs/accelerometer/on', 1.0)  # Switch the accel on
        fdm.run_ic()

        while fdm.run():
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/X'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Y'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Z'),
                                   0.0, delta=1E-8)

        del fdm

    def testOnGround(self):
        script_name = 'c1721.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.set_property_value('fcs/accelerometer/on', 1.0)  # Switch the accel on
        fdm.set_property_value('ic/psi-true-rad', 0.0)
        fdm.set_property_value('ic/lat-gc-deg', 0.0)
        fdm.run_ic()

        for i in xrange(200):
            fdm.run()

        g = fdm.get_property_value('accelerations/gravity-ft_sec2')
        theta = fdm.get_property_value('attitude/theta-rad')

        self.assertAlmostEqual(fdm.get_property_value('attitude/phi-rad'), 0.0,
                               delta=1E-8)
        self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Y'),
                               0.0, delta=1E-6)
        self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/X')
                               / (g* math.sin(theta)), 1.0, delta=6E-3)
        self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Z')
                               / (g* math.cos(theta)), -1.0, delta=6E-3)

        del fdm

    def testSteadyFlight(self):
        script_name = 'c1722.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.set_property_value('fcs/accelerometer/on', 1.0)  # Switch the accel on
        fdm.run_ic()

        while fdm.get_property_value('simulation/sim-time-sec') <= 0.5:
            fdm.run()

        g = fdm.get_property_value('accelerations/gravity-ft_sec2')
        ax =fdm.get_property_value('fcs/accelerometer/X')
        ay = fdm.get_property_value('fcs/accelerometer/Y')
        az = fdm.get_property_value('fcs/accelerometer/Z')

        self.assertAlmostEqual(math.sqrt(ax*ax+ay*ay+az*az)/g, 1.0, delta=6E-3)

        del fdm


suite = unittest.TestLoader().loadTestsFromTestCase(TestAccelerometer)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
