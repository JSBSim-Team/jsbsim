/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputTextFile.cpp
 Author:       Bertrand Coconnier
 Date started: 09/17/11
 Purpose:      Manage output of sim parameters to a text file
 Called by:    FGOutput

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
09/17/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGOutputTextFile.h"
#include "models/FGAerodynamics.h"
#include "models/FGAccelerations.h"
#include "models/FGAtmosphere.h"
#include "models/FGAuxiliary.h"
#include "models/FGPropulsion.h"
#include "models/FGMassBalance.h"
#include "models/FGExternalReactions.h"
#include "models/FGBuoyantForces.h"
#include "models/FGFCS.h"
#include "models/atmosphere/FGWinds.h"
#include "FGXMLElement.h"
#include "string_utilities.h"
#include "FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

bool FGOutputTextFile::Load(Element* el)
{
  if(!FGOutputFile::Load(el))
    return false;

  string type = el->GetAttributeValue("type");
  string delim;
  if (type == "TABULAR") {
    delim = "\t";
  } else {
    delim = ",";
  }

  SetDelimiter(delim);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputTextFile::OpenFile(void)
{
  datafile.clear();
  datafile.open(Filename);
  if (!datafile) {
    FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
    log << LogFormat::RED << LogFormat::BOLD << "\nERROR: unable to open the file "
        << LogFormat::RESET << Filename.c_str()
        << LogFormat::RED << LogFormat::BOLD << "\n       => Output to this file is disabled.\n\n"
        << LogFormat::RESET;
    Disable();
    return false;
  }

  string scratch = "";
  streambuf* buffer = datafile.rdbuf();
  ostream outstream(buffer);

  outstream.precision(10);

  outstream << "Time";
  if (SubSystems & ssSimulation) {
    // Nothing here, yet
  }
  if (SubSystems & ssAerosurfaces) {
    outstream << delimeter;
    outstream << "Aileron Command (norm)" + delimeter;
    outstream << "Elevator Command (norm)" + delimeter;
    outstream << "Rudder Command (norm)" + delimeter;
    outstream << "Flap Command (norm)" + delimeter;
    outstream << "Left Aileron Position (deg)" + delimeter;
    outstream << "Right Aileron Position (deg)" + delimeter;
    outstream << "Elevator Position (deg)" + delimeter;
    outstream << "Rudder Position (deg)" + delimeter;
    outstream << "Flap Position (deg)";
  }
  if (SubSystems & ssRates) {
    outstream << delimeter;
    outstream << "P (deg/s)" + delimeter + "Q (deg/s)" + delimeter + "R (deg/s)" + delimeter;
    outstream << "P dot (deg/s^2)" + delimeter + "Q dot (deg/s^2)" + delimeter + "R dot (deg/s^2)" + delimeter;
    outstream << "P_{inertial} (deg/s)" + delimeter + "Q_{inertial} (deg/s)" + delimeter + "R_{inertial} (deg/s)";
  }
  if (SubSystems & ssVelocities) {
    outstream << delimeter;
    outstream << "q bar (psf)" + delimeter;
    outstream << "Reynolds Number" + delimeter;
    outstream << "V_{Total} (ft/s)" + delimeter;
    outstream << "V_{Inertial} (ft/s)" + delimeter;
    outstream << "UBody" + delimeter + "VBody" + delimeter + "WBody" + delimeter;
    outstream << "UdotBody" + delimeter + "VdotBody" + delimeter + "WdotBody" + delimeter;
    outstream << "UdotBody_i" + delimeter + "VdotBody_i" + delimeter + "WdotBody_i" + delimeter;
    outstream << "BodyAccel_X" + delimeter + "BodyAccel_Y" + delimeter + "BodyAccel_Z" + delimeter;
    outstream << "Aero V_{X Body} (ft/s)" + delimeter + "Aero V_{Y Body} (ft/s)" + delimeter + "Aero V_{Z Body} (ft/s)" + delimeter;
    outstream << "V_{X_{inertial}} (ft/s)" + delimeter + "V_{Y_{inertial}} (ft/s)" + delimeter + "V_{Z_{inertial}} (ft/s)" + delimeter;
    outstream << "V_{X_{ecef}} (ft/s)" + delimeter + "V_{Y_{ecef}} (ft/s)" + delimeter + "V_{Z_{ecef}} (ft/s)" + delimeter;
    outstream << "V_{North} (ft/s)" + delimeter + "V_{East} (ft/s)" + delimeter + "V_{Down} (ft/s)";
  }
  if (SubSystems & ssForces) {
    outstream << delimeter;
    outstream << "F_{Drag} (lbs)" + delimeter + "F_{Side} (lbs)" + delimeter + "F_{Lift} (lbs)" + delimeter;
    outstream << "L/D" + delimeter;
    outstream << "F_{Aero x} (lbs)" + delimeter + "F_{Aero y} (lbs)" + delimeter + "F_{Aero z} (lbs)" + delimeter;
    outstream << "F_{Prop x} (lbs)" + delimeter + "F_{Prop y} (lbs)" + delimeter + "F_{Prop z} (lbs)" + delimeter;
    outstream << "F_{Gear x} (lbs)" + delimeter + "F_{Gear y} (lbs)" + delimeter + "F_{Gear z} (lbs)" + delimeter;
    outstream << "F_{Ext x} (lbs)" + delimeter + "F_{Ext y} (lbs)" + delimeter + "F_{Ext z} (lbs)" + delimeter;
    outstream << "F_{Buoyant x} (lbs)" + delimeter + "F_{Buoyant y} (lbs)" + delimeter + "F_{Buoyant z} (lbs)" + delimeter;
    outstream << "F_{Weight x} (lbs)" + delimeter + "F_{Weight y} (lbs)" + delimeter + "F_{Weight z} (lbs)" + delimeter;
    outstream << "F_{Total x} (lbs)" + delimeter + "F_{Total y} (lbs)" + delimeter + "F_{Total z} (lbs)";
  }
  if (SubSystems & ssMoments) {
    outstream << delimeter;
    outstream << "L_{Aero} (ft-lbs)" + delimeter + "M_{Aero} (ft-lbs)" + delimeter + "N_{Aero} (ft-lbs)" + delimeter;
    outstream << "L_{Aero MRC} (ft-lbs)" + delimeter + "M_{Aero MRC} (ft-lbs)" + delimeter + "N_{Aero MRC} (ft-lbs)" + delimeter;
    outstream << "L_{Prop} (ft-lbs)" + delimeter + "M_{Prop} (ft-lbs)" + delimeter + "N_{Prop} (ft-lbs)" + delimeter;
    outstream << "L_{Gear} (ft-lbs)" + delimeter + "M_{Gear} (ft-lbs)" + delimeter + "N_{Gear} (ft-lbs)" + delimeter;
    outstream << "L_{ext} (ft-lbs)" + delimeter + "M_{ext} (ft-lbs)" + delimeter + "N_{ext} (ft-lbs)" + delimeter;
    outstream << "L_{Buoyant} (ft-lbs)" + delimeter + "M_{Buoyant} (ft-lbs)" + delimeter + "N_{Buoyant} (ft-lbs)" + delimeter;
    outstream << "L_{Total} (ft-lbs)" + delimeter + "M_{Total} (ft-lbs)" + delimeter + "N_{Total} (ft-lbs)";
  }
  if (SubSystems & ssAtmosphere) {
    outstream << delimeter;
    outstream << "Rho (slugs/ft^3)" + delimeter;
    outstream << "Absolute Viscosity" + delimeter;
    outstream << "Kinematic Viscosity" + delimeter;
    outstream << "Temperature (R)" + delimeter;
    outstream << "P_{SL} (psf)" + delimeter;
    outstream << "P_{Ambient} (psf)" + delimeter;
    outstream << "Turbulence Magnitude (ft/sec)" + delimeter;
    outstream << "Turbulence X Direction (deg)" + delimeter;
    outstream << "Wind V_{North} (ft/s)" + delimeter + "Wind V_{East} (ft/s)" + delimeter + "Wind V_{Down} (ft/s)" + delimeter;
    outstream << "Roll Turbulence (deg/sec)" + delimeter + "Pitch Turbulence (deg/sec)" + delimeter + "Yaw Turbulence (deg/sec)";
  }
  if (SubSystems & ssMassProps) {
    outstream << delimeter;
    outstream << "I_{xx}" + delimeter;
    outstream << "I_{xy}" + delimeter;
    outstream << "I_{xz}" + delimeter;
    outstream << "I_{yx}" + delimeter;
    outstream << "I_{yy}" + delimeter;
    outstream << "I_{yz}" + delimeter;
    outstream << "I_{zx}" + delimeter;
    outstream << "I_{zy}" + delimeter;
    outstream << "I_{zz}" + delimeter;
    outstream << "Mass" + delimeter;
    outstream << "Weight" + delimeter;
    outstream << "X_{cg}" + delimeter + "Y_{cg}" + delimeter + "Z_{cg}";
  }
  if (SubSystems & ssPropagate) {
    outstream << delimeter;
    outstream << "Altitude ASL (ft)" + delimeter;
    outstream << "Altitude AGL (ft)" + delimeter;
    outstream << "Phi (deg)" + delimeter + "Theta (deg)" + delimeter + "Psi (deg)" + delimeter;
    outstream << "Q(1)_{LOCAL}" + delimeter + "Q(2)_{LOCAL}" + delimeter + "Q(3)_{LOCAL}" + delimeter + "Q(4)_{LOCAL}" +  delimeter;
    outstream << "Q(1)_{ECEF}" + delimeter + "Q(2)_{ECEF}" + delimeter + "Q(3)_{ECEF}" + delimeter + "Q(4)_{ECEF}" +  delimeter;
    outstream << "Q(1)_{ECI}" + delimeter + "Q(2)_{ECI}" + delimeter + "Q(3)_{ECI}" + delimeter + "Q(4)_{ECI}" +  delimeter;
    outstream << "Alpha (deg)" + delimeter;
    outstream << "Beta (deg)" + delimeter;
    outstream << "Latitude (deg)" + delimeter;
    outstream << "Latitude Geodetic (deg)" + delimeter;
    outstream << "Longitude (deg)" + delimeter;
    outstream << "X_{ECI} (ft)" + delimeter + "Y_{ECI} (ft)" + delimeter + "Z_{ECI} (ft)" + delimeter;
    outstream << "X_{ECEF} (ft)" + delimeter + "Y_{ECEF} (ft)" + delimeter + "Z_{ECEF} (ft)" + delimeter;
    outstream << "Earth Position Angle (deg)" + delimeter;
    outstream << "Distance AGL (ft)" + delimeter;
    outstream << "Terrain Elevation (ft)";
  }
  if (SubSystems & ssAeroFunctions) {
    scratch = Aerodynamics->GetAeroFunctionStrings(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentStrings(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssGroundReactions) {
    outstream << delimeter;
    outstream << GroundReactions->GetGroundReactionStrings(delimeter);
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    outstream << delimeter;
    outstream << Propulsion->GetPropulsionStrings(delimeter);
  }

  for (unsigned int i=0;i<OutputParameters.size();++i) {
    if (!OutputCaptions[i].empty())
      outstream << delimeter << OutputCaptions[i];
    else
      outstream << delimeter << OutputParameters[i]->GetFullyQualifiedName();
  }

  if (!PreFunctions.empty()) {
    for (unsigned int i=0;i<PreFunctions.size();i++) {
      outstream << delimeter << PreFunctions[i]->GetName();
    }
  }

  outstream << endl;
  outstream.flush();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputTextFile::Print(void)
{
  streambuf* buffer;
  string scratch = Filename.utf8Str();

  if (to_upper(scratch) == "COUT") {
    buffer = cout.rdbuf();
  } else {
    buffer = datafile.rdbuf();
  }

  ostream outstream(buffer);

  outstream.precision(10);

  outstream << FDMExec->GetSimTime();
  if (SubSystems & ssSimulation) {
  }
  if (SubSystems & ssAerosurfaces) {
    outstream << delimeter;
    outstream << FCS->GetDaCmd() << delimeter;
    outstream << FCS->GetDeCmd() << delimeter;
    outstream << FCS->GetDrCmd() << delimeter;
    outstream << FCS->GetDfCmd() << delimeter;
    outstream << FCS->GetDaLPos(ofDeg) << delimeter;
    outstream << FCS->GetDaRPos(ofDeg) << delimeter;
    outstream << FCS->GetDePos(ofDeg) << delimeter;
    outstream << FCS->GetDrPos(ofDeg) << delimeter;
    outstream << FCS->GetDfPos(ofDeg);
  }
  if (SubSystems & ssRates) {
    outstream << delimeter;
    outstream << (radtodeg*Propagate->GetPQR()).Dump(delimeter) << delimeter;
    outstream << (radtodeg*Accelerations->GetPQRdot()).Dump(delimeter) << delimeter;
    outstream << (radtodeg*Propagate->GetPQRi()).Dump(delimeter);
  }
  if (SubSystems & ssVelocities) {
    outstream << delimeter;
    outstream << Auxiliary->Getqbar() << delimeter;
    outstream << Auxiliary->GetReynoldsNumber() << delimeter;
    outstream << setprecision(12) << Auxiliary->GetVt() << delimeter;
    outstream << Propagate->GetInertialVelocityMagnitude() << delimeter;
    outstream << setprecision(12) << Propagate->GetUVW().Dump(delimeter) << delimeter;
    outstream << setprecision(12) << Accelerations->GetUVWdot().Dump(delimeter) << delimeter;
    outstream << setprecision(12) << Accelerations->GetUVWidot().Dump(delimeter) << delimeter;
    outstream << setprecision(12) << Accelerations->GetBodyAccel().Dump(delimeter) << delimeter;
    outstream << Auxiliary->GetAeroUVW().Dump(delimeter) << delimeter;
    outstream << Propagate->GetInertialVelocity().Dump(delimeter) << delimeter;
    outstream << Propagate->GetECEFVelocity().Dump(delimeter) << delimeter;
    outstream << Propagate->GetVel().Dump(delimeter);
    outstream.precision(10);
  }
  if (SubSystems & ssForces) {
    outstream << delimeter;
    outstream << Aerodynamics->GetvFw().Dump(delimeter) << delimeter;
    outstream << Aerodynamics->GetLoD() << delimeter;
    outstream << Aerodynamics->GetForces().Dump(delimeter) << delimeter;
    outstream << Propulsion->GetForces().Dump(delimeter) << delimeter;
    outstream << Accelerations->GetGroundForces().Dump(delimeter) << delimeter;
    outstream << ExternalReactions->GetForces().Dump(delimeter) << delimeter;
    outstream << BuoyantForces->GetForces().Dump(delimeter) << delimeter;
    outstream << Accelerations->GetWeight().Dump(delimeter) << delimeter;
    outstream << Accelerations->GetForces().Dump(delimeter);
  }
  if (SubSystems & ssMoments) {
    outstream << delimeter;
    outstream << Aerodynamics->GetMoments().Dump(delimeter) << delimeter;
    outstream << Aerodynamics->GetMomentsMRC().Dump(delimeter) << delimeter;
    outstream << Propulsion->GetMoments().Dump(delimeter) << delimeter;
    outstream << Accelerations->GetGroundMoments().Dump(delimeter) << delimeter;
    outstream << ExternalReactions->GetMoments().Dump(delimeter) << delimeter;
    outstream << BuoyantForces->GetMoments().Dump(delimeter) << delimeter;
    outstream << Accelerations->GetMoments().Dump(delimeter);
  }
  if (SubSystems & ssAtmosphere) {
    const auto Atmosphere = FDMExec->GetAtmosphere();
    outstream << delimeter;
    outstream << Atmosphere->GetDensity() << delimeter;
    outstream << Atmosphere->GetAbsoluteViscosity() << delimeter;
    outstream << Atmosphere->GetKinematicViscosity() << delimeter;
    outstream << Atmosphere->GetTemperature() << delimeter;
    outstream << Atmosphere->GetPressureSL() << delimeter;
    outstream << Atmosphere->GetPressure() << delimeter;
    outstream << Winds->GetTurbMagnitude() << delimeter;
    outstream << Winds->GetTurbDirection() << delimeter;
    outstream << Winds->GetTotalWindNED().Dump(delimeter) << delimeter;
    outstream << (Winds->GetTurbPQR()*radtodeg).Dump(delimeter);
  }
  if (SubSystems & ssMassProps) {
    outstream << delimeter;
    outstream << MassBalance->GetJ().Dump(delimeter) << delimeter;
    outstream << MassBalance->GetMass() << delimeter;
    outstream << MassBalance->GetWeight() << delimeter;
    outstream << MassBalance->GetXYZcg().Dump(delimeter);
  }
  if (SubSystems & ssPropagate) {
    outstream.precision(14);
    outstream << delimeter;
    outstream << Propagate->GetAltitudeASL() << delimeter;
    outstream << Propagate->GetDistanceAGL() << delimeter;
    outstream << (radtodeg*Propagate->GetEuler()).Dump(delimeter) << delimeter;
    outstream << Propagate->GetQuaternion().Dump(delimeter) << delimeter;
    FGQuaternion Qec = Propagate->GetQuaternionECEF();
    outstream << Qec.Dump(delimeter) << delimeter;
    outstream << Propagate->GetQuaternionECI().Dump(delimeter) << delimeter;
    outstream << Auxiliary->Getalpha(inDegrees) << delimeter;
    outstream << Auxiliary->Getbeta(inDegrees) << delimeter;
    outstream << Propagate->GetLatitudeDeg() << delimeter;
    outstream << Propagate->GetGeodLatitudeDeg() << delimeter;
    outstream << Propagate->GetLongitudeDeg() << delimeter;
    outstream.precision(18);
    outstream << ((FGColumnVector3)Propagate->GetInertialPosition()).Dump(delimeter) << delimeter;
    outstream << ((FGColumnVector3)Propagate->GetLocation()).Dump(delimeter) << delimeter;
    outstream.precision(14);
    outstream << Propagate->GetEarthPositionAngleDeg() << delimeter;
    outstream << Propagate->GetDistanceAGL() << delimeter;
    outstream << Propagate->GetTerrainElevation();
    outstream.precision(10);
  }
  if (SubSystems & ssAeroFunctions) {
    scratch = Aerodynamics->GetAeroFunctionValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssGroundReactions) {
    outstream << delimeter;
    outstream << GroundReactions->GetGroundReactionValues(delimeter);
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    outstream << delimeter;
    outstream << Propulsion->GetPropulsionValues(delimeter);
  }

  outstream.precision(18);
  for (unsigned int i=0;i<OutputParameters.size();++i) {
    outstream << delimeter << OutputParameters[i]->GetValue();
  }
  for (unsigned int i=0;i<PreFunctions.size();i++) {
    outstream << delimeter << PreFunctions[i]->getDoubleValue();
  }
  outstream.precision(10);

  outstream << endl;
  outstream.flush();
}
}
