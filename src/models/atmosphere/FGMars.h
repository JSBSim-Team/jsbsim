/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       FGMars.h
 Author:       Jon Berndt
 Date started: 01/05/2004
 
 ------------- Copyright (C) 2004  Jon S. Berndt (jsb@hal-pc.org) -------------
 
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
01/05/2004   JSB   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMars_H
#define FGMars_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <models/FGAtmosphere.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MARS "$Id: FGMars.h,v 1.5 2006/08/30 12:04:35 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the Martian atmosphere.
    @author Jon Berndt
    @version $Id: FGMars.h,v 1.5 2006/08/30 12:04:35 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMars : public FGAtmosphere {
public:

  /// Constructor
  FGMars(FGFDMExec*);
  /// Destructor
  //~FGMars();
  /** Runs the Martian atmosphere model; called by the Executive
      @return false if no error */
  bool Run(void);

  bool InitModel(void);

  /// Returns the temperature in degrees Rankine.
  inline double GetTemperature(void) const {return *temperature;}
  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  inline double GetDensity(void)  const {return *density;}
  /// Returns the pressure in psf.
  inline double GetPressure(void)  const {return *pressure;}
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

  /// Sets the wind components in NED frame.
  inline void SetWindNED(double wN, double wE, double wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  /// Retrieves the wind components in NED frame.
  inline FGColumnVector3& GetWindNED(void) { return vWindNED; }
  
  /** Retrieves the wind direction. The direction is defined as north=0 and
      increases counterclockwise. The wind heading is returned in radians.*/
  inline double GetWindPsi(void) const { return psiw; }
  
  inline void SetTurbGain(double tt) {TurbGain = tt;}
  inline void SetTurbRate(double tt) {TurbRate = tt;}
  
  inline double GetTurbPQR(int idx) const {return vTurbPQR(idx);}
  inline FGColumnVector3& GetTurbPQR(void) {return vTurbPQR;}
  
  //void bind(void);
  void unbind(void);

  
private:
  double rho;

  enum tType {ttStandard, ttBerndt, ttNone} turbType;

  int lastIndex;
  double h;
  double htab[8];
  double SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  double rSLtemperature,rSLdensity,rSLpressure,rSLsoundspeed; //reciprocals
  double *temperature,*density,*pressure;
  double soundspeed;
  bool useExternal;
  double exTemperature,exDensity,exPressure;
  double intTemperature, intDensity, intPressure;
  
  double MagnitudedAccelDt, MagnitudeAccel, Magnitude;
  double TurbGain;
  double TurbRate;
  FGColumnVector3 vDirectiondAccelDt;
  FGColumnVector3 vDirectionAccel;
  FGColumnVector3 vDirection;
  FGColumnVector3 vTurbulence;
  FGColumnVector3 vTurbulenceGrad;
  FGColumnVector3 vBodyTurbGrad;
  FGColumnVector3 vTurbPQR;

  FGColumnVector3 vWindNED;
  double psiw;

  void Calculate(double altitude);
  void Turbulence(void);
  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

