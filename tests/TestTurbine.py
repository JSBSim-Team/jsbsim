# TestTurbine.py
#
# Regression tests for the turbine engine model.
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

import shutil
import xml.etree.ElementTree as et
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, append_xml


def seek(x, target, accel, decel):
    if x < target:
        x += accel
        return min(x, target)
    elif x > target:
        x -= decel
        return max(x, target)
    else:
        return x

class TestTurbine(JSBSimTestCase):
    def defaultSpoolUp(self, N2norm):
        sigma = self.fdm['atmosphere/sigma']
        n = min(1.0, N2norm + 0.1)
        return self.delay / (1 + 3 * (1-n)*(1-n)*(1-n) + (1 - sigma))

    def runScript(self, n1SpoolUp, n1SpoolDown, n2SpoolUp, n2SpoolDown):
        while self.fdm.run():
            n1 = self.fdm['propulsion/engine/n1']
            n2 = self.fdm['propulsion/engine/n2']
            N2norm = (n2-self.idleN2)/self.N2_factor;

            if n2 >= 100.:
                # Trigger the engine spool down
                self.fdm['fcs/throttle-cmd-norm'] = 0.0

            if N2norm > 0.0:
                self.assertAlmostEqual(n1, newN1)
                self.assertAlmostEqual(n2, newN2)

            if  n2 > 15.0:
                sigma = self.fdm['atmosphere/sigma']
                n = min(1.0, N2norm + 0.1)
                spoolup = self.delay / (1 + 3 * (1-n)*(1-n)*(1-n) + (1 - sigma))
                throttlePos = self.fdm['fcs/throttle-cmd-norm']
                targetN1 = self.idleN1+throttlePos*self.N1_factor
                targetN2 = self.idleN2+throttlePos*self.N2_factor
                newN1 = seek(n1, targetN1, n1SpoolUp(N2norm),
                             n1SpoolDown(N2norm))
                newN2 = seek(n2, targetN2, n2SpoolUp(N2norm),
                             n2SpoolDown(N2norm))

            if N2norm == 0.0 and self.fdm['fcs/throttle-cmd-norm'] == 0.0:
                break

    def testSpoolUp(self):
        # Check that the same results are obtained whether the N1 & N2 spool up
        # are specified via the legacy <bypassratio> parameter or a <function>
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       'f16_test.xml')
        tree = et.parse(script_path)
        use_element = tree.getroot().find('use')
        aircraft_name = use_element.attrib['aircraft']
        tree = et.parse(self.sandbox.path_to_jsbsim_file('aircraft',
                                                         aircraft_name,
                                                         aircraft_name+'.xml'))

        engine_element = tree.getroot().find('propulsion/engine')
        engine_name = append_xml(engine_element.attrib['file'])
        tree = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         engine_name))
        root = tree.getroot()
        self.idleN1 = float(root.find('idlen1').text)
        maxN1 = float(root.find('maxn1').text)
        self.idleN2 = float(root.find('idlen2').text)
        maxN2 = float(root.find('maxn2').text)
        BPR = float(root.find('bypassratio').text)
        self.N1_factor = maxN1 - self.idleN1
        self.N2_factor = maxN2 - self.idleN2

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.load_script(script_path)
        self.fdm.run_ic()
        self.dt = self.fdm['simulation/dt']
        self.delay = 90.0*self.dt / (BPR + 3.0);

        self.runScript(self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*2.4,
                       self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*3.0)

        del self.fdm

        # Check N1 spool up custom function
        # Append a <function name="N1SpoolUp"> to the engine definition
        func_spoolUpDown = et.SubElement(root, 'function')
        func_spoolUpDown.attrib['name'] = 'N1SpoolUp'
        func_body = et.parse(self.sandbox.path_to_jsbsim_file('tests',
                                                              'N1SpoolUp.xml'))
        func_spoolUpDown.append(func_body.getroot())
        tree.write(engine_name)
        shutil.copy(self.sandbox.path_to_jsbsim_file('engine','direct.xml'),
                    '.')

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.set_engine_path('.')
        self.fdm.load_script(script_path)
        self.fdm.run_ic()

        self.runScript(lambda n2Norm: self.fdm['propulsion/engine/n1']*self.dt/20.0,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*2.4,
                       self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*3.0)

        del self.fdm

        # Check N1 spool down custom function
        func_spoolUpDown.attrib['name'] = 'N1SpoolDown'
        tree.write(engine_name)

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.set_engine_path('.')
        self.fdm.load_script(script_path)
        self.fdm.run_ic()

        self.runScript(self.defaultSpoolUp,
                       lambda n2Norm: self.fdm['propulsion/engine/n1']*self.dt/20.0,
                       self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*3.0)

        del self.fdm

        # Check N2 spool up custom function
        func_spoolUpDown.attrib['name'] = 'N2SpoolUp'
        tree.write(engine_name)

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.set_engine_path('.')
        self.fdm.load_script(script_path)
        self.fdm.run_ic()

        self.runScript(self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*2.4,
                       lambda n2Norm: self.fdm['propulsion/engine/n1']*self.dt/20.0,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*3.0)

        del self.fdm

        # Check N2 spool down custom function
        func_spoolUpDown.attrib['name'] = 'N2SpoolDown'
        tree.write(engine_name)

        self.fdm = CreateFDM(self.sandbox)
        self.fdm.set_engine_path('.')
        self.fdm.load_script(script_path)
        self.fdm.run_ic()

        self.runScript(self.defaultSpoolUp,
                       lambda n2Norm: self.defaultSpoolUp(n2Norm)*2.4,
                       self.defaultSpoolUp,
                       lambda n2Norm: self.fdm['propulsion/engine/n1']*self.dt/20.0)

RunTest(TestTurbine)
