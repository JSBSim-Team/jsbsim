# TestWheelSpin.py
#
# Checks the optional wheel rotational degree of freedom of BOGEY contacts
# (<wheel_radius> and <wheel_inertia>).
#
# Copyright (c) 2026 Felipegalind0
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
import shutil
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, RunTest

# Estimated C172 tire/wheel assemblies (radius in m, spin inertia in kg*m^2).
WHEELS = {'NOSE': (0.18, 0.052), 'LEFT_MAIN': (0.22, 0.124),
          'RIGHT_MAIN': (0.22, 0.124)}
FT_PER_M = 1.0 / 0.3048
SLUGFT2_PER_KGM2 = 1.0 / 1.35594
APPROACH = {'ic/h-agl-ft': 6.0, 'ic/vc-kts': 55.0, 'ic/theta-deg': 3.0,
            'ic/vd-fps': 2.0}
CRUISE = {'ic/h-agl-ft': 1500.0, 'ic/vc-kts': 95.0, 'ic/theta-deg': 1.0}


def hover(**rates):
    """Airborne, no airspeed, level, with the given body rates in rad/s."""
    ic = {'ic/h-agl-ft': 1500.0, 'ic/vc-kts': 0.0, 'ic/theta-deg': 0.0,
          'ic/phi-deg': 0.0}
    for axis, rate in rates.items():
        ic['ic/%s-rad_sec' % axis] = rate
    return ic


