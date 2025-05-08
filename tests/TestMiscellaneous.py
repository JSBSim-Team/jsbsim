# TestMiscellaneous.py
#
# A regression test that checks miscellaneous things. This is a place to collect
# all the tests that does not belong elsewhere until we find a better
# organization.
#
# Copyright (c) 2018-2022 Bertrand Coconnier
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

import os

from JSBSim_utils import CreateFDM, JSBSimTestCase, RunTest, jsbsim


class TestMiscellaneous(JSBSimTestCase):
    def test_property_access(self):
        fdm = self.create_fdm()

        # Check that the node 'qwerty' does not exist
        pm = fdm.get_property_manager()
        self.assertEqual(pm.get_node().get_fully_qualified_name(), "/fdm/jsbsim")
        self.assertFalse(pm.hasNode("qwerty"))

        # Check the default behavior of get_property_value. Non existing
        # properties return 0.0
        self.assertEqual(fdm.get_property_value("qwerty"), 0.0)

        # Verify that __getitem__ checks the existence and raises KeyError if
        # the property does not exist.
        with self.assertRaises(KeyError):
            x = fdm["qwerty"]

        # Check that we can initialize a non existing property
        fdm["qwerty"] = 42.0
        self.assertAlmostEqual(fdm.get_property_value("qwerty"), 42.0)
        self.assertAlmostEqual(fdm["qwerty"], 42.0)

        # Test the FGPropertyNode API to access property nodes
        node = pm.get_node("qwerty")
        if not node:
            self.fail()
        self.assertAlmostEqual(node.get_double_value(), 42.0)
        node.set_double_value(-1.0)
        self.assertAlmostEqual(node.get_double_value(), -1.0)
        self.assertAlmostEqual(fdm.get_property_value("qwerty"), -1.0)
        self.assertAlmostEqual(fdm["qwerty"], -1.0)

        # Test the FGPropertyNode to create new nodes
        self.assertFalse(pm.hasNode("egg"))
        self.assertIsNone(pm.get_node("egg"))  # `create` flag set to default (False)
        self.assertIsNone(pm.get_node("egg", False))  # `create` flag set to False
        egg_node = pm.get_node("egg", True)  # `create` flag set to True
        self.assertEqual(egg_node.get_name(), "egg")
        self.assertEqual(egg_node.get_fully_qualified_name(), "/fdm/jsbsim/egg")

        egg_node.set_double_value(0.25)
        self.assertAlmostEqual(egg_node.get_double_value(), 0.25)
        self.assertAlmostEqual(fdm.get_property_value("egg"), 0.25)
        self.assertAlmostEqual(fdm["egg"], 0.25)

        # Create a sub-node from a node
        self.assertIsNone(egg_node.get_node("spam"))
        spam_node = egg_node.get_node("spam", True)
        self.assertEqual(spam_node.get_name(), "spam")
        self.assertEqual(spam_node.get_fully_qualified_name(), "/fdm/jsbsim/egg/spam")

        pm2 = jsbsim.FGPropertyManager()
        root_node = pm2.get_node()
        self.assertEqual(root_node.get_name(), "")
        self.assertEqual(root_node.get_fully_qualified_name(), "/")

    def test_property_catalog(self):
        fdm = self.create_fdm()
        fdm.load_model("ball")
        fdm.run_ic()

        catalog = fdm.query_property_catalog("geod-deg")
        self.assertIsInstance(catalog, str)
        self.assertEqual(catalog, "position/lat-geod-deg (R)\nic/lat-geod-deg (RW)\n")

        catalog = fdm.get_property_catalog()
        self.assertIsInstance(catalog, list)
        self.assertGreater(len(catalog), 2)
        self.assertIn("position/lat-geod-deg (R)", catalog)
        self.assertIn("ic/lat-geod-deg (RW)", catalog)

    def test_FG_reset(self):
        # This test reproduces how FlightGear resets. The important thing is
        # that the property manager is managed by FlightGear. So it is not
        # deleted when the JSBSim instance is killed.
        pm = jsbsim.FGPropertyManager()

        self.assertFalse(pm.hasNode("fdm/jsbsim/ic/lat-geod-deg"))

        fdm = self.create_fdm(pm)
        fdm.load_model("ball")
        self.assertAlmostEqual(fdm["ic/lat-geod-deg"], 0.0)

        fdm["ic/lat-geod-deg"] = 45.0
        fdm.run_ic()

        # Delete the current instance of FGFDMExec to untie properties.
        fdm = None
        self.delete_fdm()

        # Check that the property ic/lat-geod-deg has survived the JSBSim
        # instance.
        self.assertTrue(pm.hasNode("fdm/jsbsim/ic/lat-geod-deg"))

        # Re-use the property manager just as FlightGear does.
        # Check ic/lat-geod-deg is reset to 0.0
        fdm = self.create_fdm(pm)
        self.assertAlmostEqual(fdm["ic/lat-geod-deg"], 0.0)

    def test_inherit_with_extra_args(self):
        # Regression test for issue 611
        # Make sure that extra args can be added to the constructor of classes
        # that inherit from FGFDMExec.
        class FGFDMExecWithExtraArgs(jsbsim.FGFDMExec):
            def __init__(self, root_dir, pm_root, extra):
                self.extra = extra

        pm = jsbsim.FGPropertyManager()
        root_node = pm.get_node("root", True)
        self.assertFalse(pm.hasNode("/root/fdm/jsbsim"))
        fdm = FGFDMExecWithExtraArgs(
            self.sandbox(), jsbsim.FGPropertyManager(root_node), True
        )
        self.assertEqual(fdm.get_root_dir(), self.sandbox())
        self.assertEqual(
            fdm.get_property_manager().get_node().get_fully_qualified_name(),
            "/root/fdm/jsbsim",
        )
        self.assertTrue(pm.hasNode("/root/fdm/jsbsim"))
        self.assertTrue(fdm.extra)

    def test_invalid_pointer_wont_crash(self):
        for cls in (
            jsbsim.FGPropagate,
            jsbsim.FGAuxiliary,
            jsbsim.FGAerodynamics,
            jsbsim.FGAircraft,
            jsbsim.FGMassBalance,
        ):
            x = cls(None)
            if x:
                self.fail()
            # Iterate over the methods of the class and make sure that calling a
            # method on an uninitialized instance raises the AttributeError exception.
            for method in dir(x):
                if method[0] != "_" and callable(getattr(x, method)):
                    with self.assertRaises(jsbsim.BaseError, msg=f"method {method}"):
                        getattr(x, method)()

        for cls in (jsbsim.FGLGear, jsbsim.FGEngine):
            x = cls()
            if x:
                self.fail()
            # Iterate over the methods of the class and make sure that calling a
            # method on an uninitialized instance raises the jsbsim.BaseError exception.
            for method in dir(x):
                if method[0] != "_" and callable(getattr(x, method)):
                    with self.assertRaises(jsbsim.BaseError, msg=f"method {method}"):
                        getattr(x, method)()

        grd = jsbsim.FGGroundReactions(None)
        if grd:
            self.fail()
        with self.assertRaises(jsbsim.BaseError):
            grd.get_gear_unit(0)
        with self.assertRaises(jsbsim.BaseError):
            grd.get_num_gear_units()

        atm = jsbsim.FGAtmosphere(None)
        if atm:
            self.fail()
        with self.assertRaises(jsbsim.BaseError):
            atm.set_temperature(15, 0, jsbsim.eTemperature.eCelsius)
        with self.assertRaises(jsbsim.BaseError):
            atm.get_temperature(0)
        with self.assertRaises(jsbsim.BaseError):
            atm.set_pressure_SL(jsbsim.ePressure.ePascals, 101325)

        prop = jsbsim.FGPropulsion(None)
        if prop:
            self.fail()
        with self.assertRaises(jsbsim.BaseError):
            prop.init_running(0)
        with self.assertRaises(jsbsim.BaseError):
            prop.get_num_engines()
        with self.assertRaises(jsbsim.BaseError):
            prop.get_engine(0)
        with self.assertRaises(jsbsim.BaseError):
            prop.get_steady_state()

        lin = jsbsim.FGLinearization(None)
        if lin:
            self.fail()
        with self.assertRaises(jsbsim.BaseError):
            lin.write_scicoslab("o")
        with self.assertRaises(jsbsim.BaseError):
            lin.x0
        with self.assertRaises(jsbsim.BaseError):
            lin.y0
        with self.assertRaises(jsbsim.BaseError):
            lin.u0
        with self.assertRaises(jsbsim.BaseError):
            lin.system_matrix
        with self.assertRaises(jsbsim.BaseError):
            lin.input_matrix
        with self.assertRaises(jsbsim.BaseError):
            lin.output_matrix
        with self.assertRaises(jsbsim.BaseError):
            lin.feedforward_matrix
        with self.assertRaises(jsbsim.BaseError):
            lin.state_space
        with self.assertRaises(jsbsim.BaseError):
            lin.x_names
        with self.assertRaises(jsbsim.BaseError):
            lin.y_names
        with self.assertRaises(jsbsim.BaseError):
            lin.u_names
        with self.assertRaises(jsbsim.BaseError):
            lin.x_units
        with self.assertRaises(jsbsim.BaseError):
            lin.y_units
        with self.assertRaises(jsbsim.BaseError):
            lin.u_units

        fdm = jsbsim.FGFDMExec(".")
        if not fdm.get_propagate():
            self.fail()
        if not fdm.get_ground_reactions():
            self.fail()
        if not fdm.get_auxiliary():
            self.fail()
        if not fdm.get_aerodynamics():
            self.fail()
        if not fdm.get_aircraft():
            self.fail()
        if not fdm.get_mass_balance():
            self.fail()
        if not fdm.get_atmosphere():
            self.fail()
        if not fdm.get_propulsion():
            self.fail()

        node = jsbsim.FGPropertyNode()
        if node:
            self.fail()
        with self.assertRaises(jsbsim.BaseError):
            node.get_name()
        with self.assertRaises(jsbsim.BaseError):
            node.get_fully_qualified_name()
        with self.assertRaises(jsbsim.BaseError):
            node.get_node("x", False)
        with self.assertRaises(jsbsim.BaseError):
            node.get_node("x", True)
        with self.assertRaises(jsbsim.BaseError):
            node.get_double_value()
        with self.assertRaises(jsbsim.BaseError):
            node.set_double_value(1.0)

        pm = jsbsim.FGPropertyManager()
        node = pm.get_node()
        if not node:
            self.fail()

    def test_property_manager_sharing(self):
        script_path = self.sandbox.path_to_jsbsim_file("scripts")
        pm = jsbsim.FGPropertyManager()
        root_c172 = pm.get_node("/c172", True)

        fdm_c172 = CreateFDM(self.sandbox, jsbsim.FGPropertyManager(root_c172))
        fdm_c172.load_script(os.path.join(script_path, "c1721.xml"))
        fdm_c172.run_ic()

        root_S23 = pm.get_node("/Short_S23", True)
        fdm_S23 = CreateFDM(self.sandbox, jsbsim.FGPropertyManager(root_S23))
        fdm_S23.load_script(os.path.join(script_path, "Short_S23_1.xml"))
        fdm_S23.run_ic()

        while fdm_c172["simulation/sim-time-sec"] < 1.0:
            fdm_c172.run()
            self.assertEqual(fdm_S23["simulation/sim-time-sec"], 0.0)

    def test_get_set_attributes(self):
        pm = jsbsim.FGPropertyManager()
        root_node = pm.get_node("root", True)
        # Check default values
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.WRITE))
        # Check that set No_ATTR is a no op.
        root_node.set_attribute(jsbsim.Attribute.NO_ATTR, True)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.WRITE))
        #Check READ unset
        root_node.set_attribute(jsbsim.Attribute.READ, False)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.WRITE))
        #Check WRITE unset
        root_node.set_attribute(jsbsim.Attribute.WRITE, False)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.WRITE))
        # Check that set No_ATTR is a no op.
        root_node.set_attribute(jsbsim.Attribute.NO_ATTR, True)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.WRITE))
        #Check READ set
        root_node.set_attribute(jsbsim.Attribute.READ, True)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.WRITE))
        #Check WRITE set
        root_node.set_attribute(jsbsim.Attribute.WRITE, True)
        self.assertFalse(root_node.get_attribute(jsbsim.Attribute.NO_ATTR))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.READ))
        self.assertTrue(root_node.get_attribute(jsbsim.Attribute.WRITE))

    def test_property_node_equality(self):
        pm = jsbsim.FGPropertyManager()
        root_node = pm.get_node("root", True)
        root_node2 = pm.get_node("root", False)
        self.assertIsNot(root_node, root_node2)  # The nodes are 2 different Python objects
        self.assertEqual(root_node, root_node2)  # but they are pointing to the same property node.


RunTest(TestMiscellaneous)
