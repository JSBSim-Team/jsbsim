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
                 Later updated to '76 model
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
#include "FGPropertyManager.h"

static const char *IdSrc = "$Id: FGAtmosphere.cpp,v 1.43 2002/07/26 04:49:06 jberndt Exp $";
static const char *IdHdr = ID_ATMOSPHERE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAtmosphere";
  lastIndex = 0;
  h = 0.0;
  psiw = 0.0;
  htab[0]=0;
  htab[1]=36089.239;
  htab[2]=65616.798;
  htab[3]=104986.878;
  htab[4]=154199.475;
  htab[5]=170603.675;
  htab[6]=200131.234;
  htab[7]=259186.352; //ft.

  MagnitudedAccelDt = MagnitudeAccel = Magnitude = 0.0;
  turbType = ttNone;
//  turbType = ttBerndt; // temporarily disable turbulence until fully tested
  TurbGain = 100.0;
  
  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAtmosphere::~FGAtmosphere()
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::InitModel(void)
{
  FGModel::InitModel();

  Calculate(h);
  SLtemperature = intTemperature;
  SLpressure    = intPressure;
  SLdensity     = intDensity;
  SLsoundspeed  = sqrt(SHRatio*Reng*intTemperature);
  rSLtemperature = 1.0/intTemperature;
  rSLpressure    = 1.0/intPressure;
  rSLdensity     = 1.0/intDensity;
  rSLsoundspeed  = 1.0/SLsoundspeed;
  temperature=&intTemperature;
  pressure=&intPressure;
  density=&intDensity;
  
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
    } 

    if (turbType != ttNone) {
      Turbulence();
      vWindNED += vTurbulence;
    }

    if (vWindNED(1) != 0.0) psiw = atan2( vWindNED(2), vWindNED(1) );

    if (psiw < 0) psiw += 2*M_PI;

    soundspeed = sqrt(SHRatio*Reng*(*temperature));

    State->Seta(soundspeed);

    Debug(2);

	return false;
  } else {                               // skip Run() execution this time
	return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// See reference 1

void FGAtmosphere::Calculate(double altitude)
{
  double slope, reftemp, refpress;
  int i = 0;

  i = lastIndex;
  if (altitude < htab[lastIndex]) {
    if (altitude <= 0) { 
      i = 0;
      altitude=0;
    } else {
       i = lastIndex-1;
       while (htab[i] > altitude) i--;
    }   
  } else if (altitude > htab[lastIndex+1]) {
    if (altitude >= htab[7]) {
      i = 7;
      altitude = htab[7];
    } else {
      i = lastIndex+1;
      while (htab[i+1] < altitude) i++;
    }  
  } 

  switch(i) {
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
  case 0:
  default:     // sea level
    slope     = -0.00356616; // R/ft.
    reftemp   = 518.67;    // R
    refpress  = 2116.22;    // psf
    //refdens   = 0.00237767;  // slugs/cubic ft.
    break;
  
  }
 
  if (slope == 0) {
    intTemperature = reftemp;
    intPressure = refpress*exp(-Inertial->SLgravity()/(reftemp*Reng)*(altitude-htab[i]));
    //intDensity = refdens*exp(-Inertial->SLgravity()/(reftemp*Reng)*(altitude-htab[i]));
    intDensity = intPressure/(Reng*intTemperature);
  } else {
    intTemperature = reftemp+slope*(altitude-htab[i]);
    intPressure = refpress*pow(intTemperature/reftemp,-Inertial->SLgravity()/(slope*Reng));
    //intDensity = refdens*pow(intTemperature/reftemp,-(Inertial->SLgravity()/(slope*Reng)+1));
    intDensity = intPressure/(Reng*intTemperature);
  }
  lastIndex=i;
  //cout << "Atmosphere:  h=" << altitude << " rho= " << intDensity << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::Turbulence(void)
{
  switch (turbType) {
  case ttBerndt:
    vDirectiondAccelDt(eX) = 1 - 2.0*(((double)(rand()))/RAND_MAX);
    vDirectiondAccelDt(eY) = 1 - 2.0*(((double)(rand()))/RAND_MAX);
    vDirectiondAccelDt(eZ) = 1 - 2.0*(((double)(rand()))/RAND_MAX);

    MagnitudedAccelDt = 1 - 2.0*(((double)(rand()))/RAND_MAX);
    MagnitudeAccel    += MagnitudedAccelDt*rate*State->Getdt();
    Magnitude         += MagnitudeAccel*rate*State->Getdt();

    vDirectiondAccelDt.Normalize();
    vDirectionAccel += vDirectiondAccelDt*rate*State->Getdt();
    vDirectionAccel.Normalize();
    vDirection      += vDirectionAccel*rate*State->Getdt();
    vDirection.Normalize();
    
    vTurbulence = TurbGain*Magnitude * vDirection;
    vTurbulenceGrad = TurbGain*MagnitudeAccel * vDirection;

    vBodyTurbGrad = State->GetTl2b()*vTurbulenceGrad;
    vTurbPQR(eP) = vBodyTurbGrad(eY)/Aircraft->GetWingSpan();
    if (Aircraft->GetHTailArm() != 0.0)
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/Aircraft->GetHTailArm();
    else
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/10.0;

    if (Aircraft->GetVTailArm())
      vTurbPQR(eR) = vBodyTurbGrad(eX)/Aircraft->GetVTailArm();
    else
      vTurbPQR(eR) = vBodyTurbGrad(eX)/10.0;

    break;
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::UseExternal(void) {
  temperature=&exTemperature;
  pressure=&exPressure;
  density=&exDensity;
  useExternal=true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::UseInternal(void) {
  temperature=&intTemperature;
  pressure=&intPressure;
  density=&intDensity;
  useExternal=false;
}
  

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::bind(void)
{
  typedef double (FGAtmosphere::*PMF)(int) const;
  PropertyManager->Tie("atmosphere/T-R", this,
                       &FGAtmosphere::GetTemperature);
  PropertyManager->Tie("atmosphere/rho-slugs_ft3", this,
                       &FGAtmosphere::GetDensity);
  PropertyManager->Tie("atmosphere/P-psf", this,
                       &FGAtmosphere::GetPressure);
  PropertyManager->Tie("atmosphere/a-fps", this,
                       &FGAtmosphere::GetSoundSpeed);
  PropertyManager->Tie("atmosphere/T-sl-R", this,
                       &FGAtmosphere::GetTemperatureSL);
  PropertyManager->Tie("atmosphere/rho-sl-slugs_ft3", this,
                       &FGAtmosphere::GetDensitySL);
  PropertyManager->Tie("atmosphere/P-sl-psf", this,
                       &FGAtmosphere::GetPressureSL);
  PropertyManager->Tie("atmosphere/a-sl-fps", this,
                       &FGAtmosphere::GetSoundSpeedSL);
  PropertyManager->Tie("atmosphere/theta-norm", this,
                       &FGAtmosphere::GetTemperatureRatio);
  PropertyManager->Tie("atmosphere/sigma-norm", this,
                       &FGAtmosphere::GetDensityRatio);
  PropertyManager->Tie("atmosphere/delta-norm", this,
                       &FGAtmosphere::GetPressureRatio);
  PropertyManager->Tie("atmosphere/a-norm", this,
                       &FGAtmosphere::GetSoundSpeedRatio);
  PropertyManager->Tie("atmosphere/psiw-rad", this,
                       &FGAtmosphere::GetWindPsi);
  PropertyManager->Tie("atmosphere/p-turb-rad_sec", this,1,
                       (PMF)&FGAtmosphere::GetTurbPQR);
  PropertyManager->Tie("atmosphere/q-turb-rad_sec", this,2,
                       (PMF)&FGAtmosphere::GetTurbPQR);
  PropertyManager->Tie("atmosphere/r-turb-rad_sec", this,3,
                       (PMF)&FGAtmosphere::GetTurbPQR);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::unbind(void)
{
  PropertyManager->Untie("atmosphere/T-R");
  PropertyManager->Untie("atmosphere/rho-slugs_ft3");
  PropertyManager->Untie("atmosphere/P-psf");
  PropertyManager->Untie("atmosphere/a-fps");
  PropertyManager->Untie("atmosphere/T-sl-R");
  PropertyManager->Untie("atmosphere/rho-sl-slugs_ft3");
  PropertyManager->Untie("atmosphere/P-sl-psf");
  PropertyManager->Untie("atmosphere/a-sl-fps");
  PropertyManager->Untie("atmosphere/theta-norm");
  PropertyManager->Untie("atmosphere/sigma-norm");
  PropertyManager->Untie("atmosphere/delta-norm");
  PropertyManager->Untie("atmosphere/a-norm");
  PropertyManager->Untie("atmosphere/psiw-rad");
  PropertyManager->Untie("atmosphere/p-turb-rad_sec");
  PropertyManager->Untie("atmosphere/q-turb-rad_sec");
  PropertyManager->Untie("atmosphere/r-turb-rad_sec");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGAtmosphere::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAtmosphere" << endl;
    if (from == 1) cout << "Destroyed:    FGAtmosphere" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 32) { // Turbulence
    if (frame == 0 && from == 2) {
      cout << "vTurbulence(X), vTurbulence(Y), vTurbulence(Z), "
           << "vTurbulenceGrad(X), vTurbulenceGrad(Y), vTurbulenceGrad(Z), "
           << "vDirection(X), vDirection(Y), vDirection(Z), "
           << "Magnitude, "
           << "vTurbPQR(P), vTurbPQR(Q), vTurbPQR(R), " << endl;
    } else if (from == 2) {
      cout << vTurbulence << ", " << vTurbulenceGrad << ", " << vDirection << ", " << Magnitude << ", " << vTurbPQR << endl;
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

