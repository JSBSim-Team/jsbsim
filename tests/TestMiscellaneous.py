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

from JSBSim_utils import JSBSimTestCase, RunTest, jsbsim


class TestMiscellaneous(JSBSimTestCase):
    def test_property_access(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm.run_ic()

        # Check that the node 'qwerty' does not exist
        pm = fdm.get_property_manager()
        self.assertFalse(pm.hasNode('qwerty'))

        # Check the default behavior of get_property_value. Non existing
        # properties return 0.0
        self.assertEqual(fdm.get_property_value('qwerty'), 0.0)

        # Verify that __getitem__ checks the existence and raises KeyError if
        # the property does not exist.
        with self.assertRaises(KeyError):
            x = fdm['qwerty']

        # Check that we can initialize a non existing property
        fdm['qwerty'] = 42.0
        self.assertAlmostEqual(fdm.get_property_value('qwerty'), 42.0)
        self.assertAlmostEqual(fdm['qwerty'], 42.0)

    def test_property_catalog(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm.run_ic()

        catalog = fdm.query_property_catalog('geod-deg')
        self.assertIsInstance(catalog, str)
        self.assertEqual(catalog, 'position/lat-geod-deg (R)\nic/lat-geod-deg (RW)\n')

        catalog = fdm.get_property_catalog()
        self.assertIsInstance(catalog, list)
        self.assertGreater(len(catalog), 2)
        self.assertIn('position/lat-geod-deg (R)', catalog)
        self.assertIn('ic/lat-geod-deg (RW)', catalog)

    def test_FG_reset(self):
        # This test reproduces how FlightGear resets. The important thing is
        # that the property manager is managed by FlightGear. So it is not
        # deleted when the JSBSim instance is killed.
        pm = jsbsim.FGPropertyManager()

        self.assertFalse(pm.hasNode('fdm/jsbsim/ic/lat-geod-deg'))

        fdm = self.create_fdm(pm)
        fdm.load_model('ball')
        self.assertAlmostEqual(fdm['ic/lat-geod-deg'], 0.0)

        fdm['ic/lat-geod-deg'] = 45.0
        fdm.run_ic()

        # Delete the current instance of FGFDMExec to untie properties.
        fdm = None
        self.delete_fdm()

        # Check that the property ic/lat-geod-deg has survived the JSBSim
        # instance.
        self.assertTrue(pm.hasNode('fdm/jsbsim/ic/lat-geod-deg'))

        # Re-use the property manager just as FlightGear does.
        # Check ic/lat-geod-deg is reset to 0.0
        fdm = self.create_fdm(pm)
        self.assertAlmostEqual(fdm['ic/lat-geod-deg'], 0.0)

    def test_inherit_with_extra_args(self):
        # Regression test for issue 611
        # Make sure that extra args can be added to the constructor of classes
        # that inherit from FGFDMExec.
        class FGFDMExecWithExtraArgs(jsbsim.FGFDMExec):
            def __init__(self, root_dir, pm_root, extra):
                self.extra = extra
                super().__init__()

        fdm = FGFDMExecWithExtraArgs(os.path.join(self.sandbox(), ''), None, True)
        self.assertTrue(fdm.extra)


RunTest(TestMiscellaneous)
