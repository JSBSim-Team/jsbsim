# CheckFGBug1503.py
#
# A regression test for the bug reported in FG issue 1503
# http://code.google.com/p/flightgear-bugs/issues/detail?id=1503
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

# This test checks that:
# 1. JSBSim does not hang when the parameter 'sense' of actuator <rate_limit>
#    is used.
# 2. The property 'fcs/left-aileron-pos-rad' remains equal to 0.0 during the
#    execution of the script c1724.xml when <rate_limit> is read from a
#    property.
# 3. The actuator output value is correctly driven by rate_limit.

import os, time, string
import xml.etree.ElementTree as et
from multiprocessing import Process
from scipy import stats
from JSBSim_utils import JSBSimTestCase, CreateFDM, CopyAircraftDef, RunTest


class CheckFGBug1503(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1724.xml')

        # Since we will alter the aircraft definition file, we need make a copy
        # of it and of all the files it is refering to.
        self.tree, self.aircraft_name, self.path_to_jsbsim_aircrafts = CopyAircraftDef(self.script_path, self.sandbox)

    def ScriptExecution(self, fdm, time_limit=1E+9):
        fdm.load_script(self.script_path)
        fdm.run_ic()

        while fdm.run() and fdm.get_sim_time() < time_limit:
            aileron_pos = fdm['fcs/left-aileron-pos-rad']
            self.assertEqual(aileron_pos, 0.0,
                            msg="Failed running the script %s at time step %f\nProperty fcs/left-aileron-pos-rad is non-zero (%f)" % (self.script_path, fdm.get_sim_time(), aileron_pos))

    def CheckRateValue(self, fdm, output_prop, rate_value):
        aileron_course = []

        t0 = fdm.get_sim_time()
        while fdm.run() and fdm.get_sim_time() <= t0 + 1.0:
            aileron_course += [(fdm.get_sim_time(), fdm[output_prop])]

        # Thanks to a linear regression on the values, we can check that the
        # value is following a slope equal to the rate limit. The correlation
        # coefficient r_value is also checked to verify that the output is
        # evolving linearly.
        slope, intercept, r_value, p_value, std_err = stats.linregress(aileron_course)
        self.assertTrue(abs(slope - rate_value) < 1E-9 and abs(1.0 - abs(r_value)) < 1E-9,
                        msg="The actuator rate is not linear")

    def CheckRateLimit(self, input_prop, output_prop, incr_limit, decr_limit):
        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')

        self.ScriptExecution(fdm, 1.0)

        fdm[input_prop] = 1.0

        self.CheckRateValue(fdm, output_prop, incr_limit)

        fdm[input_prop] = 0.0
        self.CheckRateValue(fdm, output_prop, decr_limit)

        # Because JSBSim internals use static pointers, we cannot rely on
        # Python garbage collector to decide when the FDM is destroyed
        # otherwise we can get dangling pointers.
        del fdm

    def test_regression_bug_1503(self):
        # First, the execution time of the script c1724.xml is measured. It
        # will be used as a reference to check if JSBSim hangs or not.
        fdm = CreateFDM(self.sandbox)
        start_time = time.time()
        self.ScriptExecution(fdm)
        exec_time = time.time() - start_time
        del fdm

        # Now the copy of the aircraft definition file will be altered: the
        # <rate_limit> element is split in two: one with the 'decr' sense, the
        # other with 'incr' sense.
        actuator_element = self.tree.getroot().find('flight_control/channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        rate_element.attrib['sense'] = 'decr'
        new_rate_element = et.SubElement(actuator_element, 'rate_limit')
        new_rate_element.attrib['sense'] = 'incr'
        new_rate_element.text = str(float(rate_element.text) * 0.5)

        self.tree.write(self.sandbox('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))

        # Run the script with the modified aircraft
        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')

        # A new process is created that launches the script. We wait for 10
        # times the reference execution time for the script completion. Beyond
        # that time, if the process is not completed, it is terminated and the
        # test is failed.
        p = Process(target=self.ScriptExecution, args=(fdm,))
        p.start()
        p.join(exec_time * 10.0)  # Wait 10 times the reference time
        alive = p.is_alive()
        if alive:
            p.terminate()
        self.assertFalse(alive, msg="The script has hanged")

    def test_actuator_rate_from_property(self):
        # Second part of the test.
        # #######################
        #
        # The test is run again but this time, <rate_limit> will be read from a
        # property instead of being read from a value hard coded in the
        # aircraft definition file. It has been reported in the bug 1503 of
        # FlightGear that for such a configuration the <actuator> output is
        # constantly increasing even if the input is null. For this script the
        # <actuator> output is stored in the property
        # fcs/left-aileron-pos-rad. The function ScriptExecution will monitor
        # that property and if it changes then the test is failed.

        tree = et.parse(os.path.join(self.path_to_jsbsim_aircrafts, self.aircraft_name+'.xml'))
        actuator_element = tree.getroot().find('flight_control/channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        flight_control_element = tree.getroot().find('flight_control')
        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/rate-limit-value'
        property.attrib['value'] = rate_element.text
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        rate_element.attrib['sense'] = 'decr'
        rate_element.text = property.text
        new_rate_element = et.SubElement(actuator_element, 'rate_limit')
        new_rate_element.attrib['sense'] = 'incr'
        new_rate_element.text = rate_element.text

        tree.write(self.sandbox('aircraft', self.aircraft_name, self.aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        self.ScriptExecution(fdm)
        del fdm

    def test_actuator_rate_is_linear(self):
        # Third part of the test.
        ########################
        #
        # The test is run again but this time we are checking that rate_limit
        # drives the actuator output value as expected. The idea is to store
        # the output value of the actuator output vs the time and check with a
        # linear regression that
        # 1. The actuator output value is evolving linearly
        # 2. The slope of the actuator output is equal to the rate_limit value
        # The test is run with the rate_limit given by a value, a property,
        # different values of the ascending and descending rates and a number
        # of combinations thereof.

        # The aircraft file definition is modified such that the actuator
        # element input is driven by a unique property. The name of this unique
        # property is built in the variable 'input_prop' below. When setting
        # that property to 1.0 (resp. -1.0) the ascending (resp. descending)
        # rate is triggered.
        tree = et.parse(os.path.join(self.path_to_jsbsim_aircrafts,
                                     self.aircraft_name+'.xml'))
        flight_control_element = tree.getroot().find('flight_control')
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')

        # Remove the hysteresis. We want to make sure we are measuring the
        # rate_limit and just that.
        hysteresis_element = actuator_element.find('hysteresis')
        actuator_element.remove(hysteresis_element)
        input_element = actuator_element.find('input')
        input_prop = actuator_element.attrib['name'].split('-')
        input_prop[-1] = 'input'
        input_prop = '-'.join(input_prop)
        input_element.text = input_prop
        output_element = actuator_element.find('output')
        output_prop = output_element.text.strip()

        # Add the new properties to <flight_control> so that we can make
        # reference to them without JSBSim complaining
        property = et.SubElement(flight_control_element, 'property')
        property.text = input_prop
        property.attrib['value'] = '0.0'
        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/rate-limit-value'
        property.attrib['value'] = '0.15'
        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/rate-limit-value2'
        property.attrib['value'] = '0.05'

        # First check with rate_limit set to 0.1

        rate_element = actuator_element.find('rate_limit')
        rate_element.text = '0.1'

        output_file = os.path.join('aircraft', self.aircraft_name,
                                   self.aircraft_name+'.xml')
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.1, -0.1)

        # Check when rate_limit is set by the property 'fcs/rate-limit-value'

        tree = et.parse(output_file)
        flight_control_element = tree.getroot().find('flight_control')
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.15, -0.15)

        # Checking when the ascending and descending rates are different.
        # First with the 2 rates set by hard coded values (0.1 and 0.2
        # respectively)

        rate_element.attrib['sense'] = 'decr'
        rate_element.text = '0.1'
        new_rate_element = et.SubElement(actuator_element, 'rate_limit')
        new_rate_element.attrib['sense'] = 'incr'
        new_rate_element.text = '0.2'
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.2, -0.1)

        # Check when the descending rate is set by a property and the ascending
        # rate is set by a value.

        rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.2, -0.15)

        # Check when the ascending rate is set by a property and the descending
        # rate is set by a value.

        rate_element.text = '0.1'
        new_rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.15, -0.1)

        # Check when the ascending and descending rates are set by properties

        rate_element.text = 'fcs/rate-limit-value2'
        tree.write(output_file)

        self.CheckRateLimit(input_prop, output_prop, 0.15, -0.05)

RunTest(CheckFGBug1503)
