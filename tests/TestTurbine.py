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
        tree = et.parse(self.sandbox.path_to_jsbsim_file('engine',
                                                         append_xml(engine_element.attrib['file'])))
        root = tree.getroot()
        idleN1 = float(root.find('idlen1').text)
        maxN1 = float(root.find('maxn1').text)
        idleN2 = float(root.find('idlen2').text)
        maxN2 = float(root.find('maxn2').text)
        BPR = float(root.find('bypassratio').text)
        N1_factor = maxN1 - idleN1
        N2_factor = maxN2 - idleN2

        fdm = CreateFDM(self.sandbox)
        fdm.load_script(script_path)
        fdm.run_ic()
        dt = fdm['simulation/dt']
        delay = 90.0*dt / (BPR + 3.0);

        while fdm.run():
            n1 = fdm['propulsion/engine/n1']
            n2 = fdm['propulsion/engine/n2']
            N2norm = (n2-idleN2)/N2_factor;

            if n2 >= 100.:
                # Trigger the engine spool down
                fdm['fcs/throttle-cmd-norm'] = 0.0

            if N2norm > 0.0:
                self.assertAlmostEqual(n1, newN1)
                self.assertAlmostEqual(n2, newN2)

            if  n2 > 15.0:
                sigma = fdm['atmosphere/sigma']
                n = min(1.0, N2norm + 0.1)
                spoolup = delay / (1 + 3 * (1-n)*(1-n)*(1-n) + (1 - sigma))
                throttlePos = fdm['fcs/throttle-cmd-norm']
                targetN1 = idleN1+throttlePos*N1_factor
                targetN2 = idleN2+throttlePos*N2_factor
                newN1 = seek(n1, targetN1, spoolup, spoolup*2.4)
                newN2 = seek(n2, targetN2, spoolup, spoolup*3.0)

        del fdm

RunTest(TestTurbine)
