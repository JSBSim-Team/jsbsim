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

import string, numpy
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, CreateFDM, ExecuteUntil, Table, CopyAircraftDef, RunTest


class TestPointMassInertia(JSBSimTestCase):
    def test_point_mass_inertia(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'J2460.xml')
        fdm = CreateFDM(self.sandbox)
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        fdm.load_script(script_path)

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        ref = Table()
        ref.ReadCSV("output.csv")

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

        inertia = numpy.zeros((3, 3))
        if string.strip(shape) == 'tube':
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

        # Because JSBSim internals use static pointers, we cannot rely on
        # Python garbage collector to decide when the FDM is destroyed
        # otherwise we can get dangling pointers.
        del fdm

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        fdm.load_script(script_path)

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        mod = Table()
        mod.ReadCSV("output.csv")

        diff = ref.compare(mod)
        self.assertTrue(diff.empty(),
                        msg='\n'+repr(diff))

RunTest(TestPointMassInertia)
