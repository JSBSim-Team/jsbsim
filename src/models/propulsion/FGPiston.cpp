/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPiston.cpp
 Author:       Jon S. Berndt, JSBSim framework
               Dave Luff, Piston engine model
               Ronald Jensen, Piston engine model
 Date started: 09/12/2000
 Purpose:      This module models a Piston engine

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) --------------

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

#include <iostream>
#include <sstream>

#include "FGFDMExec.h"
#include "FGPiston.h"
#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPiston::FGPiston(FGFDMExec* exec, Element* el, int engine_number, struct Inputs& input)
  : FGEngine(engine_number, input),
  R_air(287.3),                  // Gas constant for air J/Kg/K
  calorific_value_fuel(47.3e6),  // J/Kg
  Cp_air(1005),                  // Specific heat (constant pressure) J/Kg/K
  Cp_fuel(1700),
  standard_pressure(101320.73)
{
  Load(exec, el);

  Element *table_element;
  auto PropertyManager = exec->GetPropertyManager();

  // Defaults and initializations

  Type = etPiston;

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
  ManifoldPressureLag=1.0;
  ISFC = -1;
  volumetric_efficiency = 0.85;
  Bore = 5.125;
  Stroke = 4.375;
  Cylinders = 4;
  CylinderHeadMass = 2; //kg
  CompressionRatio = 8.5;
  Z_airbox = -999;
  Ram_Air_Factor = 1;
  PeakMeanPistonSpeed_fps = 100;
  FMEPDynamic= 18400;
  FMEPStatic = 46500;
  Cooling_Factor = 0.5144444;
  StaticFriction_HP = 1.5;
  StarterGain = 1.;
  StarterTorque = -1.;
  StarterRPM = -1.;

  // These are internal program variables

  Lookup_Combustion_Efficiency = 0;
  Mixture_Efficiency_Correlation = 0;
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
  BoostManual = 0;
  bBoostOverride = false;
  bTakeoffBoost = false;
  TakeoffBoost = 0.0;   // Default to no extra takeoff-boost
  BoostLossFactor = 0.0;   // Default to free boost

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

  // Read inputs from engine data file where present.

  if (el->FindElement("minmp"))
    MinManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("minmp","INHG");
  if (el->FindElement("maxmp"))
    MaxManifoldPressure_inHg = el->FindElementValueAsNumberConvertTo("maxmp","INHG");
  if (el->FindElement("man-press-lag"))
    ManifoldPressureLag = el->FindElementValueAsNumber("man-press-lag");
  if (el->FindElement("displacement"))
    Displacement = el->FindElementValueAsNumberConvertTo("displacement","IN3");
  if (el->FindElement("maxhp"))
    MaxHP = el->FindElementValueAsNumberConvertTo("maxhp","HP");
  if (el->FindElement("static-friction"))
    StaticFriction_HP = el->FindElementValueAsNumberConvertTo("static-friction","HP");
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
  if (el->FindElement("cylinders"))
    Cylinders = el->FindElementValueAsNumber("cylinders");
  if (el->FindElement("cylinder-head-mass"))
    CylinderHeadMass = el->FindElementValueAsNumberConvertTo("cylinder-head-mass","KG");
  if (el->FindElement("air-intake-impedance-factor"))
    Z_airbox = el->FindElementValueAsNumber("air-intake-impedance-factor");
  if (el->FindElement("ram-air-factor"))
    Ram_Air_Factor  = el->FindElementValueAsNumber("ram-air-factor");
  if (el->FindElement("cooling-factor"))
    Cooling_Factor  = el->FindElementValueAsNumber("cooling-factor");
  if (el->FindElement("starter-rpm"))
    StarterRPM  = el->FindElementValueAsNumber("starter-rpm");
  if (el->FindElement("starter-torque"))
    StarterTorque  = el->FindElementValueAsNumber("starter-torque");
  if (el->FindElement("dynamic-fmep"))
    FMEPDynamic= el->FindElementValueAsNumberConvertTo("dynamic-fmep","PA");
  if (el->FindElement("static-fmep"))
    FMEPStatic = el->FindElementValueAsNumberConvertTo("static-fmep","PA");
  if (el->FindElement("peak-piston-speed"))
    PeakMeanPistonSpeed_fps  = el->FindElementValueAsNumber("peak-piston-speed");
  if (el->FindElement("numboostspeeds")) { // Turbo- and super-charging parameters
    BoostSpeeds = (int)el->FindElementValueAsNumber("numboostspeeds");
    if (el->FindElement("boostoverride"))
      BoostOverride = (int)el->FindElementValueAsNumber("boostoverride");
    if (el->FindElement("boostmanual"))
      BoostManual = (int)el->FindElementValueAsNumber("boostmanual");
    if (el->FindElement("takeoffboost"))
      TakeoffBoost = el->FindElementValueAsNumberConvertTo("takeoffboost", "PSI");
    if (el->FindElement("boost-loss-factor"))
      BoostLossFactor = el->FindElementValueAsNumber("boost-loss-factor");
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

  Design_Oil_Temp = 358;                // degK;
  Oil_Viscosity_Index = 0.25;
  Oil_Press_Relief_Valve = 60;          // psi
  Oil_Press_RPM_Max = MaxRPM*0.75;
  if (el->FindElement("oil-pressure-relief-valve-psi"))
    Oil_Press_Relief_Valve = el->FindElementValueAsNumberConvertTo("oil-pressure-relief-valve-psi", "PSI");
  if (el->FindElement("design-oil-temp-degK"))
    Design_Oil_Temp = el->FindElementValueAsNumberConvertTo("design-oil-temp-degK", "DEGK");
  if (el->FindElement("oil-pressure-rpm-max"))
    Oil_Press_RPM_Max = el->FindElementValueAsNumber("oil-pressure-rpm-max");
  if (el->FindElement("oil-viscosity-index"))
    Oil_Viscosity_Index = el->FindElementValueAsNumber("oil-viscosity-index");

  while((table_element = el->FindNextElement("table")) != 0) {
    string name = table_element->GetAttributeValue("name");
    try {
      if (name == "COMBUSTION") {
        Lookup_Combustion_Efficiency = new FGTable(PropertyManager, table_element);
      } else if (name == "MIXTURE") {
        Mixture_Efficiency_Correlation = new FGTable(PropertyManager, table_element);
      } else {
        cerr << "Unknown table type: " << name << " in piston engine definition." << endl;
      }
    } catch (std::string& str) {
      // Make sure allocated resources are freed before rethrowing.
      // (C++ standard guarantees that a null pointer deletion is no-op).
      delete Lookup_Combustion_Efficiency;
      delete Mixture_Efficiency_Correlation;
      throw("Error loading piston engine table:" + name + ". " + str);
    }
  }


  volumetric_efficiency_reduced = volumetric_efficiency;

  if(StarterRPM < 0.) StarterRPM = 2*IdleRPM;
  if(StarterTorque < 0)
    StarterTorque = (MaxHP)*0.4; //just a wag.

  displacement_SI = Displacement * in3tom3;
  RatedMeanPistonSpeed_fps =  ( MaxRPM * Stroke) / (360); // AKA 2 * (RPM/60) * ( Stroke / 12) or 2NS

  // Create IFSC to match the engine if not provided
  if (ISFC < 0) {
      double pmep = 29.92 - MaxManifoldPressure_inHg;
      pmep *= inhgtopa  * volumetric_efficiency;
      double fmep = (FMEPDynamic * RatedMeanPistonSpeed_fps * fttom + FMEPStatic);
      double hp_loss = ((pmep + fmep) * displacement_SI * MaxRPM)/(Cycles*22371);
      ISFC = ( 1.1*Displacement * MaxRPM * volumetric_efficiency *(MaxManifoldPressure_inHg / 29.92) ) / (9411 * (MaxHP+hp_loss-StaticFriction_HP));
// cout <<"FMEP: "<< fmep <<" PMEP: "<< pmep << " hp_loss: " <<hp_loss <<endl;
  }
  if ( MaxManifoldPressure_inHg > 29.9 ) {   // Don't allow boosting with a bogus number
      MaxManifoldPressure_inHg = 29.9;
  }
  minMAP = MinManifoldPressure_inHg * inhgtopa;  // inHg to Pa
  maxMAP = MaxManifoldPressure_inHg * inhgtopa;

// For throttle
/*
 * Pm = ( Ze / ( Ze + Zi + Zt ) ) * Pa
 * Where:
 * Pm = Manifold Pressure
 * Pa = Ambient Pressre
 * Ze = engine impedance, Ze is effectively 1 / Mean Piston Speed
 * Zi = airbox impedance
 * Zt = throttle impedance
 *
 * For the calculation below throttle is fully open or Zt = 0
 *
 *
 *
 */
  if(Z_airbox < 0.0){
    double Ze=PeakMeanPistonSpeed_fps/RatedMeanPistonSpeed_fps; // engine impedence
    Z_airbox = (standard_pressure *Ze / maxMAP) - Ze; // impedence of airbox
  }
  // Constant for Throttle impedence
  Z_throttle=(PeakMeanPistonSpeed_fps/((IdleRPM * Stroke) / 360))*(standard_pressure/minMAP - 1) - Z_airbox;
  //  Z_throttle=(MaxRPM/IdleRPM )*(standard_pressure/minMAP+2); // Constant for Throttle impedence

// Default tables if not provided in the configuration file
  if(Lookup_Combustion_Efficiency == 0) {
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
  }

    // First column is Fuel/Air Ratio, second is neta (mixture efficiency)
  if( Mixture_Efficiency_Correlation == 0) {
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
  }

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);
  property_name = base_property_name + "/power-hp";
  PropertyManager->Tie(property_name, &HP);
  property_name = base_property_name + "/friction-hp";
  PropertyManager->Tie(property_name, &StaticFriction_HP);
  property_name = base_property_name + "/bsfc-lbs_hphr";
  PropertyManager->Tie(property_name, &ISFC);
  property_name = base_property_name + "/starter-norm";
  PropertyManager->Tie(property_name, &StarterGain);
  property_name = base_property_name + "/volumetric-efficiency";
  PropertyManager->Tie(property_name, &volumetric_efficiency);
  property_name = base_property_name + "/map-pa";
  PropertyManager->Tie(property_name, &MAP);
  property_name = base_property_name + "/map-inhg";
  PropertyManager->Tie(property_name, &ManifoldPressure_inHg);
  property_name = base_property_name + "/air-intake-impedance-factor";
  PropertyManager->Tie(property_name, &Z_airbox);
  property_name = base_property_name + "/ram-air-factor";
  PropertyManager->Tie(property_name, &Ram_Air_Factor);
  property_name = base_property_name + "/cooling-factor";
  PropertyManager->Tie(property_name, &Cooling_Factor);
  property_name = base_property_name + "/boost-speed";
  PropertyManager->Tie(property_name, &BoostSpeed);
  property_name = base_property_name + "/cht-degF";
  PropertyManager->Tie(property_name, this, &FGPiston::getCylinderHeadTemp_degF);
  property_name = base_property_name + "/oil-temperature-degF";
  PropertyManager->Tie(property_name, this, &FGPiston::getOilTemp_degF);
  property_name = base_property_name + "/oil-pressure-psi";
  PropertyManager->Tie(property_name, this, &FGPiston::getOilPressure_psi);
  property_name = base_property_name + "/egt-degF";
  PropertyManager->Tie(property_name, this, &FGPiston::getExhaustGasTemp_degF);
  if(BoostLossFactor > 0.0) {
    property_name = base_property_name + "/boostloss-factor";
    PropertyManager->Tie(property_name, &BoostLossFactor);
    property_name = base_property_name + "/boostloss-hp";
    PropertyManager->Tie(property_name, &BoostLossHP);
  }
  property_name = base_property_name + "/AFR";
  PropertyManager->Tie(property_name, this, &FGPiston::getAFR);

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
      BoostSwitchPressure[i] = GetStdPressure100K(BoostSwitchAltitude[i]) * psftopa;
      //cout << "BoostSwitchAlt = " << BoostSwitchAltitude[i] << ", pressure = " << BoostSwitchPressure[i] << '\n';
      // Assume there is some hysteresis on the supercharger gear switch, and guess the value for now
      BoostSwitchHysteresis = 1000;
    }
    // Now work out the supercharger pressure multiplier of this speed from the rated boost and altitude.
    RatedMAP[i] = standard_pressure + RatedBoost[i] * 6895;  // psi*6895 = Pa.
    // Sometimes a separate BCV setting for takeoff or extra power is fitted.
    if (TakeoffBoost > RatedBoost[0]) {
      // Assume that the effect on the BCV is the same whichever speed is in use.
      TakeoffMAP[i] = RatedMAP[i] + ((TakeoffBoost - RatedBoost[0]) * 6895);
      bTakeoffBoost = true;
    } else {
      TakeoffMAP[i] = RatedMAP[i];
      bTakeoffBoost = false;
    }
    BoostMul[i] = RatedMAP[i] / (GetStdPressure100K(RatedAltitude[i]) * psftopa);

  }

  if (BoostSpeeds > 0) {
    Boosted = true;
    BoostSpeed = 0;
  }
  bBoostOverride = (BoostOverride == 1 ? true : false);
  bBoostManual   = (BoostManual   == 1 ? true : false);
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

  ManifoldPressure_inHg = in.Pressure * psftoinhg; // psf to in Hg
  MAP = in.Pressure * psftopa;
  TMAP = MAP;
  double airTemperature_degK = RankineToKelvin(in.Temperature);
  OilTemp_degK = airTemperature_degK;
  CylinderHeadTemp_degK = airTemperature_degK;
  ExhaustGasTemp_degK = airTemperature_degK;
  EGT_degC = ExhaustGasTemp_degK - 273;
  Thruster->SetRPM(0.0);
  RPM = 0.0;
  OilPressure_psi = 0.0;
  BoostLossHP = 0.;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPiston::Calculate(void)
{
  // Input values.

  p_amb = in.Pressure * psftopa;
  double p = in.TotalPressure * psftopa;
  p_ram = (p - p_amb) * Ram_Air_Factor + p_amb;
  T_amb = RankineToKelvin(in.Temperature);

  RunPreFunctions();

/* The thruster controls the engine RPM because it encapsulates the gear ratio and other transmission variables */
  RPM = Thruster->GetEngineRPM();

  MeanPistonSpeed_fps =  ( RPM * Stroke) / (360); // AKA 2 * (RPM/60) * ( Stroke / 12) or 2NS

  IAS = in.Vc;

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
    ((FGPropeller*)Thruster)->SetAdvance(in.PropAdvance[EngineNumber]);
    ((FGPropeller*)Thruster)->SetFeather(in.PropFeather[EngineNumber]);
  }

  LoadThrusterInputs();
  // Filters out negative powers when the propeller is not rotating.
  double power = HP * hptoftlbssec;
  if (RPM <= 0.1) power = max(power, 0.0);
  Thruster->Calculate(power);

  RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPiston::CalcFuelNeed(void)
{
  FuelExpended = FuelFlowRate * in.TotalDeltaT;
  if (!Starved) FuelUsedLbs += FuelExpended;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPiston::InitRunning(void)
{
  Magnetos=3;
  in.MixtureCmd[EngineNumber] = in.PressureRatio*1.3;
  in.MixturePos[EngineNumber] = in.PressureRatio*1.3;
  Thruster->SetRPM( 2.0*IdleRPM/Thruster->GetGearRatio() );
  Running = true;
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

// We will 'run' with any fuel flow. If there is not enough fuel to make power it will show in doEnginePower
  fuel = FuelFlowRate > 0.0 ? 1 : 0;

  // Check if we are turning the starter motor
  if (Cranking != Starter) {
    // This check saves .../cranking from getting updated every loop - they
    // only update when changed.
    Cranking = Starter;
  }


  // Cut the engine *power* - Note: the engine will continue to
  // spin depending on prop Ixx and freestream velocity

  if ( Running ) {
    if (!spark || !fuel)    Running = false;
    if (RPM < IdleRPM*0.8 ) Running = false;
  } else { // !Running
    if ( spark && fuel) {     // start the engine if revs high enough
      if (RPM > IdleRPM*0.8)  // This allows us to in-air start
        Running = true;       // when windmilling
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
  if(bBoostManual) {
    if(BoostSpeed > BoostSpeeds-1) BoostSpeed = BoostSpeeds-1;
    if(BoostSpeed < 0) BoostSpeed = 0;
  } else {
    if(BoostSpeed < BoostSpeeds - 1) {
      // Check if we need to change to a higher boost speed
      if(p_amb < BoostSwitchPressure[BoostSpeed] - BoostSwitchHysteresis) {
        BoostSpeed++;
      }
    } if(BoostSpeed > 0) {
      // Check if we need to change to a lower boost speed
      if(p_amb > BoostSwitchPressure[BoostSpeed - 1] + BoostSwitchHysteresis) {
        BoostSpeed--;
      }
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
 * Inputs: p_amb, Throttle,
 *         MeanPistonSpeed_fps, dt
 *
 * Outputs: MAP, ManifoldPressure_inHg, TMAP, BoostLossHP
 */

void FGPiston::doMAP(void)
{
  double Zt = (1 - in.ThrottlePos[EngineNumber])*(1 - in.ThrottlePos[EngineNumber])*Z_throttle; // throttle impedence
  double Ze= MeanPistonSpeed_fps > 0 ? PeakMeanPistonSpeed_fps/MeanPistonSpeed_fps : 999999; // engine impedence

  double map_coefficient = Ze/(Ze+Z_airbox+Zt);

  // Add a variable lag to manifold pressure changes
  double dMAP=(TMAP - p_ram * map_coefficient);
  if (ManifoldPressureLag > in.TotalDeltaT) dMAP *= in.TotalDeltaT/ManifoldPressureLag;

  TMAP -=dMAP;

  // Find the mean effective pressure required to achieve this manifold pressure
  // Fixme: determine the HP consumed by the supercharger

  PMEP = (TMAP - p_amb) * volumetric_efficiency; // Fixme: p_amb should be exhaust manifold pressure

  if (Boosted) {
    // If takeoff boost is fitted, we currently assume the following throttle map:
    // (In throttle % - actual input is 0 -> 1)
    // 99 / 100 - Takeoff boost
    // In real life, most planes would be fitted with a mechanical 'gate' between
    // the rated boost and takeoff boost positions.

    bool bTakeoffPos = false;
    if (bTakeoffBoost) {
      if (in.ThrottlePos[EngineNumber] > 0.98) {
        bTakeoffPos = true;
      }
    }
    // Boost the manifold pressure.
    double boost_factor = (( BoostMul[BoostSpeed] - 1 ) / RatedRPM[BoostSpeed] ) * RPM + 1;
    MAP = TMAP * boost_factor;
    // Now clip the manifold pressure to BCV or Wastegate setting.
    if(!bBoostOverride) {
      if (bTakeoffPos) {
        if (MAP > TakeoffMAP[BoostSpeed]) MAP = TakeoffMAP[BoostSpeed];
      } else {
        if (MAP > RatedMAP[BoostSpeed]) MAP = RatedMAP[BoostSpeed];
      }
    }
  } else {
      MAP = TMAP;
  }

  if( BoostLossFactor > 0.0 )
  {
      double gamma = 1.414; // specific heat constants
      double Nstage = 1; // Nstage is the number of boost stages.
      BoostLossHP = ((Nstage * TMAP * v_dot_air * gamma) / (gamma - 1)) * (pow((MAP/TMAP),((gamma-1)/(Nstage * gamma))) - 1) * BoostLossFactor / 745.7 ; // 745.7 convert watt to hp
  } else {
      BoostLossHP = 0;
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
 *   RPM, volumetric_efficiency,
 *
 * TODO: Model inlet manifold air temperature.
 *
 * Outputs: rho_air, m_dot_air, volumetric_efficiency_reduced
 */

void FGPiston::doAirFlow(void)
{
  double gamma = 1.3; // specific heat constants
// loss of volumentric efficiency due to difference between MAP and exhaust pressure
// Eq 6-10 from The Internal Combustion Engine - Charles Taylor Vol 1
  double mratio = MAP < 1. ? CompressionRatio : p_amb/MAP;
  if (mratio > CompressionRatio) mratio = CompressionRatio;
  double ve =((gamma-1)/gamma) +( CompressionRatio -(mratio))/(gamma*( CompressionRatio - 1));

  rho_air = p_amb / (R_air * T_amb);
  double swept_volume = (displacement_SI * (RPM/60)) / 2;
  volumetric_efficiency_reduced = volumetric_efficiency *ve;
  v_dot_air = swept_volume * volumetric_efficiency_reduced;

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
  double thi_sea_level = 1.3 * in.MixturePos[EngineNumber]; // Allows an AFR of infinity:1 to 11.3075:1
  equivalence_ratio = thi_sea_level * 101325.0 / p_amb;
  m_dot_fuel = (m_dot_air * equivalence_ratio) / 14.7;
  FuelFlowRate =  m_dot_fuel * 2.2046;  // kg to lb
  if(Starved) // There is no fuel, so zero out the flows we've calculated so far
  {
    equivalence_ratio = 0.0;
    FuelFlowRate = 0.0;
    m_dot_fuel = 0.0;
  }
  FuelFlow_pph = FuelFlowRate  * 3600;
  FuelFlow_gph = FuelFlow_pph / FuelDensity;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the power produced by the engine.
 *
 * Inputs: ManifoldPressure_inHg, p_amb, RPM, T_amb, ISFC,
 *   Mixture_Efficiency_Correlation, Cycles, MaxHP, PMEP,
 *   MeanPistonSpeed_fps
 *
 * Outputs: PctPower, HP, FMEP, IndicatedHorsePower
 */

void FGPiston::doEnginePower(void)
{
  IndicatedHorsePower = -StaticFriction_HP;
  FMEP = 0;
  if (Running) {
    double ME, power;  // Convienience term for use in the calculations
    ME = Mixture_Efficiency_Correlation->GetValue(m_dot_fuel/m_dot_air);

// Guestimate engine friction losses from Figure 4.4 of "Engines: An Introduction", John Lumley
    FMEP = (-FMEPDynamic * MeanPistonSpeed_fps * fttom - FMEPStatic);

    power = 1;

    if ( Magnetos != 3 ) power *= SparkFailDrop;


    IndicatedHorsePower = (FuelFlow_pph / ISFC )* ME * power - StaticFriction_HP; //FIXME static friction should depend on oil temp and configuration;

  } else {
    // Power output when the engine is not running
    double torque, k_torque, rpm;  // Convienience term for use in the calculations

    rpm = RPM < 1.0 ? 1.0 : RPM;
    if (Cranking) {
      if(RPM<StarterRPM) k_torque = 1.0-RPM/(StarterRPM);
      else k_torque = 0;
      torque = StarterTorque*k_torque*StarterGain;
      IndicatedHorsePower = torque * rpm / 5252;
     }
  }

  // Constant is (1/2) * 60 * 745.7
  // (1/2) convert cycles, 60 minutes to seconds, 745.7 watts to hp.
  double pumping_hp = ((PMEP + FMEP) * displacement_SI * RPM)/(Cycles*22371);

HP = IndicatedHorsePower + pumping_hp - BoostLossHP;
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
                              combustion_efficiency * 0.30;
    heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
    delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;
    ExhaustGasTemp_degK = T_amb + delta_T_exhaust;
  } else {  // Drop towards ambient - guess an appropriate time constant for now
    combustion_efficiency = 0;
    dEGTdt = (RankineToKelvin(in.Temperature) - ExhaustGasTemp_degK) / 100.0;
    delta_T_exhaust = dEGTdt * in.TotalDeltaT;

    ExhaustGasTemp_degK += delta_T_exhaust;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the cylinder head temperature.
 *
 * Inputs: T_amb, IAS, rho_air, m_dot_fuel, calorific_value_fuel,
 *   combustion_efficiency, RPM, MaxRPM, Displacement, Cylinders
 *
 * Outputs: CylinderHeadTemp_degK
 */

void FGPiston::doCHT(void)
{
  double h1 = -95.0;
  double h2 = -3.95;
  double h3 = -140.0; // -0.05 * 2800 (default maxrpm)

  double arbitary_area = Displacement/360.0;
  double CpCylinderHead = 800.0;
  double MassCylinderHead = CylinderHeadMass * Cylinders;

  double temperature_difference = CylinderHeadTemp_degK - T_amb;
  double v_apparent = IAS * Cooling_Factor;
  double v_dot_cooling_air = arbitary_area * v_apparent;
  double m_dot_cooling_air = v_dot_cooling_air * rho_air;
  double dqdt_from_combustion =
    m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
  double dqdt_forced = (h2 * m_dot_cooling_air * temperature_difference) +
    (h3 * RPM * temperature_difference / MaxRPM);
  double dqdt_free = h1 * temperature_difference * arbitary_area;
  double dqdt_cylinder_head = dqdt_from_combustion + dqdt_forced + dqdt_free;

  double HeatCapacityCylinderHead = CpCylinderHead * MassCylinderHead;

  CylinderHeadTemp_degK +=
    (dqdt_cylinder_head / HeatCapacityCylinderHead) * in.TotalDeltaT;

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

  OilTemp_degK += (dOilTempdt * in.TotalDeltaT);
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
  OilPressure_psi = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * RPM;

  if (OilPressure_psi >= Oil_Press_Relief_Valve) {
    OilPressure_psi = Oil_Press_Relief_Valve;
  }

  OilPressure_psi += (Design_Oil_Temp - OilTemp_degK) * Oil_Viscosity_Index * OilPressure_psi / Oil_Press_Relief_Valve;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// This is a local copy of the same function in FGStandardAtmosphere.

double FGPiston::GetStdPressure100K(double altitude) const
{
  // Limit this equation to input altitudes of 100000 ft.
  if (altitude > 100000.0) altitude = 100000.0;

  double alt[5];
  const double coef[5] = {  2116.217,
                          -7.648932746E-2,
                           1.0925498604E-6,
                          -7.1135726027E-12,
                           1.7470331356E-17 };

  alt[0] = 1;
  for (int pwr=1; pwr<=4; pwr++) alt[pwr] = alt[pwr-1]*altitude;

  double press = 0.0;
  for (int ctr=0; ctr<=4; ctr++) press += coef[ctr]*alt[ctr];
  return press;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineLabels(const string& delimiter)
{
  std::ostringstream buf;

  buf << Name << " Power Available (engine " << EngineNumber << " in ft-lbs/sec)" << delimiter
      << Name << " HP (engine " << EngineNumber << ")" << delimiter
      << Name << " equivalent ratio (engine " << EngineNumber << ")" << delimiter
      << Name << " MAP (engine " << EngineNumber << " in inHg)" << delimiter
      << Thruster->GetThrusterLabels(EngineNumber, delimiter);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPiston::GetEngineValues(const string& delimiter)
{
  std::ostringstream buf;

  buf << (HP * hptoftlbssec) << delimiter << HP << delimiter
      << equivalence_ratio << delimiter << ManifoldPressure_inHg << delimiter
      << Thruster->GetThrusterValues(EngineNumber, delimiter);

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
      cout << "      Cylinders Head Mass: " << CylinderHeadMass         << endl;
      cout << "      Compression Ratio: "   << CompressionRatio         << endl;
      cout << "      MaxHP: "               << MaxHP                    << endl;
      cout << "      Cycles: "              << Cycles                   << endl;
      cout << "      IdleRPM: "             << IdleRPM                  << endl;
      cout << "      MaxRPM: "              << MaxRPM                   << endl;
      cout << "      Throttle Constant: "   << Z_throttle               << endl;
      cout << "      ISFC: "                << ISFC                     << endl;
      cout << "      Volumetric Efficiency: " << volumetric_efficiency    << endl;
      cout << "      PeakMeanPistonSpeed_fps: " << PeakMeanPistonSpeed_fps << endl;
      cout << "      Intake Impedance Factor: " << Z_airbox << endl;
      cout << "      Dynamic FMEP Factor: " << FMEPDynamic << endl;
      cout << "      Static FMEP Factor: " << FMEPStatic << endl;

      cout << "      Starter Motor Torque: " << StarterTorque << endl;
      cout << "      Starter Motor RPM:    " << StarterRPM << endl;

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
    }
  }
}
} // namespace JSBSim
