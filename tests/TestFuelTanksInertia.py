# TestFuelTanksInertia.py
#
# A regression test that checks that:
# * Fuel tanks inertias are updated when the tanks content is modified.
# * Fuel and oxidizer total quantities are correctly computed.
# * Fuel tanks inertias computation does not depend on the initial content but
#   rather on the tank capacity and current content.
#
# Copyright (c) 2015-2016 Bertrand Coconnier
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
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, ExecuteUntil, RunTest, CopyAircraftDef


class TestFuelTanksInertia(JSBSimTestCase):
    def test_fuel_tanks_inertia(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')

        # The aircraft c172x does not contain an <inertia_factor> tag so we
        # need to add one.
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        tank_tag = tree.getroot().find('propulsion/tank')
        inertia_factor = et.SubElement(tank_tag, 'inertia_factor')
        inertia_factor.text = '1.0'
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        contents0 = fdm['propulsion/tank/contents-lbs']
        ixx0 = fdm['propulsion/tank/local-ixx-slug_ft2']
        iyy0 = fdm['propulsion/tank/local-iyy-slug_ft2']
        izz0 = fdm['propulsion/tank/local-izz-slug_ft2']

        # Remove half of the tank contents and check that the inertias are
        # updated accordingly
        fdm['propulsion/tank/contents-lbs'] = 0.5*contents0
        contents = fdm['propulsion/tank/contents-lbs']
        ixx = fdm['propulsion/tank/local-ixx-slug_ft2']
        iyy = fdm['propulsion/tank/local-iyy-slug_ft2']
        izz = fdm['propulsion/tank/local-izz-slug_ft2']

        self.assertAlmostEqual(contents, 0.5*contents0, delta=1E-7,
                               msg="The tank content (%f lbs) should be %f lbs" % (contents, 0.5*contents0))
        self.assertAlmostEqual(ixx, 0.5*ixx0, delta=1E-7,
                               msg="The tank inertia Ixx (%f slug*ft^2) should be %f slug*ft^2" % (ixx, 0.5*ixx0))
        self.assertAlmostEqual(iyy, 0.5*iyy0, delta=1E-7,
                               msg="The tank inertia Iyy (%f slug*ft^2) should be %f slug*ft^2" % (iyy, 0.5*iyy0))
        self.assertAlmostEqual(izz, 0.5*izz0, delta=1E-7,
                               msg="The tank inertia Izz (%f slug*ft^2) should be %f slug*ft^2" % (izz, 0.5*izz0))

        # Execute the script and check that the fuel inertias have been updated
        # along with the consumption.
        ExecuteUntil(fdm, 200.0)

        contents = fdm['propulsion/tank/contents-lbs']
        ixx = fdm['propulsion/tank/local-ixx-slug_ft2']
        iyy = fdm['propulsion/tank/local-iyy-slug_ft2']
        izz = fdm['propulsion/tank/local-izz-slug_ft2']

        contents_ratio = contents / contents0
        ixx_ratio = ixx / ixx0
        iyy_ratio = iyy / iyy0
        izz_ratio = izz / izz0

        self.assertAlmostEqual(contents_ratio, ixx_ratio, delta=1E-7,
                               msg="Ixx does not vary as the tank content does\nIxx ratio=%f\nContents ratio=%f" % (ixx_ratio, contents_ratio))
        self.assertAlmostEqual(contents_ratio, iyy_ratio, delta=1E-7,
                               msg="Iyy does not vary as the tank content does\nIyy ratio=%f\nContents ratio=%f" % (iyy_ratio, contents_ratio))
        self.assertAlmostEqual(contents_ratio, izz_ratio, delta=1E-7,
                               msg="Izz does not vary as the tank content does\nIzz ratio=%f\nContents ratio=%f" % (izz_ratio, contents_ratio))

    def test_fuel_tanks_content(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'J2460.xml')
        fdm = self.create_fdm()
        fdm.load_script(script_path)
        fdm.run_ic()

        tree = et.parse(script_path)
        use_tag = tree.getroot().find('use')
        aircraft_name = use_tag.attrib['aircraft']
        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft',
                                                         aircraft_name)
        aircraft_tree = et.parse(os.path.join(aircraft_path,
                                              aircraft_name+'.xml'))

        total_fuel_quantity = 0.0
        total_oxidizer_quantity = 0.0
        for tank in aircraft_tree.findall('propulsion/tank'):
            contents = float(tank.find('contents').text)
            if tank.attrib['type'] == "FUEL":
                total_fuel_quantity += contents
            elif tank.attrib['type'] == 'OXIDIZER':
                total_oxidizer_quantity += contents

        self.assertAlmostEqual(fdm['propulsion/total-fuel-lbs'],
                               total_fuel_quantity)

        self.assertAlmostEqual(fdm['propulsion/total-oxidizer-lbs'],
                               total_oxidizer_quantity)

    def test_grain_tanks_content(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'J2460.xml')
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)

        id = 0
        for tank in tree.getroot().findall('propulsion/tank'):
            grain_config = tank.find('grain_config')
            if grain_config and grain_config.attrib['type'] == 'CYLINDRICAL':
                break
            ++id

        capacity = float(tank.find('capacity').text)
        tank.find('contents').text = str(0.5*capacity)
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        radius_tag = tank.find('radius')
        radius = float(radius_tag.text)
        if 'unit' in radius_tag.attrib and radius_tag.attrib['unit'] == 'IN':
            radius /= 12.0

        bore_diameter_tag = tank.find('grain_config/bore_diameter')
        bore_radius = 0.5*float(bore_diameter_tag.text)
        if 'unit' in bore_diameter_tag.attrib and bore_diameter_tag.attrib['unit'] == 'IN':
            bore_radius /= 12.0

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        tank_name = 'propulsion/tank[%g]' % (id,)

        self.assertAlmostEqual(fdm[tank_name+'/contents-lbs'], 0.5*capacity)
        fdm['propulsion/tank/contents-lbs'] = capacity
        mass = capacity / 32.174049  # Converting lbs to slugs
        ixx = 0.5 * mass * (radius * radius + bore_radius*bore_radius)
        self.assertAlmostEqual(fdm[tank_name+'local-ixx-slug_ft2'], ixx)

        tank.find('contents').text = '0.0'
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        self.assertAlmostEqual(fdm[tank_name+'/contents-lbs'], 0.0)
        fdm['propulsion/tank/contents-lbs'] = capacity

RunTest(TestFuelTanksInertia)
