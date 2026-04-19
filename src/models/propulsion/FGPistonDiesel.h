/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPistonDiesel.h
 Author:       Marc Traverso
 Date started: 09/12/2000

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

HISTORY
--------------------------------------------------------------------------------
14/04/2026  MTR  Derived FGPiston to create the FGPistonDiesel class definition
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPISTONDIESEL_H
#define FGPISTONDIESEL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"
#include "math/FGTable.h"

#include <cmath>
#include <memory>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a compression-ignition (diesel) piston engine with optional
    turbocharging, mechanical governor fuel-rack control, and liquid cooling.

    The model implements the thermodynamic cycle of a four-stroke diesel engine
    from first principles: volumetric air flow through an unthrottled intake,
    fuel injection metered by a centrifugal-governor-controlled fuel rack,
    indicated mean effective pressure derived from fuel energy and indicated
    efficiency, friction losses via Chen-Flynn correlation, and a turbocharger
    modeled as a first-order-lag exhaust-driven compressor.

    Thermal sub-models cover exhaust gas temperature, cylinder head temperature,
    liquid coolant temperature (with wax-element thermostat), oil temperature,
    and oil pressure.  Cold-start behavior is influenced by glow-plug state and
    coolant-temperature-dependent enrichment.

    Most parameters are derived automatically from a small set of headline
    values (displacement, rated power, rated RPM, compression ratio).  Any
    derived value can be overridden explicitly in the configuration file.

<h3>Configuration File Format:</h3>

@code
<diesel_engine name="{string}">
  <displacement unit="{IN3 | LTR | CC}"> {number} </displacement>
  <maxhp unit="{HP | WATTS}"> {number} </maxhp>
  <maxrpm> {number} </maxrpm>
  <idlerpm> {number} </idlerpm>
  <cycles> {number} </cycles>
  <bore unit="{IN | M}"> {number} </bore>
  <stroke unit="{IN | M}"> {number} </stroke>
  <cylinders> {number} </cylinders>
  <compression-ratio> {number} </compression-ratio>
  <rated-rpm> {number} </rated-rpm>
  <peak-torque-rpm> {number} </peak-torque-rpm>
  <static-friction unit="{HP | WATTS}"> {number} </static-friction>
  <air-intake-impedance-factor> {number} </air-intake-impedance-factor>
  <ram-air-factor> {number} </ram-air-factor>
  <cooling-factor> {number} </cooling-factor>
  <starter-torque> {number} </starter-torque>
  <starter-rpm> {number} </starter-rpm>
  <cylinder-head-mass unit="{KG | LBS}"> {number} </cylinder-head-mass>
  <bsfc-best> {number} </bsfc-best>
  <volumetric-efficiency> {number} </volumetric-efficiency>
  <ve-peak> {number} </ve-peak>
  <dynamic-fmep unit="{INHG | PA | ATM}"> {number} </dynamic-fmep>
  <static-fmep unit="{INHG | PA | ATM}"> {number} </static-fmep>
  <peak-piston-speed> {number} </peak-piston-speed>
  <injection-type> {DI | IDI} </injection-type>
  <afr-smoke-limit> {number} </afr-smoke-limit>
  <calorific-value-fuel> {number} </calorific-value-fuel>
  <governor-droop> {number} </governor-droop>
  <governor-tau> {number} </governor-tau>
  <idle-fuel-rack> {number} </idle-fuel-rack>
  <idle-governor-gain> {number} </idle-governor-gain>
  <cold-enrichment-max> {number} </cold-enrichment-max>
  <cold-enrichment-threshold> {number} </cold-enrichment-threshold>
  <glow-plug-threshold unit="{DEGK}"> {number} </glow-plug-threshold>
  <coolant-heat-fraction> {number} </coolant-heat-fraction>
  <boosted> {true | false} </boosted>
  <boost-max-pressure unit="{PA}"> {number} </boost-max-pressure>
  <boost-threshold-rpm> {number} </boost-threshold-rpm>
  <turbo-lag> {number} </turbo-lag>
  <boost-loss-factor> {number} </boost-loss-factor>
  <oil-pressure-relief-valve-psi> {number} </oil-pressure-relief-valve-psi>
  <design-oil-temp-degK> {number} </design-oil-temp-degK>
  <oil-pressure-rpm-max> {number} </oil-pressure-rpm-max>
  <oil-viscosity-index> {number} </oil-viscosity-index>
