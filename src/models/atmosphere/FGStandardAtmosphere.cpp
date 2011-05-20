/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGStandardAtmosphere.cpp
 Author:       Jon Berndt, Tony Peden
 Date started: 5/2011
 Purpose:      Models the 1976 U.S. Standard Atmosphere
 Called by:    FGFDMExec

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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


HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1]   Anderson, John D. "Introduction to Flight, Third Edition", McGraw-Hill,
      1989, ISBN 0-07-001641-0

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <cstdlib>
//#include "FGFDMExec.h"
#include "FGStandardAtmosphere.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGStandardAtmosphere.cpp,v 1.1 2011/05/20 03:09:51 jberndt Exp $";
static const char *IdHdr = ID_STANDARDATMOSPHERE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGStandardAtmosphere::FGStandardAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex),
                                                               h(0.0),                     // ft
                                                               SutherlandConstant(198.72), // deg Rankine
                                                               Beta(2.269690E-08)          // slug/(sec ft R^0.5)
{
  Name = "FGStandardAtmosphere";

  StdAtmosTemperatureTable = new FGTable(8);
  StdAtmosPressureTable    = new FGTable(8);

  // This is the U.S. Standard Atmosphere table for temperature in degrees
  // Rankine, based on geometric altitude. The table values are often given
  // in literature relative to geopotential altitude. 
  //
  //                        GeoMet Alt    Temp      GeoPot Alt  GeoMet Alt
  //                           (ft)      (deg R)      (km)        (km)
  //                         --------   --------    ----------  ----------
  *StdAtmosTemperatureTable <<      0.0 << 518.7  //    0.000       0.000
                            <<  36151.6 << 390.0  //   11.000      11.019
                            <<  65823.5 << 390.0  //   20.000      20.063
                            << 105518.4 << 411.6  //   32.000      32.162
                            << 155347.8 << 487.2  //   47.000      47.350
                            << 168677.8 << 487.2  //   51.000      51.413
                            << 235570.9 << 386.4  //   71.000      71.802
                            << 282152.2 << 336.5; //   84.852      86.000

  // This is the U.S. Standard Atmosphere table for pressure in pounds/square
  // foot, based on geometric altitude. The table values are often given
  // in literature relative to geopotential altitude. 
  //
  //                        GeoMet Alt   Pressure   GeoPot Alt  GeoMet Alt   Pressure
  //                           (ft)       (psf)        (km)        (km)        (Pa)
  //                         --------   --------    ----------  ---------- ----------
  *StdAtmosPressureTable <<      0.0 << 2116.2166  //    0.000       0.000  101325.00
                         <<  36151.6 <<  472.6791  //   11.000      11.019   22632.00
                         <<  65823.5 <<  114.3457  //   20.000      20.063    5474.90
                         << 105518.4 <<   18.1290  //   32.000      32.162     868.00
                         << 155347.8 <<    2.3164  //   47.000      47.350     110.90
                         << 168677.8 <<    1.3981  //   51.000      51.413      66.94
                         << 235570.9 <<    0.0826  //   71.000      71.802       3.96
                         << 282152.2 <<    0.0078; //   84.852      86.000       0.37
  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGStandardAtmosphere::~FGStandardAtmosphere()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGStandardAtmosphere::InitModel(void)
{
  Calculate(h);
  StdSLtemperature = SLtemperature = 518.67;
  StdSLpressure    = SLpressure = 2116.22;
  StdSLdensity     = SLdensity = 0.00237767;
  StdSLsoundspeed  = SLsoundspeed = sqrt(SHRatio*Reng*StdSLtemperature);

  rSLtemperature = 1/SLtemperature ;
  rSLpressure    = 1/SLpressure    ;
  rSLdensity     = 1/SLdensity     ;
  rSLsoundspeed  = 1/SLsoundspeed  ;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGStandardAtmosphere::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  RunPreFunctions();

//  double altitude = FDMExec->GetPropagate()->GetAltitudeASL();

  Calculate(altitude);
  CalculateDerived();

  RunPostFunctions();

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//

void FGStandardAtmosphere::Calculate(double altitude)
{
  Temperature = StdAtmosTemperatureTable->GetValue(altitude);
  Pressure    = StdAtmosPressureTable->GetValue(altitude);
  Density     = Pressure/(Reng*Temperature);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Calculate parameters derived from T, P and rho

void FGStandardAtmosphere::CalculateDerived(void)
{
//  T_dev = (*temperature) - GetTemperature(hNOTE_SET);

//  if (T_dev == 0.0) density_altitude = hNOTE_SET;
//  else              density_altitude = 518.67/0.00356616 * (1.0 - pow(GetDensityRatio(),0.235));

  Soundspeed = sqrt(SHRatio*Reng*(Temperature));

  Viscosity = Beta * pow(Temperature, 1.5) / (SutherlandConstant + Temperature);
  KinematicViscosity = Viscosity / Density;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard pressure at a specified altitude

double FGStandardAtmosphere::GetPressure(double altitude)
{
//  GetStdAtmosphere(altitude);
//  return atmosphere.Pressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard temperature at a specified altitude

double FGStandardAtmosphere::GetTemperature(double altitude)
{
//  GetStdAtmosphere(altitude);
//  return atmosphere.Temperature;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard density at a specified altitude

double FGStandardAtmosphere::GetDensity(double altitude)
{
//  GetStdAtmosphere(altitude);
//  return atmosphere.Density;
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
/// simply square a value
static inline double sqr(double x) { return x*x; }

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::bind(void)
{
  typedef double (FGStandardAtmosphere::*PMFv)(void) const;
  PropertyManager->Tie("atmosphere/T-R", this, (PMFv)&FGStandardAtmosphere::GetTemperature);
  PropertyManager->Tie("atmosphere/rho-slugs_ft3", this, (PMFv)&FGStandardAtmosphere::GetDensity);
  PropertyManager->Tie("atmosphere/P-psf", this, (PMFv)&FGStandardAtmosphere::GetPressure);
  PropertyManager->Tie("atmosphere/a-fps", this, &FGStandardAtmosphere::GetSoundSpeed);
  PropertyManager->Tie("atmosphere/T-sl-R", this, &FGStandardAtmosphere::GetTemperatureSL);
  PropertyManager->Tie("atmosphere/rho-sl-slugs_ft3", this, &FGStandardAtmosphere::GetDensitySL);
  PropertyManager->Tie("atmosphere/P-sl-psf", this, &FGStandardAtmosphere::GetPressureSL);
  PropertyManager->Tie("atmosphere/a-sl-fps", this, &FGStandardAtmosphere::GetSoundSpeedSL);
//  PropertyManager->Tie("atmosphere/theta", this, &FGStandardAtmosphere::GetTemperatureRatio);
//  PropertyManager->Tie("atmosphere/sigma", this, &FGStandardAtmosphere::GetDensityRatio);
//  PropertyManager->Tie("atmosphere/delta", this, &FGStandardAtmosphere::GetPressureRatio);
//  PropertyManager->Tie("atmosphere/a-ratio", this, &FGStandardAtmosphere::GetSoundSpeedRatio);
  PropertyManager->Tie("atmosphere/delta-T", this, &FGStandardAtmosphere::GetDeltaT, &FGStandardAtmosphere::SetDeltaT);
  PropertyManager->Tie("atmosphere/T-sl-dev-F", this, &FGStandardAtmosphere::GetSLTempDev, &FGStandardAtmosphere::SetSLTempDev);
//  PropertyManager->Tie("atmosphere/density-altitude", this, &FGStandardAtmosphere::GetDensityAltitude);
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

void FGStandardAtmosphere::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGStandardAtmosphere" << endl;
    if (from == 1) cout << "Destroyed:    FGStandardAtmosphere" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 128) { // 
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
