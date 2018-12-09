# TestDistributor.py
#
# Test that <distributor> is functional.
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

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel


class TestDistributor(JSBSimTestCase):
    def test_conditions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('distributor.xml')
        fdm = tripod.start()

        fdm['test/input'] = -1.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/result1'], 0.0)
        self.assertAlmostEqual(fdm['test/default'], 0.0)
        self.assertAlmostEqual(fdm['test/result2'], -1.0)
        self.assertAlmostEqual(fdm['test/reference'], 1.5)

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/result1'], 1.0)
        self.assertAlmostEqual(fdm['test/default'], 1.0)
        self.assertAlmostEqual(fdm['test/result2'], 0.0)
        self.assertAlmostEqual(fdm['test/reference'], 1.5)

        fdm['test/input'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/result1'], 1.0)
        self.assertAlmostEqual(fdm['test/default'], 1.0)
        self.assertAlmostEqual(fdm['test/result2'], 1.0)
        self.assertAlmostEqual(fdm['test/reference'], 1.5)

        fdm['test/input'] = -0.808
        fdm.run()
        self.assertAlmostEqual(fdm['test/result1'], 0.0)
        self.assertAlmostEqual(fdm['test/default'], 0.0)
        self.assertAlmostEqual(fdm['test/result2'], -1.0)
        self.assertAlmostEqual(fdm['test/reference'], 0.808)


RunTest(TestDistributor)
