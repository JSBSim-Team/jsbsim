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

        fdm['test/reference'] = 1.5
        fdm.run()

        self.assertAlmostEqual(fdm['test/sum-values'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-value-property'], 2.5)
        self.assertAlmostEqual(fdm['test/product-values'], 2.0*math.pi)
        self.assertAlmostEqual(fdm['test/product-value-property'], 2.25)

        fdm['test/input'] = -1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)

        fdm['test/input'] = 0.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], -0.5)

        fdm['test/input'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)

        fdm['test/input'] = 1.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)

        fdm['test/input'] = 3.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)

        fdm['test/input'] = 3.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 1.0)

        fdm['test/input'] = 4.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)

        fdm['test/input'] = 5.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)


RunTest(TestFunctions)
