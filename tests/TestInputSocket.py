# TestInputSocket.py
#
# A test case that checks that providing commands to JSBSim via an input socket
# is working.
#
# Copyright (c) 2015 Bertrand Coconnier
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

import sys, unittest, telnetlib, time, string, threading
import xml.etree.ElementTree as et
from JSBSim_utils import CreateFDM, SandBox, CopyAircraftDef

# This test case runs JSBSim and a telnet session in two different threads.
# As a consequence there is a significant amount of synchronization that needs
# to be handled to make sure that, for instance, the telnet session is not
# trying to read a result before the JSBSim thread had a chance to process the
# request.

class JSBSimThread(threading.Thread):
    def __init__(self, fdm, cond, end_time, t0=0.0):
        threading.Thread.__init__(self)
        self.realTime = False
        self.quit = False
        self._fdm = fdm
        self._cond = cond
        self._end_time = end_time
        self._t0 = t0
    def run(self):
        while not self.quit:
            if not self.realTime or current_sim_time < (time.time() - self._t0):
                self._cond.acquire()
                if not self._fdm.run():
                    self._cond.release()
                    return
                self._fdm.check_incremental_hold()
                current_sim_time = self._fdm.get_sim_time()
                self._cond.notify()
                self._cond.release()
                if current_sim_time > self._end_time:
                    return

class TestInputSocket(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1722.xml')

        # The aircraft c172x does not contain an <input> tag so we need
        # to add one.
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        self.root = tree.getroot()
        input_tag = et.SubElement(self.root, 'input')
        input_tag.attrib['port']='1137'
        tree.write(self.sandbox('aircraft', aircraft_name,  aircraft_name+'.xml'))

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.set_aircraft_path('aircraft')
        self.fdm.load_script(script_path)
        self.fdm.run_ic()
        self.fdm.hold()

        # Execute JSBSim in a separate thread
        self.cond = threading.Condition()
        self.thread = JSBSimThread(self.fdm, self.cond, 5., time.time())
        self.thread.start()

        # Wait for the thread to be started before connecting a telnet session
        self.cond.acquire()
        self.cond.wait()
        self.tn = telnetlib.Telnet("localhost", 1137)
        self.cond.release()

    def tearDown(self):
        self.tn.close()
        self.thread.quit = True
        self.thread.join()
        self.sandbox.erase()

    def sendCommand(self, command):
        self.cond.acquire()
        self.tn.write(command+"\n")
        # Wait for a time step to be executed before reading the output from telnet
        self.cond.wait()
        msg = self.tn.read_very_eager()
        self.cond.release()
        self.thread.join(0.1)
        return msg

    def getSimTime(self):
        self.cond.acquire()
        self.cond.wait()
        t = self.fdm.get_sim_time()
        self.cond.release()
        return t

    def getDeltaT(self):
        self.cond.acquire()
        self.cond.wait()
        dt = self.fdm.get_delta_t()
        self.cond.release()
        return dt

    def getPropertyValue(self, property):
        msg = string.split(self.sendCommand("get "+property),'\n')
        return float(string.split(msg[0], '=')[1])

    def test_input_socket(self):
        # Check that the connection has been established
        self.cond.acquire()
        self.cond.wait()
        out = self.tn.read_very_eager()
        self.cond.release()
        self.assertTrue(string.split(out, '\n')[0] == 'Connected to JSBSim server',
                        msg="Not connected to the JSBSim server.\nGot message '%s' instead" % (out,))

        # Check that "help" returns the minimum set of commands that will be
        # tested
        self.assertEqual(sorted(map(lambda x : string.strip(string.split(x, '{')[0]),
                                    string.split(self.sendCommand("help"), '\n')[2:-2])),
                         ['get', 'help', 'hold', 'info', 'iterate', 'quit', 'resume', 'set'])

        # Check the aircraft name and its version
        msg = string.split(self.sendCommand("info"), '\n')
        self.assertEqual(string.strip(string.split(msg[2], ':')[1]),
                         string.strip(self.root.attrib['name']))
        self.assertEqual(string.strip(string.split(msg[1], ':')[1]),
                         string.strip(self.root.attrib['version']))

        # Check that the simulation time is 0.0
        self.assertEqual(float(string.strip(string.split(msg[3], ':')[1])), 0.0)
        self.assertEqual(self.getSimTime(), 0.0)
        self.assertEqual(self.getPropertyValue("simulation/sim-time-sec"), 0.0)

        # Check that 'iterate' iterates the correct number of times
        self.sendCommand("iterate 19")
        self.assertEqual(self.getSimTime(), 19. * self.getDeltaT())
        self.assertAlmostEqual(self.getPropertyValue("simulation/sim-time-sec"),
                               self.getSimTime(), delta=1E-5)

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        self.thread.join(0.1)
        self.assertEqual(self.getSimTime(), 19. * self.getDeltaT())
        self.assertAlmostEqual(self.getPropertyValue("simulation/sim-time-sec"),
                               self.getSimTime(), delta=1E-5)

        # Modify the tank[0] contents via the "send" command
        half_contents = 0.5 * self.getPropertyValue("propulsion/tank/contents-lbs")
        self.sendCommand("set propulsion/tank/contents-lbs "+ str(half_contents))
        self.cond.acquire()
        self.cond.wait()
        self.assertEqual(self.fdm.get_property_value("propulsion/tank/contents-lbs"),
                         half_contents)
        self.cond.release()

        # Check the resume/hold commands
        self.thread.realTime = True
        t = self.getSimTime()
        self.sendCommand("resume")
        self.thread.join(0.5)
        self.assertNotEqual(self.getSimTime(), t)
        self.thread.join(0.5)
        self.sendCommand("hold")
        self.thread.realTime = False
        t = self.getSimTime()
        self.assertAlmostEqual(self.getPropertyValue("simulation/sim-time-sec"),
                               t, delta=1E-5)

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        self.thread.join(0.1)
        self.assertEqual(self.getSimTime(), t)
        self.assertAlmostEqual(self.getPropertyValue("simulation/sim-time-sec"),
                               t, delta=1E-5)

suite = unittest.TestLoader().loadTestsFromTestCase(TestInputSocket)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1) # 'make test' will report the test failed.
