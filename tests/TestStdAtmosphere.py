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

from JSBSim_utils import JSBSimTestCase, CreateFDM, RunTest

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
        self.Rstar = 8.31432 # J/K/mol

        # Conversion factors between SI and British units
        self.K_to_R = 1.8
        self.km_to_ft = 1000/0.3048
        self.Pa_to_psf = 1.0/47.88 # From src/FGJSBBase.cpp

        # Gradient fade out altitude
        self.gradient_fade_out_h = 91.0 # km

    def geometric_altitude(self, h_geopot):
        return (self.R_earth*h_geopot)/(self.R_earth-h_geopot)

    def check_temperature(self, fdm, T0, T_gradient):
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
        # geometric altitude of 91 km
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

    def test_std_atmosphere(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('ball')

        self.check_temperature(fdm, self.T0, 0.0)
        self.check_pressure(fdm, self.P0, self.T0, 0.0)

        del fdm

    def test_temperature_bias(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('ball')

        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R

        self.check_temperature(fdm, T_sl, 0.0)
        self.check_pressure(fdm, self.P0, T_sl, 0.0)

        del fdm

    def test_sl_pressure_bias(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('ball')

        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf

        self.check_temperature(fdm, self.T0, 0.0)
        self.check_pressure(fdm, P_sl, self.T0, 0.0)

        del fdm

    def test_pressure_and_temperature_bias(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('ball')

        delta_T_K = 15.0
        T_sl = self.T0 + delta_T_K
        fdm['atmosphere/delta-T'] = delta_T_K*self.K_to_R
        P_sl = 95000.
        fdm['atmosphere/P-sl-psf'] = P_sl*self.Pa_to_psf

        self.check_temperature(fdm, T_sl, 0.0)
        self.check_pressure(fdm, P_sl, T_sl, 0.0)

        del fdm

    def test_temperature_gradient(self):
        fdm = CreateFDM(self.sandbox)
        fdm.load_model('ball')

        graded_delta_T_K = -10.0
        fdm['atmosphere/SL-graded-delta-T'] = graded_delta_T_K*self.K_to_R

        T_gradient = graded_delta_T_K / self.gradient_fade_out_h
        self.assertAlmostEqual(T_gradient*self.K_to_R/self.km_to_ft,
                               fdm['atmosphere/SL-graded-delta-T'])

        self.check_temperature(fdm, self.T0 + graded_delta_T_K, T_gradient)
        self.check_pressure(fdm, self.P0, self.T0 + graded_delta_T_K, T_gradient)

        del fdm

    def test_temperature_gradient_and_bias(self):
        fdm = CreateFDM(self.sandbox)
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

        del fdm

    def test_pressure_and_temperature_gradient(self):
        fdm = CreateFDM(self.sandbox)
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

        del fdm

    def test_pressure_and_temperature_gradient_and_bias(self):
        fdm = CreateFDM(self.sandbox)
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

        del fdm


RunTest(TestStdAtmosphere)
