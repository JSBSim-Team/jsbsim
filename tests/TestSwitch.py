# TestSwitch.py
#
# Test that conditions in <switch> are functional.
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


class TestSwitch(JSBSimTestCase):
    def test_conditions(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('switch.xml')
        fdm = tripod.start()

        fdm['test/input'] = -1.5
        fdm['test/reference'] = 0.2
        fdm.run()
        self.assertEqual(fdm['test/sign1'], -1.0)
        self.assertEqual(fdm['test/sign2'], 1.0)
        self.assertEqual(fdm['test/compare'], -1.0)
        self.assertEqual(fdm['test/interval'], 0.0)
        self.assertEqual(fdm['test/group'], -1.0)

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertEqual(fdm['test/sign1'], 0.2)
        self.assertEqual(fdm['test/sign2'], -0.2)
        self.assertEqual(fdm['test/compare'], -1.0)
        self.assertEqual(fdm['test/interval'], 0.0)
        self.assertEqual(fdm['test/group'], -1.0)

        fdm['test/input'] = 0.1
        fdm.run()
        self.assertEqual(fdm['test/sign1'], 1.0)
        self.assertEqual(fdm['test/sign2'], -1.0)
        self.assertEqual(fdm['test/compare'], -1.0)
        self.assertEqual(fdm['test/interval'], 1.0)
        self.assertEqual(fdm['test/group'], 0.56)

        fdm['test/input'] = 0.2
        fdm.run()
        self.assertEqual(fdm['test/sign1'], 1.0)
        self.assertEqual(fdm['test/sign2'], -1.0)
        self.assertEqual(fdm['test/compare'], 1.0)
        self.assertEqual(fdm['test/interval'], 2.0)
        self.assertEqual(fdm['test/group'], -1.0)

        fdm['test/input'] = 0.235
        fdm.run()
        self.assertEqual(fdm['test/sign1'], 1.0)
        self.assertEqual(fdm['test/sign2'], -1.0)
        self.assertEqual(fdm['test/compare'], 1.0)
        self.assertEqual(fdm['test/interval'], 2.0)
        self.assertEqual(fdm['test/group'], 0.56)

        fdm['test/input'] = -1.5
        fdm['test/reference'] = -0.5
        fdm.run()
        self.assertEqual(fdm['test/compare'], -1.0)
        self.assertEqual(fdm['test/group'], -1.0)

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertEqual(fdm['test/compare'], 1.0)
        self.assertEqual(fdm['test/group'], -1.0)

        fdm['test/input'] = 0.2
        fdm.run()
        self.assertEqual(fdm['test/compare'], 1.0)
        self.assertEqual(fdm['test/group'], 0.56)

        fdm['test/input'] = 0.235
        fdm.run()
        self.assertEqual(fdm['test/compare'], 1.0)
        self.assertEqual(fdm['test/group'], 0.56)

    # Regression test to reproduce GitHub issue #176
    def test_nested(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('switch.xml')
        fdm = tripod.start()

        fdm['test/reference'] = 150
        fdm['test/input'] = 30
        fdm.run()
        self.assertEqual(fdm['test/nested'], 0)

        fdm['test/reference'] = 180
        fdm['test/input'] = 30
        fdm.run()
        self.assertEqual(fdm['test/nested'], 25)

        fdm['test/reference'] = 180
        fdm['test/input'] = 25
        fdm.run()
        self.assertEqual(fdm['test/nested'], 0)

        fdm['test/reference'] = 210
        fdm['test/input'] = 25
        fdm.run()
        self.assertEqual(fdm['test/nested'], 20)

        fdm['test/reference'] = 210
        fdm['test/input'] = 30
        fdm.run()
        self.assertEqual(fdm['test/nested'], 20)

        fdm['test/reference'] = 210
        fdm['test/input'] = 15
        fdm.run()
        self.assertEqual(fdm['test/nested'], 0)


RunTest(TestSwitch)
