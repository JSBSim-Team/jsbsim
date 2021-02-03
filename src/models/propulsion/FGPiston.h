/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPiston.h
 Author:       Jon S. Berndt
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
09/12/2000  JSB  Created
10/01/2001  DPM  Modified to use equations from Dave Luff's piston model.
11/01/2008  RKJ  Modified piston engine model for more general use.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPISTON_H
#define FGPISTON_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"
#include "math/FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define FG_MAX_BOOST_SPEEDS 3

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a Supercharged Piston engine. Based on Dave Luff's model.

<h3>Configuration File Format:</h3>

@code
<piston_engine name="{string}">
  <minmp unit="{INHG | PA | ATM}"> {number} </minmp>
  <maxmp unit="{INHG | PA | ATM}"> {number} </maxmp>
  <idlerpm> {number} </idlerpm>
  <maxrpm> {number} </maxrpm>
  <maxhp unit="{HP | WATTS}"> {number} </maxhp>
  <displacement unit="{IN3 | LTR | CC}"> {number} </displacement>
  <cycles> {number} </cycles>
  <bore unit="{IN | M}"> {number} </bore>
  <stroke unit="{IN | M}"> {number} </stroke>
  <cylinders> {number} </cylinders>
  <compression-ratio> {number} </compression-ratio>
  <sparkfaildrop> {number} </sparkfaildrop>
  <static-friction unit="{HP | WATTS}"> {number} </static-friction>
  <air-intake-impedance-factor> {number} </air-intake-impedance-factor>
  <ram-air-factor> {number} </ram-air-factor>
  <cooling-factor> {number} </cooling-factor>
  <man-press-lag> {number} </man-press-lag>
  <starter-torque> {number} </starter-torque> 
  <starter-rpm> {number} </starter-rpm> 
  <cylinder-head-mass unit="{KG | LBS}"> {number} </cylinder-head-mass>
  <bsfc unit="{LBS/HP*HR | "KG/KW*HR"}"> {number} </bsfc>
  <volumetric-efficiency> {number} </volumetric-efficiency>
  <dynamic-fmep unit="{INHG | PA | ATM}"> {number} </dynamic-fmep>
  <static-fmep unit="{INHG | PA | ATM}"> {number} </static-fmep>
  <numboostspeeds> {number} </numboostspeeds>
  <boostoverride> {0 | 1} </boostoverride>
  <boostmanual> {0 | 1} </boostmanual>
  <boost-loss-factor> {number} </boost-loss-factor>
  <ratedboost1 unit="{INHG | PA | ATM}"> {number} </ratedboost1>
  <ratedpower1 unit="{HP | WATTS}"> {number} </ratedpower1>
  <ratedrpm1> {number} </ratedrpm1>
  <ratedaltitude1 unit="{FT | M}"> {number} </ratedaltitude1>
  <ratedboost2 unit="{INHG | PA | ATM}"> {number} </ratedboost2>
  <ratedpower2 unit="{HP | WATTS}"> {number} </ratedpower2>
  <ratedrpm2> {number} </ratedrpm2>
  <ratedaltitude2 unit="{FT | M}"> {number} </ratedaltitude2>
  <ratedboost3 unit="{INHG | PA | ATM}"> {number} </ratedboost3>
  <ratedpower3 unit="{HP | WATTS}"> {number} </ratedpower3>
  <ratedrpm3> {number} </ratedrpm3>
  <ratedaltitude3 unit="{FT | M}"> {number} </ratedaltitude3>
  <takeoffboost unit="{INHG | PA | ATM}"> {number} </takeoffboost>
  <oil-pressure-relief-valve-psi> {number} </oil-pressure-relief-valve=psi>
  <design-oil-temp-degK>  {number} </design-oil-temp-degK>
  <oil-pressure-rpm-max> {number} </oil-pressure-rpm-max>
  <oil-viscosity-index> {number} </oil-viscosity-index>
</piston_engine>
@endcode

<h3>Definition of the piston engine configuration file parameters:</h3>
Basic parameters:
- \b minmp - this value is the nominal idle manifold pressure at sea-level
      without boost. Along with idlerpm, it determines throttle response slope.
- \b maxmp - this value is the nominal maximum manifold pressure at sea-level
      without boost. Along with maxrpm it determines the resistance of the
      aircraft intake system. Overridden by air-intake-impedance-factor
