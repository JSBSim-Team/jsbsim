# TestChannelRate.py
#
# Test the channel rate feature introduced by Richard Harrison.
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

import os
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest
from jsbsim import TrimFailureError


class TestChannelRate(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self, 'check_cases', 'ground_tests')

    def testChannelRate(self):
        os.environ['JSBSIM_DEBUG'] = str(0)
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts',
                                                         'systems-rate-test-0.xml'))
        fdm.run_ic()

        while fdm['simulation/sim-time-sec'] < 30:
            fdm.run()
            self.assertEqual(fdm['simulation/frame'], fdm['tests/rate-1'])
            self.assertEqual(int(fdm['simulation/frame']/4),
                             fdm['tests/rate-4'])
            self.assertEqual(fdm['simulation/sim-time-sec'],
                             fdm['tests/rate-1-dt-sum'])
            self.assertAlmostEqual(fdm['simulation/dt']*fdm['tests/rate-4']*4,
                                   fdm['tests/rate-4-dt-sum'])

        self.assertEqual(fdm['simulation/dt'], fdm['tests/rate-1-dt'])
        self.assertEqual(fdm['simulation/dt']*4, fdm['tests/rate-4-dt'])

        # Trigger the trimming and check that it fails (i.e. it raises an
        # exception TrimFailureError)
        with self.assertRaises(TrimFailureError):
            fdm['simulation/do_simple_trim'] = 1

        while fdm['simulation/sim-time-sec'] < 40:
            self.assertEqual(fdm['simulation/frame'], fdm['tests/rate-1'])
            self.assertEqual(int(fdm['simulation/frame']/4),
                             fdm['tests/rate-4'])
            self.assertEqual(fdm['simulation/sim-time-sec'],
                             fdm['tests/rate-1-dt-sum'])
            self.assertAlmostEqual(fdm['simulation/dt']*fdm['tests/rate-4']*4,
                                   fdm['tests/rate-4-dt-sum'])
            fdm.run()

        fdm.reset_to_initial_conditions(1)
        tf = fdm['tests/rate-1-dt-sum']
        xtraFrames = fdm['simulation/frame'] % 4

        while fdm['simulation/sim-time-sec'] < 30:
            fdm.run()
            self.assertEqual(fdm['simulation/frame'], fdm['tests/rate-1'])
            self.assertEqual(int((fdm['simulation/frame']-xtraFrames)/4),
                             fdm['tests/rate-4'])
            self.assertAlmostEqual(fdm['simulation/sim-time-sec'],
                                   fdm['tests/rate-1-dt-sum']-tf)
            self.assertAlmostEqual(fdm['simulation/dt']*fdm['tests/rate-4']*4,
                                   fdm['tests/rate-4-dt-sum'])

RunTest(TestChannelRate)
