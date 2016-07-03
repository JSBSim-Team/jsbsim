# TestPitotAngle.py
#
# Test the Pitot angle feature
#
# Copyright (c) 2016 Bertrand Coconnier
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

import os, math
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, CreateFDM, CopyAircraftDef, append_xml, ExecuteUntil, RunTest


class TestPitotAngle(JSBSimTestCase):
    def addPitotTube(self, root, angle):
        metrics_tag = root.find('./metrics')
        pitot_tag = et.SubElement(metrics_tag, 'pitot_angle')
        pitot_tag.attrib['unit'] = 'DEG'
        pitot_tag.text = str(angle)

    def test_CAS_ic(self):
        script_name = 'Short_S23_3.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)

        # Add a Pitot angle to the Short S23
        tree, aircraft_name, path_to_jsbsim_aircrafts = CopyAircraftDef(script_path, self.sandbox)
        self.addPitotTube(tree.getroot(), 5.0)
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        # Read the CAS specified in the IC file
        tree = et.parse(script_path)
        use_element = tree.getroot().find('use')
        IC_file = use_element.attrib['initialize']
        tree = et.parse(os.path.join(path_to_jsbsim_aircrafts,
                                     append_xml(IC_file)))
        vc_tag = tree.getroot().find('./vc')
        VCAS = float(vc_tag.text)
        if 'unit' in vc_tag.attrib and vc_tag.attrib['unit'] == 'FT/SEC':
            VCAS /= 1.68781  # Converts in kts

        # Run the IC and check that the model is initialized correctly
        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        self.assertAlmostEqual(fdm['ic/vc-kts'], VCAS, delta=1E-7)
        self.assertAlmostEqual(fdm['velocities/vc-kts'], VCAS, delta=1E-7)

    def test_pitot_angle(self):
        script_name = 'ball_chute.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)

        # Add a Pitot angle to the Cessna 172
        tree, aircraft_name, path_to_jsbsim_aircrafts = CopyAircraftDef(script_path, self.sandbox)
        root = tree.getroot()
        pitot_angle_deg = 5.0
        self.addPitotTube(root, 5.0)
        contact_tag = root.find('./ground_reactions/contact')
        contact_tag.attrib['type'] = 'STRUCTURE'
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_model('ball')
        pitot_angle = pitot_angle_deg * math.pi / 180.
        weight = fdm['inertia/weight-lbs']
        spring_tag = contact_tag.find('./spring_coeff')
        spring_coeff = float(spring_tag.text)
        print "Weight=%d Spring=%d" % (weight, spring_coeff)
        fdm['ic/h-sl-ft'] = weight / spring_coeff
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()

        ExecuteUntil(fdm, 10.)

        for i in xrange(36):
            for j in xrange(-9, 10):
                angle = math.pi * i / 18.0
                angle2 = math.pi * j / 18.0
                ca2 = math.cos(angle2)
                fdm['atmosphere/wind-north-fps'] = 10. * math.cos(angle) * ca2
                fdm['atmosphere/wind-east-fps'] = 10. * math.sin(angle) * ca2
                fdm['atmosphere/wind-down-fps'] = 10. * math.sin(angle2)
                fdm.run()

                vg = fdm['velocities/vg-fps']
                self.assertAlmostEqual(vg, 0.0, delta=1E-7)

                vt = fdm['velocities/vt-fps']
                self.assertAlmostEqual(vt, 10., delta=1E-7)

                mach = vt / fdm['atmosphere/a-fps']
                P = fdm['atmosphere/P-psf']
                pt = P * math.pow(1+0.2*mach*mach, 3.5)
                psl = fdm['atmosphere/P-sl-psf']
                rhosl = fdm['atmosphere/rho-sl-slugs_ft3']
                A = math.pow((pt-P)/psl+1.0, 1.0/3.5)
                alpha = fdm['aero/alpha-rad']
                beta = fdm['aero/beta-rad']
                vc = math.sqrt(7.0*psl/rhosl*(A-1.0))*math.cos(alpha+pitot_angle)*math.cos(beta)

                self.assertAlmostEqual(fdm['velocities/vc-kts'],
                                       max(0.0, vc) / 1.68781, delta=1E-7)

    # Check that the VCAS is correctly updated when the initial altitude is
    # modified and the aircraft has a tilted Pitot tube.
    def test_alt_mod_vs_CAS(self):
        script_name = 'Short_S23_3.xml'
        script_path = self.sandbox.path_to_jsbsim_file('scripts', script_name)

        # Add a Pitot angle to the Short S23
        tree, aircraft_name, b = CopyAircraftDef(script_path, self.sandbox)
        self.addPitotTube(tree.getroot(), 10.0)
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))
        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_model('Short_S23')
        fdm['ic/beta-deg'] = 15.0  # Add some sideslip
        fdm['ic/vc-kts'] = 172.0
        fdm['ic/h-sl-ft'] = 15000.
        self.assertAlmostEqual(fdm['ic/vc-kts'], 172.0, delta=1E-7)
        self.assertAlmostEqual(fdm['ic/beta-deg'], 15.0, delta=1E-7)
        fdm.run_ic()
        self.assertAlmostEqual(fdm['velocities/vc-kts'], 172.0, delta=1E-7)
        self.assertAlmostEqual(fdm['aero/beta-deg'], 15.0, delta=1E-7)

RunTest(TestPitotAngle)