</diesel_engine>
@endcode

<h3>Definition of the diesel engine configuration file parameters:</h3>

Basic parameters:
- \b displacement - swept volume of all cylinders.  Used to compute mass air
      flow, which drives power and cooling.
- \b maxhp - rated brake power at \b maxrpm.  Determines BSFC and starter
      torque when those are not specified explicitly.
- \b maxrpm - absolute maximum engine speed.  The overspeed governor cuts
      fuel above 95 % of this value.
- \b idlerpm - target idle speed maintained by the idle governor spring.
      The engine stalls if speed falls below 80 % of this value.
- \b cycles - designate a 2- or 4-stroke engine (only 4-stroke is currently
      supported).
- \b bore - cylinder bore (currently unused; reserved for future heat-transfer
      refinement).
- \b stroke - piston stroke.  Determines mean piston speed, which controls
      the high-RPM fall-off of volumetric efficiency and the Chen-Flynn
      friction model.
- \b cylinders - number of cylinders.  Scales cylinder head thermal mass.
- \b compression-ratio - geometric compression ratio.  Affects the
      automatic BSFC estimate and altitude lapse of volumetric efficiency.
- \b rated-rpm - engine speed at which \b maxhp is produced.  Defaults to
      85 % of \b maxrpm.
- \b peak-torque-rpm - RPM at which peak torque occurs.  Defaults to
      55 % of \b rated-rpm.
- \b static-friction - parasitic power required to turn a non-firing engine.
      Models accessory drag and windmilling resistance.  Choose a small
      percentage of \b maxhp.
- \b starter-torque - zero-RPM torque of the starter motor in lb-ft.
      Defaults to 40 % of \b maxhp.
- \b starter-rpm - unloaded peak RPM of the starter motor.

Advanced parameters:
- \b bsfc-best - best brake-specific fuel consumption in g/(kW*h).  If
      omitted, it is estimated from compression ratio, injection type,
      and displacement using an empirical correlation.
- \b volumetric-efficiency - nominal volumetric efficiency.  Turbocharged
      engines may require values above 1.  Typical range is 0.80-0.90.
- \b ve-peak - peak volumetric efficiency used by the RPM-dependent VE
      curve.  If omitted, it is back-calculated from rated airflow.
- \b air-intake-impedance-factor - pressure drop across the intake system.
      Increasing this reduces manifold pressure.  Also a run-time property.
- \b ram-air-factor - multiplier on dynamic-pressure ram-air recovery.
      Also a run-time property.
- \b dynamic-fmep - Chen-Flynn friction coefficient proportional to mean
      piston speed, in Pa/(m/s).
- \b static-fmep - Chen-Flynn friction constant term, in Pa.
- \b peak-piston-speed - mean piston speed at which intake begins to choke.
- \b injection-type - DI (direct injection) or IDI (indirect injection).
      IDI applies a 0.92 penalty to estimated brake efficiency.
- \b afr-smoke-limit - minimum air/fuel ratio below which visible smoke
      is produced.  The fuel rack is clamped to respect this limit.
- \b calorific-value-fuel - lower heating value of fuel in J/kg.  Defaults
      to 42.8 MJ/kg (EN 590 diesel).

Governor parameters:
- \b governor-droop - proportional droop of the centrifugal governor
      (dimensionless, typically 0.04-0.08).
- \b governor-tau - time constant of the mechanical governor response, in
      seconds.
- \b idle-fuel-rack - normalised fuel-rack position that sustains idle.
- \b idle-governor-gain - additional proportional gain of the idle-speed
      governor, in rack-units per RPM.

Cold-start parameters:
- \b cold-enrichment-max - maximum fuel enrichment multiplier when the
      engine is cold.
- \b cold-enrichment-threshold - coolant temperature in degrees C below which
      enrichment begins.
- \b glow-plug-threshold - ambient temperature in K below which the glow
      plug must be energised for reliable starting.

Cooling parameters:
- \b cylinder-head-mass - nominal mass per cylinder head in kg.  A larger
      value slows temperature transients.
- \b cooling-factor - overall effectiveness of the cooling installation.
      Also a run-time property.
- \b coolant-heat-fraction - fraction of fuel energy rejected to the
      coolant circuit (typically 0.25-0.35).

