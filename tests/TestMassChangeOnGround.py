# TestMassChangeOnGround.py
#
# Check that the structure of an aircraft resting on the ground stays put when
# its CG moves.
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

import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, RunTest

PAYLOAD_LBS = 1000.0  # lowers the CG of the 3000 lbs tripod by 0.25 ft
PAYLOAD_Z_FT = -1.0
# tripod.xml's 1 slug*ft2 is too stiff for a time step of 1/120 s
INERTIA_SLUG_FT2 = 1000.0
SETTLE_STEPS = 240
# The structure moves 0.25 ft when the CG move is not compensated
TOLERANCE_FT = 0.01


class TestMassChangeOnGround(JSBSimTestCase):
    def structure_jump(self, contact_type, cg_z):
        tree = et.parse(self.sandbox.path_to_jsbsim_file('tests', 'tripod.xml'))
        root = tree.getroot()
        for contact in root.findall('ground_reactions/contact'):
            contact.attrib['type'] = contact_type
        mass_balance = root.find('mass_balance')
        for axis in ('ixx', 'iyy', 'izz'):
            mass_balance.find(axis).text = str(INERTIA_SLUG_FT2)
        cg = et.SubElement(mass_balance, 'location', name='CG', unit='FT')
        for axis, value in zip('xyz', (0.0, 0.0, cg_z)):
            et.SubElement(cg, axis).text = str(value)
        payload = mass_balance.find('pointmass/location')
        payload.find('x').text = '0.0'
        payload.find('z').text = str(PAYLOAD_Z_FT)
        tree.write('tripod.xml')

        fdm = self.create_fdm()
        fdm.set_aircraft_path('.')
        fdm.load_model('tripod', False)
        # The contacts start compressed by the tripod's weight
        fdm['ic/h-agl-ft'] = 0.1
        fdm.run_ic()
        for _ in range(SETTLE_STEPS):
            fdm.run()

        def structure_height():
            return fdm['position/h-agl-ft'] - fdm['inertia/cg-z-in'] / 12.0

        before = structure_height()
        fdm['inertia/pointmass-weight-lbs'] = PAYLOAD_LBS
        fdm.run()
        return structure_height() - before

    def test_cg_at_structural_origin(self):
        self.assertAlmostEqual(self.structure_jump('BOGEY', 0.0), 0.0,
                               delta=TOLERANCE_FT)


RunTest(TestMassChangeOnGround)
