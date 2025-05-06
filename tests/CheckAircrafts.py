# CheckAircrafts.py
#
# A regression test that checks that all the aircrafts can be read by JSBSim
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

import os
from JSBSim_utils import JSBSimTestCase, CheckXMLFile, RunTest


class CheckAircrafts(JSBSimTestCase):
    def testAircrafts(self):
        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft')
        for d in os.listdir(aircraft_path):
            fullpath = os.path.join(aircraft_path, d)

            # Is d a directory ?
            if not os.path.isdir(fullpath):
                continue

            f = os.path.join(aircraft_path, d, d+'.xml')

            # Is f an aircraft definition file ?
            if not CheckXMLFile(f, 'fdm_config'):
                continue

            if d in ('blank'):
                continue

            fdm = self.create_fdm()
            self.assertTrue(fdm.load_model(d),
                            msg='Failed to load aircraft %s' % (d,))

            for f in os.listdir(fullpath):
                f = os.path.join(aircraft_path, d, f)
                if CheckXMLFile(f, 'initialize'):
                    self.assertTrue(fdm.load_ic(f, False),
                                    msg='Failed to load IC %s for aircraft %s' % (f, d))
                    try:
                        fdm.run_ic()
                    except RuntimeError:
                        self.fail('Failed to run IC %s for aircraft %s' % (f, d))

    def test_aircraft_name(self):
        fdm = self.create_fdm()
        fdm.load_model('c172x')
        aircraft = fdm.get_aircraft()
        self.assertEqual(aircraft.get_aircraft_name(), "Cessna C-172 Skyhawk II")


RunTest(CheckAircrafts)
