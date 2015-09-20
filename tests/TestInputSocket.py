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

import sys, unittest, telnetlib, time, string, threading, socket
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

    def __del_(self):
        del self._fdm

    def run(self):
        self._cond.acquire()
        current_sim_time = self._fdm.get_sim_time()
        self._cond.release()

        while not self.quit:
            if current_sim_time > self._end_time:
                return

            if not self.realTime or current_sim_time < (time.time()-self._t0):
                self._cond.acquire()
                if not self._fdm.run():
                    self._cond.release()
                    return
                self._fdm.check_incremental_hold()
                current_sim_time = self._fdm.get_sim_time()
                self._cond.notify()
                self._cond.release()


class TelnetInterface:
    def __init__(self, fdm, end_time, port):
        # Execute JSBSim in a separate thread
        self.cond = threading.Condition()
        self.thread = JSBSimThread(fdm, self.cond, end_time, time.time())
        self.thread.start()

        # Wait for the thread to be started before connecting a telnet session
        self.cond.acquire()
        self.cond.wait()
        try:
            self.tn = telnetlib.Telnet("localhost", port, 2.0)
        finally:
            self.cond.release()

    def __del__(self):
        if 'tn' in self.__dict__.keys():  # Check if the Telnet session has been succesfully open
            self.tn.close()
        self.thread.quit = True
        self.thread.join()
        del self.thread

    def sendCommand(self, command):
        self.cond.acquire()
        self.tn.write(command+"\n")
        # Wait for a time step to be executed before reading the output from
        # telnet
        self.cond.wait()
        msg = self.tn.read_very_eager()
        self.cond.release()
        self.thread.join(0.1)
        return msg

    def getSimTime(self):
        self.cond.acquire()
        self.cond.wait()
        t = self.thread._fdm.get_sim_time()
        self.cond.release()
        return t

    def getDeltaT(self):
        self.cond.acquire()
        self.cond.wait()
        dt = self.thread._fdm.get_delta_t()
        self.cond.release()
        return dt

    def getPropertyValue(self, property):
        msg = string.split(self.sendCommand("get "+property), '\n')
        return float(string.split(msg[0], '=')[1])

    def getOutput(self):
        self.cond.acquire()
        self.cond.wait()
        out = self.tn.read_very_eager()
        self.cond.release()
        return out

    def wait(self, seconds):
        self.thread.join(seconds)

    def setRealTime(self, rt):
        self.thread.realTime = rt


class TestInputSocket(unittest.TestCase):
    def setUp(self):
        self.sandbox = SandBox()
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1722.xml')

    def tearDown(self):
        self.sandbox.erase()

    def sanityCheck(self, _tn):
        # Check that the connection has been established
        out = _tn.getOutput()
        self.assertTrue(string.split(out, '\n')[0] == 'Connected to JSBSim server',
                        msg="Not connected to the JSBSim server.\nGot message '%s' instead" % (out,))

        # Check that "help" returns the minimum set of commands that will be
        # tested
        self.assertEqual(sorted(map(lambda x: string.strip(string.split(x, '{')[0]),
                                    string.split(_tn.sendCommand("help"), '\n')[2:-2])),
                         ['get', 'help', 'hold', 'info', 'iterate', 'quit', 'resume', 'set'])

    def test_no_input(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_script(self.script_path)
        fdm.run_ic()
        fdm.hold()

        with self.assertRaises(socket.error):
            TelnetInterface(fdm, 5., 1137)

    def test_input_socket(self):
        # The aircraft c172x does not contain an <input> tag so we need
        # to add one.
        tree, aircraft_name, b = CopyAircraftDef(self.script_path, self.sandbox)
        self.root = tree.getroot()
        input_tag = et.SubElement(self.root, 'input')
        input_tag.attrib['port'] = '1137'
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()
        fdm.hold()

        tn = TelnetInterface(fdm, 5., 1137)
        self.sanityCheck(tn)

        # Check the aircraft name and its version
        msg = string.split(tn.sendCommand("info"), '\n')
        self.assertEqual(string.strip(string.split(msg[2], ':')[1]),
                         string.strip(self.root.attrib['name']))
        self.assertEqual(string.strip(string.split(msg[1], ':')[1]),
                         string.strip(self.root.attrib['version']))

        # Check that the simulation time is 0.0
        self.assertEqual(float(string.strip(string.split(msg[3], ':')[1])), 0.0)
        self.assertEqual(tn.getSimTime(), 0.0)
        self.assertEqual(tn.getPropertyValue("simulation/sim-time-sec"), 0.0)

        # Check that 'iterate' iterates the correct number of times
        tn.sendCommand("iterate 19")
        self.assertEqual(tn.getSimTime(), 19. * tn.getDeltaT())
        self.assertAlmostEqual(tn.getPropertyValue("simulation/sim-time-sec"),
                               tn.getSimTime(), delta=1E-5)

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        tn.wait(0.1)
        self.assertEqual(tn.getSimTime(), 19. * tn.getDeltaT())
        self.assertAlmostEqual(tn.getPropertyValue("simulation/sim-time-sec"),
                               tn.getSimTime(), delta=1E-5)

        # Modify the tank[0] contents via the "send" command
        half_contents = 0.5 * tn.getPropertyValue("propulsion/tank/contents-lbs")
        tn.sendCommand("set propulsion/tank/contents-lbs " + str(half_contents))
        self.assertEqual(tn.getPropertyValue("propulsion/tank/contents-lbs"),
                         half_contents)

        # Check the resume/hold commands
        tn.setRealTime(True)
        t = tn.getSimTime()
        tn.sendCommand("resume")
        tn.wait(0.5)
        self.assertNotEqual(tn.getSimTime(), t)
        tn.wait(0.5)
        tn.sendCommand("hold")
        tn.setRealTime(False)
        t = tn.getSimTime()
        self.assertAlmostEqual(tn.getPropertyValue("simulation/sim-time-sec"),
                               t, delta=1E-5)

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        tn.wait(0.1)
        self.assertEqual(tn.getSimTime(), t)
        self.assertAlmostEqual(tn.getPropertyValue("simulation/sim-time-sec"),
                               t, delta=1E-5)

    def test_script_input(self):
        tree = et.parse(self.sandbox.elude(self.script_path))
        input_tag = et.SubElement(tree.getroot(), 'input')
        input_tag.attrib['port'] = '1138'
        tree.write(self.sandbox('c1722_1.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.load_script('c1722_1.xml')
        fdm.run_ic()
        fdm.hold()

        tn = TelnetInterface(fdm, 5., 1138)
        self.sanityCheck(tn)

suite = unittest.TestLoader().loadTestsFromTestCase(TestInputSocket)
test_result = unittest.TextTestRunner(verbosity=2).run(suite)
if test_result.failures or test_result.errors:
    sys.exit(-1)  # 'make test' will report the test failed.
