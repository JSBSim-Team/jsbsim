# TestTumblingBody.py
#
# Check that the implicit midpoint rule keeps the kinetic energy and the
# magnitude of the angular momentum of a body tumbling with no moment, from
# flight rates to the spin a strike gives a wreck, and that it applies the
# moment. Stepped by explicit Euler, the energy and momentum grow until they
# overflow.
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
import os
import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest

DT = 1/240
# FGPropagate::eRectEuler and eImplicitMidpoint
EULER = 1
IMPLICIT_MIDPOINT = 10
SECONDS = 10.0
# The direction NASA's tumbling brick case (Atmos_02) starts spinning about.
AXIS = (10.0, 20.0, 30.0)
# Up to 540 rad/s, a wreck spun by a strike: 2.25 rad a step.
SPINS_RAD_S = (1.7, 17.0, 52.0, 540.0)
# NASA's brick, the same brick with products of inertia, and a rod whose roll
# inertia is about 1/72000 of its pitch and yaw inertia (slug*ft2). Chosen: the
# products are smaller than the brick's moments row by row, so the inertia stays
# positive definite, and the rod is slender enough for Newton's corrections to
# meet rounding before 1E-12.
MODELS = {'brick': {},
          'tilted_brick': {'ixy': 0.0005, 'ixz': 0.001, 'iyz': 0.0008},
          'rod': {'ixx': 1E-7, 'iyy': 0.007194665, 'izz': 0.007194665}}
# Chosen: far above the rounding left once Newton has converged, far below the
# 1E-2 that explicit Euler drifts at 1.7 rad/s.
DRIFT_PER_STEP = 2E-12
# Chosen: spins the brick from rest to 53 rad/s about its roll axis in SECONDS.
ROLL_MOMENT_LBSFT = 0.01
# Chosen: far above the 3E-12 rad/s measured off after SECONDS, far below the
# 53 rad/s a missing moment would leave.
SPIN_UP_DELTA_RAD_S = 1E-6
INERTIAL_RATES = ('velocities/pi-rad_sec', 'velocities/qi-rad_sec',
                  'velocities/ri-rad_sec')


class TestTumblingBody(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self, 'check_cases', 'NASA_EOM_2015')
        for name, inertia in MODELS.items():
            tree = et.parse(self.sandbox.path_to_jsbsim_file('aircraft', 'brick',
                                                             'brick.xml'))
            mass_balance = tree.getroot().find('mass_balance')
            for tag, value in inertia.items():
                element = mass_balance.find(tag)
                if element is None:
                    element = et.SubElement(mass_balance, tag, unit='SLUG*FT2')
                element.text = str(value)
            reactions = et.SubElement(tree.getroot(), 'external_reactions')
            moment = et.SubElement(reactions, 'moment', name='roll', frame='BODY')
            direction = et.SubElement(moment, 'direction')
            for axis, value in zip('xyz', (1.0, 0.0, 0.0)):
                et.SubElement(direction, axis).text = str(value)
            os.makedirs(self.sandbox('aircraft', name))
            tree.write(self.sandbox('aircraft', name, name+'.xml'))

    def start(self, model, rates, roll_moment=0.0):
        """The model initialized with the body rates and roll moment given."""
        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.set_dt(DT)
        fdm.load_model(model)
        fdm['simulation/integrator/rate/rotational'] = IMPLICIT_MIDPOINT
        fdm['external_reactions/roll/magnitude-lbsft'] = roll_moment
        for prop, rate in zip(('ic/p-rad_sec', 'ic/q-rad_sec', 'ic/r-rad_sec'),
                              rates):
            fdm[prop] = rate
        fdm.run_ic()
        return fdm

    def inertia(self, fdm):
        """The inertia matrix, signed as FGMassBalance's getters undo it."""
        i = {a: fdm[f'inertia/i{a}-slugs_ft2']
             for a in ('xx', 'yy', 'zz', 'xy', 'xz', 'yz')}
        return ((i['xx'], -i['xy'], i['xz']),
                (-i['xy'], i['yy'], -i['yz']),
                (i['xz'], -i['yz'], i['zz']))

    def invariants(self, fdm):
        """Kinetic energy (ft*lbf) and |J*w| (slug*ft2/s)."""
        w = [fdm[p] for p in INERTIAL_RATES]
        jw = [sum(j*v for j, v in zip(row, w)) for row in self.inertia(fdm)]
        energy = 0.5*sum(a*b for a, b in zip(w, jw))
        momentum = math.sqrt(sum(a*a for a in jw))
        return energy, momentum

    def tumble(self, model, spin):
        """Energy and |J*w| after SECONDS, each relative to the start."""
        norm = math.sqrt(sum(a*a for a in AXIS))
        fdm = self.start(model, [spin*a/norm for a in AXIS])
        energy, momentum = self.invariants(fdm)
        for _ in range(round(SECONDS/DT)):
            fdm.run()
        energy_end, momentum_end = self.invariants(fdm)
        return energy_end/energy, momentum_end/momentum

    def test_keeps_energy_and_momentum(self):
        drift = round(SECONDS/DT)*DRIFT_PER_STEP
        for model in MODELS:
            for spin in SPINS_RAD_S:
                with self.subTest(model=model, spin=spin):
                    energy, momentum = self.tumble(model, spin)
                    self.assertAlmostEqual(energy, 1.0, delta=drift)
                    self.assertAlmostEqual(momentum, 1.0, delta=drift)

    def test_spins_up_under_a_moment(self):
        fdm = self.start('brick', (0.0, 0.0, 0.0), ROLL_MOMENT_LBSFT)
        roll_rate = fdm['velocities/pi-rad_sec']
        steps = round(SECONDS/DT)
        for _ in range(steps):
            fdm.run()
        spin_up = ROLL_MOMENT_LBSFT*steps*DT/fdm['inertia/ixx-slugs_ft2']
        self.assertAlmostEqual(fdm['velocities/pi-rad_sec'] - roll_rate, spin_up,
                               delta=SPIN_UP_DELTA_RAD_S)

    def test_steps_like_euler_before_run_ic(self):
        # Before RunIC FGMassBalance has not run: the inertia is still zero.
        rates = {}
        for scheme in (EULER, IMPLICIT_MIDPOINT):
            fdm = self.create_fdm()
            fdm.set_aircraft_path('aircraft')
            fdm.set_dt(DT)
            fdm.load_model('brick')
            fdm['simulation/integrator/rate/rotational'] = scheme
            fdm.run()
            rates[scheme] = [fdm[p] for p in INERTIAL_RATES]
        self.assertEqual(rates[IMPLICIT_MIDPOINT], rates[EULER])


RunTest(TestTumblingBody)
