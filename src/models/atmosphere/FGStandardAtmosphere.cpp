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
[2]   Sonntag, D. "Important New Values of the Physical Constants of 1986,
      Vapour Pressure Formulations based on the IST-90 and Psychrometer
      Formulae", Z. Meteorol., 70 (5), pp. 340-344, 1990

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "FGFDMExec.h"
#include "FGStandardAtmosphere.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Effective radius of the earth at a specific latitude per ISA 1976 (converted to ft)
// r0 = 6356766 m
const double FGStandardAtmosphere::EarthRadius = 6356766.0/FGJSBBase::fttom;
/** Sonntag constants based on ref [2]. They are valid for temperatures between
    -45 degC (-49 degF) and 60 degC (140 degF) with a precision of +/-0.35 degC
    (+/-0.63 degF) */
const double FGStandardAtmosphere::a = 611.2/FGJSBBase::psftopa; // psf
const double FGStandardAtmosphere::b = 17.62; // 1/degC
const double FGStandardAtmosphere::c = 243.12; // degC
const double FGStandardAtmosphere::Mwater = 18.016 * FGJSBBase::kgtoslug / 1000.0; // slug/mol

FGStandardAtmosphere::FGStandardAtmosphere(FGFDMExec* fdmex)
  : FGAtmosphere(fdmex), TemperatureBias(0.0), TemperatureDeltaGradient(0.0),
    VaporPressure(0.0), SaturatedVaporPressure(0.0), StdAtmosTemperatureTable(9)
{
  Name = "FGStandardAtmosphere";

  // This is the U.S. Standard Atmosphere table for temperature in degrees
  // Rankine, based on geometric altitude. The table values are often given
  // in literature relative to geopotential altitude. 
  //
  //                        GeoMet Alt    Temp      GeoPot Alt  GeoMet Alt
  //                           (ft)      (deg R)      (km)        (km)
  //                         --------   --------    ----------  ----------
  //StdAtmosTemperatureTable <<      0.00 << 518.67  //    0.000       0.000
  //                         <<  36151.80 << 389.97  //   11.000      11.019
  //                         <<  65823.90 << 389.97  //   20.000      20.063
  //                         << 105518.06 << 411.60  //   32.000      32.162
  //                         << 155348.07 << 487.20  //   47.000      47.350
  //                         << 168676.12 << 487.20  //   51.000      51.413
  //                         << 235570.77 << 386.40  //   71.000      71.802
  //                         << 282152.08 << 336.50  //   84.852      86.000
  //                         << 298556.40 << 336.50; //               91.000 - First layer in high altitude regime 

  //                            GeoPot Alt    Temp       GeoPot Alt  GeoMet Alt
  //                               (ft)      (deg R)        (km)        (km)
  //                           -----------   --------     ----------  ----------
  StdAtmosTemperatureTable <<      0.0000 << 518.67  //    0.000       0.000
                           <<  36089.2388 << 389.97  //   11.000      11.019
                           <<  65616.7979 << 389.97  //   20.000      20.063
                           << 104986.8766 << 411.57  //   32.000      32.162
                           << 154199.4751 << 487.17  //   47.000      47.350
                           << 167322.8346 << 487.17  //   51.000      51.413
                           << 232939.6325 << 386.37  //   71.000      71.802
                           << 278385.8268 << 336.5028  // 84.852      86.000
                           << 298556.4304 << 336.5028; //             91.000 - First layer in high altitude regime 

  unsigned int numRows = StdAtmosTemperatureTable.GetNumRows();
  PressureBreakpoints.resize(numRows);
  LapseRates.resize(numRows);

  // Assume the altitude to fade out the gradient at is at the highest
  // altitude in the table. Above that, other functions are used to
  // calculate temperature.
  GradientFadeoutAltitude = StdAtmosTemperatureTable(StdAtmosTemperatureTable.GetNumRows(),0);

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
  PressureBreakpoints[0] = StdSLpressure = SLpressure = Pressure = 2116.228; // psf
  TemperatureDeltaGradient = 0.0;
  TemperatureBias = 0.0;
  CalculateLapseRates();
  CalculatePressureBreakpoints();

  StdSLtemperature = SLtemperature = StdAtmosTemperatureTable(1, 1);
  StdSLdensity     = SLdensity = StdSLpressure / (Reng * StdSLtemperature);

  StdPressureBreakpoints = PressureBreakpoints;
  StdLapseRates = LapseRates;

  CalculateStdDensityBreakpoints();

  Calculate(0.0);

  StdSLsoundspeed = SLsoundspeed = Soundspeed;

//  PrintStandardAtmosphereTable();

  return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::Calculate(double altitude)
{
  FGAtmosphere::Calculate(altitude);
  SaturatedVaporPressure = CalculateVaporPressure(Temperature);

  // Make sure the partial pressure of water vapor never becomes higher than the
  // saturated vapor pressure.
  if (VaporPressure > SaturatedVaporPressure)
    VaporPressure = SaturatedVaporPressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the actual pressure as modeled at a specified altitude
// These calculations are from equations 33a and 33b in the U.S. Standard
// Atmosphere document referenced in the documentation for this code.

double FGStandardAtmosphere::GetPressure(double altitude) const
{
  double GeoPotAlt = GeopotentialAltitude(altitude);

  // Iterate through the altitudes to find the current Base Altitude
  // in the table. That is, if the current altitude (the argument passed in)
  // is 20000 ft, then the base altitude from the table is 0.0. If the
  // passed-in altitude is 40000 ft, the base altitude is 36089.2388 ft (and
  // the index "b" is 2 - the second entry in the table).
  double BaseAlt = StdAtmosTemperatureTable(1,0);
  unsigned int numRows = StdAtmosTemperatureTable.GetNumRows();
  unsigned int b;

  for (b=0; b < numRows-2; ++b) {
    double testAlt = StdAtmosTemperatureTable(b+2,0);
    if (GeoPotAlt < testAlt)
      break;
    BaseAlt = testAlt;
  }

  double Tmb = GetTemperature(GeometricAltitude(BaseAlt));
  double deltaH = GeoPotAlt - BaseAlt;
  double Lmb = LapseRates[b];

  if (Lmb != 0.0) {
    double Exp = g0*Mair / (Rstar*Lmb);
    double factor = Tmb/(Tmb + Lmb*deltaH);
    return PressureBreakpoints[b]*pow(factor, Exp);
  } else
    return PressureBreakpoints[b]*exp(-g0*Mair*deltaH/(Rstar*Tmb));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetPressureSL(ePressure unit, double pressure)
{
  double press = ConvertToPSF(pressure, unit);

  SLpressure = press;
  CalculateSLDensity();
  CalculatePressureBreakpoints();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::CalculateSLSoundSpeedAndDensity(void)
{
  SLsoundspeed = sqrt(SHRatio*Reng*SLtemperature);
  CalculateSLDensity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the modeled temperature at a specified altitude, including any bias or gradient
// effects.

double FGStandardAtmosphere::GetTemperature(double altitude) const
{
  double GeoPotAlt = GeopotentialAltitude(altitude);

  double T;

  if (GeoPotAlt >= 0.0) {
    T = StdAtmosTemperatureTable.GetValue(GeoPotAlt);

    if (GeoPotAlt <= GradientFadeoutAltitude)
      T -= TemperatureDeltaGradient * GeoPotAlt;
  }
  else {
    // We don't need to add TemperatureDeltaGradient*GeoPotAlt here because
    // the lapse rate vector already accounts for the temperature gradient.
    T = StdAtmosTemperatureTable.GetValue(0.0) + GeoPotAlt*LapseRates[0];
  }

  T += TemperatureBias;

  if (GeoPotAlt <= GradientFadeoutAltitude)
    T += TemperatureDeltaGradient * GradientFadeoutAltitude;

  return T;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Retrieves the standard temperature at a particular altitude.

double FGStandardAtmosphere::GetStdTemperature(double altitude) const
{
  double GeoPotAlt = GeopotentialAltitude(altitude);

  if (GeoPotAlt >= 0.0)
    return StdAtmosTemperatureTable.GetValue(GeoPotAlt);
  else
    return StdAtmosTemperatureTable.GetValue(0.0) + GeoPotAlt*LapseRates[0];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::GetStdPressure(double altitude) const
{
  double GeoPotAlt = GeopotentialAltitude(altitude);

  // Iterate through the altitudes to find the current Base Altitude
  // in the table. That is, if the current altitude (the argument passed in)
  // is 20000 ft, then the base altitude from the table is 0.0. If the
  // passed-in altitude is 40000 ft, the base altitude is 36089.2388 ft (and
  // the index "b" is 2 - the second entry in the table).
  double BaseAlt = StdAtmosTemperatureTable(1,0);
  unsigned int numRows = StdAtmosTemperatureTable.GetNumRows();
  unsigned int b;

  for (b=0; b < numRows-2; ++b) {
    double testAlt = StdAtmosTemperatureTable(b+2,0);
    if (GeoPotAlt < testAlt)
      break;
    BaseAlt = testAlt;
  }

  double Tmb = GetStdTemperature(GeometricAltitude(BaseAlt));
  double deltaH = GeoPotAlt - BaseAlt;
  double Lmb = LapseRates[b];

  if (Lmb != 0.0) {
    double Exp = g0*Mair / (Rstar*Lmb);
    double factor = Tmb/(Tmb + Lmb*deltaH);
    return StdPressureBreakpoints[b]*pow(factor, Exp);
  } else
    return StdPressureBreakpoints[b]*exp(-g0*Mair*deltaH/(Rstar*Tmb));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the standard density at a specified altitude

double FGStandardAtmosphere::GetStdDensity(double altitude) const
{
  return GetStdPressure(altitude)/(Reng * GetStdTemperature(altitude));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Get the density, taking into account the water vapor in the air.

double FGStandardAtmosphere::GetDensity(double altitude) const
{
  double P = GetPressure(altitude);
  double Pa = P - VaporPressure; // Partial pressure of air

  return (Pa/Reng + Mwater*VaporPressure/Rstar)/GetTemperature(altitude);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetTemperature(double t, double h, eTemperature unit)
{
  double targetTemp = ConvertToRankine(t, unit);
  double GeoPotAlt = GeopotentialAltitude(h);

  TemperatureBias = targetTemp - GetStdTemperature(h);

  if (GeoPotAlt <= GradientFadeoutAltitude)
    TemperatureBias -= TemperatureDeltaGradient * (GradientFadeoutAltitude - GeoPotAlt);

  CalculatePressureBreakpoints();

  SLtemperature = GetTemperature(0.0);
  CalculateSLSoundSpeedAndDensity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetTemperatureBias(eTemperature unit, double t)
{
  if (unit == eCelsius || unit == eKelvin)
    t *= 1.80; // If temp delta "t" is given in metric, scale up to English

  TemperatureBias = t;
  CalculatePressureBreakpoints();

  SLtemperature = GetTemperature(0.0);
  CalculateSLSoundSpeedAndDensity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This function calculates a bias based on the supplied temperature for sea
// level. The bias is applied to the entire temperature profile at all altitudes.
// Internally, the Rankine scale is used for calculations, so any temperature
// supplied must be converted to that unit.

void FGStandardAtmosphere::SetTemperatureSL(double t, eTemperature unit)
{
  SetTemperature(t, 0.0, unit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Sets a Sea Level temperature delta that is ramped out by 86 km (282,152 ft).

void FGStandardAtmosphere::SetSLTemperatureGradedDelta(eTemperature unit, double deltemp)
{
  SetTemperatureGradedDelta(deltemp, 0.0, unit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Sets a temperature delta at the supplied altitude that is ramped out by 86 km.
// After this calculation is performed, the lapse rates and pressure breakpoints
// must be recalculated. Since we are calculating a delta here and not an actual
// temperature, we only need to be concerned about a scale factor and not
// the actual temperature itself.

void FGStandardAtmosphere::SetTemperatureGradedDelta(double deltemp, double h, eTemperature unit)
{
  if (unit == eCelsius || unit == eKelvin)
    deltemp *= 1.80; // If temp delta "t" is given in metric, scale up to English

  TemperatureDeltaGradient = deltemp/(GradientFadeoutAltitude - GeopotentialAltitude(h));
  CalculateLapseRates();
  CalculatePressureBreakpoints();

  SLtemperature = GetTemperature(0.0);
  CalculateSLSoundSpeedAndDensity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  void FGStandardAtmosphere::PrintStandardAtmosphereTable()
{
  std::cout << "Altitude (ft)   Temp (F)   Pressure (psf)   Density (sl/ft3)" << std::endl;
  std::cout << "-------------   --------   --------------   ----------------" << std::endl;
  for (int i=0; i<280000; i+=1000) {
    Calculate(i);
    std::cout  << std::setw(12) << std::setprecision(2) << i
       << "  " << std::setw(9)  << std::setprecision(2) << Temperature - 459.67
       << "  " << std::setw(13) << std::setprecision(4) << Pressure
       << "  " << std::setw(18) << std::setprecision(8) << Density
       << std::endl;
  }

  // Re-execute the Run() method to reset the calculated values
  Run(false);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This function calculates (or recalculates) the lapse rate over an altitude range
// where the "bh" in this case refers to the index of the base height in the 
// StdAtmosTemperatureTable table. This function should be called anytime the 
// temperature table is altered, such as when a gradient is applied across the 
// temperature table for a range of altitudes.

void FGStandardAtmosphere::CalculateLapseRates()
{
  unsigned int numRows = StdAtmosTemperatureTable.GetNumRows();
  LapseRates.clear();

  for (unsigned int bh=0; bh < numRows-1; bh++)
  {
    double t0 = StdAtmosTemperatureTable(bh+1,1);
    double t1 = StdAtmosTemperatureTable(bh+2,1);
    double h0 = StdAtmosTemperatureTable(bh+1,0);
    double h1 = StdAtmosTemperatureTable(bh+2,0);
    LapseRates.push_back((t1 - t0) / (h1 - h0) - TemperatureDeltaGradient);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::CalculatePressureBreakpoints()
{
  PressureBreakpoints[0] = SLpressure;

  for (unsigned int b=0; b<PressureBreakpoints.size()-1; b++) {
    double BaseTemp = StdAtmosTemperatureTable(b+1,1);
    double BaseAlt = StdAtmosTemperatureTable(b+1,0);
    double UpperAlt = StdAtmosTemperatureTable(b+2,0);
    double deltaH = UpperAlt - BaseAlt;
    double Tmb = BaseTemp
                 + TemperatureBias 
                 + (GradientFadeoutAltitude - BaseAlt)*TemperatureDeltaGradient;
    if (LapseRates[b] != 0.00) {
      double Lmb = LapseRates[b];
      double Exp = g0*Mair / (Rstar*Lmb);
      double factor = Tmb/(Tmb + Lmb*deltaH);
      PressureBreakpoints[b+1] = PressureBreakpoints[b]*pow(factor, Exp);
    } else {
      PressureBreakpoints[b+1] = PressureBreakpoints[b]*exp(-g0*Mair*deltaH/(Rstar*Tmb));
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::ResetSLTemperature()
{
  TemperatureBias = TemperatureDeltaGradient = 0.0;
  CalculateLapseRates();
  CalculatePressureBreakpoints();

  SLtemperature = StdSLtemperature;
  CalculateSLSoundSpeedAndDensity();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::ResetSLPressure()
{
  SLpressure  = StdSLpressure;
  CalculateSLDensity();
  CalculatePressureBreakpoints();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::CalculateStdDensityBreakpoints()
{
  StdDensityBreakpoints.clear();
  for (unsigned int i = 0; i < StdPressureBreakpoints.size(); i++)
    StdDensityBreakpoints.push_back(StdPressureBreakpoints[i] / (Reng * StdAtmosTemperatureTable(i + 1, 1)));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::CalculateDensityAltitude(double density, double geometricAlt)
{
  // Work out which layer we're dealing with
  unsigned int b = 0;
  for (; b < StdDensityBreakpoints.size() - 2; b++) {
    if (density >= StdDensityBreakpoints[b + 1])
      break;
  }

  // Get layer properties
  double Tmb = StdAtmosTemperatureTable(b + 1, 1);
  double Hb = StdAtmosTemperatureTable(b + 1, 0);
  double Lmb = StdLapseRates[b];
  double pb = StdDensityBreakpoints[b];

  double density_altitude = 0.0;

  // https://en.wikipedia.org/wiki/Barometric_formula for density solved for H
  if (Lmb != 0.0) {
    double Exp = -1.0 / (1.0 + (g0*Mair)/(Rstar*Lmb));
    density_altitude = Hb + (Tmb / Lmb) * (pow(density / pb, Exp) - 1);
  } else {
    double Factor = -(Rstar*Tmb) / (g0*Mair);
    density_altitude = Hb + Factor * log(density / pb);
  }

  return GeometricAltitude(density_altitude);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::CalculatePressureAltitude(double pressure, double geometricAlt)
{
  // Work out which layer we're dealing with
  unsigned int b = 0;
  for (; b < StdPressureBreakpoints.size() - 2; b++) {
    if (pressure >= StdPressureBreakpoints[b + 1])
      break;
  }

  // Get layer properties
  double Tmb = StdAtmosTemperatureTable(b + 1, 1);
  double Hb = StdAtmosTemperatureTable(b + 1, 0);
  double Lmb = StdLapseRates[b];
  double Pb = StdPressureBreakpoints[b];

  double pressure_altitude = 0.0;

  if (Lmb != 0.00) {
    // Equation 33(a) from ISA document solved for H
    double Exp = -(Rstar*Lmb) / (g0*Mair);
    pressure_altitude = Hb + (Tmb / Lmb) * (pow(pressure / Pb, Exp) - 1);
  } else {
    // Equation 33(b) from ISA document solved for H
    double Factor = -(Rstar*Tmb) / (g0*Mair);
    pressure_altitude = Hb + Factor * log(pressure / Pb);
  }

  return GeometricAltitude(pressure_altitude);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::CalculateVaporPressure(double temperature)
{
  double temperature_degC = temperature/1.8-273.15;
  return a*exp(b*temperature_degC/(c+temperature_degC));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetDewPoint(eTemperature unit, double dewpoint)
{
  VaporPressure = CalculateVaporPressure(ConvertToRankine(dewpoint, unit));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::GetDewPoint(eTemperature to) const
{
  double dewpoint_degC;

  if (VaporPressure <= 0.0)
    dewpoint_degC = -c;
  else {
    double x = log(VaporPressure/a);
    dewpoint_degC = c*x / (b - x);
  }

  return ConvertFromRankine(1.8*(dewpoint_degC + 273.15), to);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetVaporPressure(ePressure unit, double Pa)
{
  VaporPressure = ConvertToPSF(Pa, unit);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::GetVaporPressure(ePressure to) const
{
  return ConvertFromPSF(VaporPressure, to);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::GetSaturatedVaporPressure(ePressure to) const
{
  return ConvertFromPSF(SaturatedVaporPressure, to);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGStandardAtmosphere::GetRelativeHumidity(void) const
{
  return 100.0*VaporPressure/SaturatedVaporPressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::SetRelativeHumidity(double RH)
{
  VaporPressure = 0.01*RH*SaturatedVaporPressure;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGStandardAtmosphere::bind(void)
{
  typedef double (FGStandardAtmosphere::*PMFi)(int) const;
  typedef void (FGStandardAtmosphere::*PMF)(int, double);
  PropertyManager->Tie("atmosphere/delta-T", this, eRankine,
                                    (PMFi)&FGStandardAtmosphere::GetTemperatureBias,
                                    (PMF)&FGStandardAtmosphere::SetTemperatureBias);
  PropertyManager->Tie("atmosphere/SL-graded-delta-T", this, eRankine,
                                    (PMFi)&FGStandardAtmosphere::GetTemperatureDeltaGradient,
                                    (PMF)&FGStandardAtmosphere::SetSLTemperatureGradedDelta);
  PropertyManager->Tie("atmosphere/P-sl-psf", this, ePSF,
                                   (PMFi)&FGStandardAtmosphere::GetPressureSL,
                                   (PMF)&FGStandardAtmosphere::SetPressureSL);
  PropertyManager->Tie("atmosphere/dew-point-R", this, eRankine,
                       (PMFi)&FGStandardAtmosphere::GetDewPoint,
                       (PMF)&FGStandardAtmosphere::SetDewPoint);
  PropertyManager->Tie("atmosphere/vapor-pressure-psf", this, ePSF,
                       (PMFi)&FGStandardAtmosphere::GetVaporPressure,
                       (PMF)&FGStandardAtmosphere::SetVaporPressure);
  PropertyManager->Tie("atmosphere/saturated-vapor-pressure-psf", this, ePSF,
                       (PMFi)&FGStandardAtmosphere::GetSaturatedVaporPressure);
  PropertyManager->Tie("atmosphere/RH", this,
                       &FGStandardAtmosphere::GetRelativeHumidity,
                       &FGStandardAtmosphere::SetRelativeHumidity);
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
    if (from == 0) std::cout << "Instantiated: FGStandardAtmosphere" << std::endl;
    if (from == 1) std::cout << "Destroyed:    FGStandardAtmosphere" << std::endl;
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
