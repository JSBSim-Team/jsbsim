# TestRotorGroundEffect.py
#
# Check that the ground effect of a rotor does not depend on the time step.
#
# Copyright (c) 2026 Alexander Kalmykov
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

ROTOR = 'propulsion/engine[0]/'
IGE_HEIGHT_FT = 6.3  # ah1s standing on its skids (reset00)
OGE_HEIGHT_FT = 500.0  # the ah1s ground effect vanishes around 50 ft
COLLECTIVE = 0.5
SETTLE_TIME_S = 10.0  # the inflow lag of the ah1s rotor is 0.09 s
# The governor has not quite brought the rotor to its nominal RPM, which the
# ground effect factor depends on.
RTOL = 1e-4
# FGRotor clamps the factor that scales the inflow in ground effect to [0.5, 1].
MIN_GROUND_EFFECT_FACTOR = 0.5


class TestRotorGroundEffect(JSBSimTestCase):
    def hover(self, dt, h_agl_ft):
        fdm = self.create_fdm()
        # FGRotor reads the time step when the model is loaded.
        fdm.set_dt(dt)
        fdm.load_model('ah1s')
        fdm.load_ic('reset00', True)
        fdm['ic/h-agl-ft'] = h_agl_ft
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()
        fdm['fcs/rpm-governor-active-norm'] = 1.0
        fdm['fcs/collective-cmd-norm'] = COLLECTIVE
        while fdm.get_sim_time() < SETTLE_TIME_S:
            fdm.run()

        nu = fdm[ROTOR + 'induced-inflow-ratio']
        ct = fdm[ROTOR + 'thrust-coefficient']
        mu = fdm[ROTOR + 'advance-ratio']
        lam = fdm[ROTOR + 'inflow-ratio']
        # Inflow over its momentum theory value Ct/(2*sqrt(mu^2+lambda^2))
        ge_factor = nu * 2.0 * math.sqrt(mu*mu + lam*lam) / ct
        return nu, ct, ge_factor

    def check_time_step_independence(self, h_agl_ft):
        nu_ref, ct_ref, ge_factor = self.hover(1/120, h_agl_ft)
        nu, ct, _ = self.hover(1/1000, h_agl_ft)
        self.assertAlmostEqual(nu/nu_ref, 1.0, delta=RTOL)
        self.assertAlmostEqual(ct/ct_ref, 1.0, delta=RTOL)
        return ge_factor

    def test_in_ground_effect(self):
        ge_factor = self.check_time_step_independence(IGE_HEIGHT_FT)
        self.assertLess(ge_factor, 1.0)
        self.assertGreaterEqual(ge_factor, MIN_GROUND_EFFECT_FACTOR)

    def test_out_of_ground_effect(self):
        ge_factor = self.check_time_step_independence(OGE_HEIGHT_FT)
        self.assertAlmostEqual(ge_factor, 1.0, delta=RTOL)

RunTest(TestRotorGroundEffect)
