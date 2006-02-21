/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPiston.cpp
 Author:       Jon S. Berndt, JSBSim framework
               Dave Luff, Piston engine model
 Date started: 09/12/2000
 Purpose:      This module models a Piston engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class descends from the FGEngine class and models a Piston engine based on
parameters given in the engine config file for this class

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sstream>

#include "FGPiston.h"
#include <models/FGPropulsion.h>
#include "FGPropeller.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGPiston.cpp,v 1.4 2006/02/21 12:25:04 jberndt Exp $";
static const char *IdHdr = ID_PISTON;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPiston::FGPiston(FGFDMExec* exec, Element* el, int engine_number)
  : FGEngine(exec, el, engine_number),
  R_air(287.3),                  // Gas constant for air J/Kg/K
  rho_fuel(800),                 // estimate
  calorific_value_fuel(47.3e6),
  Cp_air(1005),                  // Specific heat (constant pressure) J/Kg/K
  Cp_fuel(1700)
{
  string token;

  // Defaults and initializations

  Type = etPiston;

  // These items are read from the configuration file

  Cycles = 2;
  IdleRPM = 600;
  Displacement = 360;
  MaxHP = 200;
  MinManifoldPressure_inHg = 6.5;
  MaxManifoldPressure_inHg = 28.5;

  // These are internal program variables

  crank_counter = 0;
  OilTemp_degK = 298;
  ManifoldPressure_inHg = Atmosphere->GetPressure() * psftoinhg; // psf to in Hg
  minMAP = 21950;
  maxMAP = 96250;
  MAP = Atmosphere->GetPressure() * psftopa;
  CylinderHeadTemp_degK = 0.0;
  Magnetos = 0;
  ExhaustGasTemp_degK = 0.0;
  EGT_degC = 0.0;

  dt = State->Getdt();

  // Supercharging
  BoostSpeeds = 0;  // Default to no supercharging
  BoostSpeed = 0;
  Boosted = false;
  BoostOverride = 0;
  bBoostOverride = false;
  bTakeoffBoost = false;
  TakeoffBoost = 0.0;   // Default to no extra takeoff-boost
  int i;
  for (i=0; i<FG_MAX_BOOST_SPEEDS; i++) {
    RatedBoost[i] = 0.0;
    RatedPower[i] = 0.0;
    RatedAltitude[i] = 0.0;
    BoostMul[i] = 1.0;
    RatedMAP[i] = 100000;
    RatedRPM[i] = 2500;
    TakeoffMAP[i] = 100000;
  }
  for (i=0; i<FG_MAX_BOOST_SPEEDS-1; i++) {
    BoostSwitchAltitude[i] = 0.0;
    BoostSwitchPressure[i] = 0.0;
  }
  // Initialisation
  volumetric_efficiency = 0.8;  // Actually f(speed, load) but this will get us running

  // First column is thi, second is neta (combustion efficiency)
  Lookup_Combustion_Efficiency = new FGTable(12);
  *Lookup_Combustion_Efficiency << 0.00 << 0.980;
  *Lookup_Combustion_Efficiency << 0.90 << 0.980;
  *Lookup_Combustion_Efficiency << 1.00 << 0.970;
  *Lookup_Combustion_Efficiency << 1.05 << 0.950;
  *Lookup_Combustion_Efficiency << 1.10 << 0.900;
  *Lookup_Combustion_Efficiency << 1.15 << 0.850;
  *Lookup_Combustion_Efficiency << 1.20 << 0.790;
  *Lookup_Combustion_Efficiency << 1.30 << 0.700;
  *Lookup_Combustion_Efficiency << 1.40 << 0.630;
  *Lookup_Combustion_Efficiency << 1.50 << 0.570;
  *Lookup_Combustion_Efficiency << 1.60 << 0.525;
  *Lookup_Combustion_Efficiency << 2.00 << 0.345;

  Power_Mixture_Correlation = new FGTable(13);
  *Power_Mixture_Correlation << (14.7/1.6) << 78.0;
  *Power_Mixture_Correlation << 10 <<  86.0;
  *Power_Mixture_Correlation << 11 <<  93.5;
  *Power_Mixture_Correlation << 12 <<  98.0;
  *Power_Mixture_Correlation << 13 << 100.0;
  *Power_Mixture_Correlation << 14 <<  99.0;
  *Power_Mixture_Correlation << 15 <<  96.4;
  *Power_Mixture_Correlation << 16 <<  92.5;
  *Power_Mixture_Correlation << 17 <<  88.0;
  *Power_Mixture_Correlation << 18 <<  83.0;
  *Power_Mixture_Correlation << 19 <<  78.5;
  *Power_Mixture_Correlation << 20 <<  74.0;
  *Power_Mixture_Correlation << (14.7/0.6) << 58;

  // Read inputs from engine data file where present.

  if (el->FindElement("minmp")) // Should have ELSE statement telling default value used?
    MinManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("minmp","INHG");
  if (el->FindElement("maxmp"))
    MaxManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("maxmp","INHG");
  if (el->FindElement("displacement"))
    Displacement = el->FindElementValueAsNumberConvertTo("displacement","IN3");
  if (el->FindElement("maxhp"))
    MaxHP = el->FindElementValueAsNumberConvertTo("maxhp","HP");
  if (el->FindElement("cycles"))
    Cycles = el->FindElementValueAsNumber("cycles");
  if (el->FindElement("idlerpm"))
    IdleRPM = el->FindElementValueAsNumber("idlerpm");
  if (el->FindElement("maxthrottle"))
    MaxThrottle = el->FindElementValueAsNumber("maxthrottle");
  if (el->FindElement("minthrottle"))
    MinThrottle = el->FindElementValueAsNumber("minthrottle");
  if (el->FindElement("numboostspeeds")) { // Turbo- and super-charging parameters
    BoostSpeeds = (int)el->FindElementValueAsNumber("numboostspeeds");
    if (el->FindElement("boostoverride"))
      BoostOverride = (int)el->FindElementValueAsNumber("boostoverride");
    if (el->FindElement("takeoffboost"))
      TakeoffBoost = el->FindElementValueAsNumberConvertTo("takeoffboost", "PSI");
    if (el->FindElement("ratedboost1"))
      RatedBoost[0] = el->FindElementValueAsNumberConvertTo("ratedboost1", "PSI");
    if (el->FindElement("ratedboost2"))
      RatedBoost[1] = el->FindElementValueAsNumberConvertTo("ratedboost2", "PSI");
    if (el->FindElement("ratedboost3"))
      RatedBoost[2] = el->FindElementValueAsNumberConvertTo("ratedboost3", "PSI");
    if (el->FindElement("ratedpower1"))
      RatedPower[0] = el->FindElementValueAsNumberConvertTo("ratedpower1", "HP");
    if (el->FindElement("ratedpower2"))
      RatedPower[1] = el->FindElementValueAsNumberConvertTo("ratedpower2", "HP");
    if (el->FindElement("ratedpower3"))
      RatedPower[2] = el->FindElementValueAsNumberConvertTo("ratedpower3", "HP");
    if (el->FindElement("ratedrpm1"))
      RatedRPM[0] = el->FindElementValueAsNumber("ratedrpm1");
    if (el->FindElement("ratedrpm2"))
      RatedRPM[1] = el->FindElementValueAsNumber("ratedrpm2");
    if (el->FindElement("ratedrpm3"))
      RatedRPM[2] = el->FindElementValueAsNumber("ratedrpm3");
    if (el->FindElement("ratedaltitude1"))
      RatedAltitude[0] = el->FindElementValueAsNumberConvertTo("ratedaltitude1", "FT");
    if (el->FindElement("ratedaltitude2"))
      RatedAltitude[1] = el->FindElementValueAsNumberConvertTo("ratedaltitude2", "FT");
    if (el->FindElement("ratedaltitude3"))
      RatedAltitude[2] = el->FindElementValueAsNumberConvertTo("ratedaltitude3", "FT");
  }
  minMAP = MinManifoldPressure_inHg * 3376.85;  // inHg to Pa
  maxMAP = MaxManifoldPressure_inHg * 3376.85;

  // Set up and sanity-check the turbo/supercharging configuration based on the input values.
  if (TakeoffBoost > RatedBoost[0]) bTakeoffBoost = true;
  for (i=0; i<BoostSpeeds; ++i) {
    bool bad = false;
    if (RatedBoost[i] <= 0.0) bad = true;
    if (RatedPower[i] <= 0.0) bad = true;
    if (RatedAltitude[i] < 0.0) bad = true;  // 0.0 is deliberately allowed - this corresponds to unregulated supercharging.
    if (i > 0 && RatedAltitude[i] < RatedAltitude[i - 1]) bad = true;
    if (bad) {
      // We can't recover from the above - don't use this supercharger speed.
      BoostSpeeds--;
      // TODO - put out a massive error message!
      break;
    }
    // Now sanity-check stuff that is recoverable.
    if (i < BoostSpeeds - 1) {
      if (BoostSwitchAltitude[i] < RatedAltitude[i]) {
        // TODO - put out an error message
        // But we can also make a reasonable estimate, as below.
        BoostSwitchAltitude[i] = RatedAltitude[i] + 1000;
      }
      BoostSwitchPressure[i] = Atmosphere->GetPressure(BoostSwitchAltitude[i]) * psftopa;
      //cout << "BoostSwitchAlt = " << BoostSwitchAltitude[i] << ", pressure = " << BoostSwitchPressure[i] << '\n';
      // Assume there is some hysteresis on the supercharger gear switch, and guess the value for now
      BoostSwitchHysteresis = 1000;
    }
    // Now work out the supercharger pressure multiplier of this speed from the rated boost and altitude.
    RatedMAP[i] = Atmosphere->GetPressureSL() * psftopa + RatedBoost[i] * 6895;  // psi*6895 = Pa.
    // Sometimes a separate BCV setting for takeoff or extra power is fitted.
    if (TakeoffBoost > RatedBoost[0]) {
      // Assume that the effect on the BCV is the same whichever speed is in use.
      TakeoffMAP[i] = RatedMAP[i] + ((TakeoffBoost - RatedBoost[0]) * 6895);
      bTakeoffBoost = true;
    } else {
      TakeoffMAP[i] = RatedMAP[i];
      bTakeoffBoost = false;
    }
    BoostMul[i] = RatedMAP[i] / (Atmosphere->GetPressure(RatedAltitude[i]) * psftopa);

  }

  if (BoostSpeeds > 0) {
    Boosted = true;
    BoostSpeed = 0;
  }
  bBoostOverride = (BoostOverride == 1 ? true : false);

  Debug(0); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPiston::~FGPiston()
{
  Debug(1); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPiston::Calculate(void)
{
  if (FuelFlow_gph > 0.0) ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  Mixture = FCS->GetMixturePos(EngineNumber);

  //
  // Input values.
  //

  p_amb = Atmosphere->GetPressure() * psftopa;
  p_amb_sea_level = Atmosphere->GetPressureSL() * psftopa;
  T_amb = Atmosphere->GetTemperature() * (5.0 / 9.0);  // convert from Rankine to Kelvin

  RPM = Thruster->GetRPM() * Thruster->GetGearRatio();

  IAS = Auxiliary->GetVcalibratedKTS();

  doEngineStartup();
  if (Boosted) doBoostControl();
  doMAP();
  doAirFlow();
  doFuelFlow();

  //Now that the fuel flow is done check if the mixture is too lean to run the engine
  //Assume lean limit at 22 AFR for now - thats a thi of 0.668
  //This might be a bit generous, but since there's currently no audiable warning of impending
  //cutout in the form of misfiring and/or rough running its probably reasonable for now.
  if (equivalence_ratio < 0.668)
    Running = false;

  doEnginePower();
  doEGT();
  doCHT();
  doOilTemperature();
  doOilPressure();

  if (Thruster->GetType() == FGThruster::ttPropeller) {
    ((FGPropeller*)Thruster)->SetAdvance(FCS->GetPropAdvance(EngineNumber));
    ((FGPropeller*)Thruster)->SetFeather(FCS->GetPropFeather(EngineNumber));
  }

  PowerAvailable = (HP * hptoftlbssec) - Thruster->GetPowerRequired();

  return Thruster->Calculate(PowerAvailable);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Start or stop the engine.
 */

void FGPiston::doEngineStartup(void)
{
  // Check parameters that may alter the operating state of the engine.
  // (spark, fuel, starter motor etc)
  bool spark;
  bool fuel;

  // Check for spark
  Magneto_Left = false;
  Magneto_Right = false;
  // Magneto positions:
  // 0 -> off
  // 1 -> left only
  // 2 -> right only
  // 3 -> both
  if (Magnetos != 0) {
    spark = true;
  } else {
    spark = false;
  }  // neglects battery voltage, master on switch, etc for now.

  if ((Magnetos == 1) || (Magnetos > 2)) Magneto_Left = true;
  if (Magnetos > 1)  Magneto_Right = true;

  // Assume we have fuel for now
  fuel = !Starved;

  // Check if we are turning the starter motor
  if (Cranking != Starter) {
    // This check saves .../cranking from getting updated every loop - they
    // only update when changed.
    Cranking = Starter;
    crank_counter = 0;
  }

  if (Cranking) crank_counter++;  //Check mode of engine operation

  if (!Running && spark && fuel) {  // start the engine if revs high enough
    if (Cranking) {
      if ((RPM > 450) && (crank_counter > 175)) // Add a little delay to startup
        Running = true;                         // on the starter
    } else {
      if (RPM > 450)                            // This allows us to in-air start
        Running = true;                         // when windmilling
    }
  }

  // Cut the engine *power* - Note: the engine may continue to
  // spin if the prop is in a moving airstream

  if ( Running && (!spark || !fuel) ) Running = false;

  // Check for stalling (RPM = 0).
  if (Running) {
    if (RPM == 0) {
      Running = false;
    } else if ((RPM <= 480) && (Cranking)) {
      Running = false;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/**
 * Calculate the Current Boost Speed
 *
 * This function calculates the current turbo/supercharger boost speed
 * based on altitude and the (automatic) boost-speed control valve configuration.
 *
 * Inputs: p_amb, BoostSwitchPressure, BoostSwitchHysteresis
 *
 * Outputs: BoostSpeed
 */

void FGPiston::doBoostControl(void)
{
  if(BoostSpeed < BoostSpeeds - 1) {
    // Check if we need to change to a higher boost speed
    if(p_amb < BoostSwitchPressure[BoostSpeed] - BoostSwitchHysteresis) {
      BoostSpeed++;
    }
  } else if(BoostSpeed > 0) {
    // Check if we need to change to a lower boost speed
    if(p_amb > BoostSwitchPressure[BoostSpeed - 1] + BoostSwitchHysteresis) {
      BoostSpeed--;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/**
 * Calculate the manifold absolute pressure (MAP) in inches hg
 *
 * This function calculates manifold absolute pressure (MAP)
 * from the throttle position, turbo/supercharger boost control
 * system, engine speed and local ambient air density.
 *
 * TODO: changes in MP should not be instantaneous -- introduce
 * a lag between throttle changes and MP changes, to allow pressure
 * to build up or disperse.
 *
 * Inputs: minMAP, maxMAP, p_amb, Throttle
 *
 * Outputs: MAP, ManifoldPressure_inHg
 */

void FGPiston::doMAP(void)
{
  if(RPM > 10) {
    // Naturally aspirated
    MAP = minMAP + (Throttle * (maxMAP - minMAP));
    MAP *= p_amb / p_amb_sea_level;
    if(Boosted) {
      // If takeoff boost is fitted, we currently assume the following throttle map:
      // (In throttle % - actual input is 0 -> 1)
      // 99 / 100 - Takeoff boost
      // 96 / 97 / 98 - Rated boost
      // 0 - 95 - Idle to Rated boost (MinManifoldPressure to MaxManifoldPressure)
      // In real life, most planes would be fitted with a mechanical 'gate' between
      // the rated boost and takeoff boost positions.
      double T = Throttle; // processed throttle value.
      bool bTakeoffPos = false;
      if(bTakeoffBoost) {
        if(Throttle > 0.98) {
          //cout << "Takeoff Boost!!!!\n";
          bTakeoffPos = true;
        } else if(Throttle <= 0.95) {
          bTakeoffPos = false;
          T *= 1.0 / 0.95;
        } else {
          bTakeoffPos = false;
          //cout << "Rated Boost!!\n";
          T = 1.0;
        }
      }
      // Boost the manifold pressure.
      MAP *= BoostMul[BoostSpeed];
      // Now clip the manifold pressure to BCV or Wastegate setting.
      if(bTakeoffPos) {
        if(MAP > TakeoffMAP[BoostSpeed]) {
          MAP = TakeoffMAP[BoostSpeed];
        }
      } else {
        if(MAP > RatedMAP[BoostSpeed]) {
          MAP = RatedMAP[BoostSpeed];
        }
      }
    }
  } else {
    // rpm < 10 - effectively stopped.
    // TODO - add a better variation of MAP with engine speed
    MAP = Atmosphere->GetPressure() * psftopa;
  }

  // And set the value in American units as well
  ManifoldPressure_inHg = MAP / 3376.85;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the air flow through the engine.
 * Also calculates ambient air density
 * (used in CHT calculation for air-cooled engines).
 *
 * Inputs: p_amb, R_air, T_amb, MAP, Displacement,
 *   RPM, volumetric_efficiency
 *
 * TODO: Model inlet manifold air temperature.
 *
 * Outputs: rho_air, m_dot_air
 */

void FGPiston::doAirFlow(void)
{
  rho_air = p_amb / (R_air * T_amb);
  double rho_air_manifold = MAP / (R_air * T_amb);
  double displacement_SI = Displacement * in3tom3;
  double swept_volume = (displacement_SI * (RPM/60)) / 2;
  double v_dot_air = swept_volume * volumetric_efficiency;
  m_dot_air = v_dot_air * rho_air_manifold;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the fuel flow into the engine.
 *
 * Inputs: Mixture, thi_sea_level, p_amb_sea_level, p_amb, m_dot_air
 *
 * Outputs: equivalence_ratio, m_dot_fuel
 */

void FGPiston::doFuelFlow(void)
{
  double thi_sea_level = 1.3 * Mixture;
  equivalence_ratio = thi_sea_level * p_amb_sea_level / p_amb;
  m_dot_fuel = m_dot_air / 14.7 * equivalence_ratio;
  FuelFlow_gph = m_dot_fuel
    * 3600			// seconds to hours
    * 2.2046			// kg to lb
    / 6.6;			// lb to gal_us of kerosene
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the power produced by the engine.
 *
 * Currently, the JSBSim propellor model does not allow the
 * engine to produce enough RPMs to get up to a high horsepower.
 * When tested with sufficient RPM, it has no trouble reaching
 * 200HP.
 *
 * Inputs: ManifoldPressure_inHg, p_amb, p_amb_sea_level, RPM, T_amb,
 *   equivalence_ratio, Cycles, MaxHP
 *
 * Outputs: Percentage_Power, HP
 */

void FGPiston::doEnginePower(void)
{
  if (Running) {
    double T_amb_degF = KelvinToFahrenheit(T_amb);
    double T_amb_sea_lev_degF = KelvinToFahrenheit(288);

    // FIXME: this needs to be generalized
    double ManXRPM;  // Convienience term for use in the calculations
    if(Boosted) {
      // Currently a simple linear fit.
      // The zero crossing is moved up the speed-load range to reduce the idling power.
      // This will change!
      double zeroOffset = (minMAP / 2.0) * (IdleRPM / 2.0);
      ManXRPM = MAP * (RPM > RatedRPM[BoostSpeed] ? RatedRPM[BoostSpeed] : RPM);
      // The speed clip in the line above is deliberate.
      Percentage_Power = ((ManXRPM - zeroOffset) / ((RatedMAP[BoostSpeed] * RatedRPM[BoostSpeed]) - zeroOffset)) * 107.0;
      Percentage_Power -= 7.0;  // Another idle power reduction offset - see line above with 107.
      if (Percentage_Power < 0.0) Percentage_Power = 0.0;
      // Note that %power is allowed to go over 100 for boosted powerplants
      // such as for the BCV-override or takeoff power settings.
      // TODO - currently no altitude effect (temperature & exhaust back-pressure) modelled
      // for boosted engines.
    } else {
      ManXRPM = ManifoldPressure_inHg * RPM; // Note that inHg must be used for the following correlation.
      Percentage_Power = (6e-9 * ManXRPM * ManXRPM) + (8e-4 * ManXRPM) - 1.0;
      Percentage_Power += ((T_amb_sea_lev_degF - T_amb_degF) * 7 /120);
      if (Percentage_Power < 0.0) Percentage_Power = 0.0;
      else if (Percentage_Power > 100.0) Percentage_Power = 100.0;
    }

    double Percentage_of_best_power_mixture_power =
      Power_Mixture_Correlation->GetValue(14.7 / equivalence_ratio);

    Percentage_Power *= Percentage_of_best_power_mixture_power / 100.0;

    if (Boosted) {
      HP = Percentage_Power * RatedPower[BoostSpeed] / 100.0;
    } else {
      HP = Percentage_Power * MaxHP / 100.0;
    }

  } else {

    // Power output when the engine is not running
    if (Cranking) {
      if (RPM < 10) {
        HP = 3.0;   // This is a hack to prevent overshooting the idle rpm in
                    // the first time step. It may possibly need to be changed
                    // if the prop model is changed.
      } else if (RPM < 480) {
        HP = 3.0 + ((480 - RPM) / 10.0);
        // This is a guess - would be nice to find a proper starter moter torque curve
      } else {
        HP = 3.0;
      }
    } else {
      // Quick hack until we port the FMEP stuff
      if (RPM > 0.0)
        HP = -1.5;
      else
        HP = 0.0;
    }
  }
  //cout << "Power = " << HP << '\n';
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the exhaust gas temperature.
 *
 * Inputs: equivalence_ratio, m_dot_fuel, calorific_value_fuel,
 *   Cp_air, m_dot_air, Cp_fuel, m_dot_fuel, T_amb, Percentage_Power
 *
 * Outputs: combustion_efficiency, ExhaustGasTemp_degK
 */

void FGPiston::doEGT(void)
{
  double delta_T_exhaust;
  double enthalpy_exhaust;
  double heat_capacity_exhaust;
  double dEGTdt;

  if ((Running) && (m_dot_air > 0.0)) {  // do the energy balance
    combustion_efficiency = Lookup_Combustion_Efficiency->GetValue(equivalence_ratio);
    enthalpy_exhaust = m_dot_fuel * calorific_value_fuel *
                              combustion_efficiency * 0.33;
    heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
    delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;
    ExhaustGasTemp_degK = T_amb + delta_T_exhaust;
    ExhaustGasTemp_degK *= 0.444 + ((0.544 - 0.444) * Percentage_Power / 100.0);
  } else {  // Drop towards ambient - guess an appropriate time constant for now
    dEGTdt = (298.0 - ExhaustGasTemp_degK) / 100.0;
    delta_T_exhaust = dEGTdt * dt;
    ExhaustGasTemp_degK += delta_T_exhaust;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the cylinder head temperature.
 *
 * Inputs: T_amb, IAS, rho_air, m_dot_fuel, calorific_value_fuel,
 *   combustion_efficiency, RPM
 *
 * Outputs: CylinderHeadTemp_degK
 */

void FGPiston::doCHT(void)
{
  double h1 = -95.0;
  double h2 = -3.95;
  double h3 = -0.05;

  double arbitary_area = 1.0;
  double CpCylinderHead = 800.0;
  double MassCylinderHead = 8.0;

  double temperature_difference = CylinderHeadTemp_degK - T_amb;
  double v_apparent = IAS * 0.5144444;
  double v_dot_cooling_air = arbitary_area * v_apparent;
  double m_dot_cooling_air = v_dot_cooling_air * rho_air;
  double dqdt_from_combustion =
    m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
  double dqdt_forced = (h2 * m_dot_cooling_air * temperature_difference) +
    (h3 * RPM * temperature_difference);
  double dqdt_free = h1 * temperature_difference;
  double dqdt_cylinder_head = dqdt_from_combustion + dqdt_forced + dqdt_free;

  double HeatCapacityCylinderHead = CpCylinderHead * MassCylinderHead;

  CylinderHeadTemp_degK +=
    (dqdt_cylinder_head / HeatCapacityCylinderHead) * dt;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the oil temperature.
 *
 * Inputs: Percentage_Power, running flag.
 *
 * Outputs: OilTemp_degK
 */

void FGPiston::doOilTemperature(void)
{
  double idle_percentage_power = 2.3;        // approximately
  double target_oil_temp;        // Steady state oil temp at the current engine conditions
  double time_constant;          // The time constant for the differential equation

  if (Running) {
    target_oil_temp = 363;
    time_constant = 500;        // Time constant for engine-on idling.
    if (Percentage_Power > idle_percentage_power) {
      time_constant /= ((Percentage_Power / idle_percentage_power) / 10.0); // adjust for power
    }
  } else {
    target_oil_temp = 298;
    time_constant = 1000;  // Time constant for engine-off; reflects the fact
                           // that oil is no longer getting circulated
  }

  double dOilTempdt = (target_oil_temp - OilTemp_degK) / time_constant;

  OilTemp_degK += (dOilTempdt * dt);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the oil pressure.
 *
 * Inputs: RPM
 *
 * Outputs: OilPressure_psi
 */

void FGPiston::doOilPressure(void)
{
  double Oil_Press_Relief_Valve = 60; // FIXME: may vary by engine
  double Oil_Press_RPM_Max = 1800;    // FIXME: may vary by engine
  double Design_Oil_Temp = 358;	      // degK; FIXME: may vary by engine
  double Oil_Viscosity_Index = 0.25;

  OilPressure_psi = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * RPM;

  if (OilPressure_psi >= Oil_Press_Relief_Valve) {
    OilPressure_psi = Oil_Press_Relief_Valve;
  }

  OilPressure_psi += (Design_Oil_Temp - OilTemp_degK) * Oil_Viscosity_Index;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineLabels(string delimeter)
{
  std::ostringstream buf;

  buf << Name << "_PwrAvail[" << EngineNumber << "]" << delimeter
      << Name << "_HP[" << EngineNumber << "]" << delimeter
      << Name << "_equiv_ratio[" << EngineNumber << "]" << delimeter
      << Name << "_MAP[" << EngineNumber << "]" << delimeter
      << Thruster->GetThrusterLabels(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineValues(string delimeter)
{
  std::ostringstream buf;

  buf << PowerAvailable << delimeter << HP << delimeter
      << equivalence_ratio << delimeter << MAP << delimeter
      << Thruster->GetThrusterValues(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGPiston::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

      cout << "\n    Engine Name: "         << Name << endl;
      cout << "      MinManifoldPressure: " << MinManifoldPressure_inHg << endl;
      cout << "      MaxManifoldPressure: " << MaxManifoldPressure_inHg << endl;
      cout << "      MinMaP (Pa):         " << minMAP << endl;
      cout << "      MaxMaP (Pa): "         << maxMAP << endl;
      cout << "      Displacement: "        << Displacement             << endl;
      cout << "      MaxHP: "               << MaxHP                    << endl;
      cout << "      Cycles: "              << Cycles                   << endl;
      cout << "      IdleRPM: "             << IdleRPM                  << endl;
      cout << "      MaxThrottle: "         << MaxThrottle              << endl;
      cout << "      MinThrottle: "         << MinThrottle              << endl;

      cout << endl;
      cout << "      Combustion Efficiency table:" << endl;
      Lookup_Combustion_Efficiency->Print();
      cout << endl;

      cout << endl;
      cout << "      Power Mixture Correlation table:" << endl;
      Power_Mixture_Correlation->Print();
      cout << endl;

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPiston" << endl;
    if (from == 1) cout << "Destroyed:    FGPiston" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

double
FGPiston::CalcFuelNeed(void)
{
  return FuelFlow_gph / 3600 * 6 * State->Getdt() * Propulsion->GetRate();
}

} // namespace JSBSim
