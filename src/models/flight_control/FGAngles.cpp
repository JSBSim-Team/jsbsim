/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAngles.cpp
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
Created: 6/2013 Jon S. Berndt

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  The Included Angle to Heading algorithm is used to find the smallest included
  angle (the angle less than or equal to 180 degrees) to a specified heading
  from the current heading. The sense of the rotation to get to that angle is
  also calculated (positive 1 for a clockwise rotation, negative 1 for counter-
  clockwise).

  The angle to the heading is calculated as follows:

  Given an angle phi:

  V = cos(phi)i + sin(phi)j     (this is a unit vector)

  The dot product for two, 2D vectors is written:

  V1*V2 = |V1||V2|cos(phi)

  Since the magnitude of a unit vector is 1, we can write the equation as
  follows:

  V1*V2 = cos(phi)

  or,

  phi  = acos(V1*V2)

  or,

  phi  = acos[ cos(phi1)cos(phi2) + sin(phi1)sin(phi2) ]

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAngles.h"
#include "models/FGFCS.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAngles::FGAngles(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  source_angle = 0.0;
  target_angle = 0.0;
  source_angle_unit = 1.0;
  target_angle_unit = 1.0;
  output_unit = 1.0;

  auto PropertyManager = fcs->GetPropertyManager();

  if (element->FindElement("target_angle") ) {
    target_angle_pNode = PropertyManager->GetNode(element->FindElementValue("target_angle"));
    if (element->FindElement("target_angle")->HasAttribute("unit")) {
      if (element->FindElement("target_angle")->GetAttributeValue("unit") == "DEG") {
        target_angle_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Target angle is required for Angles component: " << Name << "\n";
    throw err;
  }

  if (element->FindElement("source_angle") ) {
    source_angle_pNode = PropertyManager->GetNode(element->FindElementValue("source_angle"));
    if (element->FindElement("source_angle")->HasAttribute("unit")) {
      if (element->FindElement("source_angle")->GetAttributeValue("unit") == "DEG") {
        source_angle_unit = 0.017453293;
      }
    }
  } else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Source angle is required for Angles component: " << Name << "\n";
    throw err;
  }

  unit = element->GetAttributeValue("unit");
  if (!unit.empty()) {
    if      (unit == "DEG") output_unit = 180.0/M_PI;
    else if (unit == "RAD") output_unit = 1.0;
    else {
      XMLLogException err(fcs->GetExec()->GetLogger(), element);
      err << "Unknown unit " << unit << " in angle component, " << Name << "\n";
      throw err;
    }
  } else {
    output_unit = 1.0; // Default is radians (1.0) if unspecified
  }

  bind(element, PropertyManager.get());
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAngles::~FGAngles()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAngles::Run(void )
{
  source_angle = source_angle_pNode->getDoubleValue() * source_angle_unit;
  target_angle = target_angle_pNode->getDoubleValue() * target_angle_unit;

  double x1 = cos(source_angle);
  double y1 = sin(source_angle);
  double x2 = cos(target_angle);
  double y2 = sin(target_angle);

  double x1x2_y1y2 = max(-1.0, min(x1*x2 + y1*y2, 1.0));
  double angle_to_heading_rad = acos(x1x2_y1y2);
  double x1y2 = x1*y2;
  double x2y1 = x2*y1;

  if (x1y2 >= x2y1) Output =  angle_to_heading_rad * output_unit;
  else              Output = -angle_to_heading_rad * output_unit;

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

void FGAngles::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGAngles\n";
    if (from == 1) log << "Destroyed:    FGAngles\n";
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
