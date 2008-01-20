/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden
 Date started: 11/24/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
11/24/98   JSB   Created
07/23/99   TP    Added implementation of 1959 Standard Atmosphere
                 Moved calculation of Mach number to FGPropagate
                 Updated to '76 model

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAtmosphere_H
#define FGAtmosphere_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include <math/FGColumnVector3.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.13 2008/01/20 17:46:49 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the standard atmosphere.
    @author Tony Peden, Jon Berndt
    @version $Id: FGAtmosphere.h,v 1.13 2008/01/20 17:46:49 jberndt Exp $
    @see Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
         1989, ISBN 0-07-001641-0
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAtmosphere : public FGModel {
public:

  /// Constructor
  FGAtmosphere(FGFDMExec*);
  /// Destructor
  ~FGAtmosphere();
  /** Runs the Atmosphere model; called by the Executive
      @return false if no error */
  bool Run(void);
  bool InitModel(void);
  enum tType {ttStandard, ttBerndt, ttCulp, ttNone} turbType;

  /// Returns the temperature in degrees Rankine.
  inline double GetTemperature(void) const {return *temperature;}
  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  inline double GetDensity(void)  const {return *density;}
  /// Returns the pressure in psf.
  double GetPressure(void)  const {return *pressure;}
  /// Returns the standard pressure at a specified altitude
  double GetPressure(double altitude);
  /// Returns the standard temperature at a specified altitude
  double GetTemperature(double altitude);
  /// Returns the standard density at a specified altitude
  double GetDensity(double altitude);
  /// Returns the speed of sound in ft/sec.
  inline double GetSoundSpeed(void) const {return soundspeed;}

  /// Returns the sea level temperature in degrees Rankine.
  inline double GetTemperatureSL(void) const { return SLtemperature; }
  /// Returns the sea level density in slugs/ft^3
  inline double GetDensitySL(void)  const { return SLdensity; }
  /// Returns the sea level pressure in psf.
  inline double GetPressureSL(void) const { return SLpressure; }
  /// Returns the sea level speed of sound in ft/sec.
  inline double GetSoundSpeedSL(void) const { return SLsoundspeed; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  inline double GetTemperatureRatio(void) const { return (*temperature)*rSLtemperature; }
  /// Returns the ratio of at-altitude density over the sea level value.
  inline double GetDensityRatio(void) const { return (*density)*rSLdensity; }
  /// Returns the ratio of at-altitude pressure over the sea level value.
  inline double GetPressureRatio(void) const { return (*pressure)*rSLpressure; }
  /// Returns the ratio of at-altitude sound speed over the sea level value.
  inline double GetSoundSpeedRatio(void) const { return soundspeed*rSLsoundspeed; }

  /// Tells the simulator to use an externally calculated atmosphere model.
  void UseExternal(void);
  /// Tells the simulator to use the internal atmosphere model.
  void UseInternal(void);  //this is the default
  /// Gets the boolean that tells if the external atmosphere model is being used.
  bool External(void) { return useExternal; }

  /// Provides the external atmosphere model with an interface to set the temperature.
  inline void SetExTemperature(double t)  { exTemperature=t; }
  /// Provides the external atmosphere model with an interface to set the density.
  inline void SetExDensity(double d)      { exDensity=d; }
  /// Provides the external atmosphere model with an interface to set the pressure.
  inline void SetExPressure(double p)     { exPressure=p; }

  /// Sets the temperature deviation at sea-level in degrees Fahrenheit
  inline void SetSLTempDev(double d)  { T_dev_sl = d; }
  /// Gets the temperature deviation at sea-level in degrees Fahrenheit
  inline double GetSLTempDev(void) const { return T_dev_sl; }
  /// Sets the current delta-T in degrees Fahrenheit
  inline void SetDeltaT(double d)  { delta_T = d; }
  /// Gets the current delta-T in degrees Fahrenheit
  inline double GetDeltaT(void) const  { return delta_T; }
  /// Gets the at-altitude temperature deviation in degrees Fahrenheit
  inline double GetTempDev(void) const { return T_dev; }
  /// Gets the density altitude in feet
  inline double GetDensityAltitude(void) const { return density_altitude; }

  /// Sets the wind components in NED frame.
  inline void SetWindNED(double wN, double wE, double wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  /// Retrieves the wind components in NED frame.
  inline FGColumnVector3& GetWindNED(void) { return vWindNED; }

  /// Sets gust components in NED frame.
  inline void SetGustNED(int idx, double gust) { vGustNED(idx)=gust; cout << vGustNED(idx) << endl; }

  /// Retrieves the gust components in NED frame.
  inline double GetGustNED(int idx) const {return vGustNED(idx);}

  /// Retrieves the gust components in NED frame.
  inline FGColumnVector3& GetGustNED(void) {return vGustNED;}

  /** Retrieves the wind direction. The direction is defined as north=0 and
      increases counterclockwise. The wind heading is returned in radians.*/
  inline double GetWindPsi(void) const { return psiw; }

  inline void   SetTurbType(tType tt) {turbType = tt;}
  inline tType  GetTurbType() const {return turbType;}

  inline void   SetTurbGain(double tg) {TurbGain = tg;}
  inline double GetTurbGain() const {return TurbGain;}

  inline void   SetTurbRate(double tr) {TurbRate = tr;}
  inline double GetTurbRate() const {return TurbRate;}

  inline void   SetRhythmicity(double r) {Rhythmicity=r;}
  inline double GetRhythmicity() const {return Rhythmicity;}

  /** Sets wind vortex, clockwise as seen from a point in front of aircraft,
      looking aft. Units are radians/second. */
  inline void   SetWindFromClockwise(double wC) { wind_from_clockwise=wC; }
  inline double GetWindFromClockwise(void) const {return wind_from_clockwise;}

  inline double GetTurbPQR(int idx) const {return vTurbPQR(idx);}
  double GetTurbMagnitude(void) const {return Magnitude;}
  FGColumnVector3& GetTurbDirection(void) {return vDirection;}
  inline FGColumnVector3& GetTurbPQR(void) {return vTurbPQR;}

  void bind(void);
  void unbind(void);


protected:
  double rho;

  struct atmType {double Temperature; double Pressure; double Density;};
  int lastIndex;
  double h;
  double htab[8];
  double StdSLtemperature,StdSLdensity,StdSLpressure,StdSLsoundspeed;
  double rSLtemperature,rSLdensity,rSLpressure,rSLsoundspeed; //reciprocals
  double SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  double *temperature, *density, *pressure;
  double soundspeed;
  bool useExternal;
  double exTemperature,exDensity,exPressure;
  double intTemperature, intDensity, intPressure;
  double T_dev_sl, T_dev, delta_T, density_altitude;
  atmType atmosphere;
  bool StandardTempOnly;
  bool first_pass;

  double MagnitudedAccelDt, MagnitudeAccel, Magnitude;
  double TurbGain;
  double TurbRate;
  double Rhythmicity;
  double wind_from_clockwise;
  double spike, target_time, strength;
  FGColumnVector3 vDirectiondAccelDt;
  FGColumnVector3 vDirectionAccel;
  FGColumnVector3 vDirection;
  FGColumnVector3 vTurbulence;
  FGColumnVector3 vTurbulenceGrad;
  FGColumnVector3 vBodyTurbGrad;
  FGColumnVector3 vTurbPQR;

  FGColumnVector3 vWindNED;
  double psiw;

  FGColumnVector3 vGustNED;
  bool bgustSet;

  /// Calculate the atmosphere for the given altitude, including effects of temperature deviation.
  void Calculate(double altitude);
  /// Calculate atmospheric properties other than the basic T, P and rho.
  void CalculateDerived(void);
  /// Get T, P and rho for a standard atmosphere at the given altitude.
  void GetStdAtmosphere(double altitude);
  void Turbulence(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

