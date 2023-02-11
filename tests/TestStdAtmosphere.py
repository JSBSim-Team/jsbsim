# TestStdAtmosphere.py
#
# Test that the standard atmosphere model of JSBSim is compliant with the
# International Standard Atmosphere document (1976)
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

import math

from JSBSim_utils import JSBSimTestCase, RunTest
from jsbsim import eTemperature, ePressure

def compute_pressure(P, L, dH, T_K, factor):
    if abs(L) > 1E-8:
        return P * math.pow(1+L*dH/T_K, -factor/L)
    else:
        return P * math.exp(-factor * dH / T_K)

class TestStdAtmosphere(JSBSimTestCase):
    def setUp(self, *args):
        JSBSimTestCase.setUp(self, *args)

        # ISA lapse table
        self.ISA_temperature = [
            ( 0.0,     0.0),
            (11.0,    -6.5),
            (20.0,     0.0),
            (32.0,     1.0),
            (47.0,     2.8),
            (51.0,     0.0),
            (71.0,    -2.8),
            (84.8520, -2.0)
        ]

        # ISA constants in SI units
        self.T0 = 288.15 # K
        self.R_earth = 6356.766 # km
        self.P0 = 101325 # Pa
        self.g0 = 9.80665 # m/s^2
        self.Mair = 28.9645 # g/mol
        self.Mwater = 18.016 # g/mol
        self.Rstar = 8.31432 # J/K/mol
        self.gamma = 1.4
        self.Reng = self.Rstar*1000/self.Mair

        # Conversion factors between SI and British units
        self.K_to_R = 1.8
        self.km_to_ft = 1000/0.3048
        self.Pa_to_psf = 1.0/47.88 # From src/FGJSBBase.cpp
        self.kg_to_slug = 0.06852168 # From src/FGJSBBase.cpp
        self.m_to_ft = self.km_to_ft/1000

        # Gradient fade out altitude
        self.gradient_fade_out_h = 91.0 # km

    def geometric_altitude(self, h_geopot):
        return (self.R_earth*h_geopot)/(self.R_earth-h_geopot)

    def check_temperature(self, fdm, T0, T_gradient):
        self.assertAlmostEqual(1.0, T0*self.K_to_R/fdm['atmosphere/T-sl-R'])
        a0 = math.sqrt(self.gamma*self.Reng*T0)
        self.assertAlmostEqual(1.0, a0*self.m_to_ft/fdm['atmosphere/a-sl-fps'])

        T_K = T0
        h = self.ISA_temperature[0][0]

        for alt, L in self.ISA_temperature:
            dH = alt-h

            # Check half way of the interval to make sure the temperature is
            # linearly interpolated
            T_half = T_K + 0.5*(L-T_gradient)*dH

            fdm['ic/h-sl-ft'] = self.geometric_altitude(h+0.5*dH) * self.km_to_ft
            fdm.run_ic()
            self.assertAlmostEqual(1.0, T_half*self.K_to_R/fdm['atmosphere/T-R'])

            # Check the temperature breakpoints
            T_K += (L-T_gradient)*dH

            fdm['ic/h-sl-ft'] = self.geometric_altitude(alt) * self.km_to_ft
            fdm.run_ic()
            self.assertAlmostEqual(1.0, T_K*self.K_to_R/fdm['atmosphere/T-R'])

            h = alt

        # Check that the temperature gradient has no influence beyond a
        # geopotential altitude of 91 km
        dH = self.gradient_fade_out_h - h
        T_K -= T_gradient*dH
        fdm['ic/h-sl-ft'] = self.geometric_altitude(self.gradient_fade_out_h) * self.km_to_ft
        fdm.run_ic()
        self.assertAlmostEqual(1.0, T_K*self.K_to_R/fdm['atmosphere/T-R'])

        fdm['ic/h-sl-ft'] = 100.0 * self.km_to_ft
        fdm.run_ic()
        print("T_K={}\tT-R={}".format(T_K, fdm['atmosphere/T-R']/self.K_to_R))
        self.assertAlmostEqual(1.0, T_K*self.K_to_R/fdm['atmosphere/T-R'])

        # Check negative altitudes (Dead Sea)
        h = self.ISA_temperature[0][0]
        L = self.ISA_temperature[1][1]
        alt = -1.5
        T_K = T0+(L-T_gradient)*(alt-h)

        fdm['ic/h-sl-ft'] = self.geometric_altitude(alt) * self.km_to_ft
        fdm.run_ic()
        self.assertAlmostEqual(1.0, T_K*self.K_to_R/fdm['atmosphere/T-R'])

    def check_pressure(self, fdm, P0, T0, T_gradient):
        self.assertAlmostEqual(1.0, P0*self.Pa_to_psf/fdm['atmosphere/P-sl-psf'])
        rho0 = P0/(self.Reng*T0)
        self.assertAlmostEqual(rho0*self.kg_to_slug/math.pow(self.m_to_ft,3),
                               fdm['atmosphere/rho-sl-slugs_ft3'])
        a0 = math.sqrt(self.gamma*self.Reng*T0)
        self.assertAlmostEqual(1.0, a0*self.m_to_ft/fdm['atmosphere/a-sl-fps'])

        P_Pa = P0
        T_K = T0
        h = self.ISA_temperature[0][0]
        factor = self.g0 * self.Mair / self.Rstar

        for alt, L in self.ISA_temperature:
            dH = alt-h

            P_half = compute_pressure(P_Pa, L-T_gradient, 0.5*dH, T_K, factor)
            fdm['ic/h-sl-ft'] = self.geometric_altitude(h+0.5*dH) * self.km_to_ft
            fdm.run_ic()
            self.assertAlmostEqual(1.0, P_half*self.Pa_to_psf/fdm['atmosphere/P-psf'])

            P_Pa = compute_pressure(P_Pa, L-T_gradient, dH, T_K, factor)
            fdm['ic/h-sl-ft'] = self.geometric_altitude(alt) * self.km_to_ft
            fdm.run_ic()
            self.assertAlmostEqual(1.0, P_Pa*self.Pa_to_psf/fdm['atmosphere/P-psf'])

            h = alt
            T_K += (L-T_gradient)*dH

        # Check negative altitudes (Dead Sea)
        L = self.ISA_temperature[1][1]
        alt = -1.5
        P_Pa = compute_pressure(P0, L-T_gradient, alt, T0, factor)

        fdm['ic/h-sl-ft'] = self.geometric_altitude(alt) * self.km_to_ft
        fdm.run_ic()
        self.assertAlmostEqual(1.0, P_Pa*self.Pa_to_psf/fdm['atmosphere/P-psf'])

    def test_std_atmosphere(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        self.check_temperature(fdm, self.T0, 0.0)
        self.check_pressure(fdm, self.P0, self.T0, 0.0)

    def test_temperature_bias(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R

        self.check_temperature(fdm, T_sl, 0.0)
        self.check_pressure(fdm, self.P0, T_sl, 0.0)

    def test_sl_pressure_bias(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf

        self.check_temperature(fdm, self.T0, 0.0)
        self.check_pressure(fdm, P_sl, self.T0, 0.0)

    def test_pressure_and_temperature_bias(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R
        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf

        self.check_temperature(fdm, T_sl, 0.0)
        self.check_pressure(fdm, P_sl, T_sl, 0.0)

    def test_temperature_gradient(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        T_gradient = graded_delta_T_K / self.gradient_fade_out_h
        self.assertAlmostEqual(T_gradient*self.K_to_R/self.km_to_ft,
                               fdm['atmosphere/SL-graded-delta-T'])

        self.check_temperature(fdm, self.T0 + graded_delta_T_K, T_gradient)
        self.check_pressure(fdm, self.P0, self.T0 + graded_delta_T_K, T_gradient)

    def test_temperature_gradient_and_bias(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R
        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        T_gradient = graded_delta_T_K / self.gradient_fade_out_h
        self.assertAlmostEqual(T_gradient*self.K_to_R/self.km_to_ft,
                               fdm['atmosphere/SL-graded-delta-T'])

        self.check_temperature(fdm, T_sl + graded_delta_T_K, T_gradient)
        self.check_pressure(fdm, self.P0, T_sl + graded_delta_T_K, T_gradient)

    def test_pressure_and_temperature_gradient(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf
        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        T_gradient = graded_delta_T_K / self.gradient_fade_out_h
        self.assertAlmostEqual(T_gradient*self.K_to_R/self.km_to_ft,
                               fdm['atmosphere/SL-graded-delta-T'])

        self.check_temperature(fdm, self.T0 + graded_delta_T_K, T_gradient)
        self.check_pressure(fdm, P_sl, self.T0 + graded_delta_T_K, T_gradient)

    def test_pressure_and_temperature_gradient_and_bias(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')

        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf
        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R
        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        T_gradient = graded_delta_T_K / self.gradient_fade_out_h
        self.assertAlmostEqual(T_gradient*self.K_to_R/self.km_to_ft,
                               fdm['atmosphere/SL-graded-delta-T'])

        self.check_temperature(fdm, T_sl + graded_delta_T_K, T_gradient)
        self.check_pressure(fdm, P_sl, T_sl + graded_delta_T_K, T_gradient)

    def test_set_pressure_SL(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm.run_ic()

        atmos = fdm.get_atmosphere()

        atmos.set_pressure_SL(ePressure.eInchesHg, 29.92)
        fdm.run_ic()
        self.assertAlmostEqual(2115.8849626, fdm['atmosphere/P-psf'])

    def test_set_temperature(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm.run_ic()

        atmos = fdm.get_atmosphere()

        # Check that there are no side effects if we call SetTemperature()
        # twice in a row.
        atmos.set_temperature(520, 0.0, eTemperature.eRankine)
        fdm.run_ic()
        self.assertAlmostEqual(1.0, fdm['atmosphere/T-R']/520.0)

        atmos.set_temperature(500, 0.0, eTemperature.eRankine)
        fdm.run_ic()
        self.assertAlmostEqual(1.0, fdm['atmosphere/T-R']/500.0)

        # Regression test for a bug reported in FlightGear. Checks that the
        # temperature bias is updated correctly when the temperature is forced
        # to a constant value.

        for alt in range(5000):
            h = alt*1000
            fdm['atmosphere/delta-T'] = 0.0 # Make sure there is no temperature bias
            atmos.set_temperature(354, h, eTemperature.eRankine)
            self.assertAlmostEqual(1.0, atmos.get_temperature(h)/354.0,
                                   msg='\nFailed at h={} ft'.format(h))

        # Check that it works while a temperature gradient is set
        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        atmos.set_temperature(530, 1000.0, eTemperature.eRankine)
        fdm['ic/h-sl-ft'] = 1000.
        fdm.run_ic()

        self.assertAlmostEqual(1.0, fdm['atmosphere/T-R']/530.0)

    def test_humidity_parameters(self):
        # Table: Dew point (deg C), Vapor pressure (Pa), RH, density
        humidity_table = [
            (-40.0,   19.021201,     0.815452, 1.2040321),
            (-30.0,   51.168875,     2.193645, 1.2038877),
            (-20.0,  125.965126,    5.4002118, 1.2035517),
            (-10.0,  287.031031,   12.3052182, 1.2028282),
            (  0.0,  611.2,        26.2025655, 1.2013721),
            ( 10.0, 1226.030206,   52.5607604, 1.1986102),
            ( 20.0, 2332.5960221, 100.,        1.1936395)
        ]

        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm['atmosphere/delta-T'] = 5.0*self.K_to_R
        fdm.run_ic()

        Psat = fdm['atmosphere/saturated-vapor-pressure-psf']/self.Pa_to_psf

        self.assertAlmostEqual(Psat, humidity_table[-1][1])
        self.assertAlmostEqual(fdm['atmosphere/vapor-pressure-psf'], 0.0)
        self.assertAlmostEqual(fdm['atmosphere/RH'], 0.0)
        self.assertAlmostEqual(fdm['atmosphere/dew-point-R'], 54.054)

        for Tdp, Pv, RH, rho in humidity_table:
            dew_point_R = (Tdp+273.15)*self.K_to_R
            fdm['atmosphere/dew-point-R'] = dew_point_R
            fdm.run_ic()
            self.assertAlmostEqual(fdm['atmosphere/dew-point-R'], dew_point_R)
            self.assertAlmostEqual(fdm['atmosphere/vapor-pressure-psf'],
                                   Pv*self.Pa_to_psf)
            self.assertAlmostEqual(fdm['atmosphere/RH'], RH)
            self.assertAlmostEqual(fdm['atmosphere/rho-slugs_ft3']/(self.kg_to_slug*math.pow(0.3048,3)),
                                   rho)

    def test_max_HR(self):
        max_water_ppm = [(  0.0, 35000.),
                         (  1.0, 31000.),
                         (  2.0, 28000.),
                         (  4.0, 22000.),
                         (  6.0,  8900.),
                         (  8.0,  4700.),
                         ( 10.0,  1300.),
                         ( 12.0,   230.),
                         ( 14.0,    48.),
                         ( 16.0,    38.),
                         (100.0,    38.)]

        fdm = self.create_fdm()
        fdm.load_model('ball')
        # Choose an ambient temperature high enough that the HR cannot reach 100%
        fdm['atmosphere/delta-T'] = 40.0*self.K_to_R

        for (h, ppm) in max_water_ppm:
            fdm['ic/h-sl-ft'] = self.geometric_altitude(h)*self.km_to_ft
            fdm.run_ic()

            fdm['atmosphere/RH'] = 100.
            fdm.run()

            Pv = fdm['atmosphere/vapor-pressure-psf']/self.Pa_to_psf
            P = fdm['atmosphere/P-psf']/self.Pa_to_psf
            T = fdm['atmosphere/T-R']/self.K_to_R
            rhov = Pv*self.Mwater/(self.Rstar*1000.*T)
            rhoa = (P-Pv)/(self.Reng*T)

            self.assertAlmostEqual(rhov/rhoa, ppm*1E-6)
            self.assertAlmostEqual(fdm['atmosphere/vapor-fraction-ppm']/ppm, 1.0)

    def test_temperature_lower_limit(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm['ic/h-sl-ft'] = 500000.
        fdm.run_ic()

        fdm['atmosphere/delta-T'] = -4000.
        fdm.run()
        self.assertAlmostEqual(fdm['atmosphere/T-R'], 1.8)

        fdm['atmosphere/delta-T'] = 0.0
        fdm['atmosphere/SL-graded-delta-T'] = -4000.
        fdm.run()
        self.assertAlmostEqual(fdm['atmosphere/T-sl-R'], fdm['atmosphere/T-R'])

        fdm['atmosphere/delta-T'] = -4000.
        fdm.run()
        self.assertAlmostEqual(fdm['atmosphere/T-sl-R'], 1.8)

    def test_pressure_lower_limit(self):
        fdm = self.create_fdm()
        fdm.load_model('ball')
        fdm['ic/h-sl-ft'] = 1E7
        fdm.run_ic()
        self.assertAlmostEqual(fdm['atmosphere/P-psf']*1E17, 2.08854342)
        self.assertGreater(fdm['atmosphere/pressure-altitude'], 900000.)
        self.assertLess(fdm['atmosphere/pressure-altitude'], fdm['ic/h-sl-ft'])

        fdm['atmosphere/P-sl-psf'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['atmosphere/P-sl-psf']*1E17, 2.08854342)
        self.assertAlmostEqual(fdm['atmosphere/P-psf']*1E17, 2.08854342)


RunTest(TestStdAtmosphere)