- \b man-press-lag - Delay in seconds for manifold pressure changes to take effect
- \b starter-torque - A value specifing the zero RPM torque in lb*ft the starter motor
      provides. Current default value is 40% of the horse power value.
- \b starter-rpm - A value specifing the maximum RPM the unloaded starter motor
      can achieve. Loads placed on the engine by the propeller and throttle will
      further limit RPM achieved in practice.
- \b idlerpm - this value affects the throttle fall off and the engine stops
      running if it is slowed below 80% of this value. The engine starts
      running when it reaches 80% of this value.
- \b maxrpm - this value is used to calculate air-box resistance and BSFC. It 
      also affects oil pressure among other things.
- \b maxhp - this value is the nominal power the engine creates at maxrpm. It
      will determine bsfc if that tag is not input. It also determines the
      starter motor power.
- \b displacement - this value is used to determine mass air and fuel flow
      which impacts engine power and cooling.
- \b cycles - Designate a 2 or 4 stroke engine. Currently only the 4 stroke
      engine is supported.
- \b bore - cylinder bore is currently unused.
- \b stroke - piston stroke is used to determine the mean piston speed. Longer
      strokes result in an engine that does not work as well at higher RPMs.
- \b compression-ratio - the compression ratio affects the change in volumetric
      efficiency with altitude.
- \b sparkfaildrop - this is the percentage drop in horsepower for single
      magneto operation.
- \b static-friction - this value is the power required to turn an engine that 
      is not running. Used to control and slow a windmilling propeller. Choose
      a small percentage of maxhp.

Advanced parameters
- \b bsfc - Indicated Specific Fuel Consumption. The power produced per unit of
      fuel. Higher numbers give worse fuel economy. This number may need to be
      lowered slightly from actual BSFC numbers because some internal engine 
      losses are modeled separately. Typically between 0.3 and 0.5
- \b volumetric-efficiency - the nominal volumetric efficiency of the engine.
      This is the primary way to control fuel flow  Boosted engines may require
      values above 1. Typical engines are 0.80 to 0.82
- \b air-intake-impedance-factor - this number is the pressure drop across the
      intake system. Increasing it reduces available manifold pressure. Also a 
      property for run-time adjustment.
- \b ram-air-factor - this number creates increases manifold pressure with an
      increase in dynamic pressure (aircraft speed).
      Also a property for run-time adjustment.

Cooling control:
- \b cylinders  - number of cylinders scales the cylinder head mass.
- \b cylinder-head-mass - the nominal mass of a cylinder head. A larger value
      slows changes in engine temperature
- \b cooling-factor - this number models the efficiency of the aircraft cooling
      system. Also a property for run-time adjustment.

Supercharge parameters:
- \b numboostspeed -  zero (or not present) for a naturally-aspirated engine,
      either 1, 2 or 3 for a boosted engine.  This corresponds to the number of
      supercharger speeds.  Merlin XII had 1 speed, Merlin 61 had 2, a late
      Griffon engine apparently had 3.  No known engine more than 3, although
      some German engines had continuously variable-speed superchargers.
- \b boostoverride - whether or not to clip output to the wastegate value
- \b boost-loss-factor - zero (or not present) for 'free' supercharging. A value entered
      will be used as a multiplier to the power required to compress the input air. Typical 
      value should be 1.15 to 1.20.
- \b boostmanual - whether a multispeed supercharger will manually or
      automatically shift boost speeds. On manual shifting the boost speeds is
      accomplished by controlling the property propulsion/engine/boostspeed.
- \b takeoffboost - boost in psi above sea level ambient. Typically used for
      takeoff, and emergency situations, generally for not more than five
      minutes.  This is a change in the boost control setting, not the actual
      supercharger speed, and so would only give extra power below the rated altitude.
      A typical takeoff boost for an early Merlin was about 12psi, compared
      with a rated boost of 9psi.

      When TAKEOFFBOOST is specified in the config file (and is above RATEDBOOST1), 
      the throttle position is interpreted as:
     - 0 to 0.98 : idle manifold pressure to rated boost (where attainable)
     - 0.99, 1.0 : takeoff boost (where attainable).

The next items are all appended with either 1, 2 or 3 depending on which
boostspeed they refer to:
- \b ratedboost[123] - the absolute rated boost above sea level ambient
      (14.7 PSI, 29.92 inHg) for a given boost speed.

