/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAtmosphere.cpp
 Author:       Jon Berndt, Tony Peden
 Date started: 6/2011
 Purpose:      Models an atmosphere interface class
 Called by:    FGFDMExec

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This models a base atmosphere class to serve as a common interface to any
derived atmosphere models.

HISTORY
--------------------------------------------------------------------------------
6/18/2011 Started Jon S. Berndt

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGAtmosphere.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Atmosphere constants in British units converted from the SI values specified in the 
// ISA document - https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770009539.pdf
double FGAtmosphere::Reng = Rstar / Mair;

const double FGAtmosphere::StdDaySLsoundspeed = sqrt(SHRatio*Reng*StdDaySLtemperature);

FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex),
                                               PressureAltitude(0.0),      // ft
                                               DensityAltitude(0.0)       // ft
{
  Name = "FGAtmosphere";

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

  Calculate(0.0);
  SLtemperature = Temperature = StdDaySLtemperature;
  SLpressure = Pressure = StdDaySLpressure;
  SLdensity = Density = Pressure/(Reng*Temperature);
  SLsoundspeed = Soundspeed = StdDaySLsoundspeed;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAtmosphere::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  Calculate(in.altitudeASL);

  Debug(2);
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::Calculate(double altitude)
{
  FGPropertyNode* node = PropertyManager->GetNode();
  if (!PropertyManager->HasNode("atmosphere/override/temperature"))
    Temperature = GetTemperature(altitude);
  else
    Temperature = node->GetDouble("atmosphere/override/temperature");

  if (!PropertyManager->HasNode("atmosphere/override/pressure"))
    Pressure = GetPressure(altitude);
  else
    Pressure = node->GetDouble("atmosphere/override/pressure");

  if (!PropertyManager->HasNode("atmosphere/override/density"))
    Density = GetDensity(altitude);
  else
    Density = node->GetDouble("atmosphere/override/density");

  Soundspeed  = sqrt(SHRatio*Reng*Temperature);
  PressureAltitude = CalculatePressureAltitude(Pressure, altitude);
  DensityAltitude = CalculateDensityAltitude(Density, altitude);

  Viscosity = Beta * pow(Temperature, 1.5) / (SutherlandConstant + Temperature);
  KinematicViscosity = Viscosity / Density;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::SetPressureSL(ePressure unit, double pressure)
{
  double press = ConvertToPSF(pressure, unit);

  SLpressure = press;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the modeled density at a specified altitude

double FGAtmosphere::GetDensity(double altitude) const
{
  return GetPressure(altitude)/(Reng * GetTemperature(altitude));
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the sound speed at a specified altitude

double FGAtmosphere::GetSoundSpeed(double altitude) const
{
  return sqrt(SHRatio * Reng * GetTemperature(altitude));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This function sets the sea level temperature.
// Internally, the Rankine scale is used for calculations, so any temperature
// supplied must be converted to that unit.

void FGAtmosphere::SetTemperatureSL(double t, eTemperature unit)
{
  SLtemperature = ConvertToRankine(t, unit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAtmosphere::ConvertToRankine(double t, eTemperature unit) const
{
  double targetTemp=0; // in degrees Rankine

  switch(unit) {
  case eFahrenheit:
    targetTemp = t + 459.67;
    break;
  case eCelsius:
    targetTemp = (t + 273.15) * 1.8;
    break;
  case eRankine:
    targetTemp = t;
    break;
  case eKelvin:
    targetTemp = t*1.8;
    break;
  default:
    break;
  }

  return targetTemp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAtmosphere::ConvertFromRankine(double t, eTemperature unit) const
{
  double targetTemp=0;

  switch(unit) {
  case eFahrenheit:
    targetTemp = t - 459.67;
    break;
  case eCelsius:
    targetTemp = t/1.8 - 273.15;
    break;
  case eRankine:
    targetTemp = t;
    break;
  case eKelvin:
    targetTemp = t/1.8;
    break;
  default:
    break;
  }

  return targetTemp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAtmosphere::ConvertToPSF(double p, ePressure unit) const
{
  double targetPressure=0; // Pressure in PSF

  switch(unit) {
  case ePSF:
    targetPressure = p;
    break;
  case eMillibars:
    targetPressure = p*2.08854342;
    break;
  case ePascals:
    targetPressure = p*0.0208854342;
    break;
  case eInchesHg:
    targetPressure = p*70.7180803;
    break;
  default:
    throw("Undefined pressure unit given");
  }

  return targetPressure;
}

double FGAtmosphere::ConvertFromPSF(double p, ePressure unit) const
{
  double targetPressure=0; // Pressure

  switch(unit) {
  case ePSF:
    targetPressure = p;
    break;
  case eMillibars:
    targetPressure = p/2.08854342;
    break;
  case ePascals:
    targetPressure = p/0.0208854342;
    break;
  case eInchesHg:
    targetPressure = p/70.7180803;
    break;
  default:
    throw("Undefined pressure unit given");
  }

  return targetPressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAtmosphere::bind(void)
{
  PropertyManager->Tie("atmosphere/T-R", this, &FGAtmosphere::GetTemperature);
  PropertyManager->Tie("atmosphere/rho-slugs_ft3", this, &FGAtmosphere::GetDensity);
  PropertyManager->Tie("atmosphere/P-psf", this, &FGAtmosphere::GetPressure);
  PropertyManager->Tie("atmosphere/a-fps", this, &FGAtmosphere::GetSoundSpeed);
  PropertyManager->Tie("atmosphere/T-sl-R", this, &FGAtmosphere::GetTemperatureSL);
  PropertyManager->Tie("atmosphere/rho-sl-slugs_ft3", this, &FGAtmosphere::GetDensitySL);
  PropertyManager->Tie("atmosphere/a-sl-fps", this, &FGAtmosphere::GetSoundSpeedSL);
  PropertyManager->Tie("atmosphere/theta", this, &FGAtmosphere::GetTemperatureRatio);
  PropertyManager->Tie("atmosphere/sigma", this, &FGAtmosphere::GetDensityRatio);
  PropertyManager->Tie("atmosphere/delta", this, &FGAtmosphere::GetPressureRatio);
  PropertyManager->Tie("atmosphere/a-ratio", this, &FGAtmosphere::GetSoundSpeedRatio);
  PropertyManager->Tie("atmosphere/density-altitude", this, &FGAtmosphere::GetDensityAltitude);
  PropertyManager->Tie("atmosphere/pressure-altitude", this, &FGAtmosphere::GetPressureAltitude);
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
    if (from == 0) std::cout << "Instantiated: FGAtmosphere" << std::endl;
    if (from == 1) std::cout << "Destroyed:    FGAtmosphere" << std::endl;
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
    }
  }
}

} // namespace JSBSim
