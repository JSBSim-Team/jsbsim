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
from JSBSim_utils import JSBSimTestCase, RunTest, CreateFDM, CopyAircraftDef


class TestAeroFuncFrame(JSBSimTestCase):
    def setUp(self):
        JSBSimTestCase.setUp(self)

        self.fdm = CreateFDM(self.sandbox)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'x153.xml')
        self.tree, self.aircraft_name, b = CopyAircraftDef(self.script_path, self.sandbox)

        self.aero2wind = np.mat(np.identity(3));
        self.aero2wind[0,0] *= -1.0
        self.aero2wind[2,2] *= -1.0
        self.auxilliary = self.fdm.get_auxiliary()

    def tearDown(self):
        del self.fdm
        JSBSimTestCase.tearDown(self)

    def checkForcesAndMoments(self, getForces, aeroFunc):
        self.fdm.load_script(self.script_path)
        self.fdm.run_ic()

        rp = np.mat([self.fdm['metrics/aero-rp-x-in'],
                     -self.fdm['metrics/aero-rp-y-in'],
                     self.fdm['metrics/aero-rp-z-in']])
        result = {}

        while self.fdm.run():
            for axis in aeroFunc.keys():
                result[axis] = 0.0

                for func in aeroFunc[axis]:
                    result[axis] += self.fdm[func]

            Fa, Fb = getForces(result)

            Mb_MRC = np.mat([result['ROLL'], result['PITCH'], result['YAW']])
            cg = np.mat([self.fdm['inertia/cg-x-in'],
                         -self.fdm['inertia/cg-y-in'],
                         self.fdm['inertia/cg-z-in']])
            arm_ft = (cg - rp)/12.0 # Convert from inches to ft
            Mb = Mb_MRC + np.cross(arm_ft, Fb.T)

            self.assertAlmostEqual(Fa[0,0], self.fdm['forces/fwx-aero-lbs'])
            self.assertAlmostEqual(Fa[1,0], self.fdm['forces/fwy-aero-lbs'])
            self.assertAlmostEqual(Fa[2,0], self.fdm['forces/fwz-aero-lbs'])
            self.assertAlmostEqual(Fb[0,0], self.fdm['forces/fbx-aero-lbs'])
            self.assertAlmostEqual(Fb[1,0], self.fdm['forces/fby-aero-lbs'])
            self.assertAlmostEqual(Fb[2,0], self.fdm['forces/fbz-aero-lbs'])
            self.assertAlmostEqual(Mb[0,0], self.fdm['moments/l-aero-lbsft'])
            self.assertAlmostEqual(Mb[0,1], self.fdm['moments/m-aero-lbsft'])
            self.assertAlmostEqual(Mb[0,2], self.fdm['moments/n-aero-lbsft'])

    def testAeroFrame(self):
        aeroFunc = {}

        for axis in self.tree.findall('aerodynamics/axis'):
            axisName = axis.attrib['name']
            aeroFunc[axisName] = []

            for func in axis.findall('function'):
                aeroFunc[axisName].append(func.attrib['name'])

        def getForces(result):
            Tw2b = self.auxilliary.get_Tw2b()
            Fa = np.mat([result['DRAG'], result['SIDE'], result['LIFT']]).T
            Fw = self.aero2wind * Fa
            Fb = Tw2b * Fw
            return Fa, Fb

        self.checkForcesAndMoments(getForces, aeroFunc)

    def testBodyFrame(self):
        aeroFunc = {}
        newAxisName = {'DRAG': 'X', 'SIDE': 'Y', 'LIFT': 'Z',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        for axis in self.tree.findall('aerodynamics/axis'):
            axisName = newAxisName[axis.attrib['name']]
            axis.attrib['name'] = axisName
            aeroFunc[axisName] = []

            for func in axis.findall('function'):
                aeroFunc[axisName].append(func.attrib['name'])

                if axisName == 'X' or axisName == 'Z':
                    # Convert the signs of X and Z forces so that the force
                    # along X is directed backward and the force along Z is
                    # directed upward.
                    product_tag = func.find('product')
                    value_tag = et.SubElement(product_tag, 'value')
                    value_tag.text = '-1.0'

        self.tree.write(self.sandbox('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))
        self.fdm.set_aircraft_path('aircraft')

        def getForces(result):
            Tb2w = self.auxilliary.get_Tb2w()
            Fb = np.mat([result['X'], result['Y'], result['Z']]).T
            Fw = Tb2w * Fb
            Fa = self.aero2wind * Fw
            return Fa, Fb

        self.checkForcesAndMoments(getForces, aeroFunc)

    def testAxialFrame(self):
        aeroFunc = {}
        newAxisName = {'DRAG': 'AXIAL', 'SIDE': 'SIDE', 'LIFT': 'NORMAL',
                       'ROLL': 'ROLL', 'PITCH': 'PITCH', 'YAW': 'YAW'}

        for axis in self.tree.findall('aerodynamics/axis'):
            axisName = newAxisName[axis.attrib['name']]
            axis.attrib['name'] = axisName
            aeroFunc[axisName] = []

            for func in axis.findall('function'):
                aeroFunc[axisName].append(func.attrib['name'])

        self.tree.write(self.sandbox('aircraft', self.aircraft_name,
                                     self.aircraft_name+'.xml'))
        self.fdm.set_aircraft_path('aircraft')

        def getForces(result):
            Tb2w = self.auxilliary.get_Tb2w()
            Fnative = np.mat([result['AXIAL'], result['SIDE'], result['NORMAL']]).T
            Fa = Tb2w * Fnative
            Fw = self.aero2wind * Fa
            Fb = self.aero2wind * Fnative
            return Fa, Fb

        self.checkForcesAndMoments(getForces, aeroFunc)

RunTest(TestAeroFuncFrame)
