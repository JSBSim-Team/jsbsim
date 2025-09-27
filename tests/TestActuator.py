# TestActuator.py
#
# Test the <actuator> flight control and regression test for the bug reported in
# FG issue 1503 (https://sourceforge.net/p/flightgear/codetickets/1503/)
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

import os, time, math
import xml.etree.ElementTree as et
from multiprocessing import Process
from scipy import stats
from JSBSim_utils import JSBSimTestCase, CreateFDM, CopyAircraftDef, RunTest

# This wrapper launcher is needed to handle limitations with the Windows version
# of the multiprocessing module since 'complex' objects can't be
# serialized/pickled
def SubProcessScriptExecution(sandbox, script_path):
    fdm = CreateFDM(sandbox)
    fdm.load_script(script_path)
    fdm.run_ic()

    while fdm.run():
        pass


class TestActuator(JSBSimTestCase):
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

    def CheckRateValue(self, fdm, rate_value):
        aileron_course = []

        t0 = fdm.get_sim_time()
        while fdm.run() and fdm.get_sim_time() <= t0 + 1.0:
            aileron_course += [(fdm.get_sim_time(), fdm[self.output_prop])]

        # Thanks to a linear regression on the values, we can check that the
        # value is following a slope equal to the rate limit. The correlation
        # coefficient r_value is also checked to verify that the output is
        # evolving linearly.
        slope, intercept, r_value, p_value, std_err = stats.linregress(aileron_course)
        self.assertTrue(abs(slope - rate_value) < 1E-9 and abs(1.0 - abs(r_value)) < 1E-9,
                        msg="The actuator rate is not linear")

    def CheckRateLimit(self, incr_limit, decr_limit):
        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')

        self.ScriptExecution(fdm, 1.0)

        fdm[self.input_prop] = 1.0
        self.CheckRateValue(fdm, incr_limit)

        fdm[self.input_prop] = 0.0
        self.CheckRateValue(fdm, decr_limit)

    def test_regression_bug_1503(self):
        # First, the execution time of the script c1724.xml is measured. It
        # will be used as a reference to check if JSBSim hangs or not.
        fdm = self.create_fdm()
        start_time = time.time()
        self.ScriptExecution(fdm)
        exec_time = time.time() - start_time

        # Delete the FDM instance to make sure that all files are closed and
        # released before running the same script in another process.
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

        self.tree.write(os.path.join('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))

        # A new process is created that launches the script. We wait for 50
        # times the reference execution time for the script completion. Beyond
        # that time, if the process is not completed, it is terminated and the
        # test is failed.
        p = Process(target=SubProcessScriptExecution,
                    args=(self.sandbox, self.script_path))
        p.start()
        p.join(exec_time * 50.0)  # Wait 50 times the reference time
        alive = p.is_alive()
        if alive:
            p.terminate()
        self.assertFalse(alive, msg="The script has hung")

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

        tree = et.parse(os.path.join(self.path_to_jsbsim_aircrafts,
                                     self.aircraft_name+'.xml'))
        root = tree.getroot()
        flight_control_element = root.find('flight_control')
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/rate-limit-value'
        property.attrib['value'] = rate_element.text
        rate_element.attrib['sense'] = 'decr'
        rate_element.text = property.text
        new_rate_element = et.SubElement(actuator_element, 'rate_limit')
        new_rate_element.attrib['sense'] = 'incr'
        new_rate_element.text = rate_element.text
        output_element = root.find('output')
        output_element.attrib['name'] = 'test.csv'

        tree.write(os.path.join('aircraft', self.aircraft_name,
                                self.aircraft_name+'.xml'))

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        self.ScriptExecution(fdm)

    def prepare_actuator(self):
        tree = et.parse(os.path.join(self.path_to_jsbsim_aircrafts,
                                     self.aircraft_name+'.xml'))
        flight_control_element = tree.getroot().find('flight_control')
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')

        # Remove the hysteresis. We want to make sure we are measuring the
        # rate_limit and just that.
        hysteresis_element = actuator_element.find('hysteresis_width')
        actuator_element.remove(hysteresis_element)
        input_element = actuator_element.find('input')
        self.input_prop = actuator_element.attrib['name'].split('-')
        self.input_prop[-1] = 'input'
        self.input_prop = '-'.join(self.input_prop)
        input_element.text = self.input_prop
        self.output_prop = actuator_element.find('output').text
        property = et.SubElement(flight_control_element, 'property')
        property.text = self.input_prop
        property.attrib['value'] = '0.0'

        return (tree, flight_control_element, actuator_element)

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
        tree, flight_control_element, actuator_element = self.prepare_actuator()

        # Add the new properties to <flight_control> so that we can make
        # reference to them without JSBSim complaining
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

        self.CheckRateLimit(0.1, -0.1)

        # Check when rate_limit is set by the property 'fcs/rate-limit-value'

        tree = et.parse(output_file)
        flight_control_element = tree.getroot().find('flight_control')
        actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')
        rate_element = actuator_element.find('rate_limit')
        rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(0.15, -0.15)

        # Checking when the ascending and descending rates are different.
        # First with the 2 rates set by hard coded values (0.1 and 0.2
        # respectively)

        rate_element.attrib['sense'] = 'decr'
        rate_element.text = '0.1'
        new_rate_element = et.SubElement(actuator_element, 'rate_limit')
        new_rate_element.attrib['sense'] = 'incr'
        new_rate_element.text = '0.2'
        tree.write(output_file)

        self.CheckRateLimit(0.2, -0.1)

        # Check when the descending rate is set by a property and the ascending
        # rate is set by a value.

        rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(0.2, -0.15)

        # Check when the ascending rate is set by a property and the descending
        # rate is set by a value.

        rate_element.text = '0.1'
        new_rate_element.text = 'fcs/rate-limit-value'
        tree.write(output_file)

        self.CheckRateLimit(0.15, -0.1)

        # Check when the ascending and descending rates are set by properties

        rate_element.text = 'fcs/rate-limit-value2'
        tree.write(output_file)

        self.CheckRateLimit(0.15, -0.05)

    def CheckClip(self, clipmin, clipmax, rate_limit):
        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        fdm[self.input_prop] = 2.0*clipmax
        dt = clipmax/rate_limit
        while fdm['simulation/sim-time-sec'] <= dt:
            self.assertFalse(fdm[self.saturated_prop])
            fdm.run()

        self.assertTrue(fdm[self.saturated_prop])
        self.assertAlmostEqual(fdm[self.output_prop], clipmax)

        # Check that the actuator output can't go beyond clipmax
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+dt:
            fdm.run()
            self.assertTrue(fdm[self.saturated_prop])
            self.assertAlmostEqual(fdm[self.output_prop], clipmax)

        fdm[self.input_prop] = 2.0*clipmin
        dt = (2.0*clipmax-clipmin)/rate_limit
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+dt:
            if fdm[self.output_prop] < clipmax:
                self.assertFalse(fdm[self.saturated_prop])
            else:
                self.assertTrue(fdm[self.saturated_prop])
            fdm.run()

        self.assertTrue(fdm[self.saturated_prop])
        self.assertAlmostEqual(fdm[self.output_prop], clipmin)

        # Check that the actuator output can't go below clipmin
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+dt:
            fdm.run()
            self.assertTrue(fdm[self.saturated_prop])
            self.assertAlmostEqual(fdm[self.output_prop], clipmin)

        fdm[self.input_prop] = 1E-6
        dt = (fdm[self.input_prop]-2.0*clipmin)/rate_limit
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+2.0*dt:
            if fdm[self.output_prop] > clipmin:
                self.assertFalse(fdm[self.saturated_prop])
            else:
                self.assertTrue(fdm[self.saturated_prop])
            fdm.run()

        self.assertAlmostEqual(fdm[self.output_prop], fdm[self.input_prop])

        fdm[self.fail_hardover] = 1.0
        dt = (clipmax-fdm[self.input_prop])/rate_limit
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+dt:
            if fdm[self.output_prop] < clipmax:
                self.assertFalse(fdm[self.saturated_prop])
            else:
                self.assertTrue(fdm[self.saturated_prop])
            fdm.run()

        self.assertTrue(fdm[self.saturated_prop])
        self.assertAlmostEqual(fdm[self.output_prop], clipmax)

        fdm[self.input_prop] = -1E-6
        dt = (clipmax-clipmin)/rate_limit
        t = fdm['simulation/sim-time-sec']
        while fdm['simulation/sim-time-sec'] <= t+dt:
            if fdm[self.output_prop] > clipmin and fdm[self.output_prop] < clipmax:
                self.assertFalse(fdm[self.saturated_prop])
            else:
                self.assertTrue(fdm[self.saturated_prop])
            fdm.run()

        self.assertTrue(fdm[self.saturated_prop])
        self.assertAlmostEqual(fdm[self.output_prop], clipmin)

    def test_clipto(self):
        tree, flight_control_element, actuator_element = self.prepare_actuator()
        rate_limit = float(actuator_element.find('rate_limit').text)
        self.saturated_prop = actuator_element.attrib['name']+"/saturated"
        self.fail_hardover = actuator_element.attrib['name']+"/malfunction/fail_hardover"
        clipto = actuator_element.find('clipto')
        clipmax = clipto.find('max')
        clipmin = clipto.find('min')
        output_file = os.path.join('aircraft', self.aircraft_name,
                                   self.aircraft_name+'.xml')
        tree.write(output_file)

        self.CheckClip(float(clipmin.text), float(clipmax.text), rate_limit)

        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/clip-min-value'
        property.attrib['value'] = '-0.15'
        property = et.SubElement(flight_control_element, 'property')
        property.text = 'fcs/clip-max-value'
        property.attrib['value'] = '0.05'

        # Check a property for min and a value for max
        clipmin.text = 'fcs/clip-min-value'
        tree.write(output_file)

        self.CheckClip(-0.15, float(clipmax.text), rate_limit)

        # Check a property with minus sign for min and a value for max
        clipmin.text = '-fcs/clip-max-value'
        tree.write(output_file)

        self.CheckClip(-0.05, float(clipmax.text), rate_limit)

        # Check a property for max and a value for min
        clipmin.text = '-0.1'
        clipmax.text = 'fcs/clip-max-value'
        tree.write(output_file)

        self.CheckClip(-0.1, 0.05, rate_limit)

        # Check a property with minus sign for max and a value for min
        clipmax.text = '-fcs/clip-min-value'
        tree.write(output_file)

        self.CheckClip(-0.1, 0.15, rate_limit)

        # Check a property for max and min
        clipmin.text = '-fcs/clip-max-value'
        clipmax.text = 'fcs/clip-max-value'
        tree.write(output_file)

        self.CheckClip(-0.05, 0.05, rate_limit)

        # Check the cyclic clip
        clipmin.text = str(-math.pi)
        clipmax.text = str(math.pi)
        clipto.attrib['type'] = 'cyclic'
        tree.write(output_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        fdm[self.input_prop] = 2.0*math.pi
        dt = math.pi/rate_limit
        while fdm['simulation/sim-time-sec'] <= dt:
            self.assertTrue(fdm[self.output_prop] <= math.pi)
            self.assertTrue(fdm[self.output_prop] >= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   fdm['simulation/sim-time-sec']*rate_limit)
            fdm.run()

        while fdm['simulation/sim-time-sec'] <= 2.0*dt:
            self.assertTrue(fdm[self.output_prop] >= -math.pi)
            self.assertTrue(fdm[self.output_prop] <= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   fdm['simulation/sim-time-sec']*rate_limit-2.0*math.pi)
            fdm.run()

        # Check that the output value does not go beyond 0.0
        self.assertAlmostEqual(fdm[self.output_prop], 0.0)
        fdm.run()
        self.assertAlmostEqual(fdm[self.output_prop], 0.0)

        t = fdm['simulation/sim-time-sec']
        fdm[self.input_prop] = -0.5*math.pi

        while fdm['simulation/sim-time-sec'] <= t+dt:
            self.assertTrue(fdm[self.output_prop] >= -math.pi)
            self.assertTrue(fdm[self.output_prop] <= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   (t-fdm['simulation/sim-time-sec'])*rate_limit)
            fdm.run()

        while fdm['simulation/sim-time-sec'] <= t+1.5*dt:
            self.assertTrue(fdm[self.output_prop] <= math.pi)
            self.assertTrue(fdm[self.output_prop] >= -0.5*math.pi)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   (t-fdm['simulation/sim-time-sec'])*rate_limit+2.0*math.pi)
            fdm.run()

        # Check the cyclic clip handles correctly negative numbers (GH issue
        # #211)
        # Case 1 : The interval is positive
        clipmin.text = '0.0'
        clipmax.text = str(math.pi)
        tree.write(output_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        fdm[self.input_prop] = -2.0*math.pi
        t0 = math.pi/rate_limit
        t = fdm['simulation/sim-time-sec']
        while t <= t0:
            self.assertTrue(fdm[self.output_prop] <= math.pi)
            self.assertTrue(fdm[self.output_prop] >= 0.0)
            if t == 0:
                self.assertAlmostEqual(fdm[self.output_prop], 0.0)
            else:
                self.assertAlmostEqual(fdm[self.output_prop],
                                       math.pi-t*rate_limit)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        while t <= 2.0*t0:
            self.assertTrue(fdm[self.output_prop] <= math.pi)
            self.assertTrue(fdm[self.output_prop] >= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   math.pi-(t-t0)*rate_limit)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        # Case 2 : The interval is negative
        clipmin.text = str(-math.pi)
        clipmax.text = '0.0'
        tree.write(output_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        fdm[self.input_prop] = math.pi
        dt = math.pi/rate_limit
        t = fdm['simulation/sim-time-sec']
        while t <= dt:
            self.assertAlmostEqual(fdm[self.output_prop],
                                   t*rate_limit-math.pi)
            self.assertTrue(fdm[self.output_prop] >= -math.pi-1E-8)
            self.assertTrue(fdm[self.output_prop] <= 0.0)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        t0 = t
        fdm[self.input_prop] = -2.0*math.pi
        fdm.run()
        t = fdm['simulation/sim-time-sec']

        while t <= t0+dt:
            self.assertTrue(fdm[self.output_prop] >= -math.pi)
            self.assertTrue(fdm[self.output_prop] <= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   (t0-t)*rate_limit)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

        t0 += dt

        while t <= t0+dt:
            self.assertTrue(fdm[self.output_prop] >= -math.pi)
            self.assertTrue(fdm[self.output_prop] <= 0.0)
            self.assertAlmostEqual(fdm[self.output_prop],
                                   (t0-t)*rate_limit)
            fdm.run()
            t = fdm['simulation/sim-time-sec']

    # Regression test for the bug reported in issue #200
    # JSBSim crashes when "fail hardover" is set while no <clipto> element is
    # specified.
    def test_failhardover_without_clipto(self):
        tree, flight_control_element, actuator_element = self.prepare_actuator()
        rate_limit = float(actuator_element.find('rate_limit').text)
        fail_hardover = actuator_element.attrib['name']+"/malfunction/fail_hardover"
        clipto = actuator_element.find('clipto')
        clipmax = float(clipto.find('max').text)
        actuator_element.remove(clipto)
        output_file = os.path.join('aircraft', self.aircraft_name,
                                   self.aircraft_name+'.xml')
        tree.write(output_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        # Displace the actuator in the maximum position.
        fdm[self.input_prop] = clipmax
        t = fdm['simulation/sim-time-sec']
        dt = clipmax/rate_limit
        while fdm['simulation/sim-time-sec'] <= t+dt:
            fdm.run()

        # Check the maximum position has been reached.
        self.assertAlmostEqual(fdm[self.output_prop], clipmax)

        # Trigger "fail hardover"
        fdm[fail_hardover] = 1.0
        t = fdm['simulation/sim-time-sec']
        dt = clipmax/rate_limit
        while fdm['simulation/sim-time-sec'] <= t+dt:
            fdm.run()

        # Check the actuator is failed in neutral position
        self.assertAlmostEqual(fdm[self.output_prop], 0.0)

        # Check that setting an input different from the neutral position does
        # not result in a modification of the actuator position.
        fdm[self.input_prop] = clipmax
        t = fdm['simulation/sim-time-sec']
        dt = clipmax/rate_limit
        while fdm['simulation/sim-time-sec'] <= t+dt:
            fdm.run()
            self.assertAlmostEqual(fdm[self.output_prop], 0.0)


RunTest(TestActuator)
