/*******************************************************************************
 
 Header:       FGAtmosphere.h
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden
 Date started: 11/24/98
 
 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.
 
 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.
 
 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.
 
HISTORY
--------------------------------------------------------------------------------
11/24/98   JSB   Created
07/23/99   TP   Added implementation of 1959 Standard Atmosphere
           Moved calculation of Mach number to FGTranslation
 
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAtmosphere_H
#define FGAtmosphere_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.24 2001/11/21 23:47:29 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the standard atmosphere.
    @author Tony Peden, Jon Berndt
    @version $Id: FGAtmosphere.h,v 1.24 2001/11/21 23:47:29 jberndt Exp $
*/

/******************************************************************************
CLASS DECLARATION
*******************************************************************************/

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

  /// Returns the temperature in degrees Rankine.
  inline double GetTemperature(void) {return temperature;}
  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  inline double GetDensity(void)    {return density;}
  /// Returns the pressure in psf.
  inline double GetPressure(void)   {return pressure;}
  /// Returns the speed of sound in ft/sec.
  inline double GetSoundSpeed(void) {return soundspeed;}

  /// Returns the sea level temperature in degrees Rankine.
  inline double GetTemperatureSL(void) { return SLtemperature; }
  /// Returns the sea level density in slugs/ft^3
  inline double GetDensitySL(void)     { return SLdensity; }
  /// Returns the sea level pressure in psf.
  inline double GetPressureSL(void)    { return SLpressure; }
  /// Returns the sea level speed of sound in ft/sec.
  inline double GetSoundSpeedSL(void)  { return SLsoundspeed; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  inline double GetTemperatureRatio(void)  { return temperature*rSLtemperature; }
  /// Returns the ratio of at-altitude density over the sea level value.
  inline double GetDensityRatio(void) 	  { return density*rSLdensity; }
  /// Returns the ratio of at-altitude pressure over the sea level value.
  inline double GetPressureRatio(void)     { return pressure*rSLpressure; }
  /// Returns the ratio of at-altitude sound speed over the sea level value.
  inline double GetSoundSpeedRatio(void)   { return soundspeed*rSLsoundspeed; }

  /// Tells the simulator to use an externally calculated atmosphere model.
  inline void UseExternal(void)          { useExternal=true;  }
  /// Tells the simulator to use the internal atmosphere model.
  inline void UseInternal(void)          { useExternal=false; } //this is the default
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
  inline double GetWindPsi(void) { return psiw; }
  
  inline void SetTurbGain(double tt) {TurbGain = tt;}
  
private:
  double rho;

  enum tType {ttBerndt, ttNone} turbType;

  int lastIndex;
  double h;
  double htab[8];
  double SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  double rSLtemperature,rSLdensity,rSLpressure,rSLsoundspeed; //reciprocals
  double temperature,density,pressure,soundspeed;
  bool useExternal;
  double exTemperature,exDensity,exPressure;
  
  double MagnitudedAccelDt, MagnitudeAccel, Magnitude;
  double TurbGain;
  FGColumnVector3 vDirectiondAccelDt;
  FGColumnVector3 vDirectionAccel;
  FGColumnVector3 vDirection;
  FGColumnVector3 vTurbulence;
  FGColumnVector3 vTurbulenceGrad;

  FGColumnVector3 vWindNED;
  double psiw;

  void Calculate(double altitude);
  void Turbulence(void);
  void Debug(void);
};

/******************************************************************************/
#endif

