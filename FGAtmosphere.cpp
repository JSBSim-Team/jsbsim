/*******************************************************************************

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
********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

********************************************************************************
INCLUDES
*******************************************************************************/

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
#include "FGDefs.h"
#include "FGMatrix.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGAtmosphere.cpp,v 1.8 2000/10/13 19:21:02 jsb Exp $";
static const char *IdHdr = ID_ATMOSPHERE;

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex),
                                                    vWindNED(3),
                                                    vWindUVW(3)
{
    Name = "FGAtmosphere";
    h = 0;
    Calculate(h);
    SLtemperature = temperature;
    SLpressure    = pressure;
    SLdensity     = density;
    SLsoundspeed  = sqrt(SHRATIO*Reng*temperature);
    useExternal=false;
    vWindUVW(1)=0;vWindUVW(2)=0;vWindUVW(3)=0;
    vWindNED(1)=0;vWindNED(2)=0;vWindNED(3)=0;
}


FGAtmosphere::~FGAtmosphere()
{
}


bool FGAtmosphere::Run(void)
{
    //cout << "In FGAtmosphere::Run(void)" << endl;
    if (!FGModel::Run()) {                 // if false then execute this Run()
        //do temp, pressure, and density first
        if (!useExternal) {
            //cout << "Atmosphere: Using internal model, altitude= ";
            h = Position->Geth();

            Calculate(h);
        } else {
            density = exDensity;
            pressure = exPressure;
            temperature = exTemperature;
            //switch sign of wind components so that they are
            //in aircraft reference frame.  The classic example is
            //takeoff or landing where you always want to fly
            //into the wind.  Suppose that an aircraft is
            //taking off into the wind on the runway heading
            //of pure north.  Into the wind means the wind is
            //flowing to the south (or negative) direction,
            //and we know that headwinds increase the relative
            //velocity, so to make a positive delta U from the
            //southerly wind the sign must be switched.
            vWindNED *= -1;
            vWindUVW  = State->GetTl2b()*vWindNED;
        }
        soundspeed = sqrt(SHRATIO*Reng*temperature);
        //cout << "Atmosphere: soundspeed: " << soundspeed << endl;
        State->Seta(soundspeed);


    } else {                               // skip Run() execution this time
    }
    return false;
}


void FGAtmosphere::Calculate(float altitude)
{
    //see reference [1]

    float slope,reftemp,refpress,refdens;
    int i=0;
    float htab[]={0,36089,82020,154198,173882,259183,295272,344484}; //ft.
    // cout << "Atmosphere:  h=" << altitude << " rho= " << density << endl;
    if (altitude <= htab[0]) {
        altitude=0;
    } else if (altitude >= htab[7]){
        i = 7;
        altitude = htab[7];
    } else {
        while (htab[i+1] < altitude) {
            i++;
        }
    }

    switch(i) {
    case 0:     // sea level
        slope     = -0.0035662; // R/ft.
        reftemp   = 518.688;    // R
        refpress  = 2116.17;    // psf
        refdens   = 0.0023765;  // slugs/cubic ft.
        break;
    case 1:     // 36089 ft.
        slope     = 0;
        reftemp   = 389.988;
        refpress  = 474.1;
        refdens   = 0.0007078;
        break;
    case 2:     // 82020 ft.
        slope     = 0.00164594;
        reftemp   = 389.988;
        refpress  = 52.7838;
        refdens   = 7.8849E-5;
        break;
    case 3:     // 154198 ft.
        slope     = 0;
        reftemp   = 508.788;
        refpress  = 2.62274;
        refdens   = 3.01379E-6;
        break;
    case 4:     // 173882 ft.
        slope     = -0.00246891;
        reftemp   = 508.788;
        refpress  = 1.28428;
        refdens   = 1.47035e-06;
        break;
    case 5:     // 259183 ft.
        slope     = 0;
        reftemp   = 298.188;
        refpress  = 0.0222008;
        refdens   = 4.33396e-08;
        break;
    case 6:     // 295272 ft.
        slope     = 0.00219459;
        reftemp   = 298.188;
        refpress  = 0.00215742;
        refdens   = 4.21368e-09;
        break;
    case 7:     // 344484 ft.
        slope     = 0;
        reftemp   = 406.188;
        refpress  = 0.000153755;
        refdens   = 2.20384e-10;
        break;
    }


    if (slope == 0) {
        temperature = reftemp;
        pressure = refpress*exp(-GRAVITY/(reftemp*Reng)*(altitude-htab[i]));
        density = refdens*exp(-GRAVITY/(reftemp*Reng)*(altitude-htab[i]));
    } else {
        temperature = reftemp+slope*(altitude-htab[i]);
        pressure = refpress*pow(temperature/reftemp,-GRAVITY/(slope*Reng));
        density = refdens*pow(temperature/reftemp,-(GRAVITY/(slope*Reng)+1));
    }

    //cout << "Atmosphere:  h=" << altitude << " rho= " << density << endl;

}




