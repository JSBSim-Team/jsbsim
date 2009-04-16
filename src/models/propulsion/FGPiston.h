/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPiston.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

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
#include <math/FGTable.h>
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PISTON "$Id: FGPiston.h,v 1.15 2009/04/16 06:41:18 ehofman Exp $";
#define FG_MAX_BOOST_SPEEDS 3

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models Dave Luff's Turbo/Supercharged Piston engine model.

<h3>Configuration File Format:</h3>

@code
<piston_engine name="{string}">
  <minmp unit="{INHG | PA | ATM}"> {number} </minmp> <!-- Depricated -->
  <maxmp unit="{INHG | PA | ATM}"> {number} </maxmp>
  <displacement unit="{IN3 | LTR | CC}"> {number} </displacement>
  <bore unit="{IN | M}"> {number} </bore>
  <stroke unit="{IN | M}"> {number} </stroke>
  <cylinders> {number} </cylinders>
  <compression-ratio> {number} </compression-ratio>
  <sparkfaildrop> {number} </sparkfaildrop>
  <maxhp unit="{HP | WATTS}"> {number} </maxhp>
  <cycles> {number} </cycles>
  <idlerpm> {number} </idlerpm>
  <maxrpm> {number} </maxrpm>
  <maxthrottle> {number} </maxthrottle>
  <minthrottle> {number} </minthrottle>
  <bsfc unit="{LBS/HP*HR | "KG/KW*HR"}"> {number} </bsfc>
  <volumetric_efficiency> {number} </volumetric_efficiency>
  <numboostspeeds> {number} </numboostspeeds>
  <boostoverride> {0 | 1} </boostoverride>
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
</piston_engine>
@endcode

<pre>
    Additional elements are required for a supercharged engine.  These can be
    left off a non-supercharged engine, ie. the changes are all backward
    compatible at present.

    - NUMBOOSTSPEEDS - zero (or not present) for a naturally-aspirated engine,
      either 1, 2 or 3 for a boosted engine.  This corresponds to the number of
      supercharger speeds.  Merlin XII had 1 speed, Merlin 61 had 2, a late
      Griffon engine apparently had 3.  No known engine more than 3, although
      some German engines apparently had a continuously variable-speed
      supercharger.

    - BOOSTOVERRIDE - whether the boost pressure control system (either a boost
      control valve for superchargers or wastegate for turbochargers) can be
      overriden by the pilot.  During wartime this was commonly possible, and
      known as "War Emergency Power" by the Brits.  1 or 0 in the config file.
      This isn't implemented in the model yet though, there would need to be
      some way of getting the boost control cutout lever position (on or off)
      from FlightGear first.

    - The next items are all appended with either 1, 2 or 3 depending on which
      boost speed they refer to, eg RATEDBOOST1.  The rated values seems to have
      been a common convention at the time to express the maximum continuously
      available power, and the conditions to attain that power.

    - RATEDBOOST[123] - the absolute rated boost above sea level ambient for a
      given boost speed, in psi.  Eg the Merlin XII had a rated boost of 9psi,
      giving approximately 42inHg manifold pressure up to the rated altitude.

    - RATEDALTITUDE[123] - The altitude up to which rated boost can be
      maintained.  Up to this altitude the boost is maintained constant for a
      given throttle position by the BCV or wastegate.  Beyond this altitude the
      manifold pressure must drop, since the supercharger is now at maximum
      unregulated output.  The actual pressure multiplier of the supercharger
      system is calculated at initialisation from this value.

    - RATEDPOWER[123] - The power developed at rated boost at rated altitude at
      rated rpm.

    - RATEDRPM[123] - The rpm at which rated power is developed.

    - TAKEOFFBOOST - Takeoff boost in psi above ambient.  Many aircraft had an
      extra boost setting beyond rated boost, but not totally uncontrolled as in
      the already mentioned boost-control-cutout, typically attained by pushing
      the throttle past a mechanical 'gate' preventing its inadvertant use. This
      was typically used for takeoff, and emergency situations, generally for
      not more than five minutes.  This is a change in the boost control
      setting, not the actual supercharger speed, and so would only give extra
      power below the rated altitude.  When TAKEOFFBOOST is specified in the
      config file (and is above RATEDBOOST1), then the throttle position is
      interpreted as:

    - 0 to 0.95 : idle manifold pressure to rated boost (where attainable)
    - 0.96, 0.97, 0.98 : rated boost (where attainable).
    - 0.99, 1.0 : takeoff boost (where attainable).

    A typical takeoff boost for an earlyish Merlin was about 12psi, compared
    with a rated boost of 9psi.

    It is quite possible that other boost control settings could have been used
    on some aircraft, or that takeoff/extra boost could have activated by other
    means than pushing the throttle full forward through a gate, but this will
    suffice for now.

    Note that MAXMP is still the non-boosted max manifold pressure even for
    boosted engines - effectively this is simply a measure of the pressure drop
    through the fully open throttle.
