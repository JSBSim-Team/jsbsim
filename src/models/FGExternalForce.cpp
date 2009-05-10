/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Source:       FGExternalForce.cpp
 Author:       Jon Berndt, Dave Culp
 Date started: 9/21/07

 ------------- Copyright (C) 2007  Jon S. Berndt (jsb@hal-pc.org) -------------

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

</external_reactions>

*/

#include "FGExternalForce.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGExternalForce.cpp,v 1.8 2009/05/10 10:59:48 andgi Exp $";
static const char *IdHdr = ID_EXTERNALFORCE;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGExternalForce::FGExternalForce(FGFDMExec *FDMExec, Element *el, int index): FGForce(FDMExec)
{
  Element* location_element=0;
  Element* direction_element=0;
  Element* function_element=0;
  string sFrame;
  string BasePropertyName;
  FGColumnVector3 location;
  Magnitude_Function = 0;
  magnitude = 0.0;
  azimuth = 0.0;

  PropertyManager = fdmex->GetPropertyManager();
  Name = el->GetAttributeValue("name");
  BasePropertyName = "external_reactions/" + Name;

  // The value sent to the sim through the external_forces/{force name}/magnitude
  // property will be multiplied against the unit vector, which can come in
  // initially in the direction vector. The frame in which the vector is defined
  // is specified with the frame attribute. The vector is normalized to magnitude 1.

  function_element = el->FindElement("function");
  if (function_element) {
    Magnitude_Function = new FGFunction(PropertyManager, function_element);
  } else {
    PropertyManager->Tie( BasePropertyName + "/magnitude",(FGExternalForce*)this, &FGExternalForce::GetMagnitude, &FGExternalForce::SetMagnitude);
    Magnitude_Node = PropertyManager->GetNode(BasePropertyName + "/magnitude");
  }


  // Set frame (from FGForce).
  sFrame = el->GetAttributeValue("frame");
  if (sFrame.empty()) {
    cerr << "No frame specified for external force, \"" << Name << "\"." << endl;
    cerr << "Frame set to Body" << endl;
    ttype = tNone;
  } else if (sFrame == "BODY") {
    ttype = tNone;
  } else if (sFrame == "LOCAL") {
    ttype = tLocalBody;
    PropertyManager->Tie( BasePropertyName + "/azimuth", (FGExternalForce*)this, &FGExternalForce::GetAzimuth, &FGExternalForce::SetAzimuth);
  } else if (sFrame == "WIND") {
    ttype = tWindBody;
  } else {
    cerr << "Invalid frame specified for external force, \"" << Name << "\"." << endl;
    cerr << "Frame set to Body" << endl;
    ttype = tNone;
  }
  PropertyManager->Tie( BasePropertyName + "/x", (FGExternalForce*)this, &FGExternalForce::GetX, &FGExternalForce::SetX);
  PropertyManager->Tie( BasePropertyName + "/y", (FGExternalForce*)this, &FGExternalForce::GetY, &FGExternalForce::SetY);
  PropertyManager->Tie( BasePropertyName + "/z", (FGExternalForce*)this, &FGExternalForce::GetZ, &FGExternalForce::SetZ);

  location_element = el->FindElement("location");
  if (!location_element) {
    cerr << "No location element specified in force object." << endl;
  } else {
    location = location_element->FindElementTripletConvertTo("IN");
    SetLocation(location);
  }

  direction_element = el->FindElement("direction");
  if (!direction_element) {
    cerr << "No direction element specified in force object. Default is (0,0,0)." << endl;
  } else {
    vDirection = direction_element->FindElementTripletConvertTo("IN");
    vDirection.Normalize();
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Copy constructor

FGExternalForce::FGExternalForce(const FGExternalForce& extForce) : FGForce(extForce)
{
  magnitude = extForce.magnitude;
  Frame = extForce.Frame;
  vDirection = extForce.vDirection;
  Name = extForce.Name;
  BasePropertyName = extForce.BasePropertyName;
  PropertyManager = extForce.PropertyManager;
}
  
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGExternalForce::~FGExternalForce()
{
  unbind( PropertyManager->GetNode("external_reactions"));
  delete Magnitude_Function;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGExternalForce::SetMagnitude(double mag)
{
  magnitude = mag;
  vFn = vDirection*mag;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGExternalForce::GetBodyForces(void)
{
  if (Magnitude_Function) {
    double mag = Magnitude_Function->GetValue();
    SetMagnitude(mag);
  }
  
  return FGForce::GetBodyForces();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGExternalForce::unbind(FGPropertyManager *node)
{
  int N = node->nChildren();
  for (int i=0; i<N; i++) {
    if (node->getChild(i)->nChildren() ) {
      unbind( (FGPropertyManager*)node->getChild(i) );
    } else if ( node->getChild(i)->isTied() ) {
      node->getChild(i)->untie();
    }
  }
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
      cout << "    " << Name << endl;
      cout << "    Frame: " << Frame << endl;
      cout << "    Location: (" << vXYZn(eX) << ", " << vXYZn(eY) << ", " << vXYZn(eZ) << ")" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGExternalForce" << endl;
    if (from == 1) cout << "Destroyed:    FGExternalForce" << endl;
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
