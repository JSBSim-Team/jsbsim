# TestExternalReactions.py
#
# Regression test to check that external reactions are working correctly.
#
# Copyright (c) 2017 Bertrand Coconnier
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

import numpy as np
import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, CopyAircraftDef

def getParachuteArea(tree):
    parachute_area = 1.0
    for value in tree.getroot().findall('external_reactions/force/function/product/value'):
        parachute_area *= float(value.text)
    return parachute_area

class TestExternalReactions(JSBSimTestCase):
    def getLeverArm(self, fdm, name):
        lax = (fdm['external_reactions/'+name+'/location-x-in']
               - fdm['inertia/cg-x-in'])
        lay = (fdm['external_reactions/'+name+'/location-y-in']
               - fdm['inertia/cg-y-in'])
        laz = (fdm['external_reactions/'+name+'/location-z-in']
               - fdm['inertia/cg-z-in'])
        # Convert from inches in the structural frame to feet in the body frame.
        return np.array([-lax, lay, -laz])/12

    def test_wind_frame(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       'ball_chute.xml')
        fdm = CreateFDM(self.sandbox)

        fdm.load_script(script_path)
        fdm.run_ic()

        self.assertAlmostEqual(fdm['external_reactions/parachute/location-x-in'],
                               12.0)
        self.assertAlmostEqual(fdm['external_reactions/parachute/location-y-in'],
                               0.0)
        self.assertAlmostEqual(fdm['external_reactions/parachute/location-z-in'],
                               0.0)
        self.assertAlmostEqual(fdm['external_reactions/parachute/x'], -1.0)
        self.assertAlmostEqual(fdm['external_reactions/parachute/y'], 0.0)
        self.assertAlmostEqual(fdm['external_reactions/parachute/z'], 0.0)

        tree, _, _ = CopyAircraftDef(script_path, self.sandbox)
        parachute_area = getParachuteArea(tree)

        while fdm.run():
            Tw2b = fdm.get_auxiliary().get_Tw2b()
            mag = fdm['aero/qbar-psf'] * fdm['fcs/parachute_reef_pos_norm']*parachute_area
            f = Tw2b * np.matrix([-1.0, 0.0, 0.0]).T * mag
            self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], f[0, 0])
            self.assertAlmostEqual(fdm['forces/fby-external-lbs'], f[1, 0])
            self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], f[2, 0])

            m = np.cross(self.getLeverArm(fdm,'parachute'),
                         np.array([f[0,0], f[1,0], f[2, 0]]))
            self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0])
            self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1])
            self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2])

    def test_body_frame(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('f16')
        fdm.load_ic('reset00.xml', True)
        fdm.run_ic()

        self.assertAlmostEqual(fdm['external_reactions/pushback/location-x-in'],
                               -2.98081)
        self.assertAlmostEqual(fdm['external_reactions/pushback/location-y-in'],
                               0.0)
        self.assertAlmostEqual(fdm['external_reactions/pushback/location-z-in'],
                               -1.9683)
        self.assertAlmostEqual(fdm['external_reactions/pushback/x'], 1.0)
        self.assertAlmostEqual(fdm['external_reactions/pushback/y'], 0.0)
        self.assertAlmostEqual(fdm['external_reactions/pushback/z'], 0.0)
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'],
                               0.0)

        self.assertAlmostEqual(fdm['external_reactions/hook/location-x-in'],
                               100.669)
        self.assertAlmostEqual(fdm['external_reactions/hook/location-y-in'],
                               0.0)
        self.assertAlmostEqual(fdm['external_reactions/hook/location-z-in'],
                               -28.818)
        dx = -0.9995
        dz = 0.01
        fhook = np.array([dx, 0.0, dz])
        fhook /= np.linalg.norm(fhook)

        self.assertAlmostEqual(fdm['external_reactions/hook/x'], fhook[0])
        self.assertAlmostEqual(fdm['external_reactions/hook/y'], fhook[1])
        self.assertAlmostEqual(fdm['external_reactions/hook/z'], fhook[2])
        self.assertAlmostEqual(fdm['external_reactions/hook/magnitude'], 0.0)

        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], 0.0)
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], 0.0)
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], 0.0)

        # Check the 'pushback' external force alone
        fdm['/sim/model/pushback/position-norm'] = 1.0
        fdm['/sim/model/pushback/target-speed-fps'] = 1.0
        fdm['/sim/model/pushback/kp'] = 0.05
        fdm.run()

        fpb = np.array([1.0, 0.0, 0.0]) * 0.05
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'],
                               0.05)
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], fpb[0])
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], fpb[1])
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], fpb[2])

        m = np.cross(self.getLeverArm(fdm, 'pushback'), fpb)
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0])
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1])
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2])

        # Reset the 'pushback' external force to zero
        fdm['/sim/model/pushback/position-norm'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'], 0.0)
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], 0.0)
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], 0.0)
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], 0.0)
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], 0.0)

        # Check the 'hook' external force alone
        fdm['external_reactions/hook/magnitude'] = 10.0
        fhook *= 10.0
        fdm.run()
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], fhook[0])
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], fhook[1])
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], fhook[2])

        m = np.cross(self.getLeverArm(fdm, 'hook'), fhook)
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0])
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1])
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2])

        # Add the 'pushback' force to the hook force and check that the global
        # external forces is the sum of the push back force and the hook force.
        fdm['/sim/model/pushback/position-norm'] = 1.0
        fdm.run()
        fp = fdm['systems/pushback/force']
        fpb = np.array([1.0, 0.0, 0.0]) * fp
        f = fhook + fpb
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'], fp)
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], f[0])
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], f[1])
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], f[2])

        # Modify the push back force direction and check that the global external
        # force is modified accordingly.
        fdm['external_reactions/pushback/x'] = 1.5
        fdm['external_reactions/pushback/y'] = 0.1
        fdm.run()
        fp = fdm['systems/pushback/force']
        fpb = np.array([1.5, 0.1, 0.0]) * fp
        f = fhook + fpb
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'], fp)
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], f[0])
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], f[1])
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], f[2])

        m = (np.cross(self.getLeverArm(fdm, 'pushback'), fpb)
             + np.cross(self.getLeverArm(fdm, 'hook'), fhook))
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0])
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1])
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2])

        fdm['external_reactions/hook/location-y-in'] = 50.0
        fdm.run()
        fp = fdm['systems/pushback/force']
        fpb = np.array([1.5, 0.1, 0.0]) * fp
        f = fhook + fpb
        self.assertAlmostEqual(fdm['external_reactions/pushback/magnitude'], fp)
        self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], f[0])
        self.assertAlmostEqual(fdm['forces/fby-external-lbs'], f[1])
        self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], f[2])

        m = (np.cross(self.getLeverArm(fdm, 'pushback'), fpb)
             + np.cross(self.getLeverArm(fdm, 'hook'), fhook))
        self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0])
        self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1])
        self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2])

    def test_moment(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       'ball_chute.xml')
        tree, aircraft_name, _ = CopyAircraftDef(script_path, self.sandbox)
        extReact_element = tree.getroot().find('external_reactions')
        moment_element = et.SubElement(extReact_element, 'moment')
        moment_element.attrib['name'] = 'parachute'
        moment_element.attrib['frame'] = 'WIND'
        direction_element = et.SubElement(moment_element, 'direction')
        x_element = et.SubElement(direction_element, 'x')
        x_element.text = '0.2'
        y_element = et.SubElement(direction_element, 'y')
        y_element.text = '0.0'
        z_element = et.SubElement(direction_element, 'z')
        z_element.text = '-1.5'

        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))

        fdm = CreateFDM(self.sandbox)
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(script_path)
        fdm.run_ic()

        mDir = np.array([0.2, 0.0, -1.5])
        mDir /= np.linalg.norm(mDir)
        self.assertAlmostEqual(fdm['external_reactions/parachute/l'], mDir[0])
        self.assertAlmostEqual(fdm['external_reactions/parachute/m'], mDir[1])
        self.assertAlmostEqual(fdm['external_reactions/parachute/n'], mDir[2])

        fdm['external_reactions/parachute/magnitude-lbsft'] = -3.5
        parachute_area = getParachuteArea(tree)

        while fdm.run():
            Tw2b = fdm.get_auxiliary().get_Tw2b()
            mag = fdm['aero/qbar-psf'] * fdm['fcs/parachute_reef_pos_norm']*parachute_area
            f = Tw2b * np.matrix([-1.0, 0.0, 0.0]).T * mag
            self.assertAlmostEqual(fdm['forces/fbx-external-lbs'], f[0, 0])
            self.assertAlmostEqual(fdm['forces/fby-external-lbs'], f[1, 0])
            self.assertAlmostEqual(fdm['forces/fbz-external-lbs'], f[2, 0])

            m = -3.5 * Tw2b * np.matrix(mDir).T
            fm = np.cross(self.getLeverArm(fdm,'parachute'),
                          np.array([f[0,0], f[1,0], f[2, 0]]))
            self.assertAlmostEqual(fdm['moments/l-external-lbsft'], m[0, 0] + fm[0])
            self.assertAlmostEqual(fdm['moments/m-external-lbsft'], m[1, 0] + fm[1])
            self.assertAlmostEqual(fdm['moments/n-external-lbsft'], m[2, 0] + fm[2])

RunTest(TestExternalReactions)
