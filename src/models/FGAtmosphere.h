/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
 Date started: 6/2011

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

#ifndef FGATMOSPHERE_H
#define FGATMOSPHERE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models an empty, abstract base atmosphere class.

  <h2> Properties </h2>
  @property atmosphere/T-R The current modeled temperature in degrees Rankine.
  @property atmosphere/rho-slugs_ft3
  @property atmosphere/P-psf
  @property atmosphere/a-fps
  @property atmosphere/T-sl-R
  @property atmosphere/rho-sl-slugs_ft3
  @property atmosphere/P-sl-psf
  @property atmosphere/a-sl-fps
  @property atmosphere/theta
  @property atmosphere/sigma
  @property atmosphere/delta
  @property atmosphere/a-ratio

  @author Jon Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGAtmosphere : public FGModel {
public:

  /// Enums for specifying temperature units.
  enum eTemperature {eNoTempUnit=0, eFahrenheit, eCelsius, eRankine, eKelvin};

  /// Enums for specifying pressure units.
  enum ePressure {eNoPressUnit=0, ePSF, eMillibars, ePascals, eInchesHg};

  /// Constructor
  FGAtmosphere(FGFDMExec*);

  /// Destructor
  virtual ~FGAtmosphere();

  /** Runs the atmosphere forces model; called by the Executive.
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  bool InitModel(void) override;

  //  *************************************************************************
  /// @name Temperature access functions.
  /// There are several ways to get the temperature, and several modeled temperature
  /// values that can be retrieved.
  // @{
  /// Returns the actual, modeled temperature at the current altitude in degrees Rankine.
  /// @return Modeled temperature in degrees Rankine.
  virtual double GetTemperature() const {return Temperature;}

  /// Returns the actual modeled temperature in degrees Rankine at a specified altitude.
  /// @param altitude The altitude above sea level (ASL) in feet.
  /// @return Modeled temperature in degrees Rankine at the specified altitude.
  virtual double GetTemperature(double altitude) const = 0;

  /// Returns the actual, modeled sea level temperature in degrees Rankine.
  /// @return The modeled temperature in degrees Rankine at sea level.
  virtual double GetTemperatureSL() const { return SLtemperature; }

  /// Returns the ratio of the at-current-altitude temperature as modeled
  /// over the sea level value.
  virtual double GetTemperatureRatio() const { return GetTemperature()/SLtemperature; }

  /// Returns the ratio of the temperature as modeled at the supplied altitude
  /// over the sea level value.
  virtual double GetTemperatureRatio(double h) const { return GetTemperature(h)/SLtemperature; }

  /// Sets the Sea Level temperature.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperatureSL(double t, eTemperature unit=eFahrenheit);

  /// Sets the temperature at the supplied altitude.
  /// @param t The temperature value in the unit provided.
  /// @param h The altitude in feet above sea level.
  /// @param unit The unit of the temperature.
  virtual void SetTemperature(double t, double h, eTemperature unit=eFahrenheit) = 0;
  //@}

  //  *************************************************************************
  /// @name Pressure access functions.
  //@{
  /// Returns the pressure in psf.
  virtual double GetPressure(void) const {return Pressure;}

  /// Returns the pressure at a specified altitude in psf.
  virtual double GetPressure(double altitude) const = 0;

  // Returns the sea level pressure in target units, default in psf.
  virtual double GetPressureSL(ePressure to=ePSF) const { return ConvertFromPSF(SLpressure, to);  }

  /// Returns the ratio of at-altitude pressure over the sea level value.
  virtual double GetPressureRatio(void) const { return Pressure/SLpressure; }

  /** Sets the sea level pressure for modeling.
      @param pressure The pressure in the units specified.
      @param unit the unit of measure that the specified pressure is
                  supplied in.*/
  virtual void SetPressureSL(ePressure unit, double pressure);
  //@}

  //  *************************************************************************
  /// @name Density access functions.
  //@{
  /** Returns the density in slugs/ft^3.
      This function may only be used if Run() is called first. */
  virtual double GetDensity(void)  const {return Density;}

  /** Returns the density in slugs/ft^3 at a given altitude in ft. */
  virtual double GetDensity(double altitude) const;

  /// Returns the sea level density in slugs/ft^3
  virtual double GetDensitySL(void)  const { return SLdensity; }

  /// Returns the ratio of at-altitude density over the sea level value.
  virtual double GetDensityRatio(void) const { return Density/SLdensity; }
  //@}

  //  *************************************************************************
  /// @name Speed of sound access functions.
  //@{
  /// Returns the speed of sound in ft/sec.
  virtual double GetSoundSpeed(void) const {return Soundspeed;}

  /// Returns the speed of sound in ft/sec at a given altitude in ft.
  virtual double GetSoundSpeed(double altitude) const;

  /// Returns the sea level speed of sound in ft/sec.
  virtual double GetSoundSpeedSL(void) const { return SLsoundspeed; }

  /// Returns the ratio of at-altitude sound speed over the sea level value.
  virtual double GetSoundSpeedRatio(void) const { return Soundspeed/SLsoundspeed; }
  //@}

  //  *************************************************************************
  /// @name Viscosity access functions.
  //@{
  /// Returns the absolute viscosity.
  virtual double GetAbsoluteViscosity(void) const {return Viscosity;}

  /// Returns the kinematic viscosity.
  virtual double GetKinematicViscosity(void) const {return KinematicViscosity;}
  //@}

  virtual double GetDensityAltitude() const {return DensityAltitude;}

  virtual double GetPressureAltitude() const {return PressureAltitude;}

  struct Inputs {
    double altitudeASL;
    double GeodLatitudeDeg;
    double LongitudeDeg;
  } in;

  static constexpr double StdDaySLtemperature = 518.67;
  static constexpr double StdDaySLpressure = 2116.228;
  const double StdDaySLsoundspeed;
  static constexpr double SHRatio = 1.4;

protected:
  // Sea level conditions
  double SLtemperature = 1.8;
  double SLdensity = 1.0;
  double SLpressure = 1.0;
  double SLsoundspeed = 1.0;
  // Current actual conditions at altitude
  double Temperature = 1.8;
  double Density = 0.0;
  double Pressure = 0.0;
  double Soundspeed = 0.0;
  double PressureAltitude = 0.0;
  double DensityAltitude = 0.0;

  static constexpr double SutherlandConstant = 198.72;  // deg Rankine
  static constexpr double Beta = 2.269690E-08; // slug/(sec ft R^0.5)
  double Viscosity = 0.0;
  double KinematicViscosity = 0.0;

  /// Calculate the atmosphere for the given altitude.
  virtual void Calculate(double altitude);

  /// Calculates the density altitude given any temperature or pressure bias.
  /// Calculated density for the specified geometric altitude given any temperature
  /// or pressure biases is passed in.
  /// @param density
  /// @param geometricAlt
  virtual double CalculateDensityAltitude(double density, double geometricAlt) { return geometricAlt; }

  /// Calculates the pressure altitude given any temperature or pressure bias.
  /// Calculated pressure for the specified geometric altitude given any temperature
  /// or pressure biases is passed in.
  /// @param pressure
  /// @param geometricAlt
  virtual double CalculatePressureAltitude(double pressure, double geometricAlt) { return geometricAlt; }

  /// Converts to Rankine from one of several unit systems.
  double ConvertToRankine(double t, eTemperature unit) const;

  /// Converts from Rankine to one of several unit systems.
  double ConvertFromRankine(double t, eTemperature unit) const;

  /// Converts to PSF (pounds per square foot) from one of several unit systems.
  double ConvertToPSF(double t, ePressure unit=ePSF) const;

  /// Converts from PSF (pounds per square foot) to one of several unit systems.
  double ConvertFromPSF(double t, ePressure unit=ePSF) const;

  /// Check that the pressure is within plausible boundaries.
  /// @param msg Message to display if the pressure is out of boundaries
  /// @param quiet Don't display the message if set to true
  double ValidatePressure(double p, const string& msg, bool quiet=false) const;

  /// Check that the temperature is within plausible boundaries.
  /// @param msg Message to display if the pressure is out of boundaries
  /// @param quiet Don't display the message if set to true
  double ValidateTemperature(double t, const string& msg, bool quiet=false) const;

  /// @name ISA constants
  //@{
  /// Universal gas constant - ft*lbf/R/mol
  static constexpr double Rstar = 8.31432 * kgtoslug / KelvinToRankine(fttom * fttom);
  /// Mean molecular weight for air - slug/mol
  static constexpr double Mair = 28.9645 * kgtoslug / 1000.0;
  /** Sea-level acceleration of gravity - ft/s^2.
      This constant is defined to compute the International Standard Atmosphere.
      It is by definition the sea level gravity at a latitude of 45deg. This
      value is fixed whichever gravity model is used by FGInertial.
  */
  static constexpr double g0 = 9.80665 / fttom;
  /// Specific gas constant for air - ft*lbf/slug/R
  static constexpr double Reng0 = Rstar / Mair;
  //@}

  double Reng = Reng0;

  virtual void bind(void);
  void Debug(int from) override;

public:
  static constexpr double StdDaySLdensity = StdDaySLpressure / (Reng0 * StdDaySLtemperature);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
