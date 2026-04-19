/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPistonDiesel.cpp
 Author:       Jon S. Berndt, JSBSim framework
               Dave Luff, Piston engine model
               Ronald Jensen, Piston engine model
               Marc Traverso, Diesel Piston engine model
 Date started: 09/12/2000
 Purpose:      This module models a piston diesel engine

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

This class descends from the FGEngine class and models a diesel piston engine
based on parameters given in the engine config file for this class.

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created
14/04/2026  MTR  Derived FGPiston to create the FGPistonDiesel class implementation

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include "FGFDMExec.h"
#include "FGPistonDiesel.h"
#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPistonDiesel::FGPistonDiesel(FGFDMExec* exec, Element* el, int engine_number, struct Inputs& input)
  : FGEngine(engine_number, input)
{
  Load(exec, el);

  Element* table_element = nullptr;
  auto PropertyManager = exec->GetPropertyManager();

  Type = etPistonDiesel;

  // ===== DEFAULTS =====
  // Loosely based on a small automotive diesel (~1.4 L, 4-cylinder).

  // Geometry
  Cycles = 4;
  Displacement = 360;
  Bore = 2.95;
  Stroke = 3.03;
  Cylinders = 4;
  CylinderHeadMass = 12;    // kg
  CompressionRatio = 22;

  // Performance
  MaxHP = 50;
  MaxRPM = 5000;
  IdleRPM = 800;
  RatedRPM = 0.0;
  PeakTorqueRPM = -1;       // negative = derive during init

  // Volumetric efficiency
  volumetric_efficiency = 0.85;
  VE_peak = -1;             // negative = derive during init

  // Fuel
  injection_type = InjectionType::DI;
  AFR_smoke_limit = 20.0;
  calorific_value_fuel = 42.8e6;  // J/kg (EN 590 diesel)
  Cp_fuel = 1850.0;               // J/(kg*K)
  BSFC_best = -1;                 // negative = derive during init

  // Intake
  Z_airbox = -999;
  Ram_Air_Factor = 1;
  PeakMeanPistonSpeed_fps = 100;

  // Friction (Chen-Flynn)
  FMEPDynamic = 18400;      // Pa/(m/s)
  FMEPStatic = 46500;       // Pa
  StaticFriction_HP = 1.5;

  // Cooling
  Cooling_Factor = -1;
  coolant_heat_fraction = 0.30;

  // Starter
  StarterGain = 1.0;
  StarterTorque = -1.0;     // negative = derive during init
  StarterRPM = -1.0;        // negative = derive during init

  // Governor
  governor_droop = 0.06;
  governor_tau = 0.08;       // seconds
  IdleFuelRack = 0.12;
  idle_governor_gain = 0.005;

  // Cold start
  cold_enrichment_max = 1.20;
  cold_enrichment_threshold = 40.0;   // deg C
  GlowPlugThreshold_degK = 278.0;
  GlowPlugOn = false;

  // Turbocharger (default: naturally aspirated)
  Boosted = false;
  BoostMaxPressure_Pa = 0.0;
  BoostThreshold_RPM = 0.0;
  TurboLag_tau = 0.5;       // seconds
  BoostLossFactor = 0.0;

  // Oil system
  Design_Oil_Temp = 358;    // K
  Oil_Viscosity_Index = 0.25;
  Oil_Press_Relief_Valve = 60;   // psi
  Oil_Press_RPM_Max = 0.0;      // set after MaxRPM is read

  // Calibration (derived during init)
  m_fuel_max_per_cycle = -1;
  eta_indicated_calibrated = -1;

  // Internal state
  FuelRack = 0.0;
  m_fuel_per_cycle = 0.0;
  CurrentBoost_Pa = 0.0;

  // Outputs
  rho_air = 0.0;
  volumetric_efficiency_reduced = 0.0;
  m_dot_air = 0.0;
  v_dot_air = 0.0;
  m_dot_fuel = 0.0;
  HP = 0.0;
  combustion_efficiency = 0.0;
  ExhaustGasTemp_degK = 0.0;
  EGT_degC = 0.0;
  CylinderHeadTemp_degK = 0.0;
  OilPressure_psi = 0.0;
  OilTemp_degK = 0.0;
  MeanPistonSpeed_fps = 0.0;
  CoolantTemperature_degK = 0.0;
  FuelFlow_pps = 0.0;
  MAP = 101325.0;            // Pa - ambient, no throttle

  ResetToIC();

  // ===== READ CONFIGURATION =====

  if (el->FindElement("displacement"))
    Displacement = el->FindElementValueAsNumberConvertTo("displacement", "IN3");
  if (el->FindElement("maxhp"))
    MaxHP = el->FindElementValueAsNumberConvertTo("maxhp", "HP");
  if (el->FindElement("static-friction"))
    StaticFriction_HP = el->FindElementValueAsNumberConvertTo("static-friction", "HP");
  if (el->FindElement("cycles"))
    Cycles = el->FindElementValueAsNumber("cycles");
  if (el->FindElement("idlerpm"))
    IdleRPM = el->FindElementValueAsNumber("idlerpm");
  if (el->FindElement("maxrpm"))
    MaxRPM = el->FindElementValueAsNumber("maxrpm");
  if (el->FindElement("rated-rpm"))
    RatedRPM = el->FindElementValueAsNumber("rated-rpm");
  if (el->FindElement("peak-torque-rpm"))
    PeakTorqueRPM = el->FindElementValueAsNumber("peak-torque-rpm");
  if (el->FindElement("bsfc"))
    BSFC_best = el->FindElementValueAsNumberConvertTo("bsfc", "LBS/HP*HR");
  if (el->FindElement("volumetric-efficiency"))
    volumetric_efficiency = el->FindElementValueAsNumber("volumetric-efficiency");
  if (el->FindElement("compression-ratio"))
    CompressionRatio = el->FindElementValueAsNumber("compression-ratio");
  if (el->FindElement("bore"))
    Bore = el->FindElementValueAsNumberConvertTo("bore", "IN");
  if (el->FindElement("stroke"))
    Stroke = el->FindElementValueAsNumberConvertTo("stroke", "IN");
  if (el->FindElement("cylinders"))
    Cylinders = el->FindElementValueAsNumber("cylinders");
  if (el->FindElement("cylinder-head-mass"))
    CylinderHeadMass = el->FindElementValueAsNumberConvertTo("cylinder-head-mass", "KG");
  if (el->FindElement("air-intake-impedance-factor"))
    Z_airbox = el->FindElementValueAsNumber("air-intake-impedance-factor");
  if (el->FindElement("ram-air-factor"))
    Ram_Air_Factor = el->FindElementValueAsNumber("ram-air-factor");
  if (el->FindElement("cooling-factor"))
    Cooling_Factor = el->FindElementValueAsNumber("cooling-factor");
  if (el->FindElement("starter-rpm"))
    StarterRPM = el->FindElementValueAsNumber("starter-rpm");
  if (el->FindElement("starter-torque"))
    StarterTorque = el->FindElementValueAsNumber("starter-torque");
  if (el->FindElement("dynamic-fmep"))
    FMEPDynamic = el->FindElementValueAsNumberConvertTo("dynamic-fmep", "PA");
  if (el->FindElement("static-fmep"))
    FMEPStatic = el->FindElementValueAsNumberConvertTo("static-fmep", "PA");
  if (el->FindElement("peak-piston-speed"))
    PeakMeanPistonSpeed_fps = el->FindElementValueAsNumber("peak-piston-speed");

  // Diesel-specific XML parameters
  if (el->FindElement("calorific-value-fuel"))
    calorific_value_fuel = el->FindElementValueAsNumber("calorific-value-fuel");
  if (el->FindElement("bsfc-best"))
    BSFC_best = el->FindElementValueAsNumber("bsfc-best");
  if (el->FindElement("ve-peak"))
    VE_peak = el->FindElementValueAsNumber("ve-peak");
  if (el->FindElement("injection-type")) {
    std::string itype = el->FindElementValue("injection-type");
    injection_type = (itype == "DI") ? InjectionType::DI : InjectionType::IDI;
  }
  if (el->FindElement("afr-smoke-limit"))
    AFR_smoke_limit = el->FindElementValueAsNumber("afr-smoke-limit");
  if (el->FindElement("governor-droop"))
    governor_droop = el->FindElementValueAsNumber("governor-droop");
  if (el->FindElement("governor-tau"))
    governor_tau = el->FindElementValueAsNumber("governor-tau");
  if (el->FindElement("idle-fuel-rack"))
    IdleFuelRack = el->FindElementValueAsNumber("idle-fuel-rack");
  if (el->FindElement("idle-governor-gain"))
    idle_governor_gain = el->FindElementValueAsNumber("idle-governor-gain");
  if (el->FindElement("cold-enrichment-max"))
    cold_enrichment_max = el->FindElementValueAsNumber("cold-enrichment-max");
  if (el->FindElement("cold-enrichment-threshold"))
    cold_enrichment_threshold = el->FindElementValueAsNumber("cold-enrichment-threshold");
  if (el->FindElement("glow-plug-threshold"))
    GlowPlugThreshold_degK = el->FindElementValueAsNumberConvertTo("glow-plug-threshold", "DEGK");
  if (el->FindElement("coolant-heat-fraction"))
    coolant_heat_fraction = el->FindElementValueAsNumber("coolant-heat-fraction");

  // Turbocharging
  if (el->FindElement("boosted")) {
    Boosted = (el->FindElementValue("boosted") == "true");
    if (el->FindElement("boost-max-pressure"))
      BoostMaxPressure_Pa = el->FindElementValueAsNumberConvertTo("boost-max-pressure", "PA");
    if (el->FindElement("boost-threshold-rpm"))
      BoostThreshold_RPM = el->FindElementValueAsNumber("boost-threshold-rpm");
    if (el->FindElement("turbo-lag"))
      TurboLag_tau = el->FindElementValueAsNumber("turbo-lag");
    if (el->FindElement("boost-loss-factor"))
      BoostLossFactor = el->FindElementValueAsNumber("boost-loss-factor");
  }

  // Oil system
  Oil_Press_RPM_Max = MaxRPM * 0.75;
  if (el->FindElement("oil-pressure-relief-valve-psi"))
    Oil_Press_Relief_Valve = el->FindElementValueAsNumberConvertTo("oil-pressure-relief-valve-psi", "PSI");
  if (el->FindElement("design-oil-temp-degK"))
    Design_Oil_Temp = el->FindElementValueAsNumberConvertTo("design-oil-temp-degK", "DEGK");
  if (el->FindElement("oil-pressure-rpm-max"))
    Oil_Press_RPM_Max = el->FindElementValueAsNumber("oil-pressure-rpm-max");
  if (el->FindElement("oil-viscosity-index"))
    Oil_Viscosity_Index = el->FindElementValueAsNumber("oil-viscosity-index");

  // Lookup tables
  while ((table_element = el->FindNextElement("table")) != nullptr) {
    std::string name = table_element->GetAttributeValue("name");
    try {
      if (name == "COMBUSTION") {
        Lookup_Combustion_Efficiency = std::make_unique<FGTable>(PropertyManager, table_element);
      } else {
        FGLogging log(LogLevel::ERROR);
        log << "Unknown table type: " << name << " in piston engine definition.\n";
      }
    } catch (std::string& str) {
      XMLLogException err(table_element);
      Lookup_Combustion_Efficiency.reset();
      err << "Error loading piston engine table:" + name + ". " + str << "\n";
      throw err;
    }
  }

  // ===== POST-READ VALIDATION =====

  volumetric_efficiency_reduced = volumetric_efficiency;

  if (MaxHP <= 0 || MaxRPM <= 0 || Displacement <= 0) {
    throw BaseException(
        "FGPistonDiesel: displacement, maxhp, and maxrpm are required.");
  }

  // ===== DERIVED DEFAULTS =====

  if (StarterRPM < 0.0)
    StarterRPM = 2 * IdleRPM;
  if (StarterTorque < 0.0)
    StarterTorque = MaxHP * 0.4;

  displacement_SI = Displacement * in3tom3;
  // AKA 2 * (RPM/60) * (Stroke/12)
  RatedMeanPistonSpeed_fps = (MaxRPM * Stroke) / 360.0;

  if (Oil_Press_RPM_Max <= 0.0)
    Oil_Press_RPM_Max = MaxRPM * 0.75;

  if (RatedRPM <= 0.0)
    RatedRPM = MaxRPM * 0.85;

  if (PeakTorqueRPM < 0.0)
    PeakTorqueRPM = RatedRPM * 0.55;

  // ===== DERIVE BSFC IF NOT PROVIDED =====

  if (BSFC_best < 0.0) {
    double eta_brake_best = 0.10
        + 0.35 * (1.0 - 1.0 / std::pow(CompressionRatio, 0.35));

    if (injection_type == InjectionType::IDI) {
      eta_brake_best *= 0.92;
    }

    if (displacement_SI < 0.002) {
      double size_penalty = 1.0
          - 0.03 * (2.0 - displacement_SI * 1000.0);
      eta_brake_best *= size_penalty;
    }

    BSFC_best = 3.6e9 / (eta_brake_best * calorific_value_fuel);

    if (Cooling_Factor < 0.0)
      Cooling_Factor = 5.0 * MaxHP;

    FGLogging log(LogLevel::INFO);
    log << "FGPistonDiesel: BSFC_best estimated at "
        << BSFC_best << " g/(kW*hr)"
        << " (eta_brake = " << eta_brake_best << ").\n";

    if (BSFC_best < 190.0 || BSFC_best > 320.0) {
      log << "WARNING: BSFC outside typical range (190-320), clamping.\n";
      BSFC_best = Constrain(190.0, BSFC_best, 320.0);
    }
  }

  // ===== DERIVE FUEL CALIBRATION =====

  double MaxHP_kW = MaxHP * 0.7457;
  double fuel_flow_rated_kgs = (BSFC_best * MaxHP_kW) / 3.6e6;
  double injections_per_sec = (RatedRPM / 60.0) * Cylinders / (Cycles / 2.0);
  m_fuel_max_per_cycle = fuel_flow_rated_kgs / injections_per_sec;

  // ===== DERIVE VE_PEAK IF NOT PROVIDED =====

  if (VE_peak < 0.0) {
    double m_dot_air_rated = fuel_flow_rated_kgs * AFR_smoke_limit;
    double rho_sl = standard_pressure / (R_air * 288.15);
    double swept_rate_rated = (displacement_SI * (RatedRPM / 60.0))
                               / (Cycles / 2.0);

    double VE_at_rated = m_dot_air_rated / (swept_rate_rated * rho_sl);

    double rpm_norm_rated = RatedRPM / MaxRPM;
    double curve_at_rated = 1.0 - 0.35 * std::pow(rpm_norm_rated - 0.42, 2);
    if (rpm_norm_rated > 0.75) {
      curve_at_rated -= 0.15 * std::pow(rpm_norm_rated - 0.75, 1.5);
    }

    if (curve_at_rated > 0.1) {
      VE_peak = VE_at_rated / curve_at_rated;
    } else {
      VE_peak = VE_at_rated;
    }

    FGLogging log(LogLevel::INFO);
    log << "FGPistonDiesel: VE_peak estimated at " << VE_peak << ".\n";

    if (VE_peak < 0.70 || VE_peak > 0.98) {
      log << "WARNING: VE_peak outside typical range (0.70-0.98), clamping.\n";
      VE_peak = Constrain(0.70, VE_peak, 0.98);
    }
  }

  // ===== DERIVE INDICATED EFFICIENCY =====

  double V_cylinder = displacement_SI / Cylinders;
  double MPS_rated = Stroke * intom * RatedRPM / 30.0;
  double FMEP_rated = FMEPStatic + FMEPDynamic * MPS_rated;

  double MaxTorque_Nm = (MaxHP * 745.7)
      / (RatedRPM * 2.0 * M_PI / 60.0);
  double BMEP_rated = (MaxTorque_Nm * 2.0 * M_PI * (Cycles / 2.0))
      / displacement_SI;
  double IMEP_rated = BMEP_rated + FMEP_rated;

  eta_indicated_calibrated = (IMEP_rated * V_cylinder)
      / (m_fuel_max_per_cycle * calorific_value_fuel);

  // Default combustion efficiency table if not provided
  if (!Lookup_Combustion_Efficiency) {
    // First column is fuel rack (normalised load), second is combustion efficiency
    Lookup_Combustion_Efficiency = std::make_unique<FGTable>(8);
    *Lookup_Combustion_Efficiency << 0.00 << 0.75;  // idle: poor atomization
    *Lookup_Combustion_Efficiency << 0.10 << 0.85;  // light load
    *Lookup_Combustion_Efficiency << 0.20 << 0.92;
    *Lookup_Combustion_Efficiency << 0.40 << 0.96;
    *Lookup_Combustion_Efficiency << 0.60 << 0.98;  // mid load: best
    *Lookup_Combustion_Efficiency << 0.80 << 0.97;  // high load: approaching smoke
    *Lookup_Combustion_Efficiency << 0.95 << 0.93;  // near smoke limit
    *Lookup_Combustion_Efficiency << 1.00 << 0.88;  // at smoke limit
  }

  // ===== BIND PROPERTIES =====

  std::string base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNumber);

  auto bind = [&](const char* suffix, auto* var_or_getter) {
    std::string prop = base_property_name + suffix;
    PropertyManager->Tie(prop, var_or_getter);
  };
  auto bind_method = [&](const char* suffix, auto method) {
    std::string prop = base_property_name + suffix;
    PropertyManager->Tie(prop, this, method);
  };

  bind("/power-hp",                     &HP);
  bind("/friction-hp",                  &StaticFriction_HP);
  bind("/starter-norm",                 &StarterGain);
  bind("/volumetric-efficiency",        &volumetric_efficiency);
  bind("/air-intake-impedance-factor",  &Z_airbox);
  bind("/ram-air-factor",               &Ram_Air_Factor);
  bind("/cooling-factor",               &Cooling_Factor);
  bind("/fuel-rack",                    &FuelRack);
  // Note: "/fuel-flow-rate-pps" is already bound by FGEngine::Load()
  // via GetFuelFlowRate(). Binding it again here causes a double-tie error.
  bind("/bsfc",                         &BSFC_best);
  bind("/map-pa",                       &MAP);
  bind("/glow-plug-on",                 &GlowPlugOn);
  bind("/combustion-efficiency",        &combustion_efficiency);
  bind("/m-dot-air-kgs",                &m_dot_air);
  bind("/ve-reduced",                   &volumetric_efficiency_reduced);
  bind("/cranking",                     &Cranking);
  bind("/rpm",                          &RPM);
  bind("/indicated-hp",                 &IndicatedHorsePower);
  bind("/torque-si",                    &Torque_SI);

  bind_method("/coolant-temperature-degF", &FGPistonDiesel::getCoolantTemperature_degF);
  bind_method("/cht-degF",                 &FGPistonDiesel::getCylinderHeadTemp_degF);
  bind_method("/oil-temperature-degF",     &FGPistonDiesel::getOilTemp_degF);
  bind_method("/oil-pressure-psi",         &FGPistonDiesel::getOilPressure_psi);
  bind_method("/egt-degF",                 &FGPistonDiesel::getExhaustGasTemp_degF);
  bind_method("/AFR",                      &FGPistonDiesel::getAFR);

  if (Boosted) {
    bind("/boost-pa", &CurrentBoost_Pa);
    if (BoostLossFactor > 0.0) {
      bind("/boostloss-factor", &BoostLossFactor);
      bind("/boostloss-hp",    &BoostLossHP);
    }
  }

  // Turbocharger sanity checks
  if (Boosted) {
    if (BoostMaxPressure_Pa <= 0.0) {
      FGLogging log(LogLevel::WARN);
      log << "FGPistonDiesel: boosted=true but boost-max-pressure "
          << "not set or zero. Defaulting to 1.5 bar absolute.\n";
      BoostMaxPressure_Pa = 150000.0;
    }
    if (BoostThreshold_RPM <= 0.0) {
      BoostThreshold_RPM = IdleRPM * 1.5;
      FGLogging log(LogLevel::INFO);
      log << "FGPistonDiesel: boost-threshold-rpm not set, "
          << "estimating " << BoostThreshold_RPM << " RPM.\n";
    }
    if (TurboLag_tau <= 0.0) {
      TurboLag_tau = 0.5;
    }
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPistonDiesel::~FGPistonDiesel()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPistonDiesel::ResetToIC()
{
  FGEngine::ResetToIC();

  // Intake pressure - no throttle, MAP is always ambient
  MAP = in.Pressure * psftopa;

  // Temperatures - everything starts at ambient
  double airTemperature_degK = RankineToKelvin(in.Temperature);
  OilTemp_degK = airTemperature_degK;
  CylinderHeadTemp_degK = airTemperature_degK;
  CoolantTemperature_degK = airTemperature_degK;
  ExhaustGasTemp_degK = airTemperature_degK;
  EGT_degC = ExhaustGasTemp_degK - 273.15;

  // Engine state
  Thruster->SetRPM(0.0);
  RPM = 0.0;
  HP = 0.0;
  Torque_SI = 0.0;
  IndicatedHorsePower = 0.0;
  Running = false;
  Cranking = false;

  // Fuel system
  FuelRack = 0.0;
  m_fuel_per_cycle = 0.0;
  m_dot_fuel = 0.0;
  FuelFlow_pps = 0.0;
  FuelFlowRate = 0.0;

  // Air flow
  m_dot_air = 0.0;
  v_dot_air = 0.0;
  rho_air = 0.0;
  volumetric_efficiency_reduced = volumetric_efficiency;
  combustion_efficiency = 0.0;
  MeanPistonSpeed_fps = 0.0;
  equivalence_ratio = 0.0;

  // Oil
  OilPressure_psi = 0.0;

  // Turbo
  CurrentBoost_Pa = 0.0;
  BoostLossHP = 0.0;

  // Glow plug off
  GlowPlugOn = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPistonDiesel::Calculate()
{
  p_amb = in.Pressure * psftopa;
  double p = in.TotalPressure * psftopa;
  p_ram = (p - p_amb) * Ram_Air_Factor + p_amb;
  T_amb = RankineToKelvin(in.Temperature);

  RunPreFunctions();

  // The thruster controls the engine RPM because it encapsulates
  // the gear ratio and other transmission variables.
  RPM = Thruster->GetEngineRPM();

  // AKA 2 * (RPM/60) * (Stroke/12)
  MeanPistonSpeed_fps = (RPM * Stroke) / 360.0;

  IAS = in.Vc;

  doEngineStartup();

  MAP = p_amb;
  if (Boosted) doTurbo();
  doAirFlow();
  doFuelRack();
  doFuelFlow();

  doEnginePower();
  if (Running && IndicatedHorsePower < 0.1250) Running = false;

  doEGT();
  doCHT();
  doCoolantTemperature();
  doOilTemperature();
  doOilPressure();

  if (Thruster->GetType() == FGThruster::ttPropeller) {
    auto* prop = static_cast<FGPropeller*>(Thruster);
    prop->SetAdvance(in.PropAdvance[EngineNumber]);
    prop->SetFeather(in.PropFeather[EngineNumber]);
  }

  LoadThrusterInputs();

  // Filter out negative power when the propeller is not rotating.
  double power = HP * hptoftlbssec;
  if (RPM <= 0.1) power = std::max(power, 0.0);
  Thruster->Calculate(power);

  RunPostFunctions();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPistonDiesel::CalcFuelNeed()
{
  FuelExpended = FuelFlowRate * in.TotalDeltaT;
  if (!Starved) FuelUsedLbs += FuelExpended;
  return FuelExpended;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGPistonDiesel::InitRunning()
{
  // No mixture control on diesel - set throttle to idle.
  in.ThrottleCmd[EngineNumber] = 0.0;
  in.ThrottlePos[EngineNumber] = 0.0;

  Thruster->SetRPM(2.0 * IdleRPM / Thruster->GetGearRatio());
  Running = true;
  Cranking = false;

  // Initialize temperatures to warm engine conditions
  // rather than ambient - the engine is already running.
  CoolantTemperature_degK = 273.15 + 80.0;  // 80 deg C - normal operating temp
  OilTemp_degK = 273.15 + 75.0;             // 75 deg C - warm oil
  CylinderHeadTemp_degK = 273.15 + 90.0;    // 90 deg C - warm CHT

  // Governor will settle fuel rack to idle on first frame.
  FuelRack = IdleFuelRack;

  // No boost at idle.
  CurrentBoost_Pa = 0.0;

  return 1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Start or stop the engine.
 */

void FGPistonDiesel::doEngineStartup()
{
  // Check parameters that may alter the operating state of the engine
  // (fuel, starter motor, etc.).
  // Neglects battery voltage, master-on switch, etc. for now.

  // We will 'run' with any fuel flow.  If there is not enough fuel to
  // make power it will show in doEnginePower.
  bool fuel = (FuelFlowRate > 0.0);

  // Cranking follows the starter command, but the overrunning clutch disengages
  // the starter automatically once the engine is self-sustaining.
  Cranking = Starter && !Running;

  // Cut the engine *power* - the engine will continue to spin
  // depending on prop Ixx and freestream velocity.
  if (Running) {
    if (!fuel) Running = false;
    if (RPM < IdleRPM * 0.5) Running = false;
  } else {
    if (fuel && RPM > IdleRPM * 0.5) {
      // This allows in-air start when windmilling.
      Running = true;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPistonDiesel::doTurbo()
{
  // Simple exhaust-driven turbo model.
  // Boost is a function of RPM and load - more exhaust energy
  // means more turbine power means more boost.

  double target_boost = 0.0;

  if (Running && RPM > BoostThreshold_RPM) {
    // Boost builds proportionally to RPM above threshold,
    // scaled by fuel rack (more fuel = more exhaust energy).
    double rpm_factor = (RPM - BoostThreshold_RPM)
                        / (MaxRPM - BoostThreshold_RPM);
    rpm_factor = Constrain(0.0, rpm_factor, 1.0);

    target_boost = BoostMaxPressure_Pa * rpm_factor * FuelRack;
  }

  // Turbo lag - first-order lag on spool-up and spool-down.
  double dt = in.TotalDeltaT;
  CurrentBoost_Pa += (target_boost - CurrentBoost_Pa)
                     * (1.0 - std::exp(-dt / TurboLag_tau));

  // Apply boost to intake pressure.
  MAP = p_amb + CurrentBoost_Pa;

  // Boost power loss - turbine extracts energy from exhaust.
  if (BoostLossFactor > 0.0) {
    BoostLossHP = HP * BoostLossFactor * (CurrentBoost_Pa / BoostMaxPressure_Pa);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the air flow through the engine.
 * Also calculates ambient air density (used in CHT calculation).
 *
 * Inputs: p_amb, R_air, T_amb, MAP, Displacement, RPM, volumetric_efficiency
 *
 * Outputs: rho_air, m_dot_air, volumetric_efficiency_reduced
 */

void FGPistonDiesel::doAirFlow()
{
  // Diesel: no throttle plate.  Intake is always at full pressure.
  // MAP = ambient pressure (naturally aspirated) or boost pressure (turbo).

  double intake_pressure = MAP;
  double intake_temp = T_amb;

  // Air density at ambient and intake conditions
  rho_air = p_amb / (R_air * T_amb);
  double rho_intake = intake_pressure / (R_air * intake_temp);

  // Swept volume flow rate (4-stroke: intake every other rev)
  double swept_volume = (displacement_SI * (RPM / 60.0)) / (Cycles / 2.0);

  // Volumetric efficiency: peaks around 42 % of max RPM for a generic NA diesel.
  double rpm_norm = RPM / MaxRPM;
  double ve_base = VE_peak * (1.0 - 0.35 * std::pow(rpm_norm - 0.42, 2));

  // High RPM falloff steeper than gasoline due to heavier valve train
  // and swirl requirements.
  if (rpm_norm > 0.75) {
    ve_base -= 0.15 * std::pow(rpm_norm - 0.75, 1.5);
  }

  // Airbox impedance correction
  if (Z_airbox > 0) {
    double airbox_loss = Z_airbox * m_dot_air;
    ve_base *= std::max(0.5, 1.0 - airbox_loss);
  }

  volumetric_efficiency_reduced = ve_base;

  // Residual gas correction - minimal for diesel but not zero.
  double residual_fraction = 0.03;
  if (Boosted) {
    // Positive scavenging: boost > exhaust, less residual.
    residual_fraction = std::max(0.01, 0.03 * (p_amb / intake_pressure));
  }
  volumetric_efficiency_reduced *= (1.0 - residual_fraction);

  // Clamp VE to sane range
  volumetric_efficiency_reduced = Constrain(0.0, volumetric_efficiency_reduced, 1.0);

  // Volume and mass flow
  v_dot_air = swept_volume * volumetric_efficiency_reduced;
  m_dot_air = v_dot_air * rho_intake;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the fuel rack position based on governor and driver demand.
 *
 * Models a mechanical centrifugal governor typical of Bosch VE / Lucas DPC
 * rotary distributor injection pumps.
 *
 * Inputs: ThrottlePos, RPM, m_dot_air, CoolantTemperature_degK, Running, Cranking
 *
 * Outputs: FuelRack, m_fuel_per_cycle, m_dot_fuel, FuelFlow_pps
 */

void FGPistonDiesel::doFuelRack()
{
  double dt = in.TotalDeltaT;

  // === FUEL CUTOFF ===
  // Electromagnetic shutoff solenoid - no fuel when engine is off.
  if (!Running && !Cranking) {
    // Solenoid cuts fuel — rack target is zero, but decay through
    // governor lag so that a rapid Running toggle doesn't reset
    // the rack to zero and prevent restart.
    double dt = in.TotalDeltaT;
    if (dt > 0.0) {
        FuelRack *= std::exp(-dt / governor_tau);
    } else {
        FuelRack = 0.0;
    }    m_fuel_per_cycle = 0.0;
    m_dot_fuel = 0.0;
    FuelFlow_pps = 0.0;
    return;
  }

  // === DRIVER DEMAND ===
  // Throttle lever moves the control lever on the injection pump.
  double throttle_cmd = in.ThrottlePos[EngineNumber];

  // Demanded RPM: pedal position maps to a target RPM range.
  double RPM_demanded = IdleRPM + throttle_cmd * (MaxRPM - IdleRPM);

  // === CENTRIFUGAL GOVERNOR ===
  // Compares actual RPM to demanded RPM and adjusts fuel delivery.
  // Modeled as a proportional controller with droop.
  double rpm_error = RPM_demanded - RPM;
  double governor_gain = 1.0 / (governor_droop * MaxRPM);
  double rack_governor = 0.5 + rpm_error * governor_gain;

  // === IDLE GOVERNOR ===
  // Below idle RPM, governor forces fuel up to maintain idle.
  double idle_rack;
  if (RPM < IdleRPM) {
    double idle_error = IdleRPM - RPM;
    idle_rack = IdleFuelRack + idle_error * idle_governor_gain;
  } else {
    idle_rack = IdleFuelRack;
  }

  // === MAX RPM GOVERNOR (CUTOFF) ===
  // Flyweights hit a stiffer spring - fuel drops rapidly.
  double max_rpm_rack = 1.0;
  if (RPM > MaxRPM * 0.95) {
    double overspeed = (RPM - MaxRPM * 0.95) / (MaxRPM * 0.10);
    max_rpm_rack = 1.0 - overspeed * 2.0;
  }

  // === CONTROL LEVER LIMIT ===
  // Driver's pedal sets the ceiling on fuel delivery.
  double lever_max_rack = 0.10 + throttle_cmd * 0.90;

  // === COMBINE ===
  double rack_raw = rack_governor;
  rack_raw = std::min(rack_raw, lever_max_rack);  // driver demand ceiling
  rack_raw = std::min(rack_raw, max_rpm_rack);    // overspeed protection
  rack_raw = std::max(rack_raw, idle_rack);       // idle governor floor - ALWAYS LAST

  double injections_per_sec = (RPM / 60.0) * Cylinders / (Cycles / 2.0);

  // === SMOKE LIMIT ===
  // Cannot inject more fuel than the available air can burn.
  if (!Cranking) {
    double m_air_per_cycle = 0.0;
    if (injections_per_sec > 0.0) {
        m_air_per_cycle = m_dot_air / injections_per_sec;
    }
    double m_fuel_smoke = m_air_per_cycle / AFR_smoke_limit;
    double rack_smoke_limit = 1.0;
    if (m_fuel_max_per_cycle > 0.0) {
        rack_smoke_limit = m_fuel_smoke / m_fuel_max_per_cycle;
    }
    rack_raw = std::min(rack_raw, rack_smoke_limit);
  }

// === COLD START ENRICHMENT ===
  // Thermo-wax element allows extra fuel when engine is cold.
  double CoolantTemp_degC = CoolantTemperature_degK - 273.15;
  double cold_enrichment = 1.0;
  if (CoolantTemp_degC < cold_enrichment_threshold) {
    cold_enrichment = 1.0 + (cold_enrichment_max - 1.0)
        * (cold_enrichment_threshold - CoolantTemp_degC)
        / cold_enrichment_threshold;
  }
  // Cold enrichment raises smoke limit and lever limit only
  double rack_ceiling = std::min(lever_max_rack * cold_enrichment, 1.0);
  if (rack_raw <= lever_max_rack || rack_raw > rack_ceiling) {
    rack_raw = std::min(rack_raw, rack_ceiling);
  }

  // === CRANKING ENRICHMENT ===
  // During cranking, governor isn't spinning fast enough to regulate.
  if (Cranking && !Running) {
    rack_raw = std::max(rack_raw, IdleFuelRack * 1.5);
  }

  // === GOVERNOR DYNAMIC RESPONSE ===
  // Mechanical governor has inertia - first-order lag.
  if (dt > 0.0 && !Cranking) {
    FuelRack += (rack_raw - FuelRack) * (1.0 - std::exp(-dt / governor_tau));
  } else {
    FuelRack = rack_raw;
  }

  // Final clamp
  FuelRack = Constrain(0.0, FuelRack, 1.0);

  // === FUEL QUANTITY ===
  m_fuel_per_cycle = FuelRack * m_fuel_max_per_cycle;

  // === FUEL FLOW RATE ===
  m_dot_fuel = m_fuel_per_cycle * injections_per_sec;  // kg/s
  FuelFlow_pps = m_dot_fuel * kgtolb;                  // lb/s
  FuelFlow_gph = m_dot_fuel * 3600.0 / 1000.0;         // g/h
  FuelFlowRate = FuelFlow_pps;                         // for CalcFuelNeed

  // === STARVED CHECK ===
  if (Starved) {
    FuelRack = 0.0;
    m_fuel_per_cycle = 0.0;
    m_dot_fuel = 0.0;
    FuelFlow_pps = 0.0;
    FuelFlowRate = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the fuel flow into the engine.
 *
 * Inputs: m_dot_air, m_dot_fuel, FuelRack
 *
 * Outputs: equivalence_ratio, combustion_efficiency
 */

void FGPistonDiesel::doFuelFlow()
{
  // Air/Fuel ratio
  if (m_dot_fuel > 0.0) {
    double AFR = m_dot_air / m_dot_fuel;
    // Equivalence ratio: actual FAR / stoichiometric FAR
    // Stoichiometric AFR for diesel is approximately 14.5
    equivalence_ratio = 14.5 / AFR;
  } else {
    equivalence_ratio = 0.0;
  }

  // Combustion efficiency lookup from fuel rack position.
  if (Lookup_Combustion_Efficiency) {
    combustion_efficiency = Lookup_Combustion_Efficiency->GetValue(FuelRack);
  } else {
    combustion_efficiency = 0.98;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the engine power output.
 *
 * Inputs: m_fuel_per_cycle, combustion_efficiency, RPM,
 *         eta_indicated_calibrated, FuelRack, displacement_SI,
 *         Cylinders, Cycles
 *
 * Outputs: HP, Torque_SI, IndicatedHorsePower
 */

void FGPistonDiesel::doEnginePower()
{
  if (!Running && !Cranking) {
    HP = 0.0;
    Torque_SI = 0.0;
    IndicatedHorsePower = 0.0;
    return;
  }

  double V_cylinder = displacement_SI / Cylinders;

  // === INDICATED EFFICIENCY WITH CORRECTIONS ===

  // RPM correction: efficiency drops at very low and very high RPM.
  double rpm_ratio = RPM / RatedRPM;
  double eta_rpm_correction;
  if (rpm_ratio < 0.4) {
    // Low RPM: heat losses dominate, poor spray atomization
    eta_rpm_correction = 0.92 + 0.2 * rpm_ratio;
  } else if (rpm_ratio < 0.85) {
    // Mid range: best efficiency
    eta_rpm_correction = 1.0;
  } else {
    // High RPM: less time for combustion
    eta_rpm_correction = 1.0 - 0.4 * (rpm_ratio - 0.85);
  }

  // Load correction: diesel is most efficient at 60-80% load.
  double load_ratio = FuelRack;
  double eta_load_correction;
  if (load_ratio < 0.1) {
    eta_load_correction = 0.70;
  } else if (load_ratio < 0.3) {
    eta_load_correction = 0.70 + (load_ratio - 0.1) * 1.5;
  } else if (load_ratio < 0.8) {
    eta_load_correction = 1.0;
  } else {
    eta_load_correction = 1.0 - 0.1 * (load_ratio - 0.8);
  }

  double eta_current = eta_indicated_calibrated
                       * eta_rpm_correction
                       * eta_load_correction;

  // === IMEP FROM FUEL ENERGY ===
  double IMEP = (eta_current * m_fuel_per_cycle * calorific_value_fuel
                 * combustion_efficiency) / V_cylinder;

  // === FRICTION LOSSES ===
  double MPS = Stroke * intom * RPM / 30.0;
  double FMEP_local = FMEPStatic + FMEPDynamic * MPS;

  // === BMEP AND TORQUE ===
  double BMEP = IMEP - FMEP_local;

  Torque_SI = (BMEP * displacement_SI) / (2.0 * M_PI * (Cycles / 2.0));

  // === POWER ===
  double omega = RPM * 2.0 * M_PI / 60.0;
  HP = Torque_SI * omega / 745.7;

  // === INDICATED POWER (for stall detection) ===
  double Torque_indicated = (IMEP * displacement_SI)
                            / (2.0 * M_PI * (Cycles / 2.0));
  IndicatedHorsePower = Torque_indicated * omega / 745.7;

  // === STATIC FRICTION ===
  // Additional parasitic loss from accessories (alternator, water pump, etc.)
  // Not applied during cranking: at standstill the starter torque is tiny
  // (~0.004 HP) and the 1 HP accessory drag would clamp net HP to zero,
  // preventing the engine from ever starting. FMEP already models the
  // mechanical cranking resistance.
  if (Running || !Cranking) {
    HP -= StaticFriction_HP;
  }

  // === BOOST LOSS ===
  if (Boosted && BoostLossHP > 0.0) {
    HP -= BoostLossHP;
  }

  // === STARTER MOTOR ===
  // A diesel fires by compression ignition, so combustion contributes during
  // cranking. Add starter torque on top of the combustion model so both
  // sources help spin the engine up to the Running transition RPM.
  // Clamp rpm to 1.0 so the starter produces nonzero power even at standstill.
  if (Cranking) {
    double rpm = RPM < 1.0 ? 1.0 : RPM;
    double k_torque = (rpm < StarterRPM) ? 1.0 - rpm / StarterRPM : 0.0;
    HP += StarterTorque * k_torque * StarterGain * rpm / 5252.0;
  }

  // Engine can produce negative power (being motored). Leave it negative
  // so the propeller/thruster sees the drag correctly.
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the exhaust gas temperature.
 *
 * Inputs: equivalence_ratio, m_dot_fuel, calorific_value_fuel,
 *         Cp_air, m_dot_air, T_amb
 *
 * Outputs: ExhaustGasTemp_degK, EGT_degC
 */

void FGPistonDiesel::doEGT()
{
  if (Running && m_dot_air > 0.0 && m_dot_fuel > 0.0) {
    // Exhaust enthalpy: fuel energy not converted to work and not
    // lost to coolant goes out the exhaust.
    double exhaust_fraction = 1.0 - eta_indicated_calibrated - coolant_heat_fraction;
    exhaust_fraction = Constrain(0.15, exhaust_fraction, 0.50);

    double enthalpy_exhaust = m_dot_fuel * calorific_value_fuel
                              * combustion_efficiency * exhaust_fraction;

    // Heat capacity of exhaust gas - mostly air since diesel
    // always runs lean with large excess air.
    double heat_capacity_exhaust = Cp_air * (m_dot_air + m_dot_fuel);

    double delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;
    double target_EGT = T_amb + delta_T_exhaust;

    // EGT doesn't change instantly - exhaust manifold has thermal mass.
    double egt_tau = 2.0;  // seconds
    double dt = in.TotalDeltaT;
    if (dt > 0.0) {
      ExhaustGasTemp_degK += (target_EGT - ExhaustGasTemp_degK)
                             * (1.0 - std::exp(-dt / egt_tau));
    }
  } else {
    // Engine not running - cool down toward ambient.
    double dEGTdt = (T_amb - ExhaustGasTemp_degK) / 100.0;
    ExhaustGasTemp_degK += dEGTdt * in.TotalDeltaT;
  }

  EGT_degC = ExhaustGasTemp_degK - 273.15;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the cylinder head temperature.
 *
 * Inputs: T_amb, IAS, rho_air, m_dot_fuel, calorific_value_fuel,
 *         combustion_efficiency, RPM, MaxRPM, Displacement, Cylinders
 *
 * Outputs: CylinderHeadTemp_degK
 */

void FGPistonDiesel::doCHT()
{
  double dt = in.TotalDeltaT;
  if (dt <= 0.0) return;

  double CpCylinderHead = 800.0;   // J/(kg*K) - cast iron/aluminum
  double MassCylinderHead = CylinderHeadMass * Cylinders;
  double HeatCapacityCylinderHead = CpCylinderHead * MassCylinderHead;

  // === HEAT INPUT FROM COMBUSTION ===
  // Cylinder head absorbs ~12 % of fuel energy directly.
  double dqdt_from_combustion =
      m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.12;

  // === HEAT REJECTION TO COOLANT ===
  // Primary cooling path on a liquid-cooled diesel.
  double head_to_coolant_coeff = 45.0;  // W/K

  // Coolant flow scales with RPM (water pump is crankshaft-driven).
  double pump_flow;
  if (RPM < 1.0) {
    pump_flow = 0.0;
  } else if (RPM < IdleRPM) {
    pump_flow = RPM / IdleRPM;
  } else {
    pump_flow = 1.0;
  }

  double dqdt_to_coolant = head_to_coolant_coeff * pump_flow
      * (CylinderHeadTemp_degK - CoolantTemperature_degK);

  // === HEAT REJECTION TO AIR (SURFACE RADIATION/CONVECTION) ===
  double arbitrary_area = Displacement / 360.0;
  double h_free = -95.0;  // free convection coefficient
  double temperature_difference_amb = CylinderHeadTemp_degK - T_amb;
  double dqdt_to_air = h_free * temperature_difference_amb * arbitrary_area;

  // Ram air effect on external surfaces - minor on liquid-cooled.
  double v_apparent = IAS * Cooling_Factor * 0.2;
  double dqdt_ram = -2.0 * arbitrary_area * v_apparent * temperature_difference_amb;

  // === NET HEAT BALANCE ===
  double dqdt_net = dqdt_from_combustion - dqdt_to_coolant + dqdt_to_air + dqdt_ram;

  CylinderHeadTemp_degK += (dqdt_net / HeatCapacityCylinderHead) * dt;

  // Clamp to physical limits.
  CylinderHeadTemp_degK = Constrain(T_amb, CylinderHeadTemp_degK, 273.15 + 350.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate coolant temperature.
 *
 * Liquid cooling model for diesel engine.  Heat input from combustion,
 * heat rejection through radiator scaled by airspeed.
 * Thermostat controls coolant flow through radiator.
 *
 * Inputs: m_dot_fuel, calorific_value_fuel, combustion_efficiency,
 *         T_amb, IAS, RPM, coolant_heat_fraction
 *
 * Outputs: CoolantTemperature_degK
 */

void FGPistonDiesel::doCoolantTemperature()
{
  double dt = in.TotalDeltaT;
  if (dt <= 0.0) return;

  double Q_fuel_total = m_dot_fuel * calorific_value_fuel * combustion_efficiency;
  double Q_coolant_in = Q_fuel_total * coolant_heat_fraction;  // watts

  // === THERMOSTAT ===
  // Wax thermostat: closed below 83 deg C, fully open above 89 deg C.
  double coolant_degC = CoolantTemperature_degK - 273.15;
  double thermostat;
  if (coolant_degC < 83.0) {
    thermostat = 0.05;  // small bypass flow even when closed
  } else if (coolant_degC > 89.0) {
    thermostat = 1.0;
  } else {
    thermostat = 0.05 + 0.95 * (coolant_degC - 83.0) / 6.0;
  }

  // === RADIATOR HEAT REJECTION ===
  double delta_T = CoolantTemperature_degK - T_amb;
  double airflow_factor = 0.8 + IAS * 0.015;

  double Q_radiator = thermostat * Cooling_Factor * delta_T * airflow_factor;

  // === ENGINE BLOCK RADIATION ===
  double Q_block = 0.008 * delta_T;

  // === WATER PUMP ===
  // Coolant only circulates when engine is turning.
  double pump_factor;
  if (RPM < 1.0) {
    pump_factor = 0.0;
  } else if (RPM < IdleRPM) {
    pump_factor = RPM / IdleRPM;
  } else {
    pump_factor = 1.0;
  }

  Q_radiator *= pump_factor;

  // === NET HEAT AND TEMPERATURE CHANGE ===
  double thermal_mass = 5.5 * 3500.0 + CylinderHeadMass * Cylinders * 500.0;

  double Q_net = Q_coolant_in - Q_radiator - Q_block;
  double dT = (Q_net * dt) / thermal_mass;

  CoolantTemperature_degK += dT;
  CoolantTemperature_degK = Constrain(T_amb, CoolantTemperature_degK, 273.15 + 130.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the oil temperature.
 *
 * Inputs: CylinderHeadTemp_degK, T_amb, OilPressure_psi
 *
 * Outputs: OilTemp_degK
 */

void FGPistonDiesel::doOilTemperature()
{
  double dt = in.TotalDeltaT;
  if (dt <= 0.0) return;

  // Oil target temperature: when running, oil sits above coolant temp
  // by a load-dependent margin; when stopped, oil cools toward ambient.
  double oil_target;
  if (Running) {
    double load_offset = 10.0 + HP / MaxHP * 15.0;  // 10-25 deg C above coolant
    oil_target = CoolantTemperature_degK + load_offset;
  } else {
    oil_target = T_amb;
  }

  // Oil thermal mass time constant ~150 seconds for automotive diesel.
  constexpr double oil_tau = 150.0;

  OilTemp_degK += (oil_target - OilTemp_degK)
                  * (1.0 - std::exp(-dt / oil_tau));

  OilTemp_degK = Constrain(T_amb, OilTemp_degK, 273.15 + 160.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the oil pressure.
 *
 * Inputs: RPM, MaxRPM, OilTemp_degK
 *
 * Outputs: OilPressure_psi
 */

void FGPistonDiesel::doOilPressure()
{
  // Oil pump is mechanically driven - pressure proportional to RPM
  // up to the relief valve setting.
  double rpm_ratio = (Oil_Press_RPM_Max > 0.0)
                     ? RPM / Oil_Press_RPM_Max
                     : 0.0;

  double base_pressure = Oil_Press_Relief_Valve * rpm_ratio;

  // Viscosity correction: cold oil is thicker (higher pressure),
  // hot oil is thinner (lower pressure).
  double temp_ratio = (Design_Oil_Temp > 0.0)
                      ? Design_Oil_Temp / OilTemp_degK
                      : 1.0;
  double viscosity_factor = std::pow(temp_ratio, Oil_Viscosity_Index);

  OilPressure_psi = base_pressure * viscosity_factor;

  // Relief valve limits maximum pressure.
  OilPressure_psi = std::min(OilPressure_psi, Oil_Press_Relief_Valve);

  // No pressure when engine isn't turning.
  if (RPM < 1.0) {
    OilPressure_psi = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPistonDiesel::GetEngineLabels(const std::string& delimiter)
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

std::string FGPistonDiesel::GetEngineValues(const std::string& delimiter)
{
  std::ostringstream buf;

  buf << (HP * hptoftlbssec) << delimiter
      << HP << delimiter
      << equivalence_ratio << delimiter
      << (MAP / inhgtopa) << delimiter
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
//    1: This value explicitly requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGPistonDiesel::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(LogLevel::DEBUG);
      log << "\n    Engine Name: "              << Name                      << "\n";
      log << "      Displacement: "             << Displacement              << "\n";
      log << "      Bore: "                     << Bore                      << "\n";
      log << "      Stroke: "                   << Stroke                    << "\n";
      log << "      Cylinders: "                << Cylinders                 << "\n";
      log << "      Cylinder Head Mass: "       << CylinderHeadMass          << "\n";
      log << "      Compression Ratio: "        << CompressionRatio          << "\n";
      log << "      MaxHP: "                    << MaxHP                     << "\n";
      log << "      Cycles: "                   << Cycles                    << "\n";
      log << "      IdleRPM: "                  << IdleRPM                   << "\n";
      log << "      MaxRPM: "                   << MaxRPM                    << "\n";
      log << "      Volumetric Efficiency: "    << volumetric_efficiency     << "\n";
      log << "      PeakMeanPistonSpeed_fps: "  << PeakMeanPistonSpeed_fps   << "\n";
      log << "      Intake Impedance Factor: "  << Z_airbox                  << "\n";
      log << "      Dynamic FMEP Factor: "      << FMEPDynamic               << "\n";
      log << "      Static FMEP Factor: "       << FMEPStatic                << "\n";
      log << "      Starter Motor Torque: "     << StarterTorque             << "\n";
      log << "      Starter Motor RPM:    "     << StarterRPM                << "\n";
      log << "\n";
      log << "      Combustion Efficiency table:\n";
      Lookup_Combustion_Efficiency->Print();
      log << "\n";
    }
  }
  if (debug_lvl & 2) { // Instantiation/Destruction notification
    FGLogging log(LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGPistonDiesel\n";
    if (from == 1) log << "Destroyed:    FGPistonDiesel\n";
  }
  if (debug_lvl & 4) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

} // namespace JSBSim