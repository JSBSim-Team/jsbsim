/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPiston.cpp
 Author:       Jon S. Berndt
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

#include "FGDefs.h"
#include "FGPiston.h"
#include "FGPropulsion.h"

static const char *IdSrc = "$Id: FGPiston.cpp,v 1.26 2001/10/03 22:21:55 jberndt Exp $";
static const char *IdHdr = ID_PISTON;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPiston::FGPiston(FGFDMExec* exec, FGConfigFile* Eng_cfg)
  : FGEngine(exec),
    MinManifoldPressure_inHg(6.5),
    MaxManifoldPressure_inHg(28.5),
    Displacement(360),
    MaxHP(200),
    Cycles(2),
    IdleRPM(900),
    // Set constants
    CONVERT_CUBIC_INCHES_TO_METERS_CUBED(1.638706e-5),
    R_air(287.3),
    rho_fuel(800),                 // estimate
    calorific_value_fuel(47.3e6),
    Cp_air(1005),
    Cp_fuel(1700),
    Oil_Temp(85)                   // FIXME: should be dynamic
{
  string token;

  Name = Eng_cfg->GetValue("NAME");
  Eng_cfg->GetNextConfigLine();
  while (Eng_cfg->GetValue() != "/FG_PISTON") {
    *Eng_cfg >> token;
    if      (token == "MINMP") *Eng_cfg >> MinManifoldPressure_inHg;
    else if (token == "MAXMP") *Eng_cfg >> MaxManifoldPressure_inHg;
    else if (token == "DISPLACEMENT") *Eng_cfg >> Displacement;
    else if (token == "MAXHP") *Eng_cfg >> MaxHP;
    else if (token == "CYCLES") *Eng_cfg >> Cycles;
    else if (token == "IDLERPM") *Eng_cfg >> IdleRPM;
    else if (token == "MAXTHROTTLE") *Eng_cfg >> MaxThrottle;
    else if (token == "MINTHROTTLE") *Eng_cfg >> MinThrottle;
    else if (token == "SLFUELFLOWMAX") *Eng_cfg >> SLFuelFlowMax;
    else cerr << "Unhandled token in Engine config file: " << token << endl;
  }

  if (debug_lvl > 0) {
    cout << "\n    Engine Name: " << Name << endl;
    cout << "      MinManifoldPressure: " << MinManifoldPressure_inHg << endl;
    cout << "      MaxManifoldPressure: " << MaxManifoldPressure_inHg << endl;
    cout << "      Displacement: " << Displacement << endl;
    cout << "      MaxHP: " << MaxHP << endl;
    cout << "      Cycles: " << Cycles << endl;
    cout << "      IdleRPM: " << IdleRPM << endl;
    cout << "      MaxThrottle: " << MaxThrottle << endl;
    cout << "      MinThrottle: " << MinThrottle << endl;
    cout << "      SLFuelFlowMax: " << SLFuelFlowMax << endl;
  }

  Type = etPiston;
  EngineNumber = 0;    // FIXME: this should be the actual number

  // Initialisation
  volumetric_efficiency = 0.8;  // Actually f(speed, load) but this will get us running

  if (debug_lvl & 2) cout << "Instantiated: FGPiston" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPiston::~FGPiston()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGPiston" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPiston::Calculate(float PowerRequired)
{
  float h,EngineMaxPower;

        // FIXME: calculate from actual fuel flow
  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  Mixture = FCS->GetMixturePos(EngineNumber);

  //
  // Input values.
  //
        // convert from lbs/ft2 to Pa
  p_amb = Atmosphere->GetPressure() * 48;
  p_amb_sea_level = Atmosphere->GetPressureSL() * 48;
        // convert from Rankine to Kelvin
  T_amb = Atmosphere->GetTemperature() * (5.0 / 9.0);
  RPM = Propulsion->GetThruster(EngineNumber)->GetRPM();
  if (RPM < IdleRPM)    // kludge
    RPM = IdleRPM;
  IAS = Auxiliary->GetVcalibratedKTS();

  if (Mixture >= 0.5) {
    doEngineStartup();
    doManifoldPressure();
    doAirFlow();
    doFuelFlow();
    doEnginePower();
    doEGT();
    doCHT();
    doOilPressure();
  } else {
    HP = 0;
  }

  PowerAvailable = (HP * HPTOFTLBSSEC) - PowerRequired;
  return PowerAvailable;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Look up the power/mixture correlation.
 *
 * FIXME: this should use JSBSim's interpolation support.
 */

static float Power_Mixture_Correlation(float thi_actual)
{
  float AFR_actual = 14.7 / thi_actual;
  const int NUM_ELEMENTS = 13;
  float AFR[NUM_ELEMENTS] = 
    {(14.7/1.6), 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, (14.7/0.6)};
  float mixPerPow[NUM_ELEMENTS] = 
    {78, 86, 93.5, 98, 100, 99, 96.4, 92.5, 88, 83, 78.5, 74, 58};
  float mixPerPow_actual = 0.0f;
  float factor;
  float dydx;

  int i;
  int j = NUM_ELEMENTS;

  for (i=0;i<j;i++) {
    if (i == (j-1)) {
      dydx = (mixPerPow[i] - mixPerPow[i-1]) / (AFR[i] - AFR[i-1]);
      mixPerPow_actual = mixPerPow[i] + dydx * (AFR_actual - AFR[i]);
      return mixPerPow_actual;
    }
    if ((i == 0) && (AFR_actual < AFR[i])) {
      dydx = (mixPerPow[i] - mixPerPow[i-1]) / (AFR[i] - AFR[i-1]);
      mixPerPow_actual = mixPerPow[i] + dydx * (AFR_actual - AFR[i]);
      return mixPerPow_actual;
    }
    if (AFR_actual == AFR[i]) {
      mixPerPow_actual = mixPerPow[i];
      return mixPerPow_actual;
    }
    if ((AFR_actual > AFR[i]) && (AFR_actual < AFR[i + 1])) {
      factor = (AFR_actual - AFR[i]) / (AFR[i+1] - AFR[i]);
      mixPerPow_actual = (factor * (mixPerPow[i+1] - mixPerPow[i])) + mixPerPow[i];
      return mixPerPow_actual;
    }
  }

  cerr << "ERROR: error in FGNewEngine::Power_Mixture_Correlation\n";
  return mixPerPow_actual;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Look up the combustion efficiency.
 *
 *
 * FIXME: this should use JSBSim's interpolation support.
 */

static float Lookup_Combustion_Efficiency(float thi_actual)
{
  const int NUM_ELEMENTS = 11;
  float thi[NUM_ELEMENTS] = {0.0, 0.9, 1.0, 1.05, 1.1, 1.15, 1.2, 1.3, 1.4, 1.5, 1.6};  //array of equivalence ratio values
  float neta_comb[NUM_ELEMENTS] = {0.98, 0.98, 0.97, 0.95, 0.9, 0.85, 0.79, 0.7, 0.63, 0.57, 0.525};  //corresponding array of combustion efficiency values
  //combustion efficiency values from Heywood, "Internal Combustion Engine Fundamentals", ISBN 0-07-100499-8
  float neta_comb_actual = 0.0f;
  float factor;

  int i;
  int j = NUM_ELEMENTS;  //This must be equal to the number of elements in the lookup table arrays

  for (i=0;i<j;i++) {
    if(i == (j-1)) {
      // Assume linear extrapolation of the slope between the last two points beyond the last point
      float dydx = (neta_comb[i] - neta_comb[i-1]) / (thi[i] - thi[i-1]);
      neta_comb_actual = neta_comb[i] + dydx * (thi_actual - thi[i]);
      return neta_comb_actual;
    }
    if(thi_actual == thi[i]) {
      neta_comb_actual = neta_comb[i];
      return neta_comb_actual;
    }
    if((thi_actual > thi[i]) && (thi_actual < thi[i + 1])) {
      //do linear interpolation between the two points
      factor = (thi_actual - thi[i]) / (thi[i+1] - thi[i]);
      neta_comb_actual = (factor * (neta_comb[i+1] - neta_comb[i])) + neta_comb[i];
      return neta_comb_actual;
    }
  }

  //if we get here something has gone badly wrong
  cerr << "ERROR: error in FGNewEngine::Lookup_Combustion_Efficiency\n";
  return neta_comb_actual;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Start or stop the engine.
 */

void FGPiston::doEngineStartup(void)
{
  // TODO: check magnetos, spark, starter, etc. and decide whether
  // engine is running
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/**
 * Calculate the nominal manifold pressure in inches hg
 *
 * This function calculates nominal manifold pressure directly
 * from the throttle position, and does not adjust it for the
 * difference between the pressure at sea level and the pressure
 * at the current altitude (that adjustment takes place in
 * {@link #doEnginePower}).
 *
 * TODO: changes in MP should not be instantaneous -- introduce
 * a lag between throttle changes and MP changes, to allow pressure
 * to build up or disperse.
 *
 * Inputs: MinManifoldPressure_inHg, MaxManifoldPressure_inHg, Throttle
 *
 * Outputs: ManifoldPressure_inHg
 */

void FGPiston::doManifoldPressure(void)
{
  ManifoldPressure_inHg = MinManifoldPressure_inHg +
    (Throttle * (MaxManifoldPressure_inHg - MinManifoldPressure_inHg));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the air flow through the engine.
 *
 * Inputs: p_amb, R_air, T_amb, ManifoldPressure_inHg, Displacement,
 *   RPM, volumetric_efficiency
 *
 * Outputs: rho_air, m_dot_air
 */

void FGPiston::doAirFlow(void)
{
  rho_air = p_amb / (R_air * T_amb);
  float rho_air_manifold = rho_air * ManifoldPressure_inHg / 29.6;
  float displacement_SI = Displacement * CONVERT_CUBIC_INCHES_TO_METERS_CUBED;
  float swept_volume = (displacement_SI * (RPM/60)) / 2;
  float v_dot_air = swept_volume * volumetric_efficiency;
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
  float thi_sea_level = 1.3 * Mixture;
  equivalence_ratio = thi_sea_level * p_amb_sea_level / p_amb;
  m_dot_fuel = m_dot_air / 14.7 * equivalence_ratio;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the power produced by the engine.
 *
 * <p>Currently, the JSBSim propellor model does not allow the
 * engine to produce enough RPMs to get up to a high horsepower.
 * When tested with sufficient RPM, it has no trouble reaching
 * 200HP.</p>
 *
 * Inputs: ManifoldPressure_inHg, p_amb, p_amb_sea_level, RPM, T_amb, 
 *   equivalence_ratio, Cycles, MaxHP
 *
 * Outputs: Percentage_Power, HP
 */

void FGPiston::doEnginePower(void)
{
  float True_ManifoldPressure_inHg = ManifoldPressure_inHg * p_amb / p_amb_sea_level;
  float ManXRPM = True_ManifoldPressure_inHg * RPM;
        // FIXME: this needs to be generalized
  Percentage_Power = (6e-9 * ManXRPM * ManXRPM) + (8e-4 * ManXRPM) - 1.0;
  float T_amb_degF = (T_amb * 1.8) - 459.67;
  float T_amb_sea_lev_degF = (288 * 1.8) - 459.67; 
  Percentage_Power =
    Percentage_Power + ((T_amb_sea_lev_degF - T_amb_degF) * 7 /120);
  float Percentage_of_best_power_mixture_power =
    Power_Mixture_Correlation(equivalence_ratio);
  Percentage_Power =
    Percentage_Power * Percentage_of_best_power_mixture_power / 100.0;
  if (Percentage_Power < 0.0)
    Percentage_Power = 0.0;
  else if (Percentage_Power > 100.0)
    Percentage_Power = 100.0;
  HP = Percentage_Power * MaxHP / 100.0;
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
  combustion_efficiency = Lookup_Combustion_Efficiency(equivalence_ratio);
  float enthalpy_exhaust = m_dot_fuel * calorific_value_fuel * 
    combustion_efficiency * 0.33;
  float heat_capacity_exhaust = (Cp_air * m_dot_air) + (Cp_fuel * m_dot_fuel);
  float delta_T_exhaust = enthalpy_exhaust / heat_capacity_exhaust;
  ExhaustGasTemp_degK = T_amb + delta_T_exhaust;
  ExhaustGasTemp_degK *= 0.444 + ((0.544 - 0.444) * Percentage_Power / 100.0);
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
  float h1 = -95.0;
  float h2 = -3.95;
  float h3 = -0.05;

  float arbitary_area = 1.0;
  float CpCylinderHead = 800.0;
  float MassCylinderHead = 8.0;

  float temperature_difference = CylinderHeadTemp_degK - T_amb;
  float v_apparent = IAS * 0.5144444;
  float v_dot_cooling_air = arbitary_area * v_apparent;
  float m_dot_cooling_air = v_dot_cooling_air * rho_air;
  float dqdt_from_combustion = 
    m_dot_fuel * calorific_value_fuel * combustion_efficiency * 0.33;
  float dqdt_forced = (h2 * m_dot_cooling_air * temperature_difference) + 
    (h3 * RPM * temperature_difference);
  float dqdt_free = h1 * temperature_difference;
  float dqdt_cylinder_head = dqdt_from_combustion + dqdt_forced + dqdt_free;
    
  float HeatCapacityCylinderHead = CpCylinderHead * MassCylinderHead;
    
  CylinderHeadTemp_degK = dqdt_cylinder_head / HeatCapacityCylinderHead;
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
  float Oil_Press_Relief_Valve = 60; // FIXME: may vary by engine
  float Oil_Press_RPM_Max = 1800;    // FIXME: may vary by engine
  float Design_Oil_Temp = 85;        // FIXME: may vary by engine
  float Oil_Viscosity_Index = 0.25;

  OilPressure_psi = (Oil_Press_Relief_Valve / Oil_Press_RPM_Max) * RPM;

  if (OilPressure_psi >= Oil_Press_Relief_Valve) {
    OilPressure_psi = Oil_Press_Relief_Valve;
  }

  OilPressure_psi += (Design_Oil_Temp - Oil_Temp) * Oil_Viscosity_Index;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPiston::Debug(void)
{
  //TODO: Add your source code here
}

