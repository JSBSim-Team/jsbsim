# TestPQRdot.py
#
# Regression test for the GitHub issue #1034
#
# Copyright (c) 2024 Bertrand Coconnier
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

import math
from JSBSim_utils import JSBSimTestCase, RunTest


class TestPQRdot(JSBSimTestCase):
    def test_pqrdot(self):
        script_name = "ball_orbit.xml"
        script_path = self.sandbox.path_to_jsbsim_file("scripts", script_name)
        fdm = self.create_fdm()
        fdm.load_script(script_path)

        omega = 0.00007292115
        p0 = math.radians(15.0)
        T0 = 2.0 * math.pi / p0

        fdm["ic/p-rad_sec"] = p0
        fdm.run_ic()

        pm = fdm.get_property_manager()

        sim_time = pm.get_node("simulation/sim-time-sec")
        p = pm.get_node("velocities/p-rad_sec")
        q = pm.get_node("velocities/q-rad_sec")
        r = pm.get_node("velocities/r-rad_sec")
        pdot = pm.get_node("accelerations/pdot-rad_sec2")
        qdot = pm.get_node("accelerations/qdot-rad_sec2")
        rdot = pm.get_node("accelerations/rdot-rad_sec2")
        pi = pm.get_node("velocities/pi-rad_sec")
        qi = pm.get_node("velocities/qi-rad_sec")
        ri = pm.get_node("velocities/ri-rad_sec")

        while sim_time.get_double_value() < 10.0 * T0:
            fdm.run()
            t = sim_time.get_double_value()
            self.assertAlmostEqual(p.get_double_value(), p0, delta=1e-8)
            self.assertAlmostEqual(
                q.get_double_value(), omega * math.cos(p0 * t), delta=2e-8
            )
            self.assertAlmostEqual(
                r.get_double_value(), -omega * math.sin(p0 * t), delta=2e-8
            )
            self.assertAlmostEqual(pdot.get_double_value(), 0.0, delta=1e-8)
            self.assertAlmostEqual(
                qdot.get_double_value(), -omega * p0 * math.sin(p0 * t), delta=1e-8
            )
            self.assertAlmostEqual(
                rdot.get_double_value(), -omega * p0 * math.cos(p0 * t), delta=1e-8
            )
            self.assertAlmostEqual(pi.get_double_value(), p0, delta=1e-8)
            self.assertAlmostEqual(qi.get_double_value(), 0.0, delta=1e-8)
            self.assertAlmostEqual(ri.get_double_value(), 0.0, delta=1e-8)


RunTest(TestPQRdot)
