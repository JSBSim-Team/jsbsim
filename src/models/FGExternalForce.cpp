/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Source:       FGExternalForce.cpp
 Author:       Jon Berndt, Dave Culp
 Date started: 9/21/07

 ------------- Copyright (C) 2007  Jon S. Berndt (jon@jsbsim.org) -------------

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
9/21/07  JB   Created

<external_reactions>

    <!-- Interface properties, a.k.a. property declarations -->
    <property> ... </property>

    <force name="name" frame="BODY|LOCAL|WIND">

      <function> ... </function>

      <location unit="units"> <!-- location -->
        <x> value </x>
        <y> value </y>
        <z> value </z>
      </location>
      <direction> <!-- optional for initial direction vector -->
        <x> value </x>
        <y> value </y>
        <z> value </z>
      </direction>
    </force>

    <moment name="name" frame="BODY|LOCAL|WIND">

      <function> ... </function>

      <direction> <!-- optional for initial direction vector -->
        <x> value </x>
        <y> value </y>
        <z> value </z>
      </direction>
    </force>

</external_reactions>

*/

#include "FGFDMExec.h"
#include "FGExternalForce.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyVector3::FGPropertyVector3(FGPropertyManager* pm,
                                     const std::string& baseName,
                                     const std::string& xcmp,
                                     const std::string& ycmp,
                                     const std::string& zcmp)
{
  data[0] = pm->GetNode(baseName + "/" + xcmp, true);
  data[1] = pm->GetNode(baseName + "/" + ycmp, true);
  data[2] = pm->GetNode(baseName + "/" + zcmp, true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGParameter* FGExternalForce::bind(Element *el, const string& magName,
                                   FGPropertyVector3& v)
{
  // Set frame (from FGForce).
  string sFrame = el->GetAttributeValue("frame");
  if (sFrame.empty()) {
    FGXMLLogging log(fdmex->GetLogger(), el, LogLevel::WARN);
    log << "No frame specified for external " << el->GetName() << ", \""
        << Name << "\".\nFrame set to Body\n";
    ttype = tNone;
  } else if (sFrame == "BODY") {
    ttype = tNone;
  } else if (sFrame == "LOCAL") {
    ttype = tLocalBody;
  } else if (sFrame == "WIND") {
    ttype = tWindBody;
  } else if (sFrame == "INERTIAL") {
    ttype = tInertialBody;
  } else {
    FGXMLLogging log(fdmex->GetLogger(), el, LogLevel::WARN);
    log << "Invalid frame specified for external " << el->GetName() << ", \""
        << Name << "\".\nFrame set to Body\n";
    ttype = tNone;
  }

  Element* direction_element = el->FindElement("direction");
  if (!direction_element) {
    FGXMLLogging log(fdmex->GetLogger(), el, LogLevel::WARN);
    log << "No direction element specified in " << el->GetName()
        << " object. Default is (0,0,0).\n";
  } else {
    FGColumnVector3 direction = direction_element->FindElementTripletConvertTo("IN");
    direction.Normalize();
    v = direction;
  }

  // The value sent to the sim through the external_reactions/{force
  // name}/magnitude property will be multiplied against the unit vector, which
  // can come in initially in the direction vector. The frame in which the
  // vector is defined is specified with the frame attribute. The vector is
  // normalized to magnitude 1.

  Element* function_element = el->FindElement("function");
  if (function_element) {
    return new FGFunction(fdmex, function_element);
  } else {
    auto pm = fdmex->GetPropertyManager();
    SGPropertyNode* node = pm->GetNode(magName, true);
    return new FGPropertyValue(node);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGExternalForce::setForce(Element *el)
{
  auto PropertyManager = fdmex->GetPropertyManager();
  Name = el->GetAttributeValue("name");
  string BasePropertyName = "external_reactions/" + Name;

  forceDirection = FGPropertyVector3(PropertyManager.get(), BasePropertyName,
                                     "x", "y", "z");
  forceMagnitude = bind(el, BasePropertyName + "/magnitude", forceDirection);

  Element* location_element = el->FindElement("location");
  if (!location_element) {
    FGXMLLogging log(fdmex->GetLogger(), el, LogLevel::WARN);
    log << "No location element specified in force object.\n";
  } else {
    FGColumnVector3 location = location_element->FindElementTripletConvertTo("IN");
    SetLocation(location);
  }
  PropertyManager->Tie( BasePropertyName + "/location-x-in", (FGForce*)this,
                        &FGForce::GetLocationX, &FGForce::SetLocationX);
  PropertyManager->Tie( BasePropertyName + "/location-y-in", (FGForce*)this,
                        &FGForce::GetLocationY, &FGForce::SetLocationY);
  PropertyManager->Tie( BasePropertyName + "/location-z-in", (FGForce*)this,
                        &FGForce::GetLocationZ, &FGForce::SetLocationZ);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGExternalForce::setMoment(Element *el)
{
  auto PropertyManager = fdmex->GetPropertyManager();
  Name = el->GetAttributeValue("name");
  string BasePropertyName = "external_reactions/" + Name;

  momentDirection = FGPropertyVector3(PropertyManager.get(), BasePropertyName,
                                      "l", "m", "n");
  momentMagnitude = bind(el, BasePropertyName + "/magnitude-lbsft",
                         momentDirection);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGExternalForce::~FGExternalForce()
{
  delete forceMagnitude;
  delete momentMagnitude;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGExternalForce::GetBodyForces(void)
{
  if (forceMagnitude)
    vFn = forceMagnitude->GetValue() * forceDirection;

  if (momentMagnitude)
    vMn = Transform() * (momentMagnitude->GetValue() * momentDirection);

  return FGForce::GetBodyForces();
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

void FGExternalForce::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "    " << Name;
      log << "\n    Frame: ";
      switch(ttype) {
      case tNone:
        log << "BODY";
        break;
      case tLocalBody:
        log << "LOCAL";
        break;
      case tWindBody:
        log << "WIND";
        break;
      case tInertialBody:
        log << "INERTIAL";
        break;
      default:
        log << "ERROR/UNKNOWN";
      }
      log << "\n    Location: (" << fixed << vXYZn(eX) << ", " << vXYZn(eY)
          << ", " << vXYZn(eZ) << ")\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGExternalForce\n";
    if (from == 1) log << "Destroyed:    FGExternalForce\n";
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
