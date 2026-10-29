# TestFuelTankTemperature.py
#
# A regression test that checks that:
# * A tank which specifies an initial fuel temperature publishes that
#   temperature as a property, and a tank which specifies none publishes no such
#   property rather than publishing the "no temperature set" flag.
# * The published temperature relaxes towards the total air temperature without
#   overshooting it.
# * The property is writable and the written value is the one the tank model
#   then integrates.
# * The initial temperature echoed at startup is labelled with the unit it is
#   printed in.
#
# Copyright (c) 2026 Felipegalind0
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
import xml.etree.ElementTree as et

from JSBSim_utils import CopyAircraftDef, JSBSimTestCase, RunTest
from jsbsim import FGJSBBase, FGLogger, get_logger, set_logger

# The constants of the heat balance that FGTank::Calculate runs. They are
# documented in the FGTank class documentation.
HEAT_CAPACITY = 900.0     # Joules/lbm/K
TEMP_FLOW_FACTOR = 1.115  # Watts/sq-ft/K

TANK0_TEMPERATURE = 'propulsion/tank[0]/temperature-degC'
TANK1_TEMPERATURE = 'propulsion/tank[1]/temperature-degC'

# 59 degrees Fahrenheit is exactly 15 degrees Celsius, so the conversion that
# FGTank applies to the configured value can be checked without a tolerance.
INITIAL_TEMPERATURE_DEGF = 59.0
INITIAL_TEMPERATURE_DEGC = 15.0


class StartupLog(FGLogger):
    """Collects everything JSBSim prints while a model is being loaded."""

    def __init__(self):
        self.buffer = ''

    def message(self, message: str) -> None:
        self.buffer += message