Turbocharger parameters:
- \b boosted - set to "true" to enable the exhaust-driven turbocharger
      model.
- \b boost-max-pressure - maximum compressor outlet pressure in Pa
      (absolute).  Defaults to 1.5 bar if boosted is true.
- \b boost-threshold-rpm - engine RPM below which the turbocharger
      produces no meaningful boost.  Defaults to 1.5 * \b idlerpm.
- \b turbo-lag - spool-up/spool-down time constant in seconds.
- \b boost-loss-factor - fraction of shaft power consumed by driving
      the compressor.  Zero for a free-running turbocharger.

Oil system parameters:
- \b oil-pressure-relief-valve-psi - relief-valve crack pressure.
- \b design-oil-temp-degK - design-point oil temperature used for
      viscosity normalisation.
- \b oil-pressure-rpm-max - engine RPM at which the oil pump reaches
      full rated flow.  Defaults to 75 % of \b maxrpm.
- \b oil-viscosity-index - exponent controlling the sensitivity of oil
      pressure to temperature.

    @author Jon S. Berndt (Engine framework code and framework-related mods)
    @author Dave Luff (engine operational code)
    @author David Megginson (initial porting and additional code)
    @author Ron Jensen (additional engine code)
    @author Marc Traverso (diesel adaptation)
    @see Taylor, Charles Fayette, "The Internal Combustion Engine in Theory and Practice"
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPistonDiesel : public FGEngine
{
public:
  /// Constructor
  FGPistonDiesel(FGFDMExec* exec, Element* el, int engine_number, struct Inputs& input);
  /// Destructor
  ~FGPistonDiesel();

  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

  void Calculate();
  double GetPowerAvailable() const { return HP * hptoftlbssec; }
  double CalcFuelNeed();

  void ResetToIC();

  double GetEGT() const { return EGT_degC; }

  double getExhaustGasTemp_degF() const { return KelvinToFahrenheit(ExhaustGasTemp_degK); }
  double getManifoldPressure_inHg() const { return MAP / inhgtopa; }
  double getCylinderHeadTemp_degF() const { return KelvinToFahrenheit(CylinderHeadTemp_degK); }
  double getOilPressure_psi() const { return OilPressure_psi; }
  double getOilTemp_degF() const { return KelvinToFahrenheit(OilTemp_degK); }
  double getRPM() const { return RPM; }
  double getAFR() const { return m_dot_fuel > 0.0 ? m_dot_air / m_dot_fuel : INFINITY; }
  double getCoolantTemperature_degF() const { return KelvinToFahrenheit(CoolantTemperature_degK); }
  double getBoostPa() const { return CurrentBoost_Pa; }
  double getFuelRack() const { return FuelRack; }
  double getHP() const { return HP; }
  double getTorque_SI() const { return Torque_SI; }
  double getVolumetricEfficiencyReduced() const { return volumetric_efficiency_reduced; }
  double getCombustionEfficiency() const { return combustion_efficiency; }
  double getIndicatedHorsePower() const { return IndicatedHorsePower; }

  bool getGlowPlugOn() const { return GlowPlugOn; }

private:
  // --- Simulation step functions ---
  void doEngineStartup();
  void doTurbo();
  void doAirFlow();
  void doFuelRack();
  void doFuelFlow();
  void doEnginePower();
  void doEGT();
  void doCHT();
  void doOilPressure();
  void doOilTemperature();
  void doCoolantTemperature();

  int InitRunning();

  // --- Physical constants ---
  static constexpr double R_air = 287.3;                  // J/(kg*K) - gas constant for air
  static constexpr double Cp_air = 1005.0;                // J/(kg*K) - specific heat of air (const. pressure)
  static constexpr double standard_pressure = 101320.73;  // Pa

  // --- Lookup tables ---
  std::unique_ptr<FGTable> Lookup_Combustion_Efficiency;

  // --- Configuration (read from XML or derived at init) ---

  // Geometry
  double Displacement = 0.0;             // cubic inches
  double displacement_SI = 0.0;          // cubic metres
  double Bore = 0.0;                     // inches
  double Stroke = 0.0;                   // inches
  double Cylinders = 0.0;               // count
  double CylinderHeadMass = 0.0;         // kg
  double CompressionRatio = 0.0;
  double Cycles = 0.0;                   // strokes per power cycle

  // Performance
  double MaxHP = 0.0;                    // horsepower
  double IdleRPM = 0.0;                  // rev/min
  double MaxRPM = 0.0;                   // rev/min
  double RatedRPM = 0.0;                 // rev/min
  double PeakTorqueRPM = 0.0;

  // Friction
  double StaticFriction_HP = 0.0;        // HP - accessory drag
  double FMEPDynamic = 0.0;              // Pa/(m/s) - Chen-Flynn speed term
  double FMEPStatic = 0.0;               // Pa - Chen-Flynn constant term

  // Intake
  double Z_airbox = 0.0;                 // intake impedance factor
  double PeakMeanPistonSpeed_fps = 0.0;  // ft/s - intake choking threshold
  double RatedMeanPistonSpeed_fps = 0.0; // ft/s - derived from MaxRPM and stroke
  double Ram_Air_Factor = 0.0;

  // Starter
  double StarterTorque = 0.0;            // lb*ft peak torque
  double StarterRPM = 0.0;              // peak unloaded RPM
  double StarterGain = 0.0;             // normalised starter command

  // Turbocharger
  bool   Boosted = false;
  double BoostMaxPressure_Pa = 0.0;      // maximum absolute boost pressure
  double BoostThreshold_RPM = 0.0;       // RPM below which turbo is inactive
  double TurboLag_tau = 0.0;             // spool-up time constant (seconds)
  double BoostLossFactor = 0.0;          // power extraction fraction

  // Fuel system
  double BSFC_best = 0.0;               // g/(kW*h)
  double VE_peak = 0.0;
  double eta_indicated_calibrated = 0.0;
  double calorific_value_fuel = 0.0;     // J/kg
  double Cp_fuel = 0.0;                  // J/(kg*K)
  double AFR_smoke_limit = 0.0;
  double m_fuel_max_per_cycle = 0.0;     // kg per injection event

  enum class InjectionType { DI, IDI };
  InjectionType injection_type = InjectionType::IDI;

  // Governor
  double governor_droop = 0.06;
  double governor_tau = 0.08;            // seconds
  double IdleFuelRack = 0.12;
  double idle_governor_gain = 0.005;     // rack/RPM

  // Cold start
  double cold_enrichment_max = 0.0;
  double cold_enrichment_threshold = 0.0; // deg C
  bool   GlowPlugOn = false;
  double GlowPlugThreshold_degK = 0.0;

  // Liquid cooling
  double coolant_heat_fraction = 0.0;

  // Oil system
  double Oil_Press_Relief_Valve = 0.0;   // psi
  double Oil_Press_RPM_Max = 0.0;
  double Design_Oil_Temp = 0.0;          // K
  double Oil_Viscosity_Index = 0.0;

  // --- Run-time state ---

  int crank_counter = 0;

  // Ambient / inputs
  double p_amb = 0.0;                    // Pa
  double p_ram = 0.0;                    // Pa
  double T_amb = 0.0;                    // K
  double RPM = 0.0;                      // rev/min
  double IAS = 0.0;                      // knots
  double Cooling_Factor = 0.0;

  // Intake
  double MAP = 0.0;                      // Pa - manifold/intake pressure
  double intake_pressure_Pa = 0.0;

  // Fuel
  double FuelRack = 0.0;                 // normalised 0-1
  double m_fuel_per_cycle = 0.0;         // kg per injection event
  double equivalence_ratio = 0.0;

  // Computed outputs
  double IndicatedHorsePower = 0.0;
  double PMEP = 0.0;
  double FMEP = 0.0;
  double FuelFlow_pps = 0.0;
  double rho_air = 0.0;
  double volumetric_efficiency = 0.0;
  double volumetric_efficiency_reduced = 0.0;
  double m_dot_air = 0.0;
  double v_dot_air = 0.0;
  double m_dot_fuel = 0.0;
  double HP = 0.0;
  double BoostLossHP = 0.0;
  double combustion_efficiency = 0.0;
  double ExhaustGasTemp_degK = 0.0;
  double EGT_degC = 0.0;
  double CylinderHeadTemp_degK = 0.0;
  double OilPressure_psi = 0.0;
  double OilTemp_degK = 0.0;
  double MeanPistonSpeed_fps = 0.0;
  double CoolantTemperature_degK = 0.0;
  double Torque_SI = 0.0;
  double CurrentBoost_Pa = 0.0;

  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif