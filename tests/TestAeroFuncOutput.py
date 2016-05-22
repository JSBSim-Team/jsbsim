# TestAeroFuncOutput.py
#
# Check that the aerodynamics forces are consistent with the aerodynamics
# functions output
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

import pandas as pd
from JSBSim_utils import JSBSimTestCase, RunTest, CreateFDM


class TestAeroFuncOutput(JSBSimTestCase):
    def testDragFunctions(self):
        fdm = CreateFDM(self.sandbox)
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'x153.xml')
        fdm.load_script(self.script_path)
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        fdm.run_ic()

        while fdm.run():
            pass

        results = pd.read_csv('output.csv', index_col=0)
        Fdrag = results['F_{Drag} (lbs)']
        CDmin = results['aero/coefficient/CDmin']
        CDi = results['aero/coefficient/CDi']
        self.assertAlmostEqual(abs(Fdrag/(CDmin+CDi)).max(), 1.0, delta=1E-5)

RunTest(TestAeroFuncOutput)
