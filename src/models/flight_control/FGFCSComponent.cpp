/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFCSComponent.cpp
 Author:       Jon S. Berndt
 Date started: 11/1999

 ------------- Copyright (C) 2000 -------------

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

#include "FGFCSComponent.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGFCSComponent.cpp,v 1.19 2009/06/13 02:41:58 jberndt Exp $";
static const char *IdHdr = ID_FCSCOMPONENT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCSComponent::FGFCSComponent(FGFCS* _fcs, Element* element) : fcs(_fcs)
{
  Element *input_element, *clip_el;
  Input = Output = clipmin = clipmax = 0.0;
  OutputNode = treenode = 0;
  ClipMinPropertyNode = ClipMaxPropertyNode = 0;
  clipMinSign = clipMaxSign = 1.0;
  IsOutput   = clip = false;
  string input, clip_string;

  PropertyManager = fcs->GetPropertyManager();
  if        (element->GetName() == string("lag_filter")) {
    Type = "LAG_FILTER";
  } else if (element->GetName() == string("lead_lag_filter")) {
    Type = "LEAD_LAG_FILTER";
  } else if (element->GetName() == string("washout_filter")) {
    Type = "WASHOUT_FILTER";
  } else if (element->GetName() == string("second_order_filter")) {
    Type = "SECOND_ORDER_FILTER";
  } else if (element->GetName() == string("integrator")) {
    Type = "INTEGRATOR";
  } else if (element->GetName() == string("summer")) {
    Type = "SUMMER";
  } else if (element->GetName() == string("pure_gain")) {
    Type = "PURE_GAIN";
  } else if (element->GetName() == string("scheduled_gain")) {
    Type = "SCHEDULED_GAIN";
  } else if (element->GetName() == string("aerosurface_scale")) {
    Type = "AEROSURFACE_SCALE";
  } else if (element->GetName() == string("switch")) {
    Type = "SWITCH";
  } else if (element->GetName() == string("kinematic")) {
    Type = "KINEMATIC";
  } else if (element->GetName() == string("deadband")) {
    Type = "DEADBAND";
  } else if (element->GetName() == string("fcs_function")) {
    Type = "FCS_FUNCTION";
  } else if (element->GetName() == string("pid")) {
    Type = "PID";
  } else if (element->GetName() == string("sensor")) {
    Type = "SENSOR";
  } else if (element->GetName() == string("accelerometer")) {
    Type = "ACCELEROMETER";
  } else if (element->GetName() == string("actuator")) {
    Type = "ACTUATOR";
  } else { // illegal component in this channel
    Type = "UNKNOWN";
  }

  Name = element->GetAttributeValue("name");

  FGPropertyManager *tmp=0;

  input_element = element->FindElement("input");
  while (input_element) {
    input = input_element->GetDataLine();
    if (input[0] == '-') {
      InputSigns.push_back(-1.0);
      input.erase(0,1);
    } else {
      InputSigns.push_back( 1.0);
    }
    tmp = PropertyManager->GetNode(input);
    if (tmp) {
      InputNodes.push_back( tmp );
    } else {
      cerr << fgred << "  In component: " << Name << " unknown property "
           << input << " referenced. Aborting" << reset << endl;
      exit(-1);
    }
    input_element = element->FindNextElement("input");
  }

  if (element->FindElement("output")) {
    IsOutput = true;
    OutputNode = PropertyManager->GetNode( element->FindElementValue("output"), true );
    if (!OutputNode) {
      cerr << endl << "  Unable to process property: " << element->FindElementValue("output") << endl;
      throw(string("Invalid output property name in flight control definition"));
    }
  }

  clip_el = element->FindElement("clipto");
  if (clip_el) {
    clip_string = clip_el->FindElementValue("min");
    if (!is_number(clip_string)) { // it's a property
      if (clip_string[0] == '-') {
        clipMinSign = -1.0;
        clip_string.erase(0,1);
      }
      ClipMinPropertyNode = PropertyManager->GetNode( clip_string );
    } else {
      clipmin = clip_el->FindElementValueAsNumber("min");
    }
    clip_string = clip_el->FindElementValue("max");
    if (!is_number(clip_string)) { // it's a property
      if (clip_string[0] == '-') {
        clipMaxSign = -1.0;
        clip_string.erase(0,1);
      }
      ClipMaxPropertyNode = PropertyManager->GetNode( clip_string );
    } else {
      clipmax = clip_el->FindElementValueAsNumber("max");
    }
    clip = true;
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFCSComponent::~FGFCSComponent()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSComponent::SetOutput(void)
{
  OutputNode->setDoubleValue(Output);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCSComponent::Run(void)
{
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSComponent::Clip(void)
{
  if (clip) {
    if (ClipMinPropertyNode != 0) clipmin = clipMinSign*ClipMinPropertyNode->getDoubleValue();
    if (ClipMaxPropertyNode != 0) clipmax = clipMaxSign*ClipMaxPropertyNode->getDoubleValue();
    if (Output > clipmax)      Output = clipmax;
    else if (Output < clipmin) Output = clipmin;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// The old way of naming FCS components allowed upper or lower case, spaces, etc.
// but then the names were modified to fit into a property name heirarchy. This
// was confusing (it wasn't done intentionally - it was a carryover from the early
// design). We now support the direct naming of properties in the FCS component
// name attribute. The old way is supported in code at this time, but deprecated.

void FGFCSComponent::bind(void)
{
  string tmp;
  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  } else {
    tmp = Name;
  }
  PropertyManager->Tie( tmp, this, &FGFCSComponent::GetOutput);
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

void FGFCSComponent::Debug(int from)
{
  string propsign="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) {
      cout << endl << "    Loading Component \"" << Name
                   << "\" of type: " << Type << endl;

      if (clip) {
        if (ClipMinPropertyNode != 0L) {
          if (clipMinSign < 0.0) propsign="-";
          cout << "      Minimum limit: " << propsign << ClipMinPropertyNode->GetName() << endl;
          propsign="";
        } else {
          cout << "      Minimum limit: " << clipmin << endl;
        }
        if (ClipMaxPropertyNode != 0L) {
          if (clipMaxSign < 0.0) propsign="-";
          cout << "      Maximum limit: " << propsign << ClipMaxPropertyNode->GetName() << endl;
          propsign="";
        } else {
          cout << "      Maximum limit: " << clipmax << endl;
        }
      }  
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFCSComponent" << endl;
    if (from == 1) cout << "Destroyed:    FGFCSComponent" << endl;
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