</pre>

    @author Jon S. Berndt (Engine framework code and framework-related mods)
    @author Dave Luff (engine operational code)
    @author David Megginson (initial porting and additional code)
    @author Ron Jensen (additional engine code)
    @version $Id: FGPiston.h,v 1.15 2009/04/16 06:41:18 ehofman Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPiston : public FGEngine
{
public:
  /// Constructor
  FGPiston(FGFDMExec* exec, Element* el, int engine_number);
  /// Destructor
  ~FGPiston();

  string GetEngineLabels(string delimeter);
  string GetEngineValues(string delimeter);

  double Calculate(void);
  double GetPowerAvailable(void) {return PowerAvailable;}
  double CalcFuelNeed(void);

  void ResetToIC(void);
  void SetMagnetos(int magnetos) {Magnetos = magnetos;}

  double  GetEGT(void) { return EGT_degC; }
  int     GetMagnetos(void) {return Magnetos;}

  double getExhaustGasTemp_degF(void) {return KelvinToFahrenheit(ExhaustGasTemp_degK);}
  double getManifoldPressure_inHg(void) const {return ManifoldPressure_inHg;}
  double getCylinderHeadTemp_degF(void) {return KelvinToFahrenheit(CylinderHeadTemp_degK);}
  double getOilPressure_psi(void) const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}
  double getRPM(void) {return RPM;}

protected:
  double ThrottleAngle;

private:
  int crank_counter;

  double IndicatedHorsePower;
  double PMEP;
  double FMEP;
  double SpeedSlope;
  double SpeedIntercept;
  double AltitudeSlope;
  double PowerAvailable;

  // timestep
  double dt;

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

  int InitRunning(void);

  //
  // constants
  //

  const double R_air;
  const double rho_fuel;    // kg/m^3
  const double calorific_value_fuel;  // W/Kg (approximate)
  const double Cp_air;      // J/KgK
  const double Cp_fuel;     // J/KgK

  FGTable *Lookup_Combustion_Efficiency;
  FGTable *Mixture_Efficiency_Correlation;

  //
  // Configuration
  //
  double MinManifoldPressure_inHg; // Inches Hg
  double MaxManifoldPressure_inHg; // Inches Hg
  double MaxManifoldPressure_Percent; // MaxManifoldPressure / 29.92
  double Displacement;             // cubic inches
  double displacement_SI;          // cubic meters
  double MaxHP;                    // horsepower
  double SparkFailDrop;            // drop of power due to spark failure
  double Cycles;                   // cycles/power stroke
  double IdleRPM;                  // revolutions per minute
  double MaxRPM;                   // revolutions per minute
  double Bore;                     // inches
  double Stroke;                   // inches
  double Cylinders;                // number
  double CompressionRatio;        // number

  double StarterHP;                // initial horsepower of starter motor
  int BoostSpeeds;	// Number of super/turbocharger boost speeds - zero implies no turbo/supercharging.
  int BoostSpeed;	// The current boost-speed (zero-based).
  bool Boosted;		// Set true for boosted engine.
  int BoostOverride;	// The raw value read in from the config file - should be 1 or 0 - see description below.
  bool bBoostOverride;	// Set true if pilot override of the boost regulator was fitted.
              // (Typically called 'war emergency power').
  bool bTakeoffBoost;	// Set true if extra takeoff / emergency boost above rated boost could be attained.
              // (Typically by extra throttle movement past a mechanical 'gate').
  double TakeoffBoost;	// Sea-level takeoff boost in psi. (if fitted).
  double RatedBoost[FG_MAX_BOOST_SPEEDS];	// Sea-level rated boost in psi.
  double RatedAltitude[FG_MAX_BOOST_SPEEDS];	// Altitude at which full boost is reached (boost regulation ends)
                          // and at which power starts to fall with altitude [ft].
  double RatedRPM[FG_MAX_BOOST_SPEEDS]; // Engine speed at which the rated power for each boost speed is delivered [rpm].
  double RatedPower[FG_MAX_BOOST_SPEEDS];	// Power at rated throttle position at rated altitude [HP].
  double BoostSwitchAltitude[FG_MAX_BOOST_SPEEDS - 1];	// Altitude at which switchover (currently assumed automatic)
                              // from one boost speed to next occurs [ft].
  double BoostSwitchPressure[FG_MAX_BOOST_SPEEDS - 1];  // Pressure at which boost speed switchover occurs [Pa]
  double BoostMul[FG_MAX_BOOST_SPEEDS];	// Pressure multipier of unregulated supercharger
  double RatedMAP[FG_MAX_BOOST_SPEEDS];	// Rated manifold absolute pressure [Pa] (BCV clamp)
  double TakeoffMAP[FG_MAX_BOOST_SPEEDS];	// Takeoff setting manifold absolute pressure [Pa] (BCV clamp)
  double BoostSwitchHysteresis;	// Pa.

  double minMAP;  // Pa
  double maxMAP;  // Pa
  double MAP;     // Pa
  double TMAP;    // Pa - throttle manifold pressure e.g. before the supercharger boost
  double ISFC;    // Indicated specific fuel consumption [lbs/horsepower*hour

  //
  // Inputs (in addition to those in FGEngine).
  //
  double p_amb;              // Pascals
  double T_amb;              // degrees Kelvin
  double RPM;                // revolutions per minute
  double IAS;                // knots
  bool Magneto_Left;
  bool Magneto_Right;
  int Magnetos;

  //
  // Outputs (in addition to those in FGEngine).
  //
  double rho_air;
  double volumetric_efficiency;
  double map_coefficient;
  double m_dot_air;
  double equivalence_ratio;
  double m_dot_fuel;
  double HP;
  double combustion_efficiency;
  double ExhaustGasTemp_degK;
  double EGT_degC;
  double ManifoldPressure_inHg;
  double CylinderHeadTemp_degK;
  double OilPressure_psi;
  double OilTemp_degK;
  double MeanPistonSpeed_fps;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