class TestFuelTankTemperature(JSBSimTestCase):
    def setUp(self):
        super().setUp()
        # c172x has two fuel tanks and neither of them declares a temperature,
        # so one tank can be given an initial temperature while the other is
        # left without one.
        self.script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                            'c1722.xml')

    def PrepareAircraft(self, temperature=None):
        """Copies c172x to the sandbox, optionally giving its first tank an
           initial temperature. Returns the aircraft name and the capacity of
           that tank in pounds."""
        tree, aircraft_name, _ = CopyAircraftDef(self.script_path, self.sandbox)
        tank = tree.getroot().find('propulsion/tank')
        if temperature is not None:
            et.SubElement(tank, 'temperature').text = str(temperature)
        tree.write(self.sandbox('aircraft', aircraft_name,
                                aircraft_name+'.xml'))
        return aircraft_name, float(tank.find('capacity').text)

    def test_temperature_property_follows_the_thermal_model(self):
        self.PrepareAircraft(INITIAL_TEMPERATURE_DEGF)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        # The first tank models a temperature, so it publishes one.
        self.assertAlmostEqual(fdm[TANK0_TEMPERATURE],
                               INITIAL_TEMPERATURE_DEGC,
                               msg="The tank temperature (%f degC) should be the configured %f degF i.e. %f degC" % (fdm[TANK0_TEMPERATURE], INITIAL_TEMPERATURE_DEGF, INITIAL_TEMPERATURE_DEGC))

        # The second tank models none, so it must publish none. Were the
        # property tied unconditionally it would read the -9999.0 flag.
        with self.assertRaises(KeyError):
            fdm[TANK1_TEMPERATURE]

        # The catalog reports the tank number 0 without an index and the
        # property as readable and writable.
        catalog = fdm.query_property_catalog('temperature-degC')
        self.assertEqual(catalog.rstrip().split('\n'),
                         ['propulsion/tank/temperature-degC (RW)'],
                         msg="Unexpected temperature properties in the catalog:\n"+catalog)

    def test_no_temperature_property_without_an_initial_temperature(self):
        self.PrepareAircraft()

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_script(self.script_path)
        fdm.run_ic()

        for property_name in (TANK0_TEMPERATURE, TANK1_TEMPERATURE):
            with self.assertRaises(KeyError):
                fdm[property_name]

        self.assertEqual(fdm.query_property_catalog('temperature-degC'),
                         'No matches found\n')

    def test_temperature_relaxes_towards_the_air_temperature(self):
        aircraft_name, capacity = self.PrepareAircraft(INITIAL_TEMPERATURE_DEGF)

        fdm = self.create_fdm()
        fdm.set_aircraft_path('aircraft')
        fdm.load_model(aircraft_name)
        fdm.load_ic('reset01', True)
        # Hold the aircraft down and leave its engine stopped so that neither
        # the air the tank exchanges heat with nor the mass of fuel it holds
        # moves much while the fuel temperature evolves.
        fdm['forces/hold-down'] = 1.0
        fdm.run_ic()

        # Make the fuel much warmer than the air. The tank must then relax from
        # the written value rather than from the configured one.
        temperature0 = fdm['propulsion/tat-c'] + 90.0
        fdm[TANK0_TEMPERATURE] = temperature0
        self.assertAlmostEqual(fdm[TANK0_TEMPERATURE], temperature0,
                               msg="The temperature property is not writable")

        t0 = fdm.get_sim_time()
        temperature = temperature0
        tat_min = tat_max = fdm['propulsion/tat-c']
        contents_min = contents_max = fdm['propulsion/tank[0]/contents-lbs']

        while fdm.get_sim_time() < t0 + 100.0:
            self.assertTrue(fdm.run())
            tat = fdm['propulsion/tat-c']
            contents = fdm['propulsion/tank[0]/contents-lbs']
            previous, temperature = temperature, fdm[TANK0_TEMPERATURE]

            # At every step the fuel gives heat to the colder air, so it cools
            # down, and it stops at the air temperature rather than passing it.
            self.assertLess(temperature, previous,
                            msg="At t=%f s the fuel temperature (%f degC) did not fall towards the air temperature (%f degC)" % (fdm.get_sim_time(), temperature, tat))
            self.assertGreater(temperature, tat,
                               msg="At t=%f s the fuel temperature (%f degC) overshot the air temperature (%f degC)" % (fdm.get_sim_time(), temperature, tat))

            tat_min, tat_max = min(tat_min, tat), max(tat_max, tat)
            contents_min = min(contents_min, contents)
            contents_max = max(contents_max, contents)

        elapsed = fdm.get_sim_time() - t0

        # FGTank exchanges heat through both the upper and the lower surface of
        # the tank, so the difference to the air temperature decays
        # exponentially with the time constant below. The air temperature and
        # the tank contents are not exactly constant over the run, so the
        # temperature is bracketed by the coldest/lightest and the
        # warmest/heaviest case rather than compared to a single value.
        area = 40.0 * math.pow(capacity/1975.0, 2.0/3.0)

        def relaxed(air_temperature, tank_contents):
            tau = tank_contents * HEAT_CAPACITY / (2.0*TEMP_FLOW_FACTOR*area)
            return air_temperature + (temperature0-air_temperature)*math.exp(-elapsed/tau)

        coldest = relaxed(tat_min, contents_min)
        warmest = relaxed(tat_max, contents_max)
        self.assertGreaterEqual(temperature, coldest,
                                msg="After %f seconds the fuel temperature (%f degC) cooled down faster than the heat balance allows (%f degC)" % (elapsed, temperature, coldest))
        self.assertLessEqual(temperature, warmest,
                             msg="After %f seconds the fuel temperature (%f degC) cooled down slower than the heat balance allows (%f degC)" % (elapsed, temperature, warmest))

    def test_startup_message_reports_celsius(self):
        self.PrepareAircraft(INITIAL_TEMPERATURE_DEGF)

        logger = StartupLog()
        default_logger = get_logger()
        debug_lvl = FGJSBBase().debug_lvl
        set_logger(logger)
        FGJSBBase().debug_lvl = 1
        try:
            fdm = self.create_fdm()
            fdm.set_aircraft_path('aircraft')
            fdm.load_script(self.script_path)
        finally:
            FGJSBBase().debug_lvl = debug_lvl
            set_logger(default_logger)

        # FGTank converts the configured temperature to Celsius before echoing
        # it, so the echoed value is the Celsius one and must be labelled as
        # such.
        self.assertIn('Initial temperature: %g Celsius' % (INITIAL_TEMPERATURE_DEGC,),
                      logger.buffer,
                      msg="The startup message does not report the temperature in Celsius:\n"+logger.buffer)
        self.assertNotIn('Fahrenheit', logger.buffer,
                         msg="The startup message still labels a Celsius value as Fahrenheit:\n"+logger.buffer)

        # The second tank has no initial temperature, so the message must say
        # so rather than echoing the -9999.0 flag as if it were one.
        self.assertIn('Initial temperature: not set, fuel temperature is not modeled',
                      logger.buffer,
                      msg="The startup message does not report the tank without an initial temperature:\n"+logger.buffer)
        self.assertNotIn('-9999', logger.buffer,
                         msg="The startup message still echoes the 'no temperature set' flag:\n"+logger.buffer)


RunTest(TestFuelTankTemperature)