- \b ratedpower[123] - unused
- \b ratedrpm[123] - The rpm at which rated boost is developed
- \b ratedaltitude[123] - The altitude up to which the rated boost can be
      maintained. Up to this altitude the boost is clipped to rated boost or
      takeoffboost. Beyond this altitude the manifold pressure must drop,
      since the supercharger is now at maximum unregulated output. The actual
      pressure multiplier of the supercharger system is calculated at
      initialization from this value.

    @author Jon S. Berndt (Engine framework code and framework-related mods)
    @author Dave Luff (engine operational code)
    @author David Megginson (initial porting and additional code)
    @author Ron Jensen (additional engine code)
    @see Taylor, Charles Fayette, "The Internal Combustion Engine in Theory and Practice"
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPiston : public FGEngine
{
public:
  /// Constructor
  FGPiston(FGFDMExec* exec, Element* el, int engine_number, struct Inputs& input);
  /// Destructor
  ~FGPiston();

  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

  void Calculate(void);
  double GetPowerAvailable(void) const {return (HP * hptoftlbssec);}
  double CalcFuelNeed(void);

  void ResetToIC(void);
  void SetMagnetos(int magnetos) {Magnetos = magnetos;}

  double  GetEGT(void) const { return EGT_degC; }
  int     GetMagnetos(void) const {return Magnetos;}

  double getExhaustGasTemp_degF(void) const {return KelvinToFahrenheit(ExhaustGasTemp_degK);}
  double getManifoldPressure_inHg(void) const {return ManifoldPressure_inHg;}
  double getCylinderHeadTemp_degF(void) const {return KelvinToFahrenheit(CylinderHeadTemp_degK);}
  double getOilPressure_psi(void) const {return OilPressure_psi;}
  double getOilTemp_degF (void) const {return KelvinToFahrenheit(OilTemp_degK);}
  double getRPM(void) const {return RPM;}
  double getAFR(void) const {return m_dot_fuel > 0.0 ? m_dot_air / m_dot_fuel : INFINITY;}

protected:

private:
  int crank_counter = 0;

  double IndicatedHorsePower = 0.0;
  //double IndicatedPower;
  double PMEP = 0.0;
  double FMEP = 0.0;
  double FMEPDynamic = 0.0;
  double FMEPStatic = 0.0;
  //double T_Intake;

  void doEngineStartup(void);
  void doBoostControl(void);
  void doMAP(void);
  void doAirFlow(void);
  void doFuelFlow(void);
  void doEnginePower(void);
  void doEGT(void);
  void doCHT(void);
  void doOilPressure(void);
  void doOilTemperature(void);
  double GetStdPressure100K(double altitude) const;

  int InitRunning(void);

  //
  // constants
  //

  const double R_air;
  //const double rho_fuel;    // kg/m^3
  const double calorific_value_fuel;  // J/Kg (approximate)
  const double Cp_air;      // J/KgK
  const double Cp_fuel;     // J/KgK
  const double standard_pressure; //Pa


  FGTable *Lookup_Combustion_Efficiency;
  FGTable *Mixture_Efficiency_Correlation;

  //
  // Configuration
  //
  double MinManifoldPressure_inHg = 0.0; // Inches Hg
  double MaxManifoldPressure_inHg = 0.0; // Inches Hg
  //double MaxManifoldPressure_Percent; // MaxManifoldPressure / 29.92
  double ManifoldPressureLag = 0.0;      // Manifold Pressure delay in seconds.
  double Displacement = 0.0;             // cubic inches
  double displacement_SI = 0.0;          // cubic meters
  double MaxHP = 0.0;                    // horsepower
  double StaticFriction_HP = 0.0;        // horsepower: amount subtracted from final engine power
  double SparkFailDrop = 0.0;            // drop of power due to spark failure
  double Cycles = 0.0;                   // cycles/power stroke
  double IdleRPM = 0.0;                  // revolutions per minute
  double MaxRPM = 0.0;                   // revolutions per minute
  double Bore = 0.0;                     // inches
  double Stroke = 0.0;                   // inches
  double Cylinders = 0.0;                // number
  double CylinderHeadMass = 0.0;         // kilograms
  double CompressionRatio = 0.0;         // number
  double Z_airbox = 0.0; // number representing intake impediance before the throttle
  double Z_throttle = 0.0; // number representing slope of throttle impediance
  double PeakMeanPistonSpeed_fps = 0.0; // ft/sec speed where intake valves begin to choke. Typically 33-50 fps
  double RatedMeanPistonSpeed_fps = 0.0; // ft/sec derived from MaxRPM and stroke.
  double Ram_Air_Factor = 0.0;           // number

  double StarterTorque = 0.0;// Peak Torque of the starter motor
  double StarterRPM = 0.0;   // Peak RPM of the starter motor
  double StarterGain = 0.0;  // control the torque of the starter motor.
  int BoostSpeeds = 0;  // Number of super/turbocharger boost speeds - zero implies no turbo/supercharging.
  int BoostSpeed = 0;   // The current boost-speed (zero-based).
  bool Boosted = false;     // Set true for boosted engine.
  int BoostManual = 0;  // The raw value read in from the config file - should be 1 or 0 - see description below.
  bool bBoostManual = false;    // Set true if pilot must manually control the boost speed.
  int BoostOverride = 0;    // The raw value read in from the config file - should be 1 or 0 - see description below.
  bool bBoostOverride = false;  // Set true if pilot override of the boost regulator was fitted.
              // (Typically called 'war emergency power').
  bool bTakeoffBoost = false;   // Set true if extra takeoff / emergency boost above rated boost could be attained.
              // (Typically by extra throttle movement past a mechanical 'gate').
  double TakeoffBoost = 0.0;  // Sea-level takeoff boost in psi. (if fitted).
  double RatedBoost[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0};   // Sea-level rated boost in psi.
  double RatedAltitude[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0};    // Altitude at which full boost is reached (boost regulation ends)
                          // and at which power starts to fall with altitude [ft].
  double RatedRPM[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0}; // Engine speed at which the rated power for each boost speed is delivered [rpm].
  double RatedPower[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0};   // Power at rated throttle position at rated altitude [HP].
  double BoostSwitchAltitude[FG_MAX_BOOST_SPEEDS - 1] = {0.0, 0.0};  // Altitude at which switchover (currently assumed automatic)
                              // from one boost speed to next occurs [ft].
  double BoostSwitchPressure[FG_MAX_BOOST_SPEEDS - 1] = {0.0, 0.0};  // Pressure at which boost speed switchover occurs [Pa]
  double BoostMul[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0}; // Pressure multiplier of unregulated supercharger
  double RatedMAP[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0}; // Rated manifold absolute pressure [Pa] (BCV clamp)
  double TakeoffMAP[FG_MAX_BOOST_SPEEDS] = {0.0, 0.0, 0.0};   // Takeoff setting manifold absolute pressure [Pa] (BCV clamp)
  double BoostSwitchHysteresis = 0.0; // Pa.
  double BoostLossFactor = 0.0; // multiplier for HP consumed by the supercharger

  double minMAP = 0.0;  // Pa
  double maxMAP = 0.0;  // Pa
  double MAP = 0.0;     // Pa
  double TMAP = 0.0;    // Pa - throttle manifold pressure e.g. before the supercharger boost
  double ISFC = 0.0;    // Indicated specific fuel consumption [lbs/horsepower*hour

  //
  // Inputs (in addition to those in FGEngine).
  //
  double p_amb = 0.0;              // Pascals
  double p_ram = 0.0;              // Pascals
  double T_amb = 0.0;              // degrees Kelvin
  double RPM = 0.0;                // revolutions per minute
  double IAS = 0.0;                // knots
  double Cooling_Factor = 0.0;     // normal
  bool Magneto_Left = false;
  bool Magneto_Right = false;
  int Magnetos = 0;

  double Oil_Press_Relief_Valve = 0.0;
  double Oil_Press_RPM_Max = 0.0;
  double Design_Oil_Temp = 0.0;         // degK
  double Oil_Viscosity_Index = 0.0;

  //
  // Outputs (in addition to those in FGEngine).
  //
  double rho_air = 0.0;
  double volumetric_efficiency = 0.0;
  double volumetric_efficiency_reduced = 0.0;
  //double map_coefficient;
  double m_dot_air = 0.0;
  double v_dot_air = 0.0;
  double equivalence_ratio = 0.0;
  double m_dot_fuel = 0.0;
  double HP = 0.0;
  double BoostLossHP = 0.0;
  double combustion_efficiency = 0.0;
  double ExhaustGasTemp_degK = 0.0;
  double EGT_degC = 0.0;
  double ManifoldPressure_inHg = 0.0;
  double CylinderHeadTemp_degK = 0.0;
  double OilPressure_psi = 0.0;
  double OilTemp_degK = 0.0;
  double MeanPistonSpeed_fps = 0.0;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
