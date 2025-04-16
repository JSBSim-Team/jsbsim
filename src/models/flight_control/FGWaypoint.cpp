/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGWaypoint.cpp
 Author:       Jon S. Berndt
 Date started: 6/2013

 ------------- Copyright (C) 2013 Jon S. Berndt (jon@jsbsim.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGWaypoint.h"
#include "input_output/FGXMLElement.h"
#include "math/FGLocation.h"
#include "models/FGFCS.h"
#include "models/FGInertial.h"
#include "initialization/FGInitialCondition.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGWaypoint::FGWaypoint(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element)
{
  if      (Type == "WAYPOINT_HEADING")  WaypointType = eHeading;
  else if (Type == "WAYPOINT_DISTANCE") WaypointType = eDistance;

  target_latitude_unit = 1.0;
  target_longitude_unit = 1.0;
  source_latitude_unit = 1.0;
  source_longitude_unit = 1.0;
  source = fcs->GetExec()->GetIC()->GetPosition();

  auto PropertyManager = fcs->GetPropertyManager();

  if (element->FindElement("target_latitude") ) {
    target_latitude = std::make_unique<FGPropertyValue>(element->FindElementValue("target_latitude"),
                                                        PropertyManager, element);
    if (element->FindElement("target_latitude")->HasAttribute("unit")) {
      if (element->FindElement("target_latitude")->GetAttributeValue("unit") == "DEG") {
        target_latitude_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Target latitude is required for waypoint component: " << Name << "\n";
    throw err;
  }

  if (element->FindElement("target_longitude") ) {
    target_longitude = std::make_unique<FGPropertyValue>(element->FindElementValue("target_longitude"),
                                                         PropertyManager, element);
    if (element->FindElement("target_longitude")->HasAttribute("unit")) {
      if (element->FindElement("target_longitude")->GetAttributeValue("unit") == "DEG") {
        target_longitude_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Target longitude is required for waypoint component: " << Name << "\n";
    throw err;
  }

  if (element->FindElement("source_latitude") ) {
    source_latitude = std::make_unique<FGPropertyValue>(element->FindElementValue("source_latitude"),
                                                        PropertyManager, element);
    if (element->FindElement("source_latitude")->HasAttribute("unit")) {
      if (element->FindElement("source_latitude")->GetAttributeValue("unit") == "DEG") {
        source_latitude_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Source latitude is required for waypoint component: " << Name << "\n";
    throw err;
  }

  if (element->FindElement("source_longitude") ) {
    source_longitude = std::make_unique<FGPropertyValue>(element->FindElementValue("source_longitude"),
                                                         PropertyManager, element);
    if (element->FindElement("source_longitude")->HasAttribute("unit")) {
      if (element->FindElement("source_longitude")->GetAttributeValue("unit") == "DEG") {
        source_longitude_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Source longitude is required for waypoint component: " << Name << "\n";
    throw err;
  }

  unit = element->GetAttributeValue("unit");
  if (WaypointType == eHeading) {
    if (!unit.empty()) {
      if      (unit == "DEG") eUnit = eDeg;
      else if (unit == "RAD") eUnit = eRad;
      else {
        XMLLogException err(fcs->GetExec()->GetLogger(), element);
        err << "Unknown unit " << unit << " in HEADING waypoint component, " << "\n";
        throw err;
      }
    } else {
      eUnit = eRad; // Default is radians if unspecified
    }
  } else {
    if (!unit.empty()) {
      if      (unit == "FT") eUnit = eFeet;
      else if (unit == "M")  eUnit = eMeters;
      else {
        XMLLogException err(fcs->GetExec()->GetLogger(), element);
        err << "Unknown unit " << unit << " in DISTANCE waypoint component, "
            << Name << "\n";
        throw err;
      }
    } else {
      eUnit = eFeet; // Default is feet if unspecified
    }
  }

  bind(element, PropertyManager.get());
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGWaypoint::~FGWaypoint()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGWaypoint::Run(void )
{
  double source_latitude_rad = source_latitude->GetValue() * source_latitude_unit;
  double source_longitude_rad = source_longitude->GetValue() * source_longitude_unit;
  double target_latitude_rad = target_latitude->GetValue() * target_latitude_unit;
  double target_longitude_rad = target_longitude->GetValue() * target_longitude_unit;
  source.SetPositionGeodetic(source_longitude_rad, source_latitude_rad, 0.0);

  if (fabs(target_latitude_rad) > M_PI/2.0) {
    LogException err(fcs->GetExec()->GetLogger());
    err << "\nTarget latitude in waypoint \"" << Name
        << "\" must be less than or equal to 90 degrees.\n"
        << "(is longitude being mistakenly supplied?)\n\n";
    throw err;
  }

  if (fabs(source_latitude_rad) > M_PI/2.0) {
    LogException err(fcs->GetExec()->GetLogger());
    err << "\nSource latitude in waypoint \"" << Name
        << "\" must be less than or equal to 90 degrees.\n"
        << "(is longitude being mistakenly supplied?)\n\n";
    throw err;
  }

  if (WaypointType == eHeading) {     // Calculate Heading

    double heading_to_waypoint_rad = source.GetHeadingTo(target_longitude_rad,
                                                         target_latitude_rad);

    if (eUnit == eDeg) Output = heading_to_waypoint_rad * radtodeg;
    else               Output = heading_to_waypoint_rad;

  } else {                            // Calculate Distance

    double wp_distance = source.GetDistanceTo(target_longitude_rad,
                                              target_latitude_rad);
    if (eUnit == eMeters) Output = FeetToMeters(wp_distance);
    else                  Output = wp_distance;

  }

  Clip();
  SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicitly requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGWaypoint::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGWaypoint\n";
    if (from == 1) log << "Destroyed:    FGWaypoint\n";
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
