# TestTurbulenceRandomSeed.py
#
# Check that identical wind outputs are achieved over multiple runs with the same
# atmosphere/randomseed.
#
# Copyright (c) 2025 Sean McLeod
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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest


class TestTurbulenceRandomSeed(JSBSimTestCase):

    def testTurbulenceRandomSeed(self):
        # Test that the wind turbulence random seed is reproducible as
        # FGFDMExec seed is changed between runs.
        wind_random_seed = 2
        wn1, we1, wd1 = self.captureTurbulence(wind_random_seed, 4)
        wn2, we2, wd2 = self.captureTurbulence(wind_random_seed, 5)
        for i in range(len(wn1)):
            self.assertAlmostEqual(wn1[i], wn2[i], delta=1E-8)
            self.assertAlmostEqual(we1[i], we2[i], delta=1E-8)
            self.assertAlmostEqual(wd1[i], wd2[i], delta=1E-8)

    def captureTurbulence(self, wind_seed, exec_seed):
        fdm = self.create_fdm()

        # Set random seeds for FGFDMExec and FGWinds
        fdm["simulation/randomseed"] = exec_seed
        fdm["atmosphere/randomseed"] = wind_seed

        fdm.load_model('A4') 

        # Set engine running
        fdm['propulsion/engine[0]/set-running'] = 1

        fdm['ic/h-sl-ft'] = 20000
        fdm['ic/vc-kts'] = 250
        fdm['ic/gamma-deg'] = 0

        fdm.run_ic()

        fdm['simulation/do_simple_trim'] = 1

        # Setup turbulence
        fdm["atmosphere/turb-type"] = 3
        fdm["atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps"] = 75
        fdm["atmosphere/turbulence/milspec/severity"] = 6

        wn = []
        we = []
        wd = []

        while fdm.get_sim_time() < 0.15:
            fdm.run()
            wn.append(fdm['atmosphere/total-wind-north-fps'])
            we.append(fdm['atmosphere/total-wind-east-fps'])
            wd.append(fdm['atmosphere/total-wind-down-fps'])

        return (wn, we, wd)

    def testUnassignedSeed(self):
        # Test that if no random seed is assigned for turbulence that the seed
        # for turbulence is the same as the FGFDMExec seed.
        fdm = self.create_fdm()

        # Set a random seed for FGFDMExec
        exec_seed = 47
        fdm["simulation/randomseed"] = exec_seed

        wind_seed = fdm["atmosphere/randomseed"]

        self.assertEqual(exec_seed, wind_seed)

RunTest(TestTurbulenceRandomSeed)