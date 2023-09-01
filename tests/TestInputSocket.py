# TestInputSocket.py
#
# A test case that checks that providing commands to JSBSim via an input socket
# is working.
#
# Copyright (c) 2015-2022 Bertrand Coconnier
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

import telnetlib, socket, time
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, CopyAircraftDef, RunTest


class TelnetInterface:
    def __init__(self, fdm, port):
        self.tn = telnetlib.Telnet("localhost", port)
        self.fdm = fdm
        fdm.run()

    def __del__(self):
        if (
            "tn" in self.__dict__.keys()
        ):  # Check if the Telnet session has been succesfully open
            self.tn.close()
        self.fdm = None

    def sendCommand(self, command):
        self.tn.write("{}\n".format(command).encode())
        for _ in range(50):
            self.fdm.run()
            self.fdm.check_incremental_hold()
        return self.getOutput()

    def getOutput(self):
        time.sleep(1.0) # Wait for the socket to process all the data.
        return self.tn.read_very_eager().decode()

    def getPropertyValue(self, property):
        msg = self.sendCommand(f"get {property}").split("\n")
        return float(msg[0].split("=")[1])


class TestInputSocket(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)
        self.script_path = self.sandbox.path_to_jsbsim_file("scripts", "c1722.xml")

    def sanityCheck(self, tn):
        # Check that the connection has been established
        out = tn.getOutput()
        self.assertTrue(
            out.split("\n")[0] == "Connected to JSBSim server",
            msg="Not connected to the JSBSim server.\nGot message '%s' instead"
            % (out,),
        )

        # Check that "help" returns the minimum set of commands that will be
        # tested
        self.assertEqual(
            sorted(
                map(
                    lambda x: x.split("{")[0].strip(),
                    tn.sendCommand("help").split("\n")[2:-2],
                )
            ),
            ["get", "help", "hold", "info", "iterate", "quit", "resume", "set"],
        )

    def test_no_input(self):
        fdm = self.create_fdm()
        fdm.load_script(self.script_path)
        fdm.run_ic()
        fdm.hold()

        with self.assertRaises(socket.error):
            TelnetInterface(fdm, 2222)

    def test_input_socket(self):
        # First, extract the time step from the script file
        tree = et.parse(self.script_path)
        dt = float(tree.getroot().find("run").attrib["dt"])

        # The aircraft c172x does not contain an <input> tag so we need
        # to add one.
        tree, aircraft_name, b = CopyAircraftDef(self.script_path, self.sandbox)
        root = tree.getroot()
        input_tag = et.SubElement(root, "input")
        input_tag.attrib["port"] = "1137"
        tree.write(self.sandbox("aircraft", aircraft_name, aircraft_name + ".xml"))

        fdm = self.create_fdm()
        fdm.set_aircraft_path("aircraft")
        fdm.load_script(self.script_path)
        fdm.run_ic()
        fdm.hold()

        tn = TelnetInterface(fdm, 1137)
        self.sanityCheck(tn)

        # Check the aircraft name and its version
        msg = tn.sendCommand("info").split("\n")
        self.assertEqual(msg[2].split(":")[1].strip(), root.attrib["name"].strip())
        self.assertEqual(msg[1].split(":")[1].strip(), root.attrib["version"].strip())

        # Check that the simulation time is 0.0
        self.assertEqual(float(msg[3].split(":")[1].strip()), 0.0)
        self.assertEqual(fdm.get_sim_time(), 0.0)
        self.assertEqual(tn.getPropertyValue("simulation/sim-time-sec"), 0.0)

        # Check that 'iterate' iterates the correct number of times
        tn.sendCommand("iterate 19")
        self.assertEqual(fdm.get_sim_time(), 19.0 * dt)
        self.assertAlmostEqual(
            tn.getPropertyValue("simulation/sim-time-sec"),
            fdm.get_sim_time(),
            delta=1e-5,
        )
        self.assertTrue(fdm.holding())

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        for _ in range(40):
            fdm.run()
        self.assertEqual(fdm.get_sim_time(), 19.0 * dt)
        self.assertAlmostEqual(
            tn.getPropertyValue("simulation/sim-time-sec"),
            fdm.get_sim_time(),
            delta=1e-5,
        )

        # Modify the tank[0] contents via the "send" command
        half_contents = 0.5 * tn.getPropertyValue("propulsion/tank/contents-lbs")
        tn.sendCommand("set propulsion/tank/contents-lbs " + str(half_contents))
        self.assertEqual(
            tn.getPropertyValue("propulsion/tank/contents-lbs"), half_contents
        )

        # Check the resume/hold commands
        t = fdm.get_sim_time()
        tn.sendCommand("resume")
        self.assertNotEqual(fdm.get_sim_time(), t)
        self.assertFalse(fdm.holding())
        tn.sendCommand("hold")
        t = fdm.get_sim_time()
        self.assertTrue(fdm.holding())
        self.assertAlmostEqual(
            tn.getPropertyValue("simulation/sim-time-sec"), t, delta=1e-5
        )

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        for _ in range(40):
            fdm.run()
        self.assertEqual(fdm.get_sim_time(), t)
        self.assertAlmostEqual(
            tn.getPropertyValue("simulation/sim-time-sec"), t, delta=1e-5
        )

    def test_script_input(self):
        tree = et.parse(self.script_path)
        input_tag = et.SubElement(tree.getroot(), "input")
        input_tag.attrib["port"] = "1138"
        tree.write("c1722_1.xml")

        fdm = self.create_fdm()
        fdm.load_script("c1722_1.xml")
        fdm.run_ic()
        fdm.hold()

        tn = TelnetInterface(fdm, 1138)
        self.sanityCheck(tn)


RunTest(TestInputSocket)
