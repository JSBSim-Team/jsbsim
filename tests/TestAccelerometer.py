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

import math, os
import xml.etree.ElementTree as et
import numpy as np
from JSBSim_utils import JSBSimTestCase, CopyAircraftDef, ExecuteUntil, RunTest


class TestAccelerometer(JSBSimTestCase):
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
        tree = et.parse(script_path)
        run_tag = tree.getroot().find('./run')
        run_tag.attrib['dt'] = '0.1'
        tree.write(script_name)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_name)
        # Switch the accel on
        fdm['fcs/accelerometer/on'] = 1.0
        fdm.run_ic()

        while fdm.run():
            self.assertAlmostEqual(fdm['fcs/accelerometer/X'], 0.0, delta=1E-8)
            self.assertAlmostEqual(fdm['fcs/accelerometer/Y'], 0.0, delta=1E-8)
            self.assertAlmostEqual(fdm['fcs/accelerometer/Z'], 0.0, delta=1E-8)
            self.assertAlmostEqual(fdm['accelerations/a-pilot-x-ft_sec2'], 0.0,
                                   delta=1E-8)
            self.assertAlmostEqual(fdm['accelerations/a-pilot-y-ft_sec2'], 0.0,
                                   delta=1E-8)
            self.assertAlmostEqual(fdm['accelerations/a-pilot-z-ft_sec2'], 0.0,
                                   delta=1E-8)

    def testOnGround(self):
        script_name = 'c1721.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)

        # Switch the accel on
        fdm['fcs/accelerometer/on'] = 1.0
        # Use the standard gravity (i.e. GM/r^2)
        fdm['simulation/gravity-model'] = 0
        # Simplifies the transformation to compare the accelerometer with the
        # gravity
        fdm['ic/psi-true-rad'] = 0.0
        fdm.run_ic()

        for i in range(1000):
            fdm.run()

        r = fdm['position/radius-to-vehicle-ft']
        g = fdm['accelerations/gravity-ft_sec2']
        latitude = fdm['position/lat-gc-rad']
        pitch = fdm['attitude/theta-rad']
        omega = 0.00007292115  # Earth rotation rate in rad/sec
        fc = r * math.cos(latitude) * omega * omega

        fax = fc * math.sin(latitude - pitch) + g * math.sin(pitch)
        faz = fc * math.cos(latitude - pitch) - g * math.cos(pitch)

        self.assertAlmostEqual(fdm['fcs/accelerometer/X'], fax, delta=1E-7)
        self.assertAlmostEqual(fdm['fcs/accelerometer/Y'], 0.0, delta=1E-7)
        self.assertAlmostEqual(fdm['fcs/accelerometer/Z']/faz, 1.0, delta=1E-8)

    def testSteadyFlight(self):
        script_name = 'c1722.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        # Switch the accel on
        fdm['fcs/accelerometer/on'] = 1.0
        # Use the standard gravity (i.e. GM/r^2)
        fdm['simulation/gravity-model'] = 0
        # Select an orientation such that frame transformations simplify
        fdm['ic/psi-true-rad'] = 0.0
        fdm.run_ic()

        ExecuteUntil(fdm, 0.1)

        fdm['simulation/do_simple_trim'] = 1

        r = fdm['position/radius-to-vehicle-ft']
        pitch = fdm['attitude/theta-rad']
        roll = fdm['attitude/phi-rad']
        latitude = fdm['position/lat-gc-rad']
        g = fdm['accelerations/gravity-ft_sec2']
        omega = 0.00007292115  # Earth rotation rate in rad/sec
        fc = r * math.cos(latitude) * omega * omega  # Centrifugal force

        uvw = np.array(fdm.get_propagate().get_uvw().T)[0]
        Omega = omega * np.array([math.cos(pitch - latitude),
                                  math.sin(pitch - latitude) * math.sin(roll),
                                  math.sin(pitch - latitude) * math.cos(roll)])

        # Compute the acceleration measured by the accelerometer as the sum of
        # the gravity and the centrifugal and Coriolis forces.
        fa_yz = (fc * math.cos(latitude - pitch) - g * math.cos(pitch))
        fa = np.array([(fc * math.sin(latitude - pitch) + g * math.sin(pitch)),
                       fa_yz * math.sin(roll),
                       fa_yz * math.cos(roll)]) + np.cross(2.0*Omega, uvw)

        # After the trim we are close to the equilibrium but there remains a
        # small residual that we have to take the bias into account
        fax = fa[0] + fdm['accelerations/udot-ft_sec2']
        fay = fa[1] + fdm['accelerations/vdot-ft_sec2']
        faz = fa[2] + fdm['accelerations/wdot-ft_sec2']

        # Deltas are relaxed because the tolerances of the trimming algorithm
        # are quite relaxed themselves.
        self.assertAlmostEqual(fdm['fcs/accelerometer/X'], fax, delta=1E-6)
        self.assertAlmostEqual(fdm['fcs/accelerometer/Y'], fay, delta=1E-5)
        self.assertAlmostEqual(fdm['fcs/accelerometer/Z'], faz, delta=1E-6)

    def testSpinningBodyOnOrbit(self):
        script_name = 'ball_orbit.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_model('ball')
        # Offset the CG along Y (by 30")
        fdm['inertia/pointmass-weight-lbs[1]'] = 50.0

        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft', 'ball')
        fdm.load_ic(os.path.join(aircraft_path, 'reset00.xml'), False)
        # Switch the accel on
        fdm['fcs/accelerometer/on'] = 1.0
        # Set the orientation such that the spinning axis is Z.
        fdm['ic/phi-rad'] = 0.5*math.pi

        # Set the angular velocities so that angular velocity R_{inertial} will
        # be equal to 1.0 rad/sec.
        omega = 0.00007292115  # Earth rotation rate in rad/sec
        fdm['ic/p-rad_sec'] = 0.0
        fdm['ic/q-rad_sec'] = 0.0
        fdm['ic/r-rad_sec'] = 1.0 - omega
        fdm.run_ic()

        fax = fdm['fcs/accelerometer/X']
        fay = fdm['fcs/accelerometer/Y']
        faz = fdm['fcs/accelerometer/Z']
        cgy_ft = fdm['inertia/cg-y-in'] / 12.

        self.assertAlmostEqual(fdm['accelerations/a-pilot-x-ft_sec2'], fax,
                               delta=1E-8)
        self.assertAlmostEqual(fdm['accelerations/a-pilot-y-ft_sec2'], fay,
                               delta=1E-8)
        self.assertAlmostEqual(fdm['accelerations/a-pilot-z-ft_sec2'], faz,
                               delta=1E-8)

        # Acceleration along X should be zero
        self.assertAlmostEqual(fax, 0.0, delta=1E-8)
        # Acceleration along Y should be equal to d*r_dot^2
        self.assertAlmostEqual(fay / cgy_ft, 1.0, delta=1E-8)
        # Acceleration along Z should be zero
        self.assertAlmostEqual(faz, 0.0, delta=1E-8)

    def testFailStuck(self):
        script_name = 'c1723.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddAccelerometersToAircraft(script_path)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)

        # Switch the accel on
        fdm['fcs/accelerometer/on'] = 1.0
        fdm.run_ic()

        stuck = False

        while fdm.run():
            t = fdm['simulation/sim-time-sec']
            if t > 30:
                if not stuck:
                    last_output = fdm['fcs/accelerometer/Z']
                    fdm['fcs/accelerometer_z/malfunction/fail_stuck'] = 1.0
                    stuck = True
                else:
                    self.assertAlmostEqual(fdm['fcs/accelerometer/Z'],
                                           last_output)

RunTest(TestAccelerometer)
