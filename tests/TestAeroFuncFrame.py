# TestAeroFuncFrame.py
#
# Check that the specification of aerodynamics forces in different frames is
# working as expected
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

import xml.etree.ElementTree as et
import numpy as np
import math
from JSBSim_utils import JSBSimTestCase, RunTest, CreateFDM, CopyAircraftDef


class TestAeroFuncFrame(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)

        self.fdm = CreateFDM(self.sandbox)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'x153.xml')
        self.tree, self.aircraft_name, b = CopyAircraftDef(self.script_path, self.sandbox)

        self.aero2wind = np.matrix(np.identity(3));
        self.aero2wind[0,0] *= -1.0
        self.aero2wind[2,2] *= -1.0
        self.auxilliary = self.fdm.get_auxiliary()

    def tearDown(self):
        del self.fdm
        JSBSimTestCase.tearDown(self)

    def getTs2b(self):
        alpha = self.fdm['aero/alpha-rad']
        ca = math.cos(alpha)
        sa = math.sin(alpha)
        Ts2b = np.matrix([[ca, 0., -sa],
                          [0., 1., 0.],
                          [sa, 0., ca]])
        return Ts2b

    def checkForcesAndMoments(self, getForces, getMoment, aeroFunc):
        self.fdm.load_script(self.script_path)
        self.fdm.run_ic()

        rp = np.matrix([self.fdm['metrics/aero-rp-x-in'],
                        -self.fdm['metrics/aero-rp-y-in'],
                        self.fdm['metrics/aero-rp-z-in']])
        result = {}

        while self.fdm.run():
            for axis in aeroFunc.keys():
                result[axis] = 0.0

                for func in aeroFunc[axis]:
                    result[axis] += self.fdm[func]

            Fa, Fb = getForces(result)
            Tb2s = self.getTs2b().T
            Fs = self.aero2wind * (Tb2s * Fb)

            Mb_MRC = getMoment(result)
            cg = np.matrix([self.fdm['inertia/cg-x-in'],
                            -self.fdm['inertia/cg-y-in'],
                            self.fdm['inertia/cg-z-in']])
            arm_ft = (cg - rp)/12.0 # Convert from inches to ft
            Mb = Mb_MRC + np.cross(arm_ft, Fb.T)
            Tb2w = self.auxilliary.get_Tb2w()
            Mw = Tb2w * Mb.T
            Ms = Tb2s * Mb.T

            self.assertAlmostEqual(Fa[0,0], self.fdm['forces/fwx-aero-lbs'])
            self.assertAlmostEqual(Fa[1,0], self.fdm['forces/fwy-aero-lbs'])
            self.assertAlmostEqual(Fa[2,0], self.fdm['forces/fwz-aero-lbs'])
            self.assertAlmostEqual(Fb[0,0], self.fdm['forces/fbx-aero-lbs'])
            self.assertAlmostEqual(Fb[1,0], self.fdm['forces/fby-aero-lbs'])
            self.assertAlmostEqual(Fb[2,0], self.fdm['forces/fbz-aero-lbs'])
            self.assertAlmostEqual(Fs[0,0], self.fdm['forces/fsx-aero-lbs'])
            self.assertAlmostEqual(Fs[1,0], self.fdm['forces/fsy-aero-lbs'])
            self.assertAlmostEqual(Fs[2,0], self.fdm['forces/fsz-aero-lbs'])
            self.assertAlmostEqual(Mb[0,0], self.fdm['moments/l-aero-lbsft'])
            self.assertAlmostEqual(Mb[0,1], self.fdm['moments/m-aero-lbsft'])
            self.assertAlmostEqual(Mb[0,2], self.fdm['moments/n-aero-lbsft'])
            self.assertAlmostEqual(Ms[0,0], self.fdm['moments/roll-stab-aero-lbsft'])
            self.assertAlmostEqual(Ms[1,0], self.fdm['moments/pitch-stab-aero-lbsft'])
            self.assertAlmostEqual(Ms[2,0], self.fdm['moments/yaw-stab-aero-lbsft'])
            self.assertAlmostEqual(Mw[0,0], self.fdm['moments/roll-wind-aero-lbsft'])
            self.assertAlmostEqual(Mw[1,0], self.fdm['moments/pitch-wind-aero-lbsft'])
            self.assertAlmostEqual(Mw[2,0], self.fdm['moments/yaw-wind-aero-lbsft'])

    def checkAerodynamicsFrame(self, newAxisName, getForces, getMoment, frame):
        aeroFunc = {}

        for axis in self.tree.findall('aerodynamics/axis'):
            axisName = newAxisName[axis.attrib['name']]
            axis.attrib['name'] = axisName
            if frame:
                axis.attrib['frame'] = frame
            aeroFunc[axisName] = []

            for func in axis.findall('function'):
                aeroFunc[axisName].append(func.attrib['name'])

                if (frame == 'BODY' or len(frame) == 0) and (axisName == 'X' or axisName == 'Z'):
                    # Convert the signs of X and Z forces so that the force
                    # along X is directed backward and the force along Z is
                    # directed upward.
                    product_tag = func.find('product')
                    value_tag = et.SubElement(product_tag, 'value')
                    value_tag.text = '-1.0'

        self.tree.write(self.sandbox('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))
        self.fdm.set_aircraft_path('aircraft')

        self.checkForcesAndMoments(getForces, getMoment, aeroFunc)

    def checkBodyFrame(self, frame):
        newAxisName = {'DRAG': 'X', 'SIDE': 'Y', 'LIFT': 'Z',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        def getForces(result):
            Tb2w = self.auxilliary.get_Tb2w()
            Fb = np.matrix([result['X'], result['Y'], result['Z']]).T
            Fw = Tb2w * Fb
            Fa = self.aero2wind * Fw
            return Fa, Fb

        def getMoment(result):
            return np.matrix([result['ROLL'], result['PITCH'], result['YAW']])

        self.checkAerodynamicsFrame(newAxisName, getForces, getMoment, '')

    def testBodyFrame(self):
        self.checkBodyFrame('')

    def testBodyFrameAltSyntax(self):
        self.checkBodyFrame('BODY')

    def testAxialFrame(self):
        newAxisName = {'DRAG': 'AXIAL', 'SIDE': 'SIDE', 'LIFT': 'NORMAL',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        def getForces(result):
            Tb2w = self.auxilliary.get_Tb2w()
            Fnative = np.matrix([result['AXIAL'], result['SIDE'], result['NORMAL']]).T
            Fb = self.aero2wind * Fnative
            Fw = Tb2w * Fb
            Fa = self.aero2wind * Fw
            return Fa, Fb

        def getMoment(result):
            return np.matrix([result['ROLL'], result['PITCH'], result['YAW']])

        self.checkAerodynamicsFrame(newAxisName, getForces, getMoment, '')

    def testWindFrame(self):
        newAxisName = {'DRAG': 'X', 'SIDE': 'Y', 'LIFT': 'Z',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        def getForces(result):
            Tw2b = self.auxilliary.get_Tw2b()
            Fa = np.matrix([result['X'], result['Y'], result['Z']]).T
            Fw = self.aero2wind * Fa
            Fb = Tw2b * Fw
            return Fa, Fb

        def getMoment(result):
            Tw2b = self.auxilliary.get_Tw2b()
            Mw = np.matrix([result['ROLL'], result['PITCH'], result['YAW']]).T
            Mb = Tw2b*Mw
            return Mb.T

        self.checkAerodynamicsFrame(newAxisName, getForces, getMoment, 'WIND')

    def testAeroFrame(self):
        aeroFunc = {}

        for axis in self.tree.findall('aerodynamics/axis'):
            axisName = axis.attrib['name']
            aeroFunc[axisName] = []

            for func in axis.findall('function'):
                aeroFunc[axisName].append(func.attrib['name'])

        def getForces(result):
            Tw2b = self.auxilliary.get_Tw2b()
            Fa = np.matrix([result['DRAG'], result['SIDE'], result['LIFT']]).T
            Fw = self.aero2wind * Fa
            Fb = Tw2b * Fw
            return Fa, Fb

        def getMoment(result):
            return np.matrix([result['ROLL'], result['PITCH'], result['YAW']])

        self.checkForcesAndMoments(getForces, getMoment, aeroFunc)

    def testStabilityFrame(self):
        newAxisName = {'DRAG': 'X', 'SIDE': 'Y', 'LIFT': 'Z',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        def getForces(result):
            Tb2w = self.auxilliary.get_Tb2w()
            Ts2b = self.getTs2b()
            Fs = np.matrix([result['X'], result['Y'], result['Z']]).T
            Fb = Ts2b * (self.aero2wind * Fs)
            Fw = Tb2w * Fb
            Fa = self.aero2wind * Fw
            return Fa, Fb

        def getMoment(result):
            Ts2b = self.getTs2b()
            Ms = np.matrix([result['ROLL'], result['PITCH'], result['YAW']]).T
            Mb = Ts2b*Ms
            return Mb.T

        self.checkAerodynamicsFrame(newAxisName, getForces, getMoment, 'STABILITY')

RunTest(TestAeroFuncFrame)
