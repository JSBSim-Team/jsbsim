/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAtmosphere.cpp
 Author:       Jon Berndt
               Implementation of 1959 Standard Atmosphere added by Tony Peden
 Date started: 11/24/98
 Purpose:      Models the atmosphere
 Called by:    FGSimExec

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Models the atmosphere. The equation used below was determined by a third order
curve fit using Excel. The data is from the ICAO atmosphere model.

HISTORY
--------------------------------------------------------------------------------
11/24/98   JSB   Created
07/23/99   TP    Added implementation of 1959 Standard Atmosphere
                 Moved calculation of Mach number to FGPropagate
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
#include <FGState.h>
#include <FGFDMExec.h>
#include "FGAircraft.h"
#include "FGPropagate.h"
#include "FGInertial.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGAtmosphere.cpp,v 1.20 2008/05/31 23:13:29 jberndt Exp $";
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
  SetTurbType( ttCulp );
  TurbGain = 0.0;
  TurbRate = 1.7;
  Rhythmicity = 0.1;
  spike = target_time = strength = 0.0;
  wind_from_clockwise = 0.0;

  T_dev_sl = T_dev = delta_T = 0.0;
  StandardTempOnly = false;
  first_pass = true;
  vGustNED(1) = vGustNED(2) = vGustNED(3) = 0.0; bgustSet = false;
  vTurbulence(1) = vTurbulence(2) = vTurbulence(3) = 0.0;

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAtmosphere::~FGAtmosphere()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  UseInternal();  // this is the default

  Calculate(h);
  StdSLtemperature = SLtemperature = 518.67;
  StdSLpressure    = SLpressure = 2116.22;
  StdSLdensity     = SLdensity = 0.00237767;
  StdSLsoundspeed  = SLsoundspeed = sqrt(SHRatio*Reng*StdSLtemperature);
  rSLtemperature = 1.0/StdSLtemperature;
  rSLpressure    = 1.0/StdSLpressure;
  rSLdensity     = 1.0/StdSLdensity;
  rSLsoundspeed  = 1.0/StdSLsoundspeed;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::Run(void)
{
  if (FGModel::Run()) return true;
  if (FDMExec->Holding()) return false;

  T_dev = 0.0;
  h = Propagate->Geth();

  if (!useExternal) {
    Calculate(h);
    CalculateDerived();
  } else {
    CalculateDerived();
  }

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// See reference 1

void FGAtmosphere::Calculate(double altitude)
{
  double slope, reftemp, refpress;
  int i = lastIndex;

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

  // If delta_T is set, then that is our temperature deviation at any altitude.
  // If not, then we'll estimate a deviation based on the sea level deviation (if set).

  if(!StandardTempOnly) {
    T_dev = 0.0;
    if (delta_T != 0.0) {
      T_dev = delta_T;
    } else {
      if ((altitude < 36089.239) && (T_dev_sl != 0.0)) {
        T_dev = T_dev_sl * ( 1.0 - (altitude/36089.239));
      }
    }
    reftemp+=T_dev;
  }

  if (slope == 0) {
    intTemperature = reftemp;
    intPressure = refpress*exp(-Inertial->SLgravity()/(reftemp*Reng)*(altitude-htab[i]));
    intDensity = intPressure/(Reng*intTemperature);
  } else {
    intTemperature = reftemp+slope*(altitude-htab[i]);
    intPressure = refpress*pow(intTemperature/reftemp,-Inertial->SLgravity()/(slope*Reng));
    intDensity = intPressure/(Reng*intTemperature);
  }

  lastIndex=i;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Calculate parameters derived from T, P and rho

void FGAtmosphere::CalculateDerived(void)
{
  T_dev = (*temperature) - GetTemperature(h);
  density_altitude = h + T_dev * 66.7;

  if (turbType == ttStandard || ttCulp) {
    Turbulence();
    vWindNED += vGustNED + vTurbulence;
  }
  if (vWindNED(1) != 0.0) psiw = atan2( vWindNED(2), vWindNED(1) );
  if (psiw < 0) psiw += 2*M_PI;

  soundspeed = sqrt(SHRatio*Reng*(*temperature));
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard atmospheric properties at a specified altitude

void FGAtmosphere::GetStdAtmosphere(double altitude) {
  StandardTempOnly = true;
  Calculate(altitude);
  StandardTempOnly = false;
  atmosphere.Temperature = intTemperature;
  atmosphere.Pressure = intPressure;
  atmosphere.Density = intDensity;

  // Reset the internal atmospheric state
  Calculate(h);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard pressure at a specified altitude

double FGAtmosphere::GetPressure(double altitude) {
  GetStdAtmosphere(altitude);
  return atmosphere.Pressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard temperature at a specified altitude

double FGAtmosphere::GetTemperature(double altitude) {
  GetStdAtmosphere(altitude);
  return atmosphere.Temperature;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard density at a specified altitude

double FGAtmosphere::GetDensity(double altitude) {
  GetStdAtmosphere(altitude);
  return atmosphere.Density;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// square a value, but preserve the original sign

static inline double square_signed (double value)
{
    if (value < 0)
        return value * value * -1;
    else
        return value * value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::Turbulence(void)
{
  switch (turbType) {
  case ttStandard: {
    TurbGain = TurbGain * TurbGain * 100.0;

    vDirectiondAccelDt(eX) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eY) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eZ) = 1 - 2.0*(double(rand())/double(RAND_MAX));

    MagnitudedAccelDt = 1 - 2.0*(double(rand())/double(RAND_MAX)) - Magnitude;
                                // Scale the magnitude so that it moves
                                // away from the peaks
    MagnitudedAccelDt = ((MagnitudedAccelDt - Magnitude) /
                         (1 + fabs(Magnitude)));
    MagnitudeAccel    += MagnitudedAccelDt*rate*TurbRate*State->Getdt();
    Magnitude         += MagnitudeAccel*rate*State->Getdt();
    Magnitude          = fabs(Magnitude);

    vDirectiondAccelDt.Normalize();

                                // deemphasise non-vertical forces
    vDirectiondAccelDt(eX) = square_signed(vDirectiondAccelDt(eX));
    vDirectiondAccelDt(eY) = square_signed(vDirectiondAccelDt(eY));

    vDirectionAccel += vDirectiondAccelDt*rate*TurbRate*State->Getdt();
    vDirectionAccel.Normalize();
    vDirection      += vDirectionAccel*rate*State->Getdt();

    vDirection.Normalize();

                                // Diminish turbulence within three wingspans
                                // of the ground
    vTurbulence = TurbGain * Magnitude * vDirection;
    double HOverBMAC = Auxiliary->GetHOverBMAC();
    if (HOverBMAC < 3.0)
        vTurbulence *= (HOverBMAC / 3.0) * (HOverBMAC / 3.0);

    // I don't believe these next two statements calculate the proper gradient over
    // the aircraft body. One reason is because this has no relationship with the
    // orientation or velocity of the aircraft, which it must have. What is vTurbulenceGrad
    // supposed to represent? And the direction and magnitude of the turbulence can change,
    // so both accelerations need to be accounted for, no?

    // Need to determine the turbulence change in body axes between two time points.

    vTurbulenceGrad = TurbGain*MagnitudeAccel * vDirection;
    vBodyTurbGrad = Propagate->GetTl2b()*vTurbulenceGrad;

    if (Aircraft->GetWingSpan() > 0) {
      vTurbPQR(eP) = vBodyTurbGrad(eY)/Aircraft->GetWingSpan();
    } else {
      vTurbPQR(eP) = vBodyTurbGrad(eY)/30.0;
    }
//     if (Aircraft->GetHTailArm() != 0.0)
//       vTurbPQR(eQ) = vBodyTurbGrad(eZ)/Aircraft->GetHTailArm();
//     else
//       vTurbPQR(eQ) = vBodyTurbGrad(eZ)/10.0;

    if (Aircraft->GetVTailArm() > 0)
      vTurbPQR(eR) = vBodyTurbGrad(eX)/Aircraft->GetVTailArm();
    else
      vTurbPQR(eR) = vBodyTurbGrad(eX)/10.0;

                                // Clear the horizontal forces
                                // actually felt by the plane, now
                                // that we've used them to calculate
                                // moments.
                                // Why? (JSB)
//    vTurbulence(eX) = 0.0;
//    vTurbulence(eY) = 0.0;

    break;
  }
  case ttBerndt: { // This is very experimental and incomplete at the moment.

    TurbGain = TurbGain * TurbGain * 100.0;
  
    vDirectiondAccelDt(eX) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eY) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eZ) = 1 - 2.0*(double(rand())/double(RAND_MAX));


    MagnitudedAccelDt = 1 - 2.0*(double(rand())/double(RAND_MAX)) - Magnitude;
    MagnitudeAccel    += MagnitudedAccelDt*rate*State->Getdt();
    Magnitude         += MagnitudeAccel*rate*State->Getdt();

    vDirectiondAccelDt.Normalize();
    vDirectionAccel += vDirectiondAccelDt*rate*State->Getdt();
    vDirectionAccel.Normalize();
    vDirection      += vDirectionAccel*rate*State->Getdt();

                                // Diminish z-vector within two wingspans
                                // of the ground
    double HOverBMAC = Auxiliary->GetHOverBMAC();
    if (HOverBMAC < 2.0)
        vDirection(eZ) *= HOverBMAC / 2.0;

    vDirection.Normalize();

    vTurbulence = TurbGain*Magnitude * vDirection;
    vTurbulenceGrad = TurbGain*MagnitudeAccel * vDirection;

    vBodyTurbGrad = Propagate->GetTl2b()*vTurbulenceGrad;
    vTurbPQR(eP) = vBodyTurbGrad(eY)/Aircraft->GetWingSpan();
    if (Aircraft->GetHTailArm() > 0)
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/Aircraft->GetHTailArm();
    else
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/10.0;

    if (Aircraft->GetVTailArm() > 0)
      vTurbPQR(eR) = vBodyTurbGrad(eX)/Aircraft->GetVTailArm();
    else
      vTurbPQR(eR) = vBodyTurbGrad(eX)/10.0;

    break;
  }
  case ttCulp: { 

    vTurbPQR(eP) = wind_from_clockwise;
    if (TurbGain == 0.0) return;
  
    // keep the inputs within allowable limts for this model
    if (TurbGain < 0.0) TurbGain = 0.0;
    if (TurbGain > 1.0) TurbGain = 1.0;
    if (TurbRate < 0.0) TurbRate = 0.0;
    if (TurbRate > 30.0) TurbRate = 30.0;
    if (Rhythmicity < 0.0) Rhythmicity = 0.0;
    if (Rhythmicity > 1.0) Rhythmicity = 1.0;

    // generate a sine wave corresponding to turbulence rate in hertz
    double time = FDMExec->GetSimTime();
    double sinewave = sin( time * TurbRate * 6.283185307 );

    double random = 0.0;
    if (target_time == 0.0) {
      strength = random = 1 - 2.0*(double(rand())/double(RAND_MAX));
      target_time = time + 0.71 + (random * 0.5);
    }
    if (time > target_time) {
      spike = 1.0;
      target_time = 0.0;
    }    

    // max vertical wind speed in fps, corresponds to TurbGain = 1.0
    double max_vs = 40;

    vTurbulence(1) = vTurbulence(2) = vTurbulence(3) = 0.0;
    double delta = strength * max_vs * TurbGain * (1-Rhythmicity) * spike;

    // Vertical component of turbulence.
    vTurbulence(3) = sinewave * max_vs * TurbGain * Rhythmicity;
    vTurbulence(3)+= delta;
    double HOverBMAC = Auxiliary->GetHOverBMAC();
    if (HOverBMAC < 3.0)
        vTurbulence(3) *= HOverBMAC * 0.3333;
 
    // Yaw component of turbulence.
    vTurbulence(1) = sin( delta * 3.0 );
    vTurbulence(2) = cos( delta * 3.0 );

    // Roll component of turbulence. Clockwise vortex causes left roll.
    vTurbPQR(eP) += delta * 0.04;

    spike = spike * 0.9;
    break;
  }
  default:
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::UseExternal(void)
{
  temperature=&exTemperature;
  pressure=&exPressure;
  density=&exDensity;
  useExternal=true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::UseInternal(void)
{
  temperature=&intTemperature;
  pressure=&intPressure;
  density=&intDensity;
  useExternal=false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::bind(void)
{
  typedef double (FGAtmosphere::*PMF)(int) const;
  typedef double (FGAtmosphere::*PMFv)(void) const;
  typedef void   (FGAtmosphere::*PMFd)(int,double);
  PropertyManager->Tie("atmosphere/T-R", this, (PMFv)&FGAtmosphere::GetTemperature);
  PropertyManager->Tie("atmosphere/rho-slugs_ft3", this, (PMFv)&FGAtmosphere::GetDensity);
  PropertyManager->Tie("atmosphere/P-psf", this, (PMFv)&FGAtmosphere::GetPressure);
  PropertyManager->Tie("atmosphere/a-fps", this, &FGAtmosphere::GetSoundSpeed);
  PropertyManager->Tie("atmosphere/T-sl-R", this, &FGAtmosphere::GetTemperatureSL);
  PropertyManager->Tie("atmosphere/rho-sl-slugs_ft3", this, &FGAtmosphere::GetDensitySL);
  PropertyManager->Tie("atmosphere/P-sl-psf", this, &FGAtmosphere::GetPressureSL);
  PropertyManager->Tie("atmosphere/a-sl-fps", this, &FGAtmosphere::GetSoundSpeedSL);
  PropertyManager->Tie("atmosphere/theta", this, &FGAtmosphere::GetTemperatureRatio);
  PropertyManager->Tie("atmosphere/sigma", this, &FGAtmosphere::GetDensityRatio);
  PropertyManager->Tie("atmosphere/delta", this, &FGAtmosphere::GetPressureRatio);
  PropertyManager->Tie("atmosphere/a-ratio", this, &FGAtmosphere::GetSoundSpeedRatio);
  PropertyManager->Tie("atmosphere/psiw-rad", this, &FGAtmosphere::GetWindPsi);
  PropertyManager->Tie("atmosphere/delta-T", this, &FGAtmosphere::GetDeltaT, &FGAtmosphere::SetDeltaT);
  PropertyManager->Tie("atmosphere/T-sl-dev-F", this, &FGAtmosphere::GetSLTempDev, &FGAtmosphere::SetSLTempDev);
  PropertyManager->Tie("atmosphere/density-altitude", this, &FGAtmosphere::GetDensityAltitude);
  PropertyManager->Tie("atmosphere/p-turb-rad_sec", this,1, (PMF)&FGAtmosphere::GetTurbPQR);
  PropertyManager->Tie("atmosphere/q-turb-rad_sec", this,2, (PMF)&FGAtmosphere::GetTurbPQR);
  PropertyManager->Tie("atmosphere/r-turb-rad_sec", this,3, (PMF)&FGAtmosphere::GetTurbPQR);
  PropertyManager->Tie("atmosphere/turb-rate", this, &FGAtmosphere::GetTurbRate, &FGAtmosphere::SetTurbRate);
  PropertyManager->Tie("atmosphere/turb-gain", this, &FGAtmosphere::GetTurbGain, &FGAtmosphere::SetTurbGain);
  PropertyManager->Tie("atmosphere/turb-rhythmicity", this, &FGAtmosphere::GetRhythmicity,
                                                            &FGAtmosphere::SetRhythmicity);
  PropertyManager->Tie("atmosphere/gust-north-fps", this,1, (PMF)&FGAtmosphere::GetGustNED,
                                                          (PMFd)&FGAtmosphere::SetGustNED);
  PropertyManager->Tie("atmosphere/gust-east-fps",  this,2, (PMF)&FGAtmosphere::GetGustNED,
                                                          (PMFd)&FGAtmosphere::SetGustNED);
  PropertyManager->Tie("atmosphere/gust-down-fps",  this,3, (PMF)&FGAtmosphere::GetGustNED,
                                                          (PMFd)&FGAtmosphere::SetGustNED);
  PropertyManager->Tie("atmosphere/wind-from-cw", this, &FGAtmosphere::GetWindFromClockwise,
                                                        &FGAtmosphere::SetWindFromClockwise);
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
  if (debug_lvl & 128) { // Turbulence
    if (first_pass && from == 2) {
      first_pass = false;
      cout << "vTurbulence(X), vTurbulence(Y), vTurbulence(Z), "
           << "vTurbulenceGrad(X), vTurbulenceGrad(Y), vTurbulenceGrad(Z), "
           << "vDirection(X), vDirection(Y), vDirection(Z), "
           << "Magnitude, "
           << "vTurbPQR(P), vTurbPQR(Q), vTurbPQR(R), " << endl;
    } 
    if (from == 2) {
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

} // namespace JSBSim
