/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGThruster.cpp
 Author:       Jon S. Berndt
 Date started: 08/23/00
 Purpose:      Encapsulates the thruster object

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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
08/23/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"
#include "FGThruster.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGThruster::FGThruster(FGFDMExec *FDMExec, Element *el, int num ): FGForce(FDMExec)
{
  Element* thruster_element = el->GetParent();
  Element* element;
  FGColumnVector3 location, orientation, pointing;

  Type = ttDirect;
  SetTransformType(FGForce::tCustom);

  Name = el->GetAttributeValue("name");

  GearRatio = 1.0;
  EngineNum = num;
  auto PropertyManager = FDMExec->GetPropertyManager();

// Determine the initial location and orientation of this thruster and load the
// thruster with this information.

  element = thruster_element->FindElement("location");
  if (element)  location = element->FindElementTripletConvertTo("IN");
  else {
    FGXMLLogging log(FDMExec->GetLogger(), thruster_element, LogLevel::ERROR);
    log << LogFormat::RED << "      No thruster location found."
        << LogFormat::RESET << "\n";
  }

  SetLocation(location);

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);

  property_name = base_property_name + "/x-reference-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetLocationX);
  property_name = base_property_name + "/y-reference-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetLocationY);
  property_name = base_property_name + "/z-reference-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetLocationZ);
  property_name = base_property_name + "/x-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetActingLocationX, &FGForce::SetActingLocationX);
  property_name = base_property_name + "/y-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetActingLocationY, &FGForce::SetActingLocationY);
  property_name = base_property_name + "/z-position";
  PropertyManager->Tie(property_name.c_str(), (FGForce*)this, &FGForce::GetActingLocationZ, &FGForce::SetActingLocationZ);

  element = thruster_element->FindElement("pointing");
  if (element)  {

    // This defines a fixed nozzle that has no public interface property to gimbal or reverse it.
    pointing = element->FindElementTripletConvertTo("RAD"); // The specification of RAD here is superfluous,
                                                            // and simply precludes a conversion.
    mT.InitMatrix();
    mT(1,1) = pointing(1);
    mT(2,1) = pointing(2);
    mT(3,1) = pointing(3);

  } else {

    element = thruster_element->FindElement("orient");
    if (element)  orientation = element->FindElementTripletConvertTo("RAD");

    SetAnglesToBody(orientation);
    property_name = base_property_name + "/pitch-angle-rad";
    PropertyManager->Tie( property_name.c_str(), (FGForce *)this, &FGForce::GetPitch, &FGForce::SetPitch);
    property_name = base_property_name + "/yaw-angle-rad";
    PropertyManager->Tie( property_name.c_str(), (FGForce *)this, &FGForce::GetYaw, &FGForce::SetYaw);

    if (el->GetName() == "direct") // this is a direct thruster. At this time
                                   // only a direct thruster can be reversed.
      {
        property_name = base_property_name + "/reverser-angle-rad";
        PropertyManager->Tie( property_name.c_str(), (FGThruster *)this, &FGThruster::GetReverserAngle,
                              &FGThruster::SetReverserAngle);
      }

  }

  ResetToIC();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGThruster::~FGThruster()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGThruster::ResetToIC(void)
{
  ReverserAngle = 0.0;
  Thrust = 0.0;
  SetActingLocation(vXYZn);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGThruster::GetThrusterLabels(int id, const string& delimeter)
{
  std::ostringstream buf;

  buf << Name << " Thrust (engine " << id << " in lbs)";

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGThruster::GetThrusterValues(int id, const string& delimeter)
{
  std::ostringstream buf;

  buf << Thrust;

  return buf.str();
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

void FGThruster::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGThruster\n";
    if (from == 1) log << "Destroyed:    FGThruster\n";
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
