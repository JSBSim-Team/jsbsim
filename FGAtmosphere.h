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

#define ID_ATMOSPHERE "$Id: FGAtmosphere.h,v 1.19 2001/07/29 16:00:20 apeden Exp $"

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
 
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0
 
*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAtmosphere : public FGModel {
public:

  FGAtmosphere(FGFDMExec*);
  ~FGAtmosphere();
  bool Run(void);

  inline float GetTemperature(void) {return temperature;}
  inline float GetDensity(void)    {return density;}     // use only after Run() has been called
  inline float GetPressure(void)   {return pressure;}
  inline float GetSoundSpeed(void) {return soundspeed;}

  inline float GetTemperatureSL(void) { return SLtemperature; }  //Rankine, altitude in feet
  inline float GetDensitySL(void)     { return SLdensity; }      //slugs/ft^3
  inline float GetPressureSL(void)    { return SLpressure; }     //lbs/ft^2
  inline float GetSoundSpeedSL(void)  { return SLsoundspeed; }   //ft/s

  inline float GetTemperatureRatio(void)  { return temperature*rSLtemperature; }
  inline float GetDensityRatio(void) 	  { return density*rSLdensity; }
  inline float GetPressureRatio(void)     { return pressure*rSLpressure; }
  inline float GetSoundSpeedRatio(void)   { return soundspeed*rSLsoundspeed; }

  inline void UseExternal(void)          { useExternal=true;  }
  inline void UseInternal(void)          { useExternal=false; } //this is the default
  bool External(void) { return useExternal; }

  inline void SetExTemperature(float t)  { exTemperature=t; }
  inline void SetExDensity(float d)      { exDensity=d; }
  inline void SetExPressure(float p)     { exPressure=p; }

  inline void SetWindNED(float wN, float wE, float wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  inline FGColumnVector3& GetWindNED(void) { return vWindNED; }
  
  inline float GetWindPsi(void) { return psiw; }
  
private:
  float rho;

  int lastIndex;
  float h;
  float htab[8];
  float SLtemperature,SLdensity,SLpressure,SLsoundspeed;
  float rSLtemperature,rSLdensity,rSLpressure,rSLsoundspeed; //reciprocals
  float temperature,density,pressure,soundspeed;
  bool useExternal;
  float exTemperature,exDensity,exPressure;
  
  FGColumnVector3 vWindNED;
  float psiw;

  void Calculate(float altitude);
  void Debug(void);
};

/******************************************************************************/
#endif

