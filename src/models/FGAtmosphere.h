/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden
 Date started: 11/24/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

#ifndef FGATMOSPHERE_H
#define FGATMOSPHERE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "math/FGColumnVector3.h"
#include "math/FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.26 2011/05/20 03:18:36 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the 1976 Standard Atmosphere.
    @author Tony Peden, Jon Berndt
    @version $Id: FGAtmosphere.h,v 1.26 2011/05/20 03:18:36 jberndt Exp $
    @see Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
         1989, ISBN 0-07-001641-0

    Additionally, various turbulence models are available. They are specified
    via the property <tt>atmosphere/turb-type</tt>. The following models are
    available:
    - 0: ttNone (turbulence disabled)
    - 1: ttStandard
    - 2: ttBerndt
    - 3: ttCulp
    - 4: ttMilspec (Dryden spectrum)
    - 5: ttTustin (Dryden spectrum)

    The Milspec and Tustin models are described in the Yeager report cited below.
    They both use a Dryden spectrum model whose parameters (scale lengths and intensities)
    are modelled according to MIL-F-8785C. Parameters are modelled differently
    for altitudes below 1000ft and above 2000ft, for altitudes in between they
    are interpolated linearly.

    The two models differ in the implementation of the transfer functions
    described in the milspec.

    To use one of these two models, set <tt>atmosphere/turb-type</tt> to 4 resp. 5,
    and specify values for <tt>atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps<tt>
    and <tt>atmosphere/turbulence/milspec/severity<tt> (the latter corresponds to
    the probability of exceedence curves from Fig.&nbsp;7 of the milspec, allowable
    range is 0 (disabled) to 7). <tt>atmosphere/psiw-rad</tt> is respected as well;
    note that you have to specify a positive wind magnitude to prevent psiw from
    being reset to zero.

    Reference values (cf. figures 7 and 9 from the milspec):
    <table>
      <tr><td><b>Intensity</b></td>
          <td><b><tt>windspeed_at_20ft_AGL-fps</tt></b></td>
          <td><b><tt>severity</tt></b></td></tr>
      <tr><td>light</td>
          <td>25 (15 knots)</td>
          <td>3</td></tr>
      <tr><td>moderate</td>
          <td>50 (30 knots)</td>
          <td>4</td></tr>
      <tr><td>severe</td>
          <td>75 (45 knots)</td>
          <td>6</td></tr>
    </table>

    @see Yeager, Jessie C.: "Implementation and Testing of Turbulence Models for
         the F18-HARV" (<a
         href="http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19980028448_1998081596.pdf">
         pdf</a>), NASA CR-1998-206937, 1998

    @see MIL-F-8785C: Military Specification: Flying Qualities of Piloted Aircraft

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
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);
  bool InitModel(void);
  enum tType {ttNone, ttStandard, ttCulp, ttMilspec, ttTustin} turbType;

  /// Returns the temperature in degrees Rankine.
  virtual double GetTemperature(void) const {return *temperature;}
  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  virtual double GetDensity(void)  const {return *density;}
  /// Returns the pressure in psf.
  virtual double GetPressure(void)  const {return *pressure;}
  /// Returns the standard pressure at a specified altitude
  virtual double GetPressure(double altitude);
  /// Returns the standard temperature at a specified altitude
  virtual double GetTemperature(double altitude);
  /// Returns the standard density at a specified altitude
  virtual double GetDensity(double altitude);
  /// Returns the speed of sound in ft/sec.
  virtual double GetSoundSpeed(void) const {return soundspeed;}
  /// Returns the absolute viscosity.
  virtual double GetAbsoluteViscosity(void) const {return intViscosity;}
  /// Returns the kinematic viscosity.
  virtual double GetKinematicViscosity(void) const {return intKinematicViscosity;}

  /// Returns the sea level temperature in degrees Rankine.
  virtual double GetTemperatureSL(void) const { return SLtemperature; }
  /// Returns the sea level density in slugs/ft^3
  virtual double GetDensitySL(void)  const { return SLdensity; }
  /// Returns the sea level pressure in psf.
  virtual double GetPressureSL(void) const { return SLpressure; }
  /// Returns the sea level speed of sound in ft/sec.
  virtual double GetSoundSpeedSL(void) const { return SLsoundspeed; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  virtual double GetTemperatureRatio(void) const { return (*temperature)*rSLtemperature; }
  /// Returns the ratio of at-altitude density over the sea level value.
  virtual double GetDensityRatio(void) const { return (*density)*rSLdensity; }
  /// Returns the ratio of at-altitude pressure over the sea level value.
  virtual double GetPressureRatio(void) const { return (*pressure)*rSLpressure; }
  /// Returns the ratio of at-altitude sound speed over the sea level value.
  virtual double GetSoundSpeedRatio(void) const { return soundspeed*rSLsoundspeed; }

  /// Tells the simulator to use an externally calculated atmosphere model.
  virtual void UseExternal(void);
  /// Tells the simulator to use the internal atmosphere model.
  virtual void UseInternal(void);  //this is the default
  /// Gets the boolean that tells if the external atmosphere model is being used.
  virtual bool External(void) { return useExternal; }

  /// Provides the external atmosphere model with an interface to set the temperature.
  virtual void SetExTemperature(double t)  { exTemperature=t; }
  /// Provides the external atmosphere model with an interface to set the density.
  virtual void SetExDensity(double d)      { exDensity=d; }
  /// Provides the external atmosphere model with an interface to set the pressure.
  virtual void SetExPressure(double p)     { exPressure=p; }

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
  virtual double GetDensityAltitude(void) const { return density_altitude; }

  // TOTAL WIND access functions (wind + gust + turbulence)

  /// Retrieves the total wind components in NED frame.
  virtual const FGColumnVector3& GetTotalWindNED(void) const { return vTotalWindNED; }

  /// Retrieves a total wind component in NED frame.
  virtual double GetTotalWindNED(int idx) const {return vTotalWindNED(idx);}

  // WIND access functions

  /// Sets the wind components in NED frame.
  virtual void SetWindNED(double wN, double wE, double wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  /// Sets a wind component in NED frame.
  virtual void SetWindNED(int idx, double wind) { vWindNED(idx)=wind;}

  /// Retrieves the wind components in NED frame.
  virtual FGColumnVector3& GetWindNED(void) { return vWindNED; }

  /// Retrieves a wind component in NED frame.
  virtual double GetWindNED(int idx) const {return vWindNED(idx);}

  /** Retrieves the direction that the wind is coming from.
      The direction is defined as north=0 and increases counterclockwise.
      The wind heading is returned in radians.*/
  virtual double GetWindPsi(void) const { return psiw; }

  /** Sets the direction that the wind is coming from.
      The direction is defined as north=0 and increases counterclockwise to 2*pi (radians). The
      vertical component of wind is assumed to be zero - and is forcibly set to zero. This function
      sets the vWindNED vector components based on the supplied direction. The magnitude of
      the wind set in the vector is preserved (assuming the vertical component is non-zero).
      @param dir wind direction in the horizontal plane, in radians.*/
  virtual void SetWindPsi(double dir);

  virtual void SetWindspeed(double speed);

  virtual double GetWindspeed(void) const;

  // GUST access functions

  /// Sets a gust component in NED frame.
  virtual void SetGustNED(int idx, double gust) { vGustNED(idx)=gust;}

  /// Sets a turbulence component in NED frame.
  virtual void SetTurbNED(int idx, double turb) { vTurbulenceNED(idx)=turb;}

  /// Sets the gust components in NED frame.
  virtual void SetGustNED(double gN, double gE, double gD) { vGustNED(eNorth)=gN; vGustNED(eEast)=gE; vGustNED(eDown)=gD;}

  /// Retrieves a gust component in NED frame.
  virtual double GetGustNED(int idx) const {return vGustNED(idx);}

  /// Retrieves a turbulence component in NED frame.
  virtual double GetTurbNED(int idx) const {return vTurbulenceNED(idx);}

  /// Retrieves the gust components in NED frame.
  virtual FGColumnVector3& GetGustNED(void) {return vGustNED;}

  /** Turbulence models available: ttNone, ttStandard, ttBerndt, ttCulp, ttMilspec, ttTustin */
  virtual void   SetTurbType(tType tt) {turbType = tt;}
  virtual tType  GetTurbType() const {return turbType;}

  virtual void   SetTurbGain(double tg) {TurbGain = tg;}
  virtual double GetTurbGain() const {return TurbGain;}

  virtual void   SetTurbRate(double tr) {TurbRate = tr;}
  virtual double GetTurbRate() const {return TurbRate;}

  virtual void   SetRhythmicity(double r) {Rhythmicity=r;}
  virtual double GetRhythmicity() const {return Rhythmicity;}

  virtual double GetTurbPQR(int idx) const {return vTurbPQR(idx);}
  virtual double GetTurbMagnitude(void) const {return Magnitude;}
  virtual const FGColumnVector3& GetTurbDirection(void) const {return vDirection;}
  virtual const FGColumnVector3& GetTurbPQR(void) const {return vTurbPQR;}

  virtual void   SetWindspeed20ft(double ws) { windspeed_at_20ft = ws;}
  virtual double GetWindspeed20ft() const { return windspeed_at_20ft;}

  /// allowable range: 0-7, 3=light, 4=moderate, 6=severe turbulence
  virtual void   SetProbabilityOfExceedence( int idx) {probability_of_exceedence_index = idx;}
  virtual int    GetProbabilityOfExceedence() const { return probability_of_exceedence_index;}

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
  double SutherlandConstant, Beta, intViscosity, intKinematicViscosity;
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
  FGColumnVector3 vTurbulenceGrad;
  FGColumnVector3 vBodyTurbGrad;
  FGColumnVector3 vTurbPQR;

  // Dryden turbulence model
  double windspeed_at_20ft; ///< in ft/s
  int probability_of_exceedence_index; ///< this is bound as the severity property
  FGTable *POE_Table; ///< probability of exceedence table

  double psiw;
  FGColumnVector3 vTotalWindNED;
  FGColumnVector3 vWindNED;
  FGColumnVector3 vGustNED;
  FGColumnVector3 vTurbulenceNED;

  /// Calculate the atmosphere for the given altitude, including effects of temperature deviation.
  void Calculate(double altitude);
  /// Calculate atmospheric properties other than the basic T, P and rho.
  void CalculateDerived(void);
  /// Get T, P and rho for a standard atmosphere at the given altitude.
  void GetStdAtmosphere(double altitude);
  void Turbulence(void);
  virtual void bind(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

