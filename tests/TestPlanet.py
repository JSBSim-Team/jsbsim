# TestPlanet.py
#
# Test that the <planet> section is functional.
#
# Copyright (c) 2021 Bertrand Coconnier
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

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel
from jsbsim import GeographicError


class TestPlanet(JSBSimTestCase):
    def test_moon(self):
        tripod = FlightModel(self, 'tripod')
        moon_file = self.sandbox.path_to_jsbsim_file('tests/moon.xml')
        tripod.include_planet_test_file(moon_file)
        self.fdm = tripod.start()
        self.fdm['ic/h-agl-ft'] = 0.2
        self.fdm['ic/long-gc-deg'] = 0.0
        self.fdm['ic/lat-geod-deg'] = 0.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1738100, 1.0)
        self.assertAlmostEqual(self.fdm['accelerations/gravity-ft_sec2']*0.3048, 1.62, delta=3e-3)

        self.fdm['ic/lat-geod-deg'] = 90.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1736000, 1.0)

    def test_load_planet(self):
        tripod = FlightModel(self, 'tripod')
        moon_file = self.sandbox.path_to_jsbsim_file('tests/moon.xml')
        self.fdm = tripod.start()
        self.fdm.load_planet(moon_file, False)
        self.fdm['ic/h-agl-ft'] = 0.2
        self.fdm['ic/long-gc-deg'] = 0.0
        self.fdm['ic/lat-geod-deg'] = 0.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1738100, 1.0)
        self.assertAlmostEqual(self.fdm['accelerations/gravity-ft_sec2']*0.3048, 1.62, delta=3e-3)

        self.fdm['ic/lat-geod-deg'] = 90.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/1736000, 1.0)

    def test_load_MSIS_atmosphere(self):
        tripod = FlightModel(self, 'tripod')
        MSIS_file = self.sandbox.path_to_jsbsim_file('tests/MSIS.xml')
        self.fdm = tripod.start()
        self.fdm.load_planet(MSIS_file, False)
        self.fdm['ic/h-sl-ft'] = 0.0
        self.fdm['ic/long-gc-deg'] = -70.0
        self.fdm['ic/lat-geod-deg'] = 60.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['atmosphere/T-R']*5/9, 281.46476, delta=1E-5)
        self.assertAlmostEqual(self.fdm['atmosphere/rho-slugs_ft3']/0.001940318, 1.263428, delta=1E-6)
        self.assertAlmostEqual(self.fdm['atmosphere/P-psf'], 2132.294, delta=1E-3)

    def test_mars_atmosphere(self):
        # Mars atmosphere via <planet><atmosphere model="Mars"/></planet>.
        # Reference values are the closed-form output of FGMars::Calculate at
        # altitude = 0:
        #   T = -25.68 + 459.67 = 433.99 R         (~241.1 K)
        #   P = 14.62 psf                          (~7 mbar)
        #   rho = P / (Reng * T), Reng = 53.5*44.01 (CO2)
        tripod = FlightModel(self, 'tripod')
        mars_file = self.sandbox.path_to_jsbsim_file('tests/mars.xml')
        tripod.include_planet_test_file(mars_file)
        self.fdm = tripod.start()
        self.fdm['ic/h-agl-ft'] = 0.0
        self.fdm['ic/long-gc-deg'] = 0.0
        self.fdm['ic/lat-geod-deg'] = 0.0
        self.fdm.run_ic()

        # Mars equatorial radius = 3396.2 km
        self.assertAlmostEqual(self.fdm['metrics/terrain-radius']*0.3048/3396200, 1.0)
        # Surface gravity ~3.71-3.72 m/s^2 (JSBSim uses oblate-spheroid gravity,
        # not point-mass GM/R^2, so the value is a few mGal above textbook).
        self.assertAlmostEqual(self.fdm['accelerations/gravity-ft_sec2']*0.3048, 3.72, delta=5E-2)

        self.assertAlmostEqual(self.fdm['atmosphere/T-R'], 433.99, delta=1E-2)
        self.assertAlmostEqual(self.fdm['atmosphere/P-psf'], 14.62, delta=1E-2)
        # rho = 14.62 / (53.5*44.01 * 433.99) ~ 1.4308e-5 slugs/ft^3
        self.assertAlmostEqual(self.fdm['atmosphere/rho-slugs_ft3'], 1.4308e-5, delta=1E-8)

    def test_load_Mars_atmosphere(self):
        # Same as above but using FGFDMExec::LoadPlanet at runtime instead of
        # including the planet file at FDM construction time.
        tripod = FlightModel(self, 'tripod')
        mars_file = self.sandbox.path_to_jsbsim_file('tests/mars.xml')
        self.fdm = tripod.start()
        self.fdm.load_planet(mars_file, False)
        self.fdm['ic/h-agl-ft'] = 0.0
        self.fdm['ic/long-gc-deg'] = 0.0
        self.fdm['ic/lat-geod-deg'] = 0.0
        self.fdm.run_ic()

        self.assertAlmostEqual(self.fdm['atmosphere/T-R'], 433.99, delta=1E-2)
        self.assertAlmostEqual(self.fdm['atmosphere/P-psf'], 14.62, delta=1E-2)
        self.assertAlmostEqual(self.fdm['atmosphere/rho-slugs_ft3'], 1.4308e-5, delta=1E-8)

    def test_planet_geographic_error1(self):
        # Check that a negative equatorial radius raises an exception
        tripod = FlightModel(self, 'tripod')
        moon_file = self.sandbox.path_to_jsbsim_file('tests/moon.xml')
        tree = et.parse(moon_file)
        root = tree.getroot()
        radius_tag = root.find('equatorial_radius')
        radius = float(radius_tag.text)
        radius_tag.text = str(-radius)
        moon_file = self.sandbox('moon.xml')
        tree.write(moon_file)
        tripod.include_planet_test_file(moon_file)
        with self.assertRaises(GeographicError):
            self.fdm = tripod.start()

    def test_planet_geographic_error2(self):
        # Check that a negative polar radius raises an exception
        tripod = FlightModel(self, 'tripod')
        moon_file = self.sandbox.path_to_jsbsim_file('tests/moon.xml')
        tree = et.parse(moon_file)
        root = tree.getroot()
        radius_tag = root.find('polar_radius')
        radius = float(radius_tag.text)
        radius_tag.text = str(-radius)
        moon_file = self.sandbox('moon.xml')
        tree.write(moon_file)
        tripod.include_planet_test_file(moon_file)
        with self.assertRaises(GeographicError):
            self.fdm = tripod.start()

RunTest(TestPlanet)
