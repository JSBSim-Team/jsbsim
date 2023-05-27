/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGStandardAtmosphere.h
 Author:       Jon Berndt
 Date started: 5/2011

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
5/2011   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSTANDARDATMOSPHERE_H
#define FGSTANDARDATMOSPHERE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>

#include "math/FGTable.h"
#include "models/FGAtmosphere.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the 1976 U.S. Standard Atmosphere, with the ability to modify the 
    temperature and pressure. A base feature of the model is the temperature 
    profile that is stored as an FGTable object with this data:

@code
GeoMet Alt    Temp      GeoPot Alt  GeoMet Alt
   (ft)      (deg R)      (km)        (km)
 ---------  --------    ----------  ----------
       0.0    518.67 //    0.000       0.000
   36151.6    390.0  //   11.000      11.019
   65823.5    390.0  //   20.000      20.063
  105518.4    411.6  //   32.000      32.162
  155347.8    487.2  //   47.000      47.350
  168677.8    487.2  //   51.000      51.413
  235570.9    386.4  //   71.000      71.802
  282152.2    336.5; //   84.852      86.000
@endcode

The pressure is calculated at lower altitudes through the use of two equations
that are presented in the U.S. Standard Atmosphere document (see references).
Density, kinematic viscosity, speed of sound, etc., are all calculated based
on various constants and temperature and pressure. At higher altitudes (above 
86 km (282152 ft) a different and more complicated method of calculating
pressure is used.

The temperature may be modified through the use of several methods. Ultimately,
these access methods allow the user to modify the sea level standard
temperature, and/or the sea level standard pressure, so that the entire profile
will be consistently and accurately calculated.

  <h2> Properties </h2>
  @property atmosphere/delta-T
  @property atmosphere/T-sl-dev-F

  @author Jon Berndt
  @see "U.S. Standard Atmosphere, 1976", NASA TM-X-74335
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGStandardAtmosphere : public FGAtmosphere {
public:
  /// Constructor
  FGStandardAtmosphere(FGFDMExec*);

  /// Destructor
  virtual ~FGStandardAtmosphere();

  bool InitModel(void) override;

  //  *************************************************************************
  /// @name Temperature access functions.
  /// There are several ways to get the temperature, and several modeled
  /// temperature values that can be retrieved. The U.S. Standard Atmosphere
  /// temperature either at a specified altitude, or at sea level can be
  /// retrieved. These two temperatures do NOT include the effects of any bias
  /// or delta gradient that may have been supplied by the user. The modeled
  /// temperature and the modeled temperature at sea level can also be
  /// retrieved. These two temperatures DO include the effects of an optionally
  /// user-supplied bias or delta gradient.
  // @{
  /// Returns the actual modeled temperature in degrees Rankine at a specified
  /// altitude.
  /// @param altitude The altitude above sea level (ASL) in feet.
  /// @return Modeled temperature in degrees Rankine at the specified altitude.
  double GetTemperature(double altitude) const override;

  /// Returns the standard temperature in degrees Rankine at a specified
  /// altitude.
  /// @param altitude The altitude in feet above sea level (ASL) to get the
  ///                 temperature at.
  /// @return The STANDARD temperature in degrees Rankine at the specified
  ///         altitude.
  virtual double GetStdTemperature(double altitude) const;

  /// Returns the standard sea level temperature in degrees Rankine.
  /// @return The STANDARD temperature at sea level in degrees Rankine.
  virtual double GetStdTemperatureSL() const { return StdSLtemperature; }

  /// Returns the ratio of the standard temperature at the supplied altitude 
  /// over the standard sea level temperature.
  virtual double GetStdTemperatureRatio(double h) const { return GetStdTemperature(h)/StdSLtemperature; }

  /// Returns the temperature bias over the sea level value in degrees Rankine.
  virtual double GetTemperatureBias(eTemperature to) const
  { if (to == eCelsius || to == eKelvin) return TemperatureBias/1.80; else return TemperatureBias; }

  /// Returns the temperature gradient to be applied on top of the standard
  /// temperature gradient.
  virtual double GetTemperatureDeltaGradient(eTemperature to)
  { if (to == eCelsius || to == eKelvin) return TemperatureDeltaGradient/1.80; else return TemperatureDeltaGradient; }

  /// Sets the Sea Level temperature, if it is to be different than the
  /// standard.
  /// This function will calculate a bias - a difference - from the standard
  /// atmosphere temperature and will apply that bias to the entire
  /// temperature profile. This is one way to set the temperature bias. Using
  /// the SetTemperatureBias function will replace the value calculated by
  /// this function.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  void SetTemperatureSL(double t, eTemperature unit=eFahrenheit) override;

  /// Sets the temperature at the supplied altitude, if it is to be different
  /// than the standard temperature.
  /// This function will calculate a bias - a difference - from the standard
  /// atmosphere temperature at the supplied altitude and will apply that
  /// calculated bias to the entire temperature profile. If a graded delta is
  /// present, that will be included in the calculation - that is, regardless
  /// of any graded delta present, a temperature bias will be determined so that
  /// the temperature requested in this function call will be reached.
  /// @param t The temperature value in the unit provided.
  /// @param h The altitude in feet above sea level.
  /// @param unit The unit of the temperature.
  void SetTemperature(double t, double h, eTemperature unit=eFahrenheit) override;

  /// Sets the temperature bias to be added to the standard temperature at all
  /// altitudes.
  /// This function sets the bias - the difference - from the standard
  /// atmosphere temperature. This bias applies to the entire
  /// temperature profile. Another way to set the temperature bias is to use the
  /// SetSLTemperature function, which replaces the value calculated by
  /// this function with a calculated bias.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperatureBias(eTemperature unit, double t);

  /// Sets a Sea Level temperature delta that is ramped out by 86 km.
  /// The value of the delta is used to calculate a delta gradient that is
  /// applied to the temperature at all altitudes below 86 km (282152 ft). 
  /// For instance, if a temperature of 20 degrees F is supplied, the delta
  /// gradient would be 20/282152 - or, about 7.09E-5 degrees/ft. At sea level,
  /// the full 20 degrees would be added to the standard temperature,
  /// but that 20 degree delta would be reduced by 7.09E-5 degrees for every
  /// foot of altitude above sea level, so that by 86 km, there would be no
  /// further delta added to the standard temperature.
  /// The graded delta can be used along with the a bias to tailor the
  /// temperature profile as desired.
  /// @param t the sea level temperature delta value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetSLTemperatureGradedDelta(eTemperature unit, double t);

  /// Sets the temperature delta value at the supplied altitude/elevation above
  /// sea level, to be added to the standard temperature and ramped out by
  /// 86 km.
  /// This function computes the sea level delta from the standard atmosphere
  /// temperature at sea level.
  /// @param t the temperature skew value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperatureGradedDelta(double t, double h, eTemperature unit=eFahrenheit);

  /// This function resets the model to apply no bias or delta gradient to the
  /// temperature.
  /// The delta gradient and bias values are reset to 0.0, and the standard
  /// temperature is used for the entire temperature profile at all altitudes.
  virtual void ResetSLTemperature();
  //@}

  //  *************************************************************************
  /// @name Pressure access functions.
  //@{
  /// Returns the pressure at a specified altitude in psf.
  double GetPressure(double altitude) const override;

  /// Returns the standard pressure at the specified altitude.
  virtual double GetStdPressure(double altitude) const;

  /** Sets the sea level pressure for modeling an off-standard pressure
      profile. This could be useful in the case where the pressure at an
      airfield is known or set for a particular simulation run.
      @param pressure The pressure in the units specified.
      @param unit the unit of measure that the specified pressure is
                       supplied in.*/
  void SetPressureSL(ePressure unit, double pressure) override;

  /** Resets the sea level to the Standard sea level pressure, and recalculates
      dependent parameters so that the pressure calculations are standard. */
  virtual void ResetSLPressure();
  //@}

  //  *************************************************************************
  /// @name Density access functions.
  //@{
  /// Returns the standard density at a specified altitude
  virtual double GetStdDensity(double altitude) const;
  //@}

  //  *************************************************************************
  ///@name Humidity access functions
  //@{
  /** Sets the dew point.
      @param dewpoint The dew point in the units specified
      @param unit The unit of measure that the specified dew point is supplied
                  in. */
  void SetDewPoint(eTemperature unit, double dewpoint);
  /** Returns the dew point.
      @param to The unit of measure that the dew point should be supplied in. */
  double GetDewPoint(eTemperature to) const;
  /** Sets the partial pressure of water vapor.
      @param Pv The vapor pressure in the units specified
      @param unit The unit of measure that the specified vapor pressure is
                  supplied in. */
  void SetVaporPressure(ePressure unit, double Pv);
  /** Returns the partial pressure of water vapor.
      @param to The unit of measure that the water vapor should be supplied in.
  */
  double GetVaporPressure(ePressure to) const;
  /** Returns the saturated pressure of water vapor.
      @param to The unit of measure that the water vapor should be supplied in.
  */
  double GetSaturatedVaporPressure(ePressure to) const;
  /** Sets the relative humidity.
      @param RH The relative humidity in percent. */
  void SetRelativeHumidity(double RH);
  /// Returns the relative humidity in percent.
  double GetRelativeHumidity(void) const;
  /** Sets the vapor mass per million of dry air mass units.
      @param frac The fraction of water in ppm of dry air. */
  void SetVaporMassFractionPPM(double frac);
  /// Returns the vapor mass per million of dry air mass units (ppm).
  double GetVaporMassFractionPPM(void) const;
  //@}

  /// Prints the U.S. Standard Atmosphere table.
  virtual void PrintStandardAtmosphereTable();

protected:
  /// Standard sea level conditions
  double StdSLtemperature, StdSLdensity, StdSLpressure, StdSLsoundspeed;

  double TemperatureBias;
  double TemperatureDeltaGradient;
  double GradientFadeoutAltitude;
  double VaporMassFraction;
  double SaturatedVaporPressure;

  FGTable StdAtmosTemperatureTable;
  FGTable MaxVaporMassFraction;
  std::vector<double> LapseRates;
  std::vector<double> PressureBreakpoints;
  std::vector<double> StdPressureBreakpoints;
  std::vector<double> StdDensityBreakpoints;
  std::vector<double> StdLapseRates;

  void Calculate(double altitude) override;

  /// Recalculate the lapse rate vectors when the temperature profile is altered
  /// in a way that would change the lapse rates, such as when a gradient is
  /// applied.
  /// This function is also called to initialize the lapse rate vector.
  void CalculateLapseRates();

  /// Calculate (or recalculate) the atmospheric pressure breakpoints at the 
  /// altitudes in the standard temperature table.
  void CalculatePressureBreakpoints(double SLpress);

  /// Calculate the atmospheric density breakpoints at the 
  /// altitudes in the standard temperature table.
  void CalculateStdDensityBreakpoints();

  /// Convert a geometric altitude to a geopotential altitude
  double GeopotentialAltitude(double geometalt) const { return (geometalt * EarthRadius) / (EarthRadius + geometalt); }

  /// Convert a geopotential altitude to a geometric altitude
  double GeometricAltitude(double geopotalt) const { return (geopotalt * EarthRadius) / (EarthRadius - geopotalt); }

  /** Calculates the density altitude given any temperature or pressure bias.
  Calculated density for the specified geometric altitude given any temperature
  or pressure biases is passed in.
  @param density
  @param geometricAlt
  @see
  https://en.wikipedia.org/wiki/Density_altitude
  https://wahiduddin.net/calc/density_altitude.htm
  */
  double CalculateDensityAltitude(double density, double geometricAlt) override;

  /** Calculates the pressure altitude given any temperature or pressure bias.
  Calculated density for the specified geometric altitude given any temperature
  or pressure biases is passed in.
  @param pressure
  @param geometricAlt
  @see
  https://en.wikipedia.org/wiki/Pressure_altitude
  */
  double CalculatePressureAltitude(double pressure, double geometricAlt) override;

  /// Calculate the pressure of water vapor with the Magnus formula.
  double CalculateVaporPressure(double temperature);

  /// Validate the value of the vapor mass fraction
  void ValidateVaporMassFraction(double geometricAlt);

  /// Calculate the SL density
  void CalculateSLDensity(void) { SLdensity = SLpressure / (Reng * SLtemperature); }

  /// Calculate the SL density and sound speed
  void CalculateSLSoundSpeedAndDensity(void);

  void bind(void) override;
  void Debug(int from) override;

  /// Earth radius in ft as defined for ISA 1976
  static constexpr double EarthRadius = 6356766.0 / fttom;
  /** Sonntag constants based on ref [2]. They are valid for temperatures
      between -45 degC (-49 degF) and 60 degC (140 degF) with a precision of
      +/-0.35 degC (+/-0.63 degF) */
  static constexpr double a = 611.2/psftopa; // psf
  static constexpr double b = 17.62; // 1/degC
  static constexpr double c = 243.12; // degC
  /// Mean molecular weight for water - slug/mol
  static constexpr double Mwater = 18.016 * kgtoslug / 1000.0;
  static constexpr double Rdry = Rstar / Mair;
  static constexpr double Rwater = Rstar / Mwater;
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
