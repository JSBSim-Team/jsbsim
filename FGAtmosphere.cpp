/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAtmosphere.cpp
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden 
 Date started: 11/24/98
 Purpose:      Models the atmosphere
 Called by:    FGSimExec

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Models the atmosphere. The equation used below was determined by a third order
curve fit using Excel. The data is from the ICAO atmosphere model.

HISTORY
--------------------------------------------------------------------------------
11/24/98   JSB   Created
07/23/99   TP    Added implementation of 1959 Standard Atmosphere
                 Moved calculation of Mach number to FGTranslation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

static const char *IdSrc = "$Id: FGAtmosphere.cpp,v 1.23 2001/11/12 05:06:27 jberndt Exp $";
static const char *IdHdr = ID_ATMOSPHERE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex),
                                               vWindNED(3)
{
  Name = "FGAtmosphere";
  lastIndex=0;
  h = 0;
  htab[0]=0;
  htab[1]=36089.239;
  htab[2]=65616.798;
  htab[3]=104986.878;
  htab[4]=154199.475;
  htab[5]=170603.675;
  htab[6]=200131.234;
  htab[7]=259186.352; //ft.

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAtmosphere::~FGAtmosphere()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGAtmosphere" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::InitModel(void)
{
  FGModel::InitModel();

  Calculate(h);
  SLtemperature = temperature;
  SLpressure    = pressure;
  SLdensity     = density;
  SLsoundspeed  = sqrt(SHRatio*Reng*temperature);
  rSLtemperature = 1.0/temperature;
  rSLpressure    = 1.0/pressure;
  rSLdensity     = 1.0/density;
  rSLsoundspeed  = 1.0/SLsoundspeed;
  useExternal=false;
  
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    //do temp, pressure, and density first
    if (!useExternal) {
      h = Position->Geth();
      Calculate(h);
    } else {
      density = exDensity;
      pressure = exPressure;
      temperature = exTemperature;
    }

    if (vWindNED(1) != 0.0) psiw = atan2( vWindNED(2), vWindNED(1) );

    if (psiw < 0) psiw += 2*M_PI;

    soundspeed = sqrt(SHRatio*Reng*temperature);

    State->Seta(soundspeed);

  } else {                               // skip Run() execution this time
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::Calculate(float altitude)
{
  //see reference [1]

  float slope,reftemp,refpress;
  int i=0; bool lookup = false;
  // cout << "Atmosphere:  h=" << altitude << " rho= " << density << endl;
  i=lastIndex;
  if(altitude < htab[lastIndex]) {
    if (altitude <= 0) { 
      i=0; altitude=0;
    } else {
       i=lastIndex-1;
       while (htab[i] > altitude) { i--; }
    }   
  } else if (altitude > htab[lastIndex+1]){
    if (altitude >= htab[7]){
      i = 7; altitude = htab[7];
    } else {
      i=lastIndex+1;
      while(htab[i+1] < altitude) { i++; } 
    }  
  } 

  switch(i) {
  case 0:     // sea level
    slope     = -0.00356616; // R/ft.
    reftemp   = 518.67;    // R
    refpress  = 2116.22;    // psf
    //refdens   = 0.00237767;  // slugs/cubic ft.
    break;
  case 1:     // 36089 ft.
    slope     = 0;
    reftemp   = 389.97;
    refpress  = 472.452;
    //refdens   = 0.000706032;
    break;
  case 2:     // 65616 ft.
    slope     = 0.00054864;
    reftemp   = 389.97;
    refpress  = 114.636;
    //refdens   = 0.000171306;
    break;
  case 3:     // 104986 ft.
    slope     = 0.00153619;
    reftemp   = 411.57;
    refpress  = 8.36364;
    //refdens   = 1.18422e-05;
    break;
  case 4:     // 154199 ft.
    slope     = 0;
    reftemp   = 487.17;
    refpress  = 0.334882;
    //refdens   = 4.00585e-7;
    break;
  case 5:     // 170603 ft.
    slope     = -0.00109728;
    reftemp   = 487.17;
    refpress  = 0.683084;
    //refdens   = 8.17102e-7;
    break;
  case 6:     // 200131 ft.
    slope     = -0.00219456;
    reftemp   = 454.17;
    refpress  = 0.00684986;
    //refdens   = 8.77702e-9;
    break;
  case 7:     // 259186 ft.
    slope     = 0;
    reftemp   = 325.17;
    refpress  = 0.000122276;
    //refdens   = 2.19541e-10;
    break;
  }
 
  if (slope == 0) {
    temperature = reftemp;
    pressure = refpress*exp(-Inertial->SLgravity()/(reftemp*Reng)*(altitude-htab[i]));
    //density = refdens*exp(-Inertial->SLgravity()/(reftemp*Reng)*(altitude-htab[i]));
    density = pressure/(Reng*temperature);
  } else {
    temperature = reftemp+slope*(altitude-htab[i]);
    pressure = refpress*pow(temperature/reftemp,-Inertial->SLgravity()/(slope*Reng));
    //density = refdens*pow(temperature/reftemp,-(Inertial->SLgravity()/(slope*Reng)+1));
    density = pressure/(Reng*temperature);
  }
  lastIndex=i;
  //cout << "Atmosphere:  h=" << altitude << " rho= " << density << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::Debug(void)
{
    //TODO: Add your source code here
}

