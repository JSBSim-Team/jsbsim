# CheckScripts.py
#
# A regression test that checks that all the scripts can be read by JSBSim
# without issuing errors.
#
# Copyright (c) 2015 Bertrand Coconnier
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

import fpectl
from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class CheckScripts(JSBSimTestCase):
    def testScripts(self):
        fpectl.turnon_sigfpe()

        for s in self.script_list():
            fdm = CreateFDM(self.sandbox)
            self.assertTrue(fdm.load_script(s),
                            msg="Failed to load script %s" % (s,))
            fdm.run_ic()

            del fdm

RunTest(CheckScripts)
