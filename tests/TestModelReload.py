"""Regression for property bindings across model replacement and failed loads."""

import gc
import os
import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest


class TestModelReload(JSBSimTestCase):
    def check_live_bindings(self, fdm):
        # Exercise setters and getters owned by both the executive and newly
        # allocated models; comparing only load return codes missed stale ties.
        fdm.set_dt(0.01)
        self.assertEqual(fdm["simulation/dt"], 0.01)
        fdm["simulation/randomseed"] = 123
        self.assertEqual(fdm["simulation/randomseed"], 123)
        fdm["simulation/reset"] = 2
        self.assertEqual(fdm.get_sim_time(), 0.0)
        fdm["ic/h-sl-ft"] = 3000
        fdm["ic/vc-kts"] = 90
        self.assertTrue(fdm.run_ic())
        self.assertAlmostEqual(fdm["position/h-sl-ft"], 3000, places=6)
        self.assertAlmostEqual(fdm["velocities/vc-kts"], 90, places=6)
        fdm["simulation/pause"] = 1
        self.assertTrue(fdm.holding())
        self.assertTrue(fdm.run())
        self.assertEqual(fdm["simulation/sim-time-sec"], 0.0)
        fdm["simulation/pause"] = 0
        self.assertFalse(fdm.holding())
        self.assertTrue(fdm.run())
        self.assertAlmostEqual(fdm["simulation/sim-time-sec"], 0.01)
        fdm["simulation/terminate"] = 1
        self.assertFalse(fdm.run())
        fdm["simulation/terminate"] = 0
        self.assertTrue(fdm.run())
        fdm["simulation/reset"] = 2
        self.assertEqual(fdm.get_sim_time(), 0.0)

    def check_sequence(self, models):
        fdm = self.create_fdm()
        for model in models:
            self.assertEqual(fdm.load_model(model), model == "c172p")
            if model == "c172p":
                self.check_live_bindings(fdm)
                catalog = fdm.get_property_catalog()
                self.assertEqual(len(catalog), len(set(catalog)))
        # Trigger destruction inside the regression, after the final load or
        # failed load, rather than relying on process exit for cleanup.
        self.delete_fdm()
        del fdm
        gc.collect()

    def test_repeated_successful_replacements(self):
        self.check_sequence(["c172p", "c172p", "c172p"])

    def test_failed_replacement_then_destruction(self):
        self.check_sequence(["c172p", "missing-aircraft"])

    def test_success_failure_and_recovery(self):
        self.check_sequence(["c172p", "c172p", "missing-aircraft", "c172p"])

    def test_two_failures_and_recovery(self):
        self.check_sequence(["c172p", "first-missing-aircraft",
                             "second-missing-aircraft", "c172p"])

    def test_initial_failure_then_destruction(self):
        self.check_sequence(["missing-aircraft"])

    def test_partial_xml_failure_and_recovery(self):
        fdm = self.create_fdm()
        self.assertTrue(fdm.load_model("c172p"))
        source = self.sandbox.path_to_jsbsim_file("aircraft", "c172p", "c172p.xml")
        tree = et.parse(source)
        # Metrics are loaded before the required mass_balance section. This
        # reaches a partial load, rather than just a missing-file failure.
        tree.getroot().remove(tree.getroot().find("mass_balance"))
        os.mkdir("partial-model")
        tree.write(os.path.join("partial-model", "partial-model.xml"))
        fdm.set_aircraft_path(self.sandbox())
        self.assertFalse(fdm.load_model("partial-model"))
        self.assertFalse(fdm.load_model("partial-model"))
        fdm.set_aircraft_path(self.sandbox.path_to_jsbsim_file("aircraft"))
        self.assertTrue(fdm.load_model("c172p"))
        self.check_live_bindings(fdm)
        self.delete_fdm()
        del fdm
        gc.collect()


RunTest(TestModelReload)
