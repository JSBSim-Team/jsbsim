# TestSensorRandomSeed.py
#
# Test that a sensor with noise and a sensor specific random seed produces the
# exact same output over multiple runs even as the FGFDMExec random seed changes.
#
# Copyright (c) 2025 Sean McLeod
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
from JSBSim_utils import JSBSimTestCase, CopyAircraftDef, RunTest


class TestSensorRandomSeed(JSBSimTestCase):
    def AddSensorToAircraft(self, script_path):
        tree, aircraft_name, _ = CopyAircraftDef(script_path, self.sandbox)
        system_tag = et.SubElement(tree.getroot(), 'system')
        system_tag.attrib['file'] = 'sensorrandomseed'
        tree.write(self.sandbox('aircraft', aircraft_name, aircraft_name+'.xml'))

    def captureSensorData(self, exec_seed):
        script_name = '737_cruise.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.AddSensorToAircraft(script_path)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.set_systems_path(self.currentdir)
        fdm.load_script(script_path)

        fdm.run_ic()
        fdm['simulation/randomseed'] = exec_seed

        sensor_data = []

        while fdm.run():
            sensor_data.append(fdm['aero/sensor/qbar'])

        return sensor_data

    def testSensorSeed(self):
        # Run the test with different random seeds for the FGFDMExec
        # but a fixed random seed for the sensor and confirm that the sensor
        # data is the same.

        # Get the sensor data with some random seed for the FGFDMExec
        sensor_data1 = self.captureSensorData(3)
        # Get the sensor data with a different random seed for the FGFDMExec
        sensor_data2 = self.captureSensorData(4)
        # Check that the two sets of sensor data are equal
        for i in range(len(sensor_data1)):
            self.assertAlmostEqual(sensor_data1[i], sensor_data2[i], delta=1E-8)


RunTest(TestSensorRandomSeed)
