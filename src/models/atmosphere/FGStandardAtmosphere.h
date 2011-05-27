/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGStandardAtmosphere.h
 Author:       Jon Berndt
 Date started: 5/2011

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

#ifndef FGSTANDARDATMOSPHERE_H
#define FGSTANDARDATMOSPHERE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "math/FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STANDARDATMOSPHERE "$Id: FGStandardAtmosphere.h,v 1.4 2011/05/27 12:25:50 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the 1976 Standard Atmosphere.

  <h2> Properties </h2>
  @property atmosphere/T-R
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
  @property atmosphere/delta-T
  @property atmosphere/T-sl-dev-F

  @author Jon Berndt
  @see "U.S. Standard Atmosphere, 1976", NASA TM-X-74335
  @version $Id: FGStandardAtmosphere.h,v 1.4 2011/05/27 12:25:50 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGStandardAtmosphere : public FGModel {
public:

  /// Enums for specifying temperature units.
  enum eTemperature {eNone=0, eFahrenheit, eCelsius, eRankine, eKelvin};

  /// Constructor
  FGStandardAtmosphere(FGFDMExec*);
  /// Destructor
  ~FGStandardAtmosphere();
  /** Runs the standard atmosphere forces model; called by the Executive.
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
  // @{
  /// Returns the temperature in degrees Rankine.
  virtual double GetTemperature(void) const {return Temperature;}

  /// Returns the standard temperature in degrees Rankine at a specified altitude.
  virtual double GetTemperature(double altitude) const;

  /// Returns the sea level temperature in degrees Rankine.
  virtual double GetTemperatureSL(void) const { return SLtemperature; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  virtual double GetTemperatureRatio(void) const { return Temperature*rSLtemperature; }

  /// Gets the temperature deviation at sea-level in degrees Fahrenheit
  virtual double GetSLTempDev(void) const { return T_dev_sl; }

  /// Gets the current delta-T in degrees Fahrenheit
  virtual double GetDeltaT(void) const  { return delta_T; }

  /// Gets the at-altitude temperature deviation in degrees Fahrenheit
  virtual double GetTempDev(void) const { return T_dev; }

  /// Sets the temperature deviation at sea-level in degrees Fahrenheit
  virtual void SetSLTempDev(double d)  { T_dev_sl = d; }

  /// Sets the current delta-T in degrees Fahrenheit
  virtual void SetDeltaT(double d)  { delta_T = d; }

  /// Sets the Sea Level temperature, if it is to be different than the standard.
  /// This function will calculate a bias - a difference - from the standard
  /// atmosphere temperature and will apply that bias to the entire
  /// temperature profile. This is one way to set the temperature bias. Using
  /// the SetTemperatureBias function will replace the value calculated by
  /// this function.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetSLTemperature(double t, eTemperature unit=eFahrenheit);

  /// Sets the temperature at the current flight altitude, if it is to be different
  /// than the standard temperature.
  /// This function will calculate a bias - a difference - from the standard
  /// atmosphere temperature at the current flight altitude and will apply that
  /// calculated bias to the entire temperature profile.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperature(double t, enum unit=eFahrenheit);

  /// Sets the temperature bias to be added to the standard temperature at all altitudes.
  /// This function sets the bias - the difference - from the standard
  /// atmosphere temperature. This bias applies to the entire
  /// temperature profile. Another way to set the temperature bias is to use the
  /// SetSLTemperature function, which replaces the value calculated by
  /// this function with a calculated bias.
  /// @param t the temperature value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperatureBias(double t, enum unit=eFahrenheit);

  /// Sets the Sea Level temperature skew value, to be added to the standard temperature,
  /// ramped out at high altitude.
  /// This function sets the skew - the "distorted" difference - from the standard
  /// atmosphere temperature at sea level. The value of the skew applied to the 
  /// temperature profile is ramped out as altitude increases, becoming 0.0 at 91 km.
  /// The skew can be used along with the bias to tailor the temperature profile as desired.
  /// @param t the temperature skew value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetSLTemperatureSkew(double t, enum unit=eFahrenheit);

  /// Sets the temperature skew value at the current altitude, to be added to
  /// the standard temperature, ramped out at high altitude.
  /// This function computes the sea level skew - the "distorted" difference -
  /// from the standard atmosphere temperature at sea level. The value of the
  /// skew applied to the temperature profile is ramped out as altitude increases,
  /// becoming 0.0 at 91 km. The skew can be used along with the bias to tailor
  /// the temperature profile as desired.
  /// @param t the temperature skew value in the unit provided.
  /// @param unit the unit of the temperature.
  virtual void SetTemperatureSkew(double t, enum unit=eFahrenheit);

  /// This function resets the model to use no bias or skew to the temperature.
  /// The skew and bias values are reset to 0.0, and the standard temperature
  /// is used for the entire temperature profile at all altitudes.
  virtual void ResetSLTemperature();
  //@}

  //  *************************************************************************
  /// @name Pressure access functions.
  //@{
  /// Returns the pressure in psf.
  virtual double GetPressure(void)  const {return Pressure;}

  /// Returns the standard pressure at a specified altitude
  virtual double GetPressure(double altitude) const;

  /// Returns the sea level pressure in psf.
  virtual double GetPressureSL(void) const { return SLpressure; }

  /// Returns the ratio of at-altitude pressure over the sea level value.
  virtual double GetPressureRatio(void) const { return Pressure*rSLpressure; }
  //@}

  //  *************************************************************************
  /// @name Density access functions.
  //@{
  /** Returns the density in slugs/ft^3.
      This function may only be used if Run() is called first. */
  virtual double GetDensity(void)  const {return Density;}

  /// Returns the standard density at a specified altitude
  virtual double GetDensity(double altitude);

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

  /// Gets the density altitude in feet
//  virtual double GetDensityAltitude(void) const { return density_altitude; }

protected:
  double StdSLtemperature, StdSLdensity, StdSLpressure, StdSLsoundspeed; // Standard sea level conditions
  double    SLtemperature,    SLdensity,    SLpressure,    SLsoundspeed; // Sea level conditions
  double      Temperature,      Density,      Pressure,      Soundspeed; // Current actual conditions at altitude
  double   rSLtemperature,   rSLdensity,   rSLpressure,   rSLsoundspeed; // Reciprocal of sea level conditions

  double PressureAltitude;
  double DensityAltitude;

  double delta_T;
  double T_dev_sl;
  double T_dev;

  FGTable* StdAtmosTemperatureTable;
  FGTable* StdAtmosPressureTable;

  const double SutherlandConstant, Beta;
  double Viscosity, KinematicViscosity;

  /// Calculate the atmosphere for the given altitude, including effects of temperature deviation.
  void Calculate(double altitude);

  virtual void bind(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

