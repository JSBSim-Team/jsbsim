"""Regression for steady turbine fuel flow during zero-time trim."""

import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest

FUEL_FLOW = "propulsion/engine[0]/fuel-flow-rate-gph"
# The F16 FCS maps the command to a 0..2 position range, so commands at or
# below 0.49 stay in the dry range and 1.0 requests augmentation.
DRY_COMMANDS = (0.0, 0.2, 0.35, 0.49)
ALL_COMMANDS = DRY_COMMANDS + (1.0,)


class TestTurbineTrimFuelFlow(JSBSimTestCase):
    def setUp(self):
        super().setUp()
        # Use the standard turbine fixture without script events that could
        # overwrite the throttle while the trim value is being read back.
        script = et.parse(self.sandbox.path_to_jsbsim_file(
            "scripts", "f16_test.xml"))
        self.use = script.getroot().find("use")
        self.aircraft_name = self.use.attrib["aircraft"]

    def trimmed_fdm(self):
        fdm = self.create_fdm()
        self.assertTrue(fdm.load_model(self.aircraft_name))
        self.assertTrue(fdm.load_ic(self.use.attrib["initialize"], True))
        fdm.set_dt(1.0 / 120.0)
        self.assertTrue(fdm.run_ic())
        fdm["propulsion/set-running"] = -1
        return fdm

    def trim_at(self, fdm, command):
        fdm["fcs/throttle-cmd-norm[0]"] = command
        fdm["fcs/throttle-pos-norm[0]"] = command
        before_time = fdm.get_sim_time()
        self.assertTrue(fdm.run_ic())
        self.assertEqual(fdm.get_sim_time(), before_time)
        return fdm[FUEL_FLOW]

    def test_trim_fuel_flow_needs_no_seeking(self):
        # Fuel flow is rate limited while time advances, so a trim value that
        # already is the steady one cannot be sought away on the next frame.
        # That frame still moves the airplane slightly, which shifts the thrust
        # lookups, so allow a small relative change rather than an exact match.
        for command in ALL_COMMANDS:
            with self.subTest(command=command):
                fdm = self.trimmed_fdm()
                trimmed = self.trim_at(fdm, command)
                self.assertGreater(trimmed, 0.0)
                self.assertTrue(fdm.run())
                self.assertLess(abs(fdm[FUEL_FLOW] / trimmed - 1.0), 2e-3)
                del fdm

    def test_trim_fuel_flow_does_not_retain_the_previous_setting(self):
        # Each zero-time trim must describe its own operating point, whatever
        # the engine reported before it.
        reference = {}
        for command in ALL_COMMANDS:
            fdm = self.trimmed_fdm()
            reference[command] = self.trim_at(fdm, command)
            del fdm

        fdm = self.trimmed_fdm()
        for command in (1.0, 0.0, 0.49, 0.2, 1.0, 0.35, 0.0):
            with self.subTest(command=command):
                self.assertAlmostEqual(self.trim_at(fdm, command)
                                       / reference[command], 1.0, places=6)

        dry = [reference[command] for command in DRY_COMMANDS]
        self.assertEqual(dry, sorted(dry))
        self.assertGreater(dry[-1], dry[0])
        self.assertGreater(reference[1.0], dry[-1])

    def test_trim_reports_the_tsfc_of_its_own_operating_point(self):
        fdm = self.trimmed_fdm()
        self.trim_at(fdm, 0.49)
        high = fdm["propulsion/engine[0]/tsfc"]
        self.trim_at(fdm, 0.0)
        idle = fdm["propulsion/engine[0]/tsfc"]
        # The simplified TSFC rises as N2norm falls, so the two operating
        # points must not report the same corrected value.
        self.assertGreater(idle, high)


RunTest(TestTurbineTrimFuelFlow)
