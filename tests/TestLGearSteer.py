# TestLGearSteer.py
#
# Checks that landing gear steering is working
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
from JSBSim_utils import JSBSimTestCase, RunTest, CopyAircraftDef
import fpectl


class TestLGearSteer(JSBSimTestCase):
    def test_direct_steer(self):
        fdm = self.create_fdm()
        fdm.load_model('c172r')
        fdm.load_ic('reset00', True)
        fdm.run_ic()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 0.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 0.0)

        # Should be part of a unit test in C++ ?
        fpectl.turnon_sigfpe()

        grndreact = fdm.get_ground_reactions()
        for i in range(grndreact.get_num_gear_units()):
            gear = grndreact.get_gear_unit(i)
            self.assertEqual(gear.get_steer_norm(), 0.0)

        fpectl.turnoff_sigfpe()

        fdm['fcs/steer-pos-deg'] = 5.0
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 5.0)
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 0.0)
        fdm['fcs/steer-cmd-norm'] = 1.0
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 10.0)

    def test_steer_with_fcs(self):
        fdm = self.create_fdm()
        fdm.load_model('L410')
        fdm.load_ic('reset00', True)
        fdm.run_ic()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 0.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 0.0)
        fdm['fcs/steer-cmd-norm'] = 1.0
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 5.0)
        fdm['/controls/switches/full-steering-sw'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 0.0)
        fdm['/controls/switches/full-steering-sw'] = 2.0
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], 1.0)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 45.0)
        fdm['fcs/steer-cmd-norm'] = -0.5
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-cmd-norm'], -0.5)
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], -22.5)

    def steerType(self, hasSteerPosDeg, hasSteeringAngle, hasCastered):
        self.tree.write(self.sandbox('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()
        pm = fdm.get_property_manager()
        self.assertTrue(pm.hasNode('fcs/steer-pos-deg') == hasSteerPosDeg)
        self.assertTrue(pm.hasNode('gear/unit/steering-angle-deg')
                        == hasSteeringAngle)
        self.assertTrue(pm.hasNode('gear/unit/castered') == hasCastered)

        return fdm

    def isCastered(self):
        fdm = self.steerType(True, True, True)
        self.assertTrue(fdm['gear/unit/castered'])
        fdm['fcs/steer-cmd-norm'] = 0.5
        fdm.run()
        self.assertEqual(fdm['gear/unit/steering-angle-deg'], 0.0)
        # self.assertEqual(fdm['fcs/steer-pos-deg'],
        #                  fdm['gear/unit/steering-angle-deg'])

        fdm['fcs/steer-pos-deg'] = 20.0
        self.assertEqual(fdm['gear/unit/steering-angle-deg'], 0.0)
        # self.assertEqual(fdm['fcs/steer-pos-deg'],
        #                  fdm['gear/unit/steering-angle-deg'])

        fdm['gear/unit/castered'] = 0.0
        fdm['fcs/steer-cmd-norm'] = 1.0
        fdm.run()
        self.assertEqual(fdm['gear/unit/steering-angle-deg'],
                         float(self.max_steer_tag.text))
        # self.assertEqual(fdm['fcs/steer-pos-deg'],
        #                  fdm['gear/unit/steering-angle-deg'])

    def isSteered(self):
        fdm = self.steerType(True, False, False)
        fdm['fcs/steer-cmd-norm'] = 0.5
        fdm.run()
        self.assertEqual(fdm['fcs/steer-pos-deg'],
                         0.5*float(self.max_steer_tag.text))

    def test_steer_type(self):
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1721.xml')
        self.tree, self.aircraft_name, b = CopyAircraftDef(self.script_path,
                                                           self.sandbox)
        root = self.tree.getroot()
        self.max_steer_tag = root.find('ground_reactions/contact/max_steer')

        # Check the fixed type
        self.max_steer_tag.text = '0.0'
        fdm = self.steerType(False, False, False)

        # Check the castered type
        self.max_steer_tag.text = '360.0'
        self.isCastered()

        # Check the steered type
        self.max_steer_tag.text = '10.0'
        fdm = self.steerType(True, False, False)
        fdm['fcs/steer-cmd-norm'] = 0.5
        fdm.run()
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg'], 5.0)

        bogey_tag = root.find('ground_reactions/contact//max_steer/..')
        castered_tag = et.SubElement(bogey_tag, 'castered')
        castered_tag.text = '1.0'

        # Check that the bogey is castered no matter what is the value
        # of <max_steer>
        self.max_steer_tag.text = '10.0'
        self.isCastered()

        self.max_steer_tag.text = '0.0'
        self.isCastered()

        self.max_steer_tag.text = '360.0'
        self.isCastered()

        # Check the fixed type
        castered_tag.text = '0.0'
        self.max_steer_tag.text = '0.0'
        fdm = self.steerType(False, False, False)

        # Check the steered type
        self.max_steer_tag.text = '10.0'
        self.isSteered()

        # Check the steered type with 360.0
        self.max_steer_tag.text = '360.0'
        self.isSteered()

RunTest(TestLGearSteer)
