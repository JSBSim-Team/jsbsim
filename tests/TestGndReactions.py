# TestGndReactions.py
#
# Checks that ground reactions are working
#
# Copyright (c) 2018 Bertrand Coconnier
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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestGndReactions(JSBSimTestCase):
    def test_ground_reactions(self):
        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path(self.sandbox.path_to_jsbsim_file('tests'))
        fdm.load_model('tripod', False)

        fdm['ic/h-sl-ft'] = 0.0
        # Let's go to the North pole to get rid of the centrifugal ad Coriolis
        # accelerations.
        fdm['ic/lat-gc-deg'] = 90.0
        fdm['simulation/integrator/rate/rotational'] = 0
        fdm['simulation/integrator/rate/translational'] = 1
        fdm['simulation/integrator/position/rotational'] = 0
        fdm['simulation/integrator/position/translational'] = 1

        fdm.run_ic()
        fdm.do_trim(2)

        k = 10000.0 # Contact stiffness
        n = 3 # Number of contacts
        weight = fdm['forces/fbz-weight-lbs']
        self.assertAlmostEqual(n*k*fdm['contact/unit[0]/compression-ft']/weight,
                               1.0)
        self.assertAlmostEqual(n*k*fdm['contact/unit[1]/compression-ft']/weight,
                               1.0)
        self.assertAlmostEqual(n*k*fdm['contact/unit[2]/compression-ft']/weight,
                               1.0)

        del fdm

RunTest(TestGndReactions)
