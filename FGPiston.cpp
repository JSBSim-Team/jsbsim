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

#include "FGPiston.h"
#include "FGPropulsion.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGPiston.cpp,v 1.58 2003/01/22 15:53:34 jberndt Exp $";
static const char *IdHdr = ID_PISTON;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPiston::FGPiston(FGFDMExec* exec, FGConfigFile* Eng_cfg) : FGEngine(exec),
  R_air(287.3),
  rho_fuel(800),                 // estimate
  calorific_value_fuel(47.3e6),
  Cp_air(1005),
  Cp_fuel(1700)
{
  string token;

  MinManifoldPressure_inHg = 6.5;
  MaxManifoldPressure_inHg = 28.5;
  Displacement = 360;
  MaxHP = 200;
  Cycles = 2;
  IdleRPM = 600;

  Name = Eng_cfg->GetValue("NAME");
  Eng_cfg->GetNextConfigLine();
  while (Eng_cfg->GetValue() != string("/FG_PISTON")) {
    *Eng_cfg >> token;
    if      (token == "MINMP") *Eng_cfg >> MinManifoldPressure_inHg;
    else if (token == "MAXMP") *Eng_cfg >> MaxManifoldPressure_inHg;
    else if (token == "DISPLACEMENT") *Eng_cfg >> Displacement;
    else if (token == "MAXHP") *Eng_cfg >> MaxHP;
    else if (token == "CYCLES") *Eng_cfg >> Cycles;
    else if (token == "IDLERPM") *Eng_cfg >> IdleRPM;
    else if (token == "MAXTHROTTLE") *Eng_cfg >> MaxThrottle;
    else if (token == "MINTHROTTLE") *Eng_cfg >> MinThrottle;
    else cerr << "Unhandled token in Engine config file: " << token << endl;
  }

  Type = etPiston;
  crank_counter = 0;
  EngineNumber = 0;
  OilTemp_degK = 298;
  ManifoldPressure_inHg = Atmosphere->GetPressure() * psftoinhg; // psf to in Hg

  dt = State->Getdt();

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

  Debug(0); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPiston::~FGPiston()
{
  Debug(1); // Call Debug() routine from constructor if needed
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPiston::Calculate(double PowerRequired)
{
  ConsumeFuel();

  Throttle = FCS->GetThrottlePos(EngineNumber);
  Mixture = FCS->GetMixturePos(EngineNumber);

  //
  // Input values.
  //

  p_amb = Atmosphere->GetPressure() * 48;              // convert from lbs/ft2 to Pa
  p_amb_sea_level = Atmosphere->GetPressureSL() * 48;
  T_amb = Atmosphere->GetTemperature() * (5.0 / 9.0);  // convert from Rankine to Kelvin

  RPM = Propulsion->GetThruster(EngineNumber)->GetRPM();
    
  IAS = Auxiliary->GetVcalibratedKTS();

    doEngineStartup();
    doManifoldPressure();
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

  PowerAvailable = (HP * hptoftlbssec) - PowerRequired;
  return PowerAvailable;
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
  if (Running || Cranking) {
    ManifoldPressure_inHg = MinManifoldPressure_inHg +
            (Throttle * (MaxManifoldPressure_inHg - MinManifoldPressure_inHg));
  } else {
    ManifoldPressure_inHg = Atmosphere->GetPressure() * psftoinhg; // psf to in Hg
  }  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 * Calculate the air flow through the engine.
 *
 * At this point, ManifoldPressure_inHg still represents the sea-level
 * MP, not adjusted for altitude.
 *
 * Inputs: p_amb, R_air, T_amb, ManifoldPressure_inHg, Displacement,
 *   RPM, volumetric_efficiency
 *
 * Outputs: rho_air, m_dot_air
 */

void FGPiston::doAirFlow(void)
{
  rho_air = p_amb / (R_air * T_amb);
  double rho_air_manifold = rho_air * ManifoldPressure_inHg / 29.6;
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
  ManifoldPressure_inHg *= p_amb / p_amb_sea_level;
  if (Running) {	
    double ManXRPM = ManifoldPressure_inHg * RPM;
        // FIXME: this needs to be generalized
    Percentage_Power = (6e-9 * ManXRPM * ManXRPM) + (8e-4 * ManXRPM) - 1.0;
    double T_amb_degF = (T_amb * 1.8) - 459.67;
    double T_amb_sea_lev_degF = (288 * 1.8) - 459.67; 
    Percentage_Power =
      Percentage_Power + ((T_amb_sea_lev_degF - T_amb_degF) * 7 /120);
    double Percentage_of_best_power_mixture_power =
      Power_Mixture_Correlation->GetValue(14.7 / equivalence_ratio);
    Percentage_Power =
      Percentage_Power * Percentage_of_best_power_mixture_power / 100.0;
    if (Percentage_Power < 0.0)
      Percentage_Power = 0.0;
    else if (Percentage_Power > 100.0)
      Percentage_Power = 100.0;
    HP = Percentage_Power * MaxHP / 100.0;
  } else {  
    // Power output when the engine is not running
    if (Cranking) {
      if (RPM < 10) {
        HP = 3.0;	// This is a hack to prevent overshooting the idle rpm in the first time step
                    // It may possibly need to be changed if the prop model is changed.
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
