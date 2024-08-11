/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGInertial.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the inertial frame forces (coriolis and centrifugal)

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGInertial.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"
#include "GeographicLib/Geodesic.hpp"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGInertial::FGInertial(FGFDMExec* fgex)
  : FGModel(fgex)
{
  Name = "Earth";

  // Earth defaults
  double RotationRate    = 0.00007292115;
  GM              = 14.0764417572E15;   // WGS84 value
  J2              = 1.08262982E-03;     // WGS84 value for J2
  a               = 20925646.32546;     // WGS84 semimajor axis length in feet
  b               = 20855486.5951;      // WGS84 semiminor axis length in feet
  gravType = gtWGS84;

  // Lunar defaults
  /*
  double RotationRate    = 0.0000026617;
  GM              = 1.7314079E14;         // Lunar GM
  J2              = 2.033542482111609E-4; // value for J2
  a               = 5702559.05;           // semimajor axis length in feet
  b               = 5695439.63;           // semiminor axis length in feet
  */

  vOmegaPlanet = { 0.0, 0.0, RotationRate };
  GroundCallback = std::make_unique<FGDefaultGroundCallback>(a, b);

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInertial::~FGInertial(void)
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::Load(Element* el)
{
  if (!Upload(el, true)) return false;

  Name = el->GetAttributeValue("name");

  if (el->FindElement("semimajor_axis"))
    a = el->FindElementValueAsNumberConvertTo("semimajor_axis", "FT");
  else if (el->FindElement("equatorial_radius"))
    a = el->FindElementValueAsNumberConvertTo("equatorial_radius", "FT");
  if (el->FindElement("semiminor_axis"))
    b = el->FindElementValueAsNumberConvertTo("semiminor_axis", "FT");
  else if (el->FindElement("polar_radius"))
    b = el->FindElementValueAsNumberConvertTo("polar_radius", "FT");
  // Trigger GeographicLib exceptions if the equatorial or polar radii are
  // ill-defined.
  // This intercepts the exception before being thrown by a destructor.
  GeographicLib::Geodesic geod(a, 1.-b/a);

  if (el->FindElement("rotation_rate")) {
    double RotationRate = el->FindElementValueAsNumberConvertTo("rotation_rate", "RAD/SEC");
    vOmegaPlanet = {0., 0., RotationRate};
  }
  if (el->FindElement("GM"))
    GM = el->FindElementValueAsNumberConvertTo("GM", "FT3/SEC2");
  if (el->FindElement("J2"))
    J2 = el->FindElementValueAsNumber("J2"); // Dimensionless

  GroundCallback->SetEllipse(a, b);

  // Messages to warn the user about possible inconsistencies.
  if (debug_lvl > 0) {
    FGLogging log(FDMExec->GetLogger(), LogLevel::WARN);
    if (a != b && J2 == 0.0)
      log << "Gravitational constant J2 is null for a non-spherical planet." << endl;
    if (a == b && J2 != 0.0)
      log << "Gravitational constant J2 is non-zero for a spherical planet." << endl;
  }

  Debug(2);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInertial::Run(bool Holding)
{
  // Fast return if we have nothing to do ...
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false;

  // Gravitation accel
  switch (gravType) {
  case gtStandard:
    {
      double radius = in.Position.GetRadius();
      vGravAccel = -(GetGAccel(radius) / radius) * in.Position;
    }
    break;
  case gtWGS84:
    vGravAccel = GetGravityJ2(in.Position);
    break;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGInertial::GetTl2ec(const FGLocation& location) const
{
  FGColumnVector3 North, Down, East{-location(eY), location(eX), 0.};

  switch (gravType) {
  case gtStandard:
    {
      Down = location;
      Down *= -1.0;
    }
    break;
  case gtWGS84:
    {
      FGLocation sea_level = location;
      sea_level.SetPositionGeodetic(location.GetLongitude(),
                                    location.GetGeodLatitudeRad(), 0.0);
      Down = GetGravityJ2(location);
      Down -= vOmegaPlanet*(vOmegaPlanet*sea_level);}
    }
  Down.Normalize();
  East.Normalize();
  North = East*Down;

  return FGMatrix33{North(eX), East(eX), Down(eX),
                    North(eY), East(eY), Down(eY),
                    North(eZ), 0.0,      Down(eZ)};
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGInertial::GetGAccel(double r) const
{
  return GM/(r*r);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// Calculate the WGS84 gravitation value in ECEF frame. Pass in the ECEF
// position via the position parameter. The J2Gravity value returned is in ECEF
// frame, and therefore may need to be expressed (transformed) in another frame,
// depending on how it is used. See Stevens and Lewis eqn. 1.4-16.

FGColumnVector3 FGInertial::GetGravityJ2(const FGLocation& position) const
{
  FGColumnVector3 J2Gravity;

  // Gravitation accel
  double r = position.GetRadius();
  double sinLat = sin(position.GetLatitude());

  double adivr = a/r;
  double preCommon = 1.5*J2*adivr*adivr;
  double xy = 1.0 - 5.0*(sinLat*sinLat);
  double z = 3.0 - 5.0*(sinLat*sinLat);
  double GMOverr2 = GM/(r*r);

  J2Gravity(1) = -GMOverr2 * ((1.0 + (preCommon * xy)) * position(eX)/r);
  J2Gravity(2) = -GMOverr2 * ((1.0 + (preCommon * xy)) * position(eY)/r);
  J2Gravity(3) = -GMOverr2 * ((1.0 + (preCommon *  z)) * position(eZ)/r);

  return J2Gravity;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGInertial::SetAltitudeAGL(FGLocation& location, double altitudeAGL)
{
  FGColumnVector3 vDummy;
  FGLocation contact;
  contact.SetEllipse(a, b);
  GroundCallback->GetAGLevel(location, contact, vDummy, vDummy, vDummy);
  double groundHeight = contact.GetGeodAltitude();
  double longitude = location.GetLongitude();
  double geodLat = location.GetGeodLatitudeRad();
  location.SetPositionGeodetic(longitude, geodLat,
                               groundHeight + altitudeAGL);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGInertial::SetGravityType(int gt)
{
  // Messages to warn the user about possible inconsistencies.
  FGLogging log(FDMExec->GetLogger(), LogLevel::WARN);
  switch (gt)
  {
  case eGravType::gtStandard:
    if (a != b)
      log << "Standard gravity model has been set for a non-spherical planet" << endl;
    break;
  case eGravType::gtWGS84:
    if (J2 == 0.0)
      log << "WGS84 gravity model has been set without specifying the J2 gravitational constant." << endl;
  }

  gravType = gt;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGInertial::bind(void)
{
  PropertyManager->Tie("inertial/sea-level-radius_ft", &in.Position,
                       &FGLocation::GetSeaLevelRadius);
  PropertyManager->Tie("simulation/gravity-model", this, &FGInertial::GetGravityType,
                       &FGInertial::SetGravityType);
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

void FGInertial::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) {} // Constructor
    if (from == 2) { // Loading
      log << endl << "  Planet " << Name << endl
          << "    Semi major axis: " << a << endl
          << "    Semi minor axis: " << b << endl
          << "    Rotation rate  : " << scientific << vOmegaPlanet(eZ) << endl
          << "    GM             : " << GM << endl
          << "    J2             : " << J2 << endl << defaultfloat << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGInertial" << endl;
    if (from == 1) log << "Destroyed:    FGInertial" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
