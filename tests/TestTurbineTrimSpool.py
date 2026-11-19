"""Regression for observable turbine spool speeds during zero-time trim."""

import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest


class TestTurbineTrimSpool(JSBSimTestCase):
    def test_zero_time_trim_updates_both_spools(self):
        # Use the standard turbine fixture without script events that could
        # overwrite the throttle during the continuity check.
        script = et.parse(self.sandbox.path_to_jsbsim_file(
            "scripts", "f16_test.xml"))
        use = script.getroot().find("use")
        aircraft_name = use.attrib["aircraft"]
        aircraft = et.parse(self.sandbox.path_to_jsbsim_file(
            "aircraft", aircraft_name, aircraft_name + ".xml"))
        engine_name = aircraft.getroot().find("propulsion/engine").attrib["file"]
        engine = et.parse(self.sandbox.path_to_jsbsim_file(
            "engine", engine_name + ".xml")).getroot()
        idle_n1 = float(engine.find("idlen1").text)
        maximum_n1 = float(engine.find("maxn1").text)
        idle_n2 = float(engine.find("idlen2").text)
        maximum_n2 = float(engine.find("maxn2").text)

        fdm = self.create_fdm()
        self.assertTrue(fdm.load_model(aircraft_name))
        self.assertTrue(fdm.load_ic(use.attrib["initialize"], True))
        fdm.set_dt(1.0 / 120.0)
        self.assertTrue(fdm.run_ic())
        fdm["propulsion/set-running"] = -1

        for throttle in (0.0, 0.35, 1.0, 0.0):
            with self.subTest(throttle=throttle):
                fdm["fcs/throttle-cmd-norm[0]"] = throttle
                fdm["fcs/throttle-pos-norm[0]"] = throttle
                before_time = fdm.get_sim_time()
                self.assertTrue(fdm.run_ic())
                self.assertEqual(fdm.get_sim_time(), before_time)
                # The F16 FCS maps the command to a 0..2 position range.
                # Positions above 1 request augmentation, not higher dry
                # spool speeds. Assert the fixture mapping independently.
                throttle_position = fdm["fcs/throttle-pos-norm[0]"]
                self.assertAlmostEqual(throttle_position, 2.0 * throttle,
                                       places=7)
                dry_throttle = max(0.0, min(1.0, throttle_position))
                self.assertAlmostEqual(
                    fdm["propulsion/engine[0]/n1"],
                    idle_n1 + dry_throttle * (maximum_n1 - idle_n1), places=7)
                self.assertAlmostEqual(
                    fdm["propulsion/engine[0]/n2"],
                    idle_n2 + dry_throttle * (maximum_n2 - idle_n2), places=7)

        # The first integrated idle sample must not repair stale full-power
        # indications. No application-side property writes mask the defect.
        before_n1 = fdm["propulsion/engine[0]/n1"]
        before_n2 = fdm["propulsion/engine[0]/n2"]
        self.assertTrue(fdm.run())
        self.assertGreater(fdm.get_sim_time(), before_time)
        self.assertAlmostEqual(fdm["propulsion/engine[0]/n1"], before_n1, places=5)
        self.assertAlmostEqual(fdm["propulsion/engine[0]/n2"], before_n2, places=5)


RunTest(TestTurbineTrimSpool)
