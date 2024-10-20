# TestInputSocket.py
#
# A test case that checks that providing commands to JSBSim via an input socket
# is working.
#
# Copyright (c) 2015-2024 Bertrand Coconnier
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

import asyncio
import xml.etree.ElementTree as et

import telnetlib3
from JSBSim_utils import CopyAircraftDef, JSBSimTestCase, RunTest


class TelnetInterface:
    reader = None
    writer = None

    async def run(self, port, shell):
        self.reader, self.writer = await telnetlib3.open_connection(
            "localhost", port, shell=shell
        )
        await self.writer.protocol.waiter_closed

    async def get_output(self):
        msg = await self.reader.read(1024)
        lines = msg.split("\r\n")
        if lines[-1] == "JSBSim> ":
            return "\n".join(lines[:-1])
        else:
            prompt = await self.reader.read(1024)
            assert prompt == "JSBSim> "
            return "\n".join(lines)

    async def send_command(self, command):
        self.writer.write(f"{command}\n")
        await self.writer.drain()
        return await self.get_output()

    async def get_property_value(self, property_name):
        msg = (await self.send_command(f"get {property_name}")).split("\n")
        return float(msg[0].split("=")[1])


class TestInputSocket(JSBSimTestCase):
    def setUp(self, *args):
        super().setUp(*args)
        self._fdm = self.create_fdm()
        self.telnet = TelnetInterface()
        self.script_path = self.sandbox.path_to_jsbsim_file("scripts", "c1722.xml")
        self.assertion_failed = False

    def tearDown(self):
        self.assertFalse(self.assertion_failed)
        super().tearDown()
        self._fdm = None
        self.telnet = None

    def assertEqual(self, first, second, msg=None):
        try:
            super().assertEqual(first, second, msg)
        except AssertionError as e:
            print(e, flush=True)
            self.assertion_failed = True

    def assertAlmostEqual(self, first, second, places=None, msg=None, delta=None):
        try:
            super().assertAlmostEqual(first, second, places, msg, delta)
        except AssertionError as e:
            print(e, flush=True)
            self.assertion_failed = True

    async def run_fdm(self):
        while True:
            for _ in range(50):
                if not self._fdm.run():
                    return
                self._fdm.check_incremental_hold()
            await asyncio.sleep(0.1)

    def test_no_input(self):
        self._fdm.load_script(self.script_path)
        self._fdm.run_ic()
        self._fdm.hold()

        with self.assertRaises(OSError):
            asyncio.run(self.run_test(2222, self.sanity_check))

    def test_input_socket(self):
        # First, extract the time step from the script file
        tree = et.parse(self.script_path)
        dt = float(tree.getroot().find("run").attrib["dt"])

        # The aircraft c172x does not contain an <input> tag so we need
        # to add one.
        tree, aircraft_name, _ = CopyAircraftDef(self.script_path, self.sandbox)
        root = tree.getroot()
        input_tag = et.SubElement(root, "input")
        input_tag.attrib["port"] = "1137"
        tree.write(self.sandbox("aircraft", aircraft_name, aircraft_name + ".xml"))

        self._fdm.set_aircraft_path("aircraft")
        self._fdm.load_script(self.script_path)
        self._fdm.run_ic()
        self._fdm.hold()

        asyncio.run(self.run_test(1137, lambda r, w: self.shell(root, dt, r, w)))

    async def shell(self, root, dt, reader, writer):
        await self.sanity_check(reader, writer)
        msg = (await self.telnet.send_command("info")).split("\n")

        # Check the aircraft name and its version
        self.assertEqual(msg[2].split(":")[1].strip(), root.attrib["name"].strip())
        self.assertEqual(msg[1].split(":")[1].strip(), root.attrib["version"].strip())

        # Check that the simulation time is 0.0
        self.assertEqual(float(msg[3].split(":")[1].strip()), 0.0)
        self.assertEqual(self._fdm.get_sim_time(), 0.0)
        self.assertEqual(
            await self.telnet.get_property_value("simulation/sim-time-sec"),
            0.0,
        )

        # Check that 'iterate' iterates the correct number of times
        await self.telnet.send_command("iterate 19")
        self.assertEqual(self._fdm.get_sim_time(), 19.0 * dt)
        self.assertAlmostEqual(
            await self.telnet.get_property_value("simulation/sim-time-sec"),
            self._fdm.get_sim_time(),
            delta=1e-5,
        )
        self.assertTrue(self._fdm.holding())

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        await asyncio.sleep(0.5)
        self.assertEqual(self._fdm.get_sim_time(), 19.0 * dt)
        self.assertAlmostEqual(
            await self.telnet.get_property_value("simulation/sim-time-sec"),
            self._fdm.get_sim_time(),
            delta=1e-5,
        )

        # Modify the tank[0] contents via the "send" command
        half_contents = 0.5 * (
            await self.telnet.get_property_value("propulsion/tank/contents-lbs")
        )
        await self.telnet.send_command(
            "set propulsion/tank/contents-lbs " + str(half_contents)
        )
        self.assertEqual(
            await self.telnet.get_property_value("propulsion/tank/contents-lbs"),
            half_contents,
        )

        # Check the resume/hold commands
        t = self._fdm.get_sim_time()
        await self.telnet.send_command("resume")
        self.assertNotEqual(self._fdm.get_sim_time(), t)
        self.assertFalse(self._fdm.holding())
        await self.telnet.send_command("hold")
        t = self._fdm.get_sim_time()
        self.assertTrue(self._fdm.holding())
        self.assertAlmostEqual(
            await self.telnet.get_property_value("simulation/sim-time-sec"),
            t,
            delta=1e-5,
        )

        # Wait a little bit and make sure that the simulation time has not
        # changed meanwhile thus confirming that the simulation is on hold.
        await asyncio.sleep(0.5)
        self.assertEqual(self._fdm.get_sim_time(), t)
        self.assertAlmostEqual(
            await self.telnet.get_property_value("simulation/sim-time-sec"),
            t,
            delta=1e-5,
        )

        writer.close()

    async def sanity_check(self, _, __):
        out = await self.telnet.get_output()

        self.assertTrue(
            out == "Connected to JSBSim server",
            msg=f"Not connected to the JSBSim server.\nGot message '{out}' instead",
        )

        out = await self.telnet.send_command("help")

        # Check that "help" returns the minimum set of commands that will be
        # tested
        self.assertEqual(
            sorted(
                map(
                    lambda x: x.split("{")[0].strip(),
                    out.split("\n")[2:-1],
                )
            ),
            ["get", "help", "hold", "info", "iterate", "quit", "resume", "set"],
        )

    async def run_test(self, port, shell):
        telnet_task = asyncio.create_task(self.telnet.run(port, shell))
        fdm_task = asyncio.create_task(self.run_fdm())

        done, pending = await asyncio.wait(
            [telnet_task, fdm_task], return_when=asyncio.FIRST_COMPLETED
        )

        # Cancel the fdm_task if it is still pending
        for task in pending:
            task.cancel()

        # Handle exceptions if any
        for task in done:
            if task.exception():
                raise task.exception()

    def test_script_input(self):
        tree = et.parse(self.script_path)
        input_tag = et.SubElement(tree.getroot(), "input")
        input_tag.attrib["port"] = "1138"
        tree.write("c1722_1.xml")

        self._fdm.load_script("c1722_1.xml")
        self._fdm.run_ic()
        self._fdm.hold()

        asyncio.run(self.run_test(1138, self.sanity_check))


RunTest(TestInputSocket)
