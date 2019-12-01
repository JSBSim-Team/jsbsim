# TestPointMassInertia.py
#
# A test that checks that specifying explicitely point mass inertias works.
#
# Copyright (c) 2014 Bertrand Coconnier
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

import numpy as np
import pandas as pd
import xml.etree.ElementTree as et
from JSBSim_utils import (JSBSimTestCase, ExecuteUntil, CopyAircraftDef,
                          isDataMatching, FindDifferences, RunTest)


class TestPointMassInertia(JSBSimTestCase):
    def test_point_mass_inertia(self):
        fdm = self.create_fdm()
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        self.load_script('J2460')

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        ref = pd.read_csv("output.csv", index_col=0)

        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'J2460.xml')
        tree, aircraft_name, path_to_jsbsim_aircrafts = CopyAircraftDef(script_path, self.sandbox)

        pointmass_element = tree.getroot().find('mass_balance/pointmass//form/..')
        weight_element = pointmass_element.find('weight')
        weight = float(weight_element.text)
        form_element = pointmass_element.find('form')
        radius_element = form_element.find('radius')
        radius, length = (0.0, 0.0)
        if radius_element is not None:
            radius = float(radius_element.text)
        length_element = form_element.find('length')
        if length_element is not None:
            length = float(length_element.text)
        shape = form_element.attrib['shape']
        pointmass_element.remove(form_element)

        inertia = np.zeros((3, 3))
        if shape.strip() == 'tube':
            inertia[0, 0] = radius * radius
            inertia[1, 1] = (6.0 * inertia[0, 0] + length * length) / 12.0
            inertia[2, 2] = inertia[1, 1]

        inertia = inertia * weight / 32.174049  # converting slugs to lbs

        ixx_element = et.SubElement(pointmass_element, 'ixx')
        ixx_element.text = str(inertia[0, 0])
        iyy_element = et.SubElement(pointmass_element, 'iyy')
        iyy_element.text = str(inertia[1, 1])
        izz_element = et.SubElement(pointmass_element, 'izz')
        izz_element.text = str(inertia[2, 2])

        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        self.load_script('J2460')

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        mod = pd.read_csv("output.csv", index_col=0)

        # Check the data are matching i.e. the time steps are the same between
        # the two data sets and that the output data are also the same.
        self.assertTrue(isDataMatching(ref, mod))

        # Find all the data that are differing by more than 1E-8 between the
        # two data sets.
        diff = FindDifferences(ref, mod, 1E-8)
        self.longMessage = True
        self.assertEqual(len(diff), 0, msg='\n'+diff.to_string())

    def testInertiaMatrix(self):
        root = self.get_aircraft_xml_tree('f16_test').getroot()
        ixz_element = root.find('mass_balance/ixz')
        fdm = self.create_fdm()
        self.load_script('f16_test')
        fdm.run_ic()

        # Check that Jinv is indeed the inverse of J.
        mass_balance = fdm.get_mass_balance()
        J = mass_balance.get_J()
        Jinv = mass_balance.get_Jinv()
        identity = J*Jinv
        self.assertAlmostEqual(identity[0,0], 1.0)
        self.assertAlmostEqual(identity[0,1], 0.0)
        self.assertAlmostEqual(identity[0,2], 0.0)
        self.assertAlmostEqual(identity[1,0], 0.0)
        self.assertAlmostEqual(identity[1,1], 1.0)
        self.assertAlmostEqual(identity[1,2], 0.0)
        self.assertAlmostEqual(identity[2,0], 0.0)
        self.assertAlmostEqual(identity[2,1], 0.0)
        self.assertAlmostEqual(identity[2,2], 1.0)

        fdm['inertia/pointmass-weight-lbs'] = 0.0
        fdm['propulsion/tank[0]/contents-lbs'] = 0.0
        fdm['propulsion/tank[1]/contents-lbs'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['inertia/weight-lbs'],
                               fdm['inertia/empty-weight-lbs'])
        self.assertAlmostEqual(fdm['inertia/ixz-slugs_ft2'],
                               float(ixz_element.text))


RunTest(TestPointMassInertia)
