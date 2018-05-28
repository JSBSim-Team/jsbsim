# TestTemplateFucntions.py
#
# Check template functions
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
import numpy as np
import pandas as pd
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest, ExecuteUntil


class TestTemplateFunctions(JSBSimTestCase):
    def testUnitConversion(self):
        shutil.copy(self.sandbox.path_to_jsbsim_file('tests',
                                                     'output2.xml'),'.')
        shutil.copy(self.sandbox.path_to_jsbsim_file('scripts',
                                                     'unitconversions.xml'),'.')
        script_path = self.sandbox.path_to_jsbsim_file('scripts', 'c1723.xml')
        fdm = CreateFDM(self.sandbox)
        fdm.set_output_directive('output2.xml')
        fdm.load_script(script_path)
        fdm.run_ic()
        ExecuteUntil(fdm, 10.)

        ref = pd.read_csv("output2.csv", index_col=0)
        self.assertAlmostEqual(np.abs(ref['/fdm/jsbsim/aero/alpha-deg']-
                                      ref['template/alpha-deg']).max(), 0.0)
        self.assertAlmostEqual(np.abs(ref['pre/p-aero-deg_sec']-
                                      ref['template/p-aero-deg_sec']).max(), 0.0)

RunTest(TestTemplateFunctions)
