# TestFunctions.py
#
# Test that the <fcs_function> component is functional.
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

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel


class TestFunctions(JSBSimTestCase):
    def test_functions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('function.xml')
        fdm = tripod.start()

        self.assertAlmostEqual(fdm['test/sum-values'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-value-property'], 1.0)
        self.assertAlmostEqual(fdm['test/product-values'], 2.0*math.pi)
        self.assertAlmostEqual(fdm['test/product-value-property'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)

        fdm['test/reference'] = 1.5
        fdm.run()

        self.assertAlmostEqual(fdm['test/sum-values'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-value-property'], 2.5)
        self.assertAlmostEqual(fdm['test/product-values'], 2.0*math.pi)
        self.assertAlmostEqual(fdm['test/product-value-property'], 2.25)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)

        fdm['test/input'] = -1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(-1.0))

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)

        fdm['test/input'] = 0.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -0.5)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(0.5))

        fdm['test/input'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(1.0))

        fdm['test/input'] = 1.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(1.5))

        fdm['test/input'] = 3.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(3.0))

        fdm['test/input'] = 3.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(3.5))

        fdm['test/input'] = 4.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(4.0))

        fdm['test/input'] = 5.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(5.0))

    def test_rotations(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('function.xml')
        fdm = tripod.start()

        for a in range(36):
            alpha = (a-17)*10
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/alpha'] = alpha
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/alpha_local'], alpha)

        fdm['test/alpha'] = 0.

        for b in range(17):
            beta = (b-8)*10
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/beta'] = beta
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/beta_local'], beta)

        fdm['test/alpha'] = 10.
        fdm['test/beta'] = 0.
        fdm['test/theta'] = -10.
        for c in range(36):
            gamma = (c-17)*10
            fdm['test/gamma'] = gamma
            fdm.run()
            self.assertAlmostEqual(fdm['test/alpha_local'], 0.0)
            self.assertAlmostEqual(fdm['test/beta_local'], 0.0)

        fdm['test/alpha'] = 0.
        fdm['test/beta'] = 10.
        fdm['test/theta'] = 0.
        fdm['test/psi'] = 10.
        for c in range(36):
            gamma = (c-17)*10
            fdm['test/gamma'] = gamma
            fdm.run()
            self.assertAlmostEqual(fdm['test/alpha_local'], 0.0)
            self.assertAlmostEqual(fdm['test/beta_local'], 0.0)

        alpha = 10.
        fdm['test/alpha'] = alpha
        fdm['test/beta'] =  5.
        fdm['test/phi'] = 0.
        fdm['test/psi'] = 0.
        for t in range(18):
            theta = (t-8)*10
            fdm['test/theta'] = theta
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/alpha_local'], alpha+theta)
                self.assertAlmostEqual(fdm['test/beta_local'], 5.0)


RunTest(TestFunctions)
