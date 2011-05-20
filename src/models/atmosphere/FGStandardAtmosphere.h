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

#define ID_STANDARDATMOSPHERE "$Id: FGStandardAtmosphere.h,v 1.1 2011/05/20 03:10:00 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the 1976 Standard Atmosphere.
    @author Jon Berndt
    @version $Id: FGStandardAtmosphere.h,v 1.1 2011/05/20 03:10:00 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGStandardAtmosphere : public FGModel {
public:

  /// Constructor
  FGStandardAtmosphere(FGFDMExec*);
  /// Destructor
  ~FGStandardAtmosphere();
  /** Runs the standard atmosphere forces model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);
  bool InitModel(void);

  /// Returns the temperature in degrees Rankine.
  virtual double GetTemperature(void) const {return Temperature;}

  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  virtual double GetDensity(void)  const {return Density;}

  /// Returns the pressure in psf.
  virtual double GetPressure(void)  const {return Pressure;}

  /// Returns the standard pressure at a specified altitude
  virtual double GetPressure(double altitude);

  /// Returns the standard temperature at a specified altitude
  virtual double GetTemperature(double altitude);

  /// Returns the standard density at a specified altitude
  virtual double GetDensity(double altitude);

  /// Returns the speed of sound in ft/sec.
  virtual double GetSoundSpeed(void) const {return Soundspeed;}

  /// Returns the absolute viscosity.
  virtual double GetAbsoluteViscosity(void) const {return Viscosity;}

  /// Returns the kinematic viscosity.
  virtual double GetKinematicViscosity(void) const {return KinematicViscosity;}

  /// Returns the sea level temperature in degrees Rankine.
  virtual double GetTemperatureSL(void) const { return SLtemperature; }

  /// Returns the sea level density in slugs/ft^3
  virtual double GetDensitySL(void)  const { return SLdensity; }

  /// Returns the sea level pressure in psf.
  virtual double GetPressureSL(void) const { return SLpressure; }

  /// Returns the sea level speed of sound in ft/sec.
  virtual double GetSoundSpeedSL(void) const { return SLsoundspeed; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  virtual double GetTemperatureRatio(void) const { return Temperature*rSLtemperature; }

  /// Returns the ratio of at-altitude density over the sea level value.
  virtual double GetDensityRatio(void) const { return Density*rSLdensity; }

  /// Returns the ratio of at-altitude pressure over the sea level value.
  virtual double GetPressureRatio(void) const { return Pressure*rSLpressure; }

  /// Returns the ratio of at-altitude sound speed over the sea level value.
  virtual double GetSoundSpeedRatio(void) const { return Soundspeed*rSLsoundspeed; }

  /// Sets the temperature deviation at sea-level in degrees Fahrenheit
  virtual void SetSLTempDev(double d)  { T_dev_sl = d; }

  /// Gets the temperature deviation at sea-level in degrees Fahrenheit
  virtual double GetSLTempDev(void) const { return T_dev_sl; }

  /// Sets the current delta-T in degrees Fahrenheit
  virtual void SetDeltaT(double d)  { delta_T = d; }

  /// Gets the current delta-T in degrees Fahrenheit
  virtual double GetDeltaT(void) const  { return delta_T; }

  /// Gets the at-altitude temperature deviation in degrees Fahrenheit
  virtual double GetTempDev(void) const { return T_dev; }

  /// Gets the density altitude in feet
//  virtual double GetDensityAltitude(void) const { return density_altitude; }

protected:
  double h;
  double StdSLtemperature, StdSLdensity, StdSLpressure, StdSLsoundspeed; // Standard sea level conditions
  double    SLtemperature,    SLdensity,    SLpressure,    SLsoundspeed; // Sea level conditions
  double      Temperature,      Density,      Pressure,      Soundspeed; // Current actual conditions at altitude
  double   rSLtemperature,   rSLdensity,   rSLpressure,   rSLsoundspeed; // Reciprocal of sea level conditions

  double delta_T;
  double T_dev_sl;
  double T_dev;

  FGTable* StdAtmosTemperatureTable;
  FGTable* StdAtmosPressureTable;

  const double SutherlandConstant, Beta;
  double Viscosity, KinematicViscosity;

  /// Calculate the atmosphere for the given altitude, including effects of temperature deviation.
  void Calculate(double altitude);

  /// Calculate atmospheric properties other than the basic T, P and rho.
  void CalculateDerived(void);

  virtual void bind(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

