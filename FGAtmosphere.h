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
#include "FGMatrix.h"

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
 
[1]    Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0
 
*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAtmosphere : public FGModel {
public:

  FGAtmosphere(FGFDMExec*);
  ~FGAtmosphere(void);
  bool Run(void);

  inline float GetTemperature(void) {return temperature;}
  inline float GetDensity(void)    {return density;}     // use only after Run() has been called
  inline float GetPressure(void)   {return pressure;}
  inline float GetSoundSpeed(void) {return soundspeed;}

  inline float GetTemperatureSL(void) { return SLtemperature; }  //Rankine, altitude in feet
  inline float GetDensitySL(void)     { return SLdensity; }      //slugs/ft^3
  inline float GetPressureSL(void)    { return SLpressure; }     //lbs/ft^2
  inline float GetSoundSpeedSL(void)  { return SLsoundspeed; }   //ft/s

  inline float GetTemperatureRatio(void)  { return temperature/SLtemperature; }
  inline float GetDensityRatio(void) 	  { return density/SLdensity; }
  inline float GetPressureRatio(void)     { return pressure/SLpressure; }
  inline float GetSoundSpeedRatio(void)   { return soundspeed/SLsoundspeed; }

  inline void UseExternal(void)          { useExternal=true;  }
  inline void UseInternal(void)          { useExternal=false; } //this is the default

  inline void SetExTemperature(float t)  { exTemperature=t; }
  inline void SetExDensity(float d)      { exDensity=d; }
  inline void SetExPressure(float p)     { exPressure=p; }

  inline void SetWindNED(float wN, float wE, float wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  inline float GetWindN(void) { return vWindNED(1); }
  inline float GetWindE(void) { return vWindNED(2); }
  inline float GetWindD(void) { return vWindNED(3); }

  inline FGColumnVector GetWindUVW(void) { return vWindUVW; }

protected:

private:
  float rho;

  float h;
  float SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  float temperature,density,pressure,soundspeed;
  bool useExternal;
  float exTemperature,exDensity,exPressure;
  FGColumnVector vWindNED,vWindUVW;

  void Calculate(float altitude);

};



/******************************************************************************/
#endif
