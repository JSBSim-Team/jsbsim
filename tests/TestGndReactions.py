# TestGndReactions.py
#
# Checks that ground reactions are working
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

import math
import numpy as np
from JSBSim_utils import JSBSimTestCase, RunTest


class TestGndReactions(JSBSimTestCase):
    def test_centered_mass(self):
        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox.path_to_jsbsim_file('tests'))
        fdm.load_model('tripod', False)

        fdm['ic/h-sl-ft'] = 0.1
        # Let's go to the North pole to get rid of the centrifugal and Coriolis
        # accelerations.
        fdm['ic/lat-gc-deg'] = 90.0

        fdm.run_ic()
        fdm.do_trim(2)

        k = 10000.0 # Contact stiffness
        n = 3 # Number of contacts
        weight = fdm['forces/fbz-weight-lbs']
        f = 0.1 # friction coefficient

        self.assertAlmostEqual(fdm['forces/fbz-gear-lbs']/weight, -1.0, delta=1E-6)
        self.assertAlmostEqual(n*k*fdm['contact/unit[0]/compression-ft']/weight,
                               1.0, delta=1E-6)
        self.assertAlmostEqual(n*k*fdm['contact/unit[1]/compression-ft']/weight,
                               1.0, delta=1E-6)
        self.assertAlmostEqual(n*k*fdm['contact/unit[2]/compression-ft']/weight,
                               1.0, delta=1E-6)
        # Check that the overall friction forces are negligible
        self.assertAlmostEqual(abs(fdm['forces/fbx-gear-lbs'])/weight, 0.0, delta=1E-6)
        self.assertAlmostEqual(abs(fdm['forces/fby-gear-lbs'])/weight, 0.0)

        # Check that each gear friction force follow the Coulomb friction law.
        grndreact = fdm.get_ground_reactions()
        for i in range(grndreact.get_num_gear_units()):
            gear = grndreact.get_gear_unit(i)
            max_friction_force = f*abs(gear.get_body_z_force())
            self.assertTrue(abs(gear.get_body_x_force()) <= max_friction_force)
            self.assertTrue(abs(gear.get_body_y_force()) <= max_friction_force)

    def test_eccentric_mass(self):
        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox.path_to_jsbsim_file('tests'))
        fdm.load_model('tripod', False)

        fdm['ic/h-sl-ft'] = 0.1
        # Let's go to the North pole to get rid of the centrifugal and Coriolis
        # accelerations.
        fdm['ic/lat-gc-deg'] = 90.0
        fdm['inertia/pointmass-weight-lbs'] = 1000.

        fdm.run_ic()
        fdm.do_trim(2)
        fdm.run()

        k = 10000.0 # Contact stiffness
        theta = fdm['attitude/theta-rad']
        f = 0.1 # friction coefficient
        ft_to_inch = 12

        f_weight = np.array([[fdm['forces/fbx-weight-lbs'],
                              fdm['forces/fby-weight-lbs'],
                              fdm['forces/fbz-weight-lbs']]])
        f_total_gear = np.array([[fdm['forces/fbx-gear-lbs'],
                                  fdm['forces/fby-gear-lbs'],
                                  fdm['forces/fbz-gear-lbs']]])
        Tb2l = np.matrix([[ math.cos(theta), 0.0, math.sin(theta)],
                          [             0.0, 1.0,             0.0],
                          [-math.sin(theta), 0.0, math.cos(theta)]])
        Ts2b = np.matrix([[-1.0, 0.0,  0.0],
                          [ 0.0, 1.0,  0.0],
                          [ 0.0, 0.0, -1.0]])
        f_total_gear = Tb2l*f_total_gear.T
        f_weight = Tb2l*f_weight.T
        # Weight extraction: easier than computing the gravity acceleration at
        # the North pole and multiplying by the mass.
        weight = f_weight[2,0]

        self.assertAlmostEqual(f_total_gear[2,0]/weight, -1.0, delta=1E-6)

        # Check that the overall friction forces are negligible
        self.assertAlmostEqual(f_total_gear[0,0]/weight, 0.0, delta=1E-6)
        self.assertAlmostEqual(f_total_gear[1,0]/weight, 0.0, delta=1E-6)

        grndreact = fdm.get_ground_reactions()
        CGx = fdm['inertia/cg-x-in'] * math.cos(theta)
        FN_total = 0.0
        My_total = 0.0
        Mz_total = 0.0

        for i in range(grndreact.get_num_gear_units()):
            gear = grndreact.get_gear_unit(i)
            f_gear = Tb2l*np.array([[gear.get_body_x_force()],
                                    [gear.get_body_y_force()],
                                    [gear.get_body_z_force()]])
            FN = f_gear[2,0]
            # Check that the friction forces follow the Coulomb friction law.
            max_friction_force = f*abs(FN)
            self.assertTrue(abs(f_gear[0,0]) <= max_friction_force)
            self.assertTrue(abs(f_gear[1,0]) <= max_friction_force)

            h = fdm['contact/unit[{}]/compression-ft'.format(i)]

            # Check that the gear contact point has been moved by h*cos(theta)
            self.assertAlmostEqual((gear.get_acting_location()-gear.get_location())[2,0],
                                   h*ft_to_inch*math.cos(theta))
            FN_total += FN

            # Check that the gear reaction force is only due to the spring force
            # (i.e. no damping force)
            self.assertAlmostEqual(FN/(k*h), -1.0, delta=1E-6)

            d = (Tb2l*Ts2b*gear.get_location())[0,0] + CGx
            acting_location = Tb2l*Ts2b*gear.get_acting_location()
            acting_location[0,0] += CGx

            Mz = f_gear[0,0]*acting_location[1,0] - f_gear[1,0]*acting_location[0,0]
            Mz_total += Mz

            # Check that the x position of the gear in the local frame is not
            # modified by the compression of the leg (i.e. that it has been
            # moved upward/vertically)
            self.assertAlmostEqual(acting_location[0,0]/d, 1.0)
            My = FN*d
            My_total += My

        # Check that the sum of the individual gear normal forces is again
        # equal to the weight...
        self.assertAlmostEqual(FN_total/weight, -1.0, delta=1E-6)
        # ...and that their resulting moment is zero.
        self.assertAlmostEqual(My_total/My, 0.0, delta=1E-6)
        self.assertAlmostEqual(Mz_total/Mz, 0.0, delta=1E-6)


RunTest(TestGndReactions)
