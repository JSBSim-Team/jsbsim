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

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.21 2001/10/31 12:35:58 apeden Exp $"

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
    @version $Id: FGAtmosphere.h,v 1.21 2001/10/31 12:35:58 apeden Exp $
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

  /// Returns the temperature in degrees Rankine.
  inline float GetTemperature(void) {return temperature;}
  /** Returns the density in slugs/ft^3.
      <i>This function may <b>only</b> be used if Run() is called first.</i> */
  inline float GetDensity(void)    {return density;}
  /// Returns the pressure in psf.
  inline float GetPressure(void)   {return pressure;}
  /// Returns the speed of sound in ft/sec.
  inline float GetSoundSpeed(void) {return soundspeed;}

  /// Returns the sea level temperature in degrees Rankine.
  inline float GetTemperatureSL(void) { return SLtemperature; }
  /// Returns the sea level density in slugs/ft^3
  inline float GetDensitySL(void)     { return SLdensity; }
  /// Returns the sea level pressure in psf.
  inline float GetPressureSL(void)    { return SLpressure; }
  /// Returns the sea level speed of sound in ft/sec.
  inline float GetSoundSpeedSL(void)  { return SLsoundspeed; }

  /// Returns the ratio of at-altitude temperature over the sea level value.
  inline float GetTemperatureRatio(void)  { return temperature*rSLtemperature; }
  /// Returns the ratio of at-altitude density over the sea level value.
  inline float GetDensityRatio(void) 	  { return density*rSLdensity; }
  /// Returns the ratio of at-altitude pressure over the sea level value.
  inline float GetPressureRatio(void)     { return pressure*rSLpressure; }
  /// Returns the ratio of at-altitude sound speed over the sea level value.
  inline float GetSoundSpeedRatio(void)   { return soundspeed*rSLsoundspeed; }

  /// Tells the simulator to use an externally calculated atmosphere model.
  inline void UseExternal(void)          { useExternal=true;  }
  /// Tells the simulator to use the internal atmosphere model.
  inline void UseInternal(void)          { useExternal=false; } //this is the default
  /// Gets the boolean that tells if the external atmosphere model is being used.
  bool External(void) { return useExternal; }

  /// Provides the external atmosphere model with an interface to set the temperature.
  inline void SetExTemperature(float t)  { exTemperature=t; }
  /// Provides the external atmosphere model with an interface to set the density.
  inline void SetExDensity(float d)      { exDensity=d; }
  /// Provides the external atmosphere model with an interface to set the pressure.
  inline void SetExPressure(float p)     { exPressure=p; }

  /// Sets the wind components in NED frame.
  inline void SetWindNED(float wN, float wE, float wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  /// Retrieves the wind components in NED frame.
  inline FGColumnVector3& GetWindNED(void) { return vWindNED; }
  
  /** Retrieves the wind direction. The direction is defined as north=0 and
      increases counterclockwise. The wind heading is returned in radians.*/
  inline float GetWindPsi(void) { return psiw; }
  
private:
  double rho;

  int lastIndex;
  double h;
  double htab[8];
  double SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  double rSLtemperature,rSLdensity,rSLpressure,rSLsoundspeed; //reciprocals
  double temperature,density,pressure,soundspeed;
  bool useExternal;
  double exTemperature,exDensity,exPressure;
  
  FGColumnVector3 vWindNED;
  double psiw;

  void Calculate(float altitude);
  void Debug(void);
};

/******************************************************************************/
#endif