class TestWheelSpin(JSBSimTestCase):
    def load_c172p(self, wheels=None, ic=APPROACH, max_steer=None):
        """Loads a sandboxed copy of c172p, optionally with wheel spin DOFs
        and with the max_steer of some contacts changed (in degrees)."""
        aircraft = self.sandbox('aircraft')
        c172_dir = os.path.join(aircraft, 'c172p')
        if not os.path.exists(c172_dir):
            shutil.copytree(self.sandbox.path_to_jsbsim_file('aircraft', 'c172p'),
                            c172_dir)
        xml_file = os.path.join(c172_dir, 'c172p.xml')
        tree = et.parse(os.path.join(
            self.sandbox.path_to_jsbsim_file('aircraft', 'c172p'), 'c172p.xml'))
        for contact in tree.getroot().iter('contact'):
            name = contact.attrib.get('name')
            if contact.attrib.get('type') == 'BOGEY' and wheels and name in wheels:
                radius, inertia = wheels[name]
                et.SubElement(contact, 'wheel_radius', unit='M').text = str(radius)
                et.SubElement(contact, 'wheel_inertia', unit='KG*M2').text = str(inertia)
            if max_steer and name in max_steer:
                contact.find('max_steer').text = str(max_steer[name])
        tree.write(xml_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path(aircraft)
        self.assertTrue(fdm.load_model('c172p'))
        fdm['ic/lat-geod-deg'] = 34.0
        fdm['ic/long-gc-deg'] = -118.0
        for name, value in ic.items():
            fdm[name] = value
        self.assertTrue(fdm.run_ic())
        return fdm

    def landing(self, wheels):
        """14 s: touchdown, rollout, full brakes from 9 s."""
        fdm = self.load_c172p(wheels)
        spin = fdm.get_property_manager().hasNode('gear/unit[1]/wheel-spin-rad_sec')
        rows = []
        for step in range(1680):
            brake = 1.0 if step >= 1080 else 0.0
            fdm['fcs/left-brake-cmd-norm'] = brake
            fdm['fcs/right-brake-cmd-norm'] = brake
            self.assertTrue(fdm.run())
            row = {'u': fdm['velocities/u-fps'], 'q': fdm['velocities/q-rad_sec'],
                   'wow': fdm['gear/unit[1]/WOW']}
            if spin:
                for unit in range(3):
                    row['spin%d' % unit] = fdm['gear/unit[%d]/wheel-spin-rad_sec' % unit]
                row['slip'] = fdm['gear/unit[1]/wheel-tread-slip-fps']
            rows.append(row)
        mass = fdm['inertia/weight-lbs'] / 32.174
        self.delete_fdm()
        return rows, mass

    def test_opt_in(self):
        fdm = self.load_c172p()
        pm = fdm.get_property_manager()
        self.assertFalse(pm.hasNode('gear/unit[0]/wheel-spin-rad_sec'))
        self.assertFalse(pm.hasNode('gear/unit[1]/wheel-tread-slip-fps'))
        self.delete_fdm()

        # Invalid values disable the DOF of that contact only.
        fdm = self.load_c172p({'NOSE': (0.18, 0.052), 'LEFT_MAIN': (-0.22, 0.124)})
        pm = fdm.get_property_manager()
        self.assertTrue(pm.hasNode('gear/unit[0]/wheel-spin-rad_sec'))
        self.assertFalse(pm.hasNode('gear/unit[1]/wheel-spin-rad_sec'))
        self.assertFalse(pm.hasNode('gear/unit[2]/wheel-spin-rad_sec'))

    def test_touchdown_spin_up(self):
        legacy, _ = self.landing(None)
        rows, mass = self.landing(WHEELS)
        touch = next(i for i, row in enumerate(rows) if row['wow'] > 0.5)
        self.assertGreater(touch, 10, 'The approach must start airborne')
        for row in rows[:touch]:
            self.assertEqual(row['spin1'], 0.0)  # No spin-up without contact
        self.assertGreater(rows[touch]['slip'], 80.0)

        settled = next(i for i in range(touch + 1, len(rows))
                       if abs(rows[i]['slip']) < 0.01)
        self.assertLessEqual(settled - touch, 12)
        radius_ft = 0.22 * FT_PER_M
        for row in rows[touch:]:
            self.assertGreaterEqual(row['spin1'], 0.0)
            self.assertLessEqual(row['spin1'] * radius_ft, row['u'] * 1.02 + 0.5)

        # The wheels' angular momentum comes out of the aircraft's momentum.
        # That momentum uses the spin state in the ground frame, whereas the
        # property reports spin relative to the airframe. On this wings-level
        # rollout the unsteered wheels roll on level ground, so every spin axis
        # is body -Y (up to the small bank and sideslip of the rollout) and the
        # ground-frame spin is the reported spin minus q.
        after = settled + 24
        lost = legacy[after]['u'] - rows[after]['u']
        wheel_momentum = 0.0
        for unit, name in enumerate(('NOSE', 'LEFT_MAIN', 'RIGHT_MAIN')):
            radius, inertia = WHEELS[name]
            spin = rows[after]['spin%d' % unit] - rows[after]['q']
            wheel_momentum += inertia * SLUGFT2_PER_KGM2 * spin / (radius * FT_PER_M)
        predicted = wheel_momentum / mass
        self.assertGreater(lost, 0.7 * predicted)
        self.assertLess(lost, 1.6 * predicted)

        max_q = max(abs(row['q']) for row in rows)
        max_q_legacy = max(abs(row['q']) for row in legacy)
        self.assertLess(max_q, 2.0 * max_q_legacy + 0.05)

    def test_rolling_resistance_and_braking_match_legacy(self):
        legacy, _ = self.landing(None)
        rows, _ = self.landing(WHEELS)

        def drop(data, start, end):
            return data[start]['u'] - data[end]['u']

        for start, end in ((240, 1000), (1100, 1400)):
            self.assertAlmostEqual(drop(rows, start, end) / drop(legacy, start, end),
                                   1.0, delta=0.05)
        # Anti-skid equivalence: a braked wheel keeps rolling.
        for row in rows[1100:1400]:
            self.assertLess(abs(row['slip']), 1.0)

    def test_ground_start_rolls_without_slip(self):
        fdm = self.load_c172p(WHEELS, {'ic/h-agl-ft': 0.5, 'ic/vc-kts': 12.0,
                                       'ic/theta-deg': 0.0})
        self.assertTrue(fdm.run())
        self.assertEqual(fdm['gear/unit[1]/WOW'], 1.0)
        self.assertLess(abs(fdm['gear/unit[1]/wheel-tread-slip-fps']), 0.05)
        self.assertGreater(fdm['gear/unit[1]/wheel-spin-rad_sec'], 20.0)

    def test_airborne_spin_down(self):
        fdm = self.load_c172p(WHEELS, CRUISE)
        q_start = fdm['velocities/q-rad_sec']
        fdm['gear/unit[1]/wheel-spin-rad_sec'] = 100.0
        fdm['gear/unit[2]/wheel-spin-rad_sec'] = 100.0
        fdm['fcs/right-brake-cmd-norm'] = 1.0
        for _ in range(120):  # 1 s
            self.assertTrue(fdm.run())
        # Drag removes 13 ft/s^2 of tread speed from the spin relative to the
        # airframe. The spin state itself does not follow the airframe, so the
        # change in pitch rate about the body -Y axle adds to the relative spin.
        expected = (100.0 - 13.0 / (0.22 * FT_PER_M)
                    + fdm['velocities/q-rad_sec'] - q_start)
        self.assertAlmostEqual(fdm['gear/unit[1]/wheel-spin-rad_sec'], expected,
                               delta=0.2)
        self.assertEqual(fdm['gear/unit[2]/wheel-spin-rad_sec'], 0.0)

    def test_airborne_brake_holds_wheel_to_airframe(self):
        spin = 'gear/unit[1]/wheel-spin-rad_sec'
        for q in (0.4, -0.4):
            fdm = self.load_c172p(WHEELS, hover(q=q))
            self.assertAlmostEqual(fdm['velocities/q-rad_sec'], q, delta=1e-12)
            # RunIC leaves the spin state at zero, so relative to the pitching
            # airframe (the axle is body -Y) the wheel turns at +q.
            self.assertAlmostEqual(fdm[spin], q, delta=1e-12)
            fdm[spin] = 0.0
            self.assertAlmostEqual(fdm[spin], 0.0, delta=1e-12)
            # A braked wheel stays at rest relative to the airframe.
            fdm['fcs/left-brake-cmd-norm'] = 1.0
            for _ in range(120):
                self.assertTrue(fdm.run())
                self.assertEqual(fdm['gear/unit[1]/WOW'], 0.0)
                self.assertLess(abs(fdm[spin]), 1e-10)
            self.delete_fdm()

    def test_run_ic_keeps_spin_state(self):
        spin = 'gear/unit[1]/wheel-spin-rad_sec'
        fdm = self.load_c172p(WHEELS, hover(q=0.4))
        fdm[spin] = 0.7  # Spin state 0.7 - 0.4 = 0.3 rad/s
        # RunIC keeps the gear state (and must not damp it during its dt = 0
        # passes) while the airframe rate about the axle becomes -q = 0.2.
        fdm['ic/q-rad_sec'] = -0.2
        self.assertTrue(fdm.run_ic())
        self.assertAlmostEqual(fdm['velocities/q-rad_sec'], -0.2, delta=1e-12)
        self.assertAlmostEqual(fdm[spin], 0.3 - 0.2, delta=1e-12)
        # A reset to the initial conditions zeroes the spin state.
        fdm.reset_to_initial_conditions(0)
        self.assertAlmostEqual(fdm['velocities/q-rad_sec'], -0.2, delta=1e-12)
        self.assertAlmostEqual(fdm[spin], -0.2, delta=1e-12)

    def test_airborne_drag_acts_on_relative_spin(self):
        spin = 'gear/unit[1]/wheel-spin-rad_sec'
        radius_ft = 0.22 * FT_PER_M

        def first_step(seed, brake):
            fdm = self.load_c172p(WHEELS, CRUISE)
            for rate in ('p', 'q', 'r'):
                self.assertEqual(fdm['velocities/%s-rad_sec' % rate], 0.0)
            dt = fdm.get_delta_t()
            self.assertEqual(dt, 1.0 / 120.0)
            q_before = fdm['velocities/q-rad_sec']
            fdm[spin] = seed
            fdm['fcs/left-brake-cmd-norm'] = brake
            self.assertTrue(fdm.run())
            self.assertEqual(fdm['gear/unit[1]/WOW'], 0.0)
            # The spin state keeps its value until it is damped, so the change
            # of pitch rate about the body -Y axle adds to the relative spin.
            relative = seed + fdm['velocities/q-rad_sec'] - q_before
            decrement = (13.0 + 100.0 * brake) * dt / radius_ft
            result = fdm[spin]
            self.delete_fdm()
            return result, math.copysign(max(0.0, abs(relative) - decrement),
                                         relative)

        for brake in (0.0, 0.5, 1.0):
            for seed in (100.0, -100.0):
                result, expected = first_step(seed, brake)
                self.assertAlmostEqual(result, expected, delta=1e-8)
                self.assertGreater(result * seed, 0.0)

        # Drag stops the wheel relative to the airframe without reversing it.
        result, _ = first_step(0.01, 1.0)
        self.assertGreaterEqual(result, 0.0)
        self.assertLess(abs(result), 1e-10)

    def test_airborne_axle_follows_steering(self):
        spin = 'gear/unit[0]/wheel-spin-rad_sec'
        fdm = self.load_c172p(WHEELS, hover(p=0.3, q=0.4), max_steer={'NOSE': 90})
        p = fdm['velocities/p-rad_sec']
        q = fdm['velocities/q-rad_sec']
        self.assertAlmostEqual(p, 0.3, delta=1e-12)
        self.assertAlmostEqual(q, 0.4, delta=1e-12)
        # The spin state is zero after RunIC. Unsteered, the axle is body -Y.
        self.assertAlmostEqual(fdm[spin], q, delta=1e-12)
        # Steered 90 degrees, the axle (gear up x wheel forward) is body +X.
        # Checked before any time step can re-center the steering.
        fdm['fcs/steer-pos-deg[0]'] = 90.0
        self.assertAlmostEqual(fdm['fcs/steer-pos-deg[0]'], 90.0, delta=1e-12)
        self.assertAlmostEqual(fdm[spin], -p, delta=1e-12)


RunTest(TestWheelSpin)
