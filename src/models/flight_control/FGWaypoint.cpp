/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGWaypoint.cpp
 Author:       Jon S. Berndt
 Date started: 6/2013

 ------------- Copyright (C) 2013 Jon S. Berndt (jon@jsbsim.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGWaypoint.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

IDENT(IdSrc,"$Id: FGWaypoint.cpp,v 1.5 2014/01/13 10:46:10 ehofman Exp $");
IDENT(IdHdr,ID_WAYPOINT);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGWaypoint::FGWaypoint(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  if      (Type == "WAYPOINT_HEADING")  WaypointType = eHeading;
  else if (Type == "WAYPOINT_DISTANCE") WaypointType = eDistance;

  target_latitude_unit = 1.0;
  target_longitude_unit = 1.0;
  source_latitude_unit = 1.0;
  source_longitude_unit = 1.0;

  if (element->FindElement("target_latitude") ) {
    target_latitude_pNode = PropertyManager->GetNode(element->FindElementValue("target_latitude"));
    if (element->FindElement("target_latitude")->HasAttribute("unit")) {
      if (element->FindElement("target_latitude")->GetAttributeValue("unit") == "DEG") {
        target_latitude_unit = 0.017453293;
      }
    }
  } else {
    throw("Target latitude is required for waypoint component: "+Name);
  }

  if (element->FindElement("target_longitude") ) {
    target_longitude_pNode = PropertyManager->GetNode(element->FindElementValue("target_longitude"));
    if (element->FindElement("target_longitude")->HasAttribute("unit")) {
      if (element->FindElement("target_longitude")->GetAttributeValue("unit") == "DEG") {
        target_longitude_unit = 0.017453293;
      }
    }
  } else {
    throw("Target longitude is required for waypoint component: "+Name);
  }

  if (element->FindElement("source_latitude") ) {
    source_latitude_pNode = PropertyManager->GetNode(element->FindElementValue("source_latitude"));
    if (element->FindElement("source_latitude")->HasAttribute("unit")) {
      if (element->FindElement("source_latitude")->GetAttributeValue("unit") == "DEG") {
        source_latitude_unit = 0.017453293;
      }
    }
  } else {
    throw("Source latitude is required for waypoint component: "+Name);
  }

  if (element->FindElement("source_longitude") ) {
    source_longitude_pNode = PropertyManager->GetNode(element->FindElementValue("source_longitude"));
    if (element->FindElement("source_longitude")->HasAttribute("unit")) {
      if (element->FindElement("source_longitude")->GetAttributeValue("unit") == "DEG") {
        source_longitude_unit = 0.017453293;
      }
    }
  } else {
    throw("Source longitude is required for waypoint component: "+Name);
  }

  if (element->FindElement("radius")) {
    radius = element->FindElementValueAsNumberConvertTo("radius", "FT");
  } else {
    radius = 21144000; // Radius of Earth in feet.
  }

  unit = element->GetAttributeValue("unit");
  if (WaypointType == eHeading) {
    if (!unit.empty()) {
    if      (unit == "DEG") eUnit = eDeg;
    else if (unit == "RAD") eUnit = eRad;
    else throw("Unknown unit "+unit+" in HEADING waypoint component, "+Name);
  } else {
      eUnit = eRad; // Default is radians if unspecified
    }
  } else {
    if (!unit.empty()) {
    if      (unit == "FT") eUnit = eFeet;
    else if (unit == "M")  eUnit = eMeters;
    else throw("Unknown unit "+unit+" in DISTANCE waypoint component, "+Name);
    } else {
      eUnit = eFeet; // Default is feet if unspecified
    }
  }

  FGFCSComponent::bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGWaypoint::~FGWaypoint()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  The calculations, below, implement the Haversine formulas to calculate
//  heading and distance to a set of lat/long coordinates from the current
//  position. The latitude and longitude are expected to be in radian units
//  and are measured from the 0 meridian and the equator, with positive 
//  longitude being east from there, and positive latitude being north.
//
//  The basic equations are (lat1, long1 are source positions; lat2
//  long2 are target positions):
//
//  R = earth’s radius
//  Δlat = lat2 − lat1
//  Δlong = long2 − long1
//
//  For the heading angle calculation:
//
//  θ = atan2(sin(Δlong)∙cos(lat2), cos(lat1)∙sin(lat2) − sin(lat1) ∙cos(lat2)∙cos(Δlong) )
//
//  For the waypoint distance calculation:
//
//  a = sin²(Δlat/2) + cos(lat1)∙cos(lat2)∙sin²(Δlong/2)
//  c = 2∙atan2(√a, √(1−a))
//  d = R∙c

bool FGWaypoint::Run(void )
{
  target_latitude = target_latitude_pNode->getDoubleValue() * target_latitude_unit;
  target_longitude = target_longitude_pNode->getDoubleValue() * target_longitude_unit;
  source_latitude = source_latitude_pNode->getDoubleValue() * source_latitude_unit;
  source_longitude = source_longitude_pNode->getDoubleValue() * source_longitude_unit;

  double delta_lat_rad = target_latitude  - source_latitude;
  double delta_lon_rad = target_longitude - source_longitude;

  if (WaypointType == eHeading) {     // Calculate Heading

    double Y = sin(delta_lon_rad) * cos(target_latitude);
    double X = (cos(source_latitude) * sin(target_latitude))
               - (sin(source_latitude) * cos(target_latitude) * cos(delta_lon_rad));

    double heading_to_waypoint_rad = atan2(Y, X);
    if (heading_to_waypoint_rad < 0) heading_to_waypoint_rad += 2.0*M_PI;

    double heading_to_waypoint = 0;
    if (eUnit == eDeg) heading_to_waypoint = heading_to_waypoint_rad * radtodeg;
    else               heading_to_waypoint = heading_to_waypoint_rad;

    Output = heading_to_waypoint;

  } else {                            // Calculate Distance

    double distance_a = pow(sin(delta_lat_rad/2.0), 2.0)
                        + (cos(source_latitude) * cos(target_latitude)
                          * (pow(sin(delta_lon_rad/2.0), 2.0)));

    double wp_distance = 2.0 * radius * atan2(pow(distance_a, 0.5), pow((1.0 - distance_a), 0.5));

    if (eUnit == eMeters) {
      Output = FeetToMeters(wp_distance);
    } else {
      Output = wp_distance;
    }
  }

  Clip();
  if (IsOutput) SetOutput();

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

void FGWaypoint::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGWaypoint" << endl;
    if (from == 1) cout << "Destroyed:    FGWaypoint" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
