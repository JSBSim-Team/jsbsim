/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGNullAtmosphere.cpp
 Author:       Alexander Kalmykov
 Date started: 04/03/2026
 Purpose:      Models a vacuum for airless bodies (Moon, Mercury, etc.)

 ------------- Copyright (C) 2026  Alexander Kalmykov -------------------------

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
 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGNullAtmosphere.h"
#include "FGFDMExec.h"
#include "input_output/FGLog.h"

namespace JSBSim {

// Cosmic microwave background: ~2.725 K = 4.905 Rankine
static constexpr double VacuumTemperature = 4.905;

FGNullAtmosphere::FGNullAtmosphere(FGFDMExec* fdmex) : FGAtmosphere(fdmex)
{
  Name = "FGNullAtmosphere";

  SLtemperature = VacuumTemperature;
  SLdensity = 0.0;
  SLpressure = 0.0;
  SLsoundspeed = 0.0;

  Temperature = VacuumTemperature;
  Density = 0.0;
  Pressure = 0.0;
  Soundspeed = 0.0;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGNullAtmosphere::Calculate(double altitude)
{
  Temperature = VacuumTemperature;
  Pressure = 0.0;
  Density = 0.0;
  Soundspeed = 0.0;
  Viscosity = 0.0;
  KinematicViscosity = 0.0;
  PressureAltitude = 0.0;
  DensityAltitude = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGNullAtmosphere::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 2) {
    FGLogging log(LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGNullAtmosphere (vacuum)\n";
    if (from == 1) log << "Destroyed:    FGNullAtmosphere\n";
  }
}

} // namespace JSBSim
