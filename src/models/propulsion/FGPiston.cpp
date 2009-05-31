/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPiston.cpp
 Author:       Jon S. Berndt, JSBSim framework
               Dave Luff, Piston engine model
               Ronald Jensen, Piston engine model
 Date started: 09/12/2000
 Purpose:      This module models a Piston engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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

static const char *IdSrc = "$Id: FGPiston.cpp,v 1.38 2009/05/31 21:09:13 andgi Exp $";
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
  dt = State->Getdt();

  // These items are read from the configuration file
  // Defaults are from a Lycoming O-360, more or less

  Cycles = 4;
  IdleRPM = 600;
  MaxRPM = 2800;
  Displacement = 360;
  SparkFailDrop = 1.0;
  MaxHP = 200;
  MinManifoldPressure_inHg = 6.5;
  MaxManifoldPressure_inHg = 28.5;
  ISFC = -1;
  volumetric_efficiency = -0.1;
  Bore = 5.125;
  Stroke = 4.375;
  Cylinders = 4;
  CompressionRatio = 8.5;

  // These are internal program variables

  crank_counter = 0;
  Magnetos = 0;
  minMAP = 21950;
  maxMAP = 96250;

  ResetToIC();

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

  Mixture_Efficiency_Correlation = new FGTable(15);
  *Mixture_Efficiency_Correlation << 0.05000 << 0.00000;
  *Mixture_Efficiency_Correlation << 0.05137 << 0.00862;
  *Mixture_Efficiency_Correlation << 0.05179 << 0.21552;
  *Mixture_Efficiency_Correlation << 0.05430 << 0.48276;
  *Mixture_Efficiency_Correlation << 0.05842 << 0.70690;
  *Mixture_Efficiency_Correlation << 0.06312 << 0.83621;
  *Mixture_Efficiency_Correlation << 0.06942 << 0.93103;
  *Mixture_Efficiency_Correlation << 0.07786 << 1.00000;
  *Mixture_Efficiency_Correlation << 0.08845 << 1.00000;
  *Mixture_Efficiency_Correlation << 0.09270 << 0.98276;
  *Mixture_Efficiency_Correlation << 0.10120 << 0.93103;
  *Mixture_Efficiency_Correlation << 0.11455 << 0.72414;
  *Mixture_Efficiency_Correlation << 0.12158 << 0.45690;
  *Mixture_Efficiency_Correlation << 0.12435 << 0.23276;
  *Mixture_Efficiency_Correlation << 0.12500 << 0.00000;


  // Read inputs from engine data file where present.

  if (el->FindElement("minmp")) // Should have ELSE statement telling default value used?
    MinManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("minmp","INHG");
  if (el->FindElement("maxmp"))
    MaxManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("maxmp","INHG");
  if (el->FindElement("displacement"))
    Displacement = el->FindElementValueAsNumberConvertTo("displacement","IN3");
  if (el->FindElement("maxhp"))
    MaxHP = el->FindElementValueAsNumberConvertTo("maxhp","HP");
  if (el->FindElement("sparkfaildrop"))
    SparkFailDrop = Constrain(0, 1 - el->FindElementValueAsNumber("sparkfaildrop"), 1);
  if (el->FindElement("cycles"))
    Cycles = el->FindElementValueAsNumber("cycles");
  if (el->FindElement("idlerpm"))
    IdleRPM = el->FindElementValueAsNumber("idlerpm");
  if (el->FindElement("maxrpm"))
    MaxRPM = el->FindElementValueAsNumber("maxrpm");
  if (el->FindElement("maxthrottle"))
    MaxThrottle = el->FindElementValueAsNumber("maxthrottle");
  if (el->FindElement("minthrottle"))
    MinThrottle = el->FindElementValueAsNumber("minthrottle");
  if (el->FindElement("bsfc"))
    ISFC = el->FindElementValueAsNumberConvertTo("bsfc", "LBS/HP*HR");
  if (el->FindElement("volumetric-efficiency"))
    volumetric_efficiency = el->FindElementValueAsNumber("volumetric-efficiency");
  if (el->FindElement("compression-ratio"))
    CompressionRatio = el->FindElementValueAsNumber("compression-ratio");
  if (el->FindElement("bore"))
    Bore = el->FindElementValueAsNumberConvertTo("bore","IN");
  if (el->FindElement("stroke"))
    Stroke = el->FindElementValueAsNumberConvertTo("stroke","IN");
  if (el->FindElement("stroke"))
    Cylinders = el->FindElementValueAsNumber("cylinders");
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

  StarterHP = sqrt(MaxHP) * 0.4;
  displacement_SI = Displacement * in3tom3;

  // Create IFSC and VE to match the engine if not provided
  int calculated_ve=0;
  if (volumetric_efficiency < 0) {
      volumetric_efficiency = MaxManifoldPressure_inHg / 29.92;
      calculated_ve=1;
  }
  if (ISFC < 0) {
      double pmep = MaxManifoldPressure_inHg > 29.92 ? 0 : 29.92 - MaxManifoldPressure_inHg;
      pmep *= inhgtopa;
      double fmep = (18400 * (2*(Stroke/12)*(MaxRPM/60)) * fttom + 46500)/2;
      double hp_loss = ((pmep + fmep) * displacement_SI * MaxRPM)/(Cycles*22371);
      ISFC = ( Displacement * MaxRPM * volumetric_efficiency ) / (9411 * (MaxHP+hp_loss));
// cout <<"FMEP: "<< fmep <<" PMEP: "<< pmep << " hp_loss: " <<hp_loss <<endl;
  }
  if ( MaxManifoldPressure_inHg > 29.9 ) {   // Don't allow boosting with a bogus number
      MaxManifoldPressure_inHg = 29.9;
      if (calculated_ve) volumetric_efficiency = 1.0;
  }
  minMAP = MinManifoldPressure_inHg * inhgtopa;  // inHg to Pa
  maxMAP = MaxManifoldPressure_inHg * inhgtopa;

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  property_name = base_property_name + "/power-hp";
  PropertyManager->Tie(property_name, &HP);
  property_name = base_property_name + "/bsfc-lbs_hphr";
  PropertyManager->Tie(property_name, &ISFC);
  property_name = base_property_name + "/volumetric-efficiency";
  PropertyManager->Tie(property_name, &volumetric_efficiency);
  property_name = base_property_name + "/map-pa";
  PropertyManager->Tie(property_name, &MAP);
  property_name = base_property_name + "/map-inhg";
  PropertyManager->Tie(property_name, &ManifoldPressure_inHg);

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
  delete Lookup_Combustion_Efficiency;
  delete Mixture_Efficiency_Correlation;
  Debug(1); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPiston::ResetToIC(void)
{
  FGEngine::ResetToIC();

  ManifoldPressure_inHg = Atmosphere->GetPressure() * psftoinhg; // psf to in Hg
  MAP = Atmosphere->GetPressure() * psftopa;
  TMAP = MAP;
  double airTemperature_degK = RankineToKelvin(Atmosphere->GetTemperature());
  OilTemp_degK = airTemperature_degK;
  CylinderHeadTemp_degK = airTemperature_degK;
  ExhaustGasTemp_degK = airTemperature_degK;
  EGT_degC = ExhaustGasTemp_degK - 273;
  Thruster->SetRPM(0.0);
  RPM = 0.0;
  OilPressure_psi = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPiston::Calculate(void)
{
  if (FuelFlow_gph > 0.0) ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  // calculate the throttle plate angle.  1 unit is approx pi/2 radians.
  ThrottleAngle = MinThrottle+((MaxThrottle-MinThrottle)*Throttle );
  Mixture = FCS->GetMixturePos(EngineNumber);

  //
  // Input values.
  //

  p_amb = Atmosphere->GetPressure() * psftopa;
  T_amb = RankineToKelvin(Atmosphere->GetTemperature());

  RPM = Thruster->GetRPM() * Thruster->GetGearRatio();
  MeanPistonSpeed_fps =  ( RPM * Stroke) / (360); // AKA 2 * (RPM/60) * ( Stroke / 12) or 2NS

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
//  if (equivalence_ratio < 0.668)
//    Running = false;

  doEnginePower();
  if (IndicatedHorsePower < 0.1250) Running = false;

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

double FGPiston::CalcFuelNeed(void)
{
  double dT = State->Getdt() * Propulsion->GetRate();
  FuelExpended = FuelFlowRate * dT;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPiston::InitRunning(void) {
  Magnetos=3;
  //Thruster->SetRPM( 1.1*IdleRPM/Thruster->GetGearRatio() );
  Thruster->SetRPM( 1000 );
  Running=true;
  return 1;
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
      if ((RPM > IdleRPM*0.8) && (crank_counter > 175)) // Add a little delay to startup
        Running = true;                         // on the starter
    } else {
      if (RPM > IdleRPM*0.8)                            // This allows us to in-air start
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
    } else if ((RPM <= IdleRPM *0.8 ) && (Cranking)) {
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
 * Inputs: p_amb, Throttle, ThrottleAngle,
 *         MeanPistonSpeed_fps, dt
 *
 * Outputs: MAP, ManifoldPressure_inHg, TMAP
 */

void FGPiston::doMAP(void)
{
 // estimate throttle plate area.
  double throttle_area = ThrottleAngle*ThrottleAngle;
 // Internal Combustion Engine in Theory and Practice, Volume 2.  Charles Fayette Taylor.  Revised Edition, 1985 fig 6-13
  double map_coefficient = 1-((MeanPistonSpeed_fps*MeanPistonSpeed_fps)/(24978*throttle_area));

  if ( map_coefficient < 0.1 ) map_coefficient = 0.1;

  // Add a one second lag to manifold pressure changes
  double dMAP = (TMAP - p_amb * map_coefficient) * dt;
  TMAP -=dMAP;

  // Find the mean effective pressure required to achieve this manifold pressure
  // Fixme: determine the HP consumed by the supercharger

  PMEP = TMAP - p_amb; // Fixme: p_amb should be exhaust manifold pressure

  if (Boosted) {
    // If takeoff boost is fitted, we currently assume the following throttle map:
    // (In throttle % - actual input is 0 -> 1)
    // 99 / 100 - Takeoff boost
    // In real life, most planes would be fitted with a mechanical 'gate' between
    // the rated boost and takeoff boost positions.

    bool bTakeoffPos = false;
    if (bTakeoffBoost) {
      if (Throttle > 0.98) {
        bTakeoffPos = true;
      }
    }
    // Boost the manifold pressure.
    double boost_factor = BoostMul[BoostSpeed] * RPM/RatedRPM[BoostSpeed];
    if (boost_factor < 1.0) boost_factor = 1.0;  // boost will never reduce the MAP
    MAP = TMAP * boost_factor;
    // Now clip the manifold pressure to BCV or Wastegate setting.
    if (bTakeoffPos) {
      if (MAP > TakeoffMAP[BoostSpeed]) MAP = TakeoffMAP[BoostSpeed];
    } else {
      if (MAP > RatedMAP[BoostSpeed]) MAP = RatedMAP[BoostSpeed];
    }
  } else {
      MAP = TMAP;
  }

  // And set the value in American units as well
  ManifoldPressure_inHg = MAP / inhgtopa;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the air flow through the engine.
 * Also calculates ambient air density
 * (used in CHT calculation for air-cooled engines).
 *
 * Inputs: p_amb, R_air, T_amb, MAP, Displacement,
 *   RPM, volumetric_efficiency, ThrottleAngle
 *
 * TODO: Model inlet manifold air temperature.
 *
 * Outputs: rho_air, m_dot_air
 */

void FGPiston::doAirFlow(void)
{
  double gamma = 1.4; // specific heat constants
// loss of volumentric efficiency due to difference between MAP and exhaust pressure
  double ve =((gamma-1)/gamma)+( CompressionRatio -(p_amb/MAP))/(gamma*( CompressionRatio - 1));

  rho_air = p_amb / (R_air * T_amb);
  double swept_volume = (displacement_SI * (RPM/60)) / 2;
  double v_dot_air = swept_volume * volumetric_efficiency *ve;

  double rho_air_manifold = MAP / (R_air * T_amb);
  m_dot_air = v_dot_air * rho_air_manifold;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the fuel flow into the engine.
 *
 * Inputs: Mixture, thi_sea_level, p_amb, m_dot_air
 *
 * Outputs: equivalence_ratio, m_dot_fuel
 */

void FGPiston::doFuelFlow(void)
{
  double thi_sea_level = 1.3 * Mixture; // Allows an AFR of infinity:1 to 11.3075:1
  equivalence_ratio = thi_sea_level * 101325.0 / p_amb;
//  double AFR = 10+(12*(1-Mixture));// mixture 10:1 to 22:1
//  m_dot_fuel = m_dot_air / AFR;
  m_dot_fuel = (m_dot_air * equivalence_ratio) / 14.7;
  FuelFlowRate =  m_dot_fuel * 2.2046;  // kg to lb
  FuelFlow_pph = FuelFlowRate  * 3600;  // seconds to hours
  FuelFlow_gph = FuelFlow_pph / 6.0;    // Assumes 6 lbs / gallon
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
 * Inputs: ManifoldPressure_inHg, p_amb, RPM, T_amb,
 *   Mixture_Efficiency_Correlation, Cycles, MaxHP, PMEP,
 *
 * Outputs: PctPower, HP
 */

void FGPiston::doEnginePower(void)
{
  IndicatedHorsePower = 0;
  FMEP = 0;
  if (Running) {
    // FIXME: this needs to be generalized
    double ME, percent_RPM, power;  // Convienience term for use in the calculations
    ME = Mixture_Efficiency_Correlation->GetValue(m_dot_fuel/m_dot_air);

    percent_RPM = RPM/MaxRPM;
// Guestimate engine friction as a percentage of rated HP + a percentage of rpm + a percentage of Indicted HP
//    friction = 1 - (percent_RPM * percent_RPM * percent_RPM/10);
    FMEP = (-18400 * MeanPistonSpeed_fps * fttom - 46500);

    power = 1;

    if ( Magnetos != 3 ) power *= SparkFailDrop;


    IndicatedHorsePower = (FuelFlow_pph / ISFC )* ME * power;

  } else {
    // Power output when the engine is not running
    if (Cranking) {
      if (RPM < 10) {
        IndicatedHorsePower = StarterHP;
      } else if (RPM < IdleRPM*0.8) {
        IndicatedHorsePower = StarterHP + ((IdleRPM*0.8 - RPM) / 8.0);
        // This is a guess - would be nice to find a proper starter moter torque curve
      } else {
        IndicatedHorsePower = StarterHP;
      }
    }
  }

  // Constant is (1/2) * 60 * 745.7
  // (1/2) convert cycles, 60 minutes to seconds, 745.7 watts to hp.
  double pumping_hp = ((PMEP + FMEP) * displacement_SI * RPM)/(Cycles*22371);

  HP = IndicatedHorsePower + pumping_hp - 1.5; //FIXME 1.5 static friction should depend on oil temp and configuration
//  cout << "pumping_hp " <<pumping_hp << FMEP << PMEP <<endl;
  PctPower = HP / MaxHP ;
//  cout << "Power = " << HP << "  RPM = " << RPM << "  Running = " << Running << "  Cranking = " << Cranking << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the exhaust gas temperature.
 *
 * Inputs: equivalence_ratio, m_dot_fuel, calorific_value_fuel,
 *   Cp_air, m_dot_air, Cp_fuel, m_dot_fuel, T_amb, PctPower
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
    ExhaustGasTemp_degK *= 0.444 + ((0.544 - 0.444) * PctPower);
  } else {  // Drop towards ambient - guess an appropriate time constant for now
    combustion_efficiency = 0;
    dEGTdt = (RankineToKelvin(Atmosphere->GetTemperature()) - ExhaustGasTemp_degK) / 100.0;
    delta_T_exhaust = dEGTdt * dt;
    ExhaustGasTemp_degK += delta_T_exhaust;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the cylinder head temperature.
 *
 * Inputs: T_amb, IAS, rho_air, m_dot_fuel, calorific_value_fuel,
 *   combustion_efficiency, RPM, MaxRPM, Displacement
 *
 * Outputs: CylinderHeadTemp_degK
 */

void FGPiston::doCHT(void)
{
  double h1 = -95.0;
  double h2 = -3.95;
  double h3 = -140.0; // -0.05 * 2800 (default maxrpm)

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
    (h3 * RPM * temperature_difference / MaxRPM);
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
 * Inputs: CylinderHeadTemp_degK, T_amb, OilPressure_psi.
 *
 * Outputs: OilTemp_degK
 */

void FGPiston::doOilTemperature(void)
{
  double target_oil_temp;        // Steady state oil temp at the current engine conditions
  double time_constant;          // The time constant for the differential equation
  double efficiency = 0.667;     // The aproximate oil cooling system efficiency // FIXME: may vary by engine

//  Target oil temp is interpolated between ambient temperature and Cylinder Head Tempurature
//  target_oil_temp = ( T_amb * efficiency ) + (CylinderHeadTemp_degK *(1-efficiency)) ;
  target_oil_temp = CylinderHeadTemp_degK + efficiency * (T_amb - CylinderHeadTemp_degK) ;

  if (OilPressure_psi > 5.0 ) {
    time_constant = 5000 / OilPressure_psi; // Guess at a time constant for circulated oil.
                                            // The higher the pressure the faster it reaches
					    // target temperature.  Oil pressure should be about
					    // 60 PSI yielding a TC of about 80.
  } else {
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
 * Inputs: RPM, MaxRPM, OilTemp_degK
 *
 * Outputs: OilPressure_psi
 */

void FGPiston::doOilPressure(void)
{
  double Oil_Press_Relief_Valve = 60; // FIXME: may vary by engine
  double Oil_Press_RPM_Max = MaxRPM * 0.75;    // 75% of max rpm FIXME: may vary by engine
  double Design_Oil_Temp = 358;          // degK; FIXME: may vary by engine
  double Oil_Viscosity_Index = 0.25;

  OilPressure_psi = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * RPM;

  if (OilPressure_psi >= Oil_Press_Relief_Valve) {
    OilPressure_psi = Oil_Press_Relief_Valve;
  }

  OilPressure_psi += (Design_Oil_Temp - OilTemp_degK) * Oil_Viscosity_Index * OilPressure_psi / Oil_Press_Relief_Valve;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineLabels(string delimeter)
{
  std::ostringstream buf;

  buf << Name << " Power Available (engine " << EngineNumber << " in HP)" << delimeter
      << Name << " HP (engine " << EngineNumber << ")" << delimeter
      << Name << " equivalent ratio (engine " << EngineNumber << ")" << delimeter
      << Name << " MAP (engine " << EngineNumber << " in inHg)" << delimeter
      << Thruster->GetThrusterLabels(EngineNumber, delimeter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineValues(string delimeter)
{
  std::ostringstream buf;

  buf << PowerAvailable << delimeter << HP << delimeter
      << equivalence_ratio << delimeter << ManifoldPressure_inHg << delimeter
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
      cout << "      MaxMaP (Pa):         " << maxMAP << endl;
      cout << "      Displacement: "        << Displacement             << endl;
      cout << "      Bore: "                << Bore                     << endl;
      cout << "      Stroke: "              << Stroke                   << endl;
      cout << "      Cylinders: "           << Cylinders                << endl;
      cout << "      Compression Ratio: "   << CompressionRatio         << endl;
      cout << "      MaxHP: "               << MaxHP                    << endl;
      cout << "      Cycles: "              << Cycles                   << endl;
      cout << "      IdleRPM: "             << IdleRPM                  << endl;
      cout << "      MaxThrottle: "         << MaxThrottle              << endl;
      cout << "      MinThrottle: "         << MinThrottle              << endl;
      cout << "      ISFC: "                << ISFC                     << endl;
      cout << "      Volumentric Efficiency: " << volumetric_efficiency    << endl;

      cout << endl;
      cout << "      Combustion Efficiency table:" << endl;
      Lookup_Combustion_Efficiency->Print();
      cout << endl;

      cout << endl;
      cout << "      Mixture Efficiency Correlation table:" << endl;
      Mixture_Efficiency_Correlation->Print();
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
} // namespace JSBSim
