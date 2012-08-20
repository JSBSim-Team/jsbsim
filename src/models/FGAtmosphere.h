/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
 Date started: 6/2011

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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
5/2011   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGATMOSPHERE_H
#define FGATMOSPHERE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "models/FGModel.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.31 2012/08/20 12:28:50 jberndt Exp $"

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
  @version $Id: FGAtmosphere.h,v 1.31 2012/08/20 12:28:50 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAtmosphere : public FGModel {
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
  bool Run(bool Holding);

  bool InitModel(void);

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
  virtual double GetTemperatureSL() const { return GetTemperature(0.0); }

  /// Returns the ratio of the at-current-altitude temperature as modeled
  /// over the sea level value.
  virtual double GetTemperatureRatio() const { return GetTemperature()*rSLtemperature; }

  /// Returns the ratio of the temperature as modeled at the supplied altitude
  /// over the sea level value.
  virtual double GetTemperatureRatio(double h) const { return GetTemperature(h)*rSLtemperature; }

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
  virtual double GetPressure(void)  const {return Pressure;}

  /// Returns the pressure at a specified altitude in psf.
  virtual double GetPressure(double altitude) const = 0;

  // Returns the sea level pressure in target units, default in psf.
  virtual double GetPressureSL(ePressure to=ePSF) const { return ConvertFromPSF(SLpressure, to);  }

  /// Returns the ratio of at-altitude pressure over the sea level value.
  virtual double GetPressureRatio(void) const { return Pressure*rSLpressure; }

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
  virtual double GetDensityRatio(void) const { return Density*rSLdensity; }
  //@}

  //  *************************************************************************
  /// @name Speed of sound access functions.
  //@{
  /// Returns the speed of sound in ft/sec.
  virtual double GetSoundSpeed(void) const {return Soundspeed;}

  /// Returns the sea level speed of sound in ft/sec.
  virtual double GetSoundSpeedSL(void) const { return SLsoundspeed; }

  /// Returns the ratio of at-altitude sound speed over the sea level value.
  virtual double GetSoundSpeedRatio(void) const { return Soundspeed*rSLsoundspeed; }
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
  } in;

protected:
  double    SLtemperature,    SLdensity,    SLpressure,    SLsoundspeed; // Sea level conditions
  double      Temperature,      Density,      Pressure,      Soundspeed; // Current actual conditions at altitude
  double   rSLtemperature,   rSLdensity,   rSLpressure,   rSLsoundspeed; // Reciprocal of sea level conditions

  double PressureAltitude;
  double DensityAltitude;

  const double SutherlandConstant, Beta;
  double Viscosity, KinematicViscosity;

  /// Calculate the atmosphere for the given altitude.
  void Calculate(double altitude);

  // Converts to Rankine from one of several unit systems.
  virtual double ConvertToRankine(double t, eTemperature unit) const;
  
  // Converts to PSF (pounds per square foot) from one of several unit systems.
  virtual double ConvertToPSF(double t, ePressure unit=ePSF) const;

  // Converts from PSF (pounds per square foot) to one of several unit systems.
  virtual double ConvertFromPSF(double t, ePressure unit=ePSF) const;

  virtual void bind(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

