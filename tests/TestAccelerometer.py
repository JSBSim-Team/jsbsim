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

import sys, unittest, math, os
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
        # Switch the accel on
        fdm.set_property_value('fcs/accelerometer/on', 1.0)
        fdm.run_ic()

        while fdm.run():
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/X'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Y'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('fcs/accelerometer/Z'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-x-ft_sec2'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-y-ft_sec2'),
                                   0.0, delta=1E-8)
            self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-z-ft_sec2'),
                                   0.0, delta=1E-8)

        del fdm

    def testOnGround(self):
        script_name = 'c1721.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)

        # Switch the accel on
        fdm.set_property_value('fcs/accelerometer/on', 1.0)
        # Use the standard gravity (i.e. GM/r^2)
        fdm.set_property_value('simulation/gravity-model', 0)
        # Simplifies the transformation to compare the accelerometer with the
        # gravity
        fdm.set_property_value('ic/psi-true-rad', 0.0)
        fdm.run_ic()

        for i in xrange(500):
            fdm.run()

        ax = fdm.get_property_value('accelerations/udot-ft_sec2')
        ay = fdm.get_property_value('accelerations/vdot-ft_sec2')
        az = fdm.get_property_value('accelerations/wdot-ft_sec2')
        g = fdm.get_property_value('accelerations/gravity-ft_sec2')
        theta = fdm.get_property_value('attitude/theta-rad')

        # There is a lag of one time step between the computations of the
        # accelerations and the update of the accelerometer
        fdm.run()
        fax = fdm.get_property_value('fcs/accelerometer/X')
        fay = fdm.get_property_value('fcs/accelerometer/Y')
        faz = fdm.get_property_value('fcs/accelerometer/Z')

        fax -= ax
        faz -= az

        self.assertAlmostEqual(fay, 0.0, delta=1E-6)
        self.assertAlmostEqual(fax / (g * math.sin(theta)), 1.0, delta=1E-5)
        self.assertAlmostEqual(faz / (g * math.cos(theta)), -1.0, delta=1E-7)

        del fdm

    def testSteadyFlight(self):
        script_name = 'c1722.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        # Switch the accel on
        fdm.set_property_value('fcs/accelerometer/on', 1.0)
        # Use the standard gravity (i.e. GM/r^2)
        fdm.set_property_value('simulation/gravity-model', 0)
        # Simplifies the transformation to compare the accelerometer with the
        # gravity
        fdm.set_property_value('ic/psi-true-rad', 0.0)
        fdm.run_ic()

        while fdm.get_property_value('simulation/sim-time-sec') <= 0.5:
            fdm.run()

        fdm.set_property_value('simulation/do_simple_trim', 1)
        ax = fdm.get_property_value('accelerations/udot-ft_sec2')
        ay = fdm.get_property_value('accelerations/vdot-ft_sec2')
        az = fdm.get_property_value('accelerations/wdot-ft_sec2')
        g = fdm.get_property_value('accelerations/gravity-ft_sec2')
        theta = fdm.get_property_value('attitude/theta-rad')

        # There is a lag of one time step between the computations of the
        # accelerations and the update of the accelerometer
        fdm.run()
        fax = fdm.get_property_value('fcs/accelerometer/X')
        fay = fdm.get_property_value('fcs/accelerometer/Y')
        faz = fdm.get_property_value('fcs/accelerometer/Z')

        fax -= ax
        fay -= ay
        faz -= az

        # Deltas are relaxed because the tolerances of the trimming algorithm
        # are quite relaxed themselves.
        self.assertAlmostEqual(faz / (g * math.cos(theta)), -1.0, delta=1E-5)
        self.assertAlmostEqual(fax / (g * math.sin(theta)), 1.0, delta=1E-5)
        self.assertAlmostEqual(math.sqrt(fax*fax+fay*fay+faz*faz)/g, 1.0, delta=1E-6)

        del fdm

    def testSpinningBodyOnOrbit(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_model('ball')
        # Offset the CG along Y (by 30")
        fdm.set_property_value('inertia/pointmass-weight-lbs[1]', 50.0)

        aircraft_path = self.sandbox.elude(self.sandbox.path_to_jsbsim_file('aircraft', 'ball'))
        fdm.load_ic(os.path.join(aircraft_path, 'reset00.xml'), False)
        # Switch the accel on
        fdm.set_property_value('fcs/accelerometer/on', 1.0)
        # Set the orientation such that the spinning axis is Z.
        fdm.set_property_value('ic/phi-rad', 0.5*math.pi)

        # Set the angular velocities to 0.0 in the ECEF frame. The angular
        # velocity R_{inertial} will therefore be equal to the Earth rotation
        # rate 7.292115E-5 rad/sec
        fdm.set_property_value('ic/p-rad_sec', 0.0)
        fdm.set_property_value('ic/q-rad_sec', 0.0)
        fdm.set_property_value('ic/r-rad_sec', 0.0)
        fdm.run_ic()

        fax = fdm.get_property_value('fcs/accelerometer/X')
        fay = fdm.get_property_value('fcs/accelerometer/Y')
        faz = fdm.get_property_value('fcs/accelerometer/Z')
        cgy_ft = fdm.get_property_value('inertia/cg-y-in') / 12.
        omega = 0.00007292115 # Earth rotation rate in rad/sec

        self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-x-ft_sec2'),
                               fax, delta=1E-8)
        self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-y-ft_sec2'),
                               fay, delta=1E-8)
        self.assertAlmostEqual(fdm.get_property_value('accelerations/a-pilot-z-ft_sec2'),
                               faz, delta=1E-8)

        # Acceleration along X should be zero
        self.assertAlmostEqual(fax, 0.0, delta=1E-8)
        # Acceleration along Y should be equal to r*omega^2
        self.assertAlmostEqual(fay / (cgy_ft * omega * omega), 1.0, delta=1E-7)
        # Acceleration along Z should be zero
        self.assertAlmostEqual(faz, 0.0, delta=1E-8)

suite = unittest.TestLoader().loadTestsFromTestCase(TestAccelerometer)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
