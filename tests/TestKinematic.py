# TestKinematic.py
#
# Regression tests of the <kinematic> system
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
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, CopyAircraftDef, ExecuteUntil


class TestKinematic(JSBSimTestCase):
    def testKinematicTiming(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('c172r')
        fdm.load_ic('reset00', True)
        fdm.run_ic()
        self.assertEqual(fdm['fcs/flap-cmd-norm'], 0.0)
        self.assertEqual(fdm['fcs/flap-pos-deg'], 0.0)

        # Test the flap down sequence. The flap command is set to a value
        # higher than 1.0 to check that JSBSim clamps it to 1.0
        fdm['fcs/flap-cmd-norm'] = 1.5
        t = fdm['simulation/sim-time-sec']

        while t < 2.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 5.*t)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t < 4.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 10.*(t-1.))
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t < 5.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 30.)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        # Test the flap up sequence with an interruption at 7.5 deg
        fdm['fcs/flap-cmd-norm'] = 0.25

        while t < 7.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 30.-10.*(t-5.))
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t < 7.5:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 10.-5.*(t-7.))
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t < 8.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 7.5)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        # Complete the flap up sequence. The flap command is set to a value
        # lower than 0.0 to check that JSBSim clamps it to 0.0
        fdm['fcs/flap-cmd-norm'] = -1.

        while t < 9.5:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 10.-5.*(t-7.5))
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t < 10.0:
            self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 0.0)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

    def testKinematicAndTrim(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('p51d')
        fdm.load_ic('reset01', True)
        self.assertEqual(fdm['gear/gear-cmd-norm'], 1.0)
        # Set the landing gears up. Since the command is equal to 1.0, the
        # <kinematic> system will trigger the gear down sequence.
        fdm['gear/gear-pos-norm'] = 0.0
        fdm.run_ic()

        # The test succeeds if the trim does not raise an exception i.e. if the
        # <kinematic> system does not interfer with the trim on ground
        # algorithm.
        fdm['simulation/do_simple_trim'] = 2  # Ground trim

        # Check that the gear is down after the trim as requested by
        # gear/gear-cmd-norm
        self.assertAlmostEqual(fdm['gear/gear-pos-norm'], 1.0)

        # Check that the gear is not moving to another position after trim.
        fdm.run()
        self.assertAlmostEqual(fdm['gear/gear-pos-norm'], 1.0)

    def testKinematicSetInitialValue(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('p51d')
        fdm.load_ic('reset01', True)
        fdm.run_ic()

        fdm['gear/gear-cmd-norm'] = 0.5
        fdm['gear/gear-pos-norm'] = 0.5

        while fdm['simulation/sim-time-sec'] < 1.0:
            fdm.run()
            self.assertAlmostEqual(fdm['gear/gear-cmd-norm'], 0.5)
            self.assertAlmostEqual(fdm['gear/gear-pos-norm'], 0.5)

    def testKinematicNoScale(self):
        # Test the <nocale/> feature
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1721.xml')
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        kinematic_tag = tree.getroot().find('flight_control/channel/kinematic')
        et.SubElement(kinematic_tag, 'noscale')
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_model(aircraft_name)

        fdm.load_ic('reset00', True)
        fdm.run_ic()
        fdm['fcs/flap-cmd-norm'] = 12.
        ExecuteUntil(fdm, 2.2)
        self.assertAlmostEqual(fdm['fcs/flap-pos-deg'], 12.)

RunTest(TestKinematic)
