/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGWaypoint.h
 Author:       Jon Berndt
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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGWAYPOINT_H
#define FGWAYPOINT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGFCSComponent.h"
#include "math/FGLocation.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a Waypoint object. 
    The waypoint_heading component returns the heading to a specified waypoint
    lat/long from another specified point.
    The waypoint_distance component returns the distance between

    @code
    <waypoint_heading name="component_name" unit="DEG|RAD">
      <target_latitude unit="DEG|RAD">  property_name </target_latitude>
      <target_longitude unit="DEG|RAD"> property_name </target_longitude>
      <source_latitude unit="DEG|RAD">  property_name </source_latitude>
      <source_longitude unit="DEG|RAD"> property_name </source_longitude>
      [<clipto>
        <min> {[-]property name | value} </min>
        <max> {[-]property name | value} </max>
      </clipto>]
      [<output> {property} </output>]
    </waypoint_heading>

    <waypoint_distance name="component_name" unit="FT|M">
      <target_latitude unit="DEG|RAD">  property_name </target_latitude>
      <target_longitude unit="DEG|RAD"> property_name </target_longitude>
      <source_latitude unit="DEG|RAD">  property_name </source_latitude>
      <source_longitude unit="DEG|RAD"> property_name </source_longitude>
      [<radius> {value} </radius>]
      [<clipto>
        <min> {[-]property name | value} </min>
        <max> {[-]property name | value} </max>
      </clipto>]
      [<output> {property} </output>]
    </waypoint_distance>
    @endcode

    @author Jon S. Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGWaypoint  : public FGFCSComponent
{
public:
  FGWaypoint(FGFCS* fcs, Element* element);
  ~FGWaypoint();

  bool Run(void) override;

private:
  FGLocation source;
  std::unique_ptr<FGPropertyValue> target_latitude;
  std::unique_ptr<FGPropertyValue> target_longitude;
  std::unique_ptr<FGPropertyValue> source_latitude;
  std::unique_ptr<FGPropertyValue> source_longitude;
  double target_latitude_unit;
  double target_longitude_unit;
  double source_latitude_unit;
  double source_longitude_unit;
  double radius;
  std::string unit;
  enum {eNone=0, eDeg, eRad, eFeet, eMeters} eUnit;
  enum {eNoType=0, eHeading, eDistance} WaypointType;

  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
