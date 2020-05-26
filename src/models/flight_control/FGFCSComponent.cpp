/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFCSComponent.cpp
 Author:       Jon S. Berndt
 Date started: 11/1999

 ------------- Copyright (C) 2000 -------------

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

#include "FGFCSComponent.h"
#include "models/FGFCS.h"
#include "math/FGParameterValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCSComponent::FGFCSComponent(FGFCS* _fcs, Element* element) : fcs(_fcs)
{
  Input = Output = delay_time = 0.0;
  delay = index = 0;
  ClipMin = ClipMax = new FGRealValue(0.0);
  clip = cyclic_clip = false;
  dt = fcs->GetChannelDeltaT();

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
  } else if (element->GetName() == string("magnetometer")) {
    Type = "MAGNETOMETER";
  } else if (element->GetName() == string("gyro")) {
    Type = "GYRO";
  } else if (element->GetName() == string("actuator")) {
    Type = "ACTUATOR";
  } else if (element->GetName() == string("waypoint_heading")) {
    Type = "WAYPOINT_HEADING";
  } else if (element->GetName() == string("waypoint_distance")) {
    Type = "WAYPOINT_DISTANCE";
  } else if (element->GetName() == string("angle")) {
    Type = "ANGLE";
  } else if (element->GetName() == string("distributor")) {
    Type = "DISTRIBUTOR";
  } else { // illegal component in this channel
    Type = "UNKNOWN";
  }

  Name = element->GetAttributeValue("name");

  Element *init_element = element->FindElement("init");
  while (init_element) {
    InitNodes.push_back(new FGPropertyValue(init_element->GetDataLine(),
                                            PropertyManager ));
    init_element = element->FindNextElement("init");
  }
  
  Element *input_element = element->FindElement("input");
  while (input_element) {
    InputNodes.push_back(new FGPropertyValue(input_element->GetDataLine(),
                                             PropertyManager ));

    input_element = element->FindNextElement("input");
  }

  Element *out_elem = element->FindElement("output");
  while (out_elem) {
    string output_node_name = out_elem->GetDataLine();
    bool node_exists = PropertyManager->HasNode(output_node_name);
    FGPropertyNode* OutputNode = PropertyManager->GetNode( output_node_name, true );
    if (!OutputNode) {
      cerr << out_elem->ReadFrom() << "  Unable to process property: "
           << output_node_name << endl;
      throw(string("Invalid output property name in flight control definition"));
    }
    OutputNodes.push_back(OutputNode);
    // If the node has just been created then it must be initialized to a
    // sensible value since FGPropertyNode::GetNode() does not take care of
    // that.  If the node was already existing, its current value is kept
    // unchanged.
    if (!node_exists)
      OutputNode->setDoubleValue(Output);
    out_elem = element->FindNextElement("output");
  }

  Element* delay_elem = element->FindElement("delay");
  if ( delay_elem ) {
    delay_time = delay_elem->GetDataAsNumber();
    string delayType = delay_elem->GetAttributeValue("type");
    if (delayType.length() > 0) {
      if (delayType == "time") {
        delay = (unsigned int)(delay_time / dt);
      } else if (delayType == "frames") {
        delay = (unsigned int)delay_time;
      } else {
        cerr << "Unallowed delay type" << endl;
      }
    } else {
      delay = (unsigned int)(delay_time / dt);
    }
    output_array.resize(delay);
    for (unsigned int i=0; i<delay; i++) output_array[i] = 0.0;
  }

  Element *clip_el = element->FindElement("clipto");
  if (clip_el) {
    Element* el = clip_el->FindElement("min");
    if (!el) {
      cerr << clip_el->ReadFrom()
           << "Element <min> is missing, <clipto> is ignored." << endl;
      return;
    }

    ClipMin = new FGParameterValue(el, PropertyManager);

    el = clip_el->FindElement("max");
    if (!el) {
      cerr << clip_el->ReadFrom()
           << "Element <max> is missing, <clipto> is ignored." << endl;
      ClipMin = nullptr;
      return;
    }

    ClipMax = new FGParameterValue(el, PropertyManager);

    if (clip_el->GetAttributeValue("type") == "cyclic")
      cyclic_clip = true;

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

void FGFCSComponent::ResetPastStates(void)
{
  index = 0;
  for (auto &elm: output_array)
    elm = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSComponent::SetOutput(void)
{
  for (auto node: OutputNodes)
    node->setDoubleValue(Output);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSComponent::Delay(void)
{
  if (fcs->GetTrimStatus()) {
    // Update the whole history while trim routines are executing.
    // Don't want to model delays while calculating a trim solution.
    std::fill(output_array.begin(), output_array.end(), Output);
  }
  else {
    output_array[index] = Output;
    if ((unsigned int)index == delay-1) index = 0;
    else index++;
    Output = output_array[index];
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSComponent::Clip(void)
{
  if (clip) {
    double vmin = ClipMin->GetValue();
    double vmax = ClipMax->GetValue();
    double range = vmax - vmin;

    if (range < 0.0) {
      cerr << "Trying to clip with a max value " << ClipMax->GetName()
           << " lower than the min value " << ClipMin->GetName()
           << endl;
      throw("JSBSim aborts");
      return;
    }

    if (cyclic_clip && range != 0.0) {
      double value = Output - vmin;
      Output = fmod(value, range) + vmin;
      if (Output < vmin)
        Output += range;
    }
    else
      Output = Constrain(vmin, Output, vmax);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// The old way of naming FCS components allowed upper or lower case, spaces,
// etc. but then the names were modified to fit into a property name
// hierarchy. This was confusing (it wasn't done intentionally - it was a
// carryover from the early design). We now support the direct naming of
// properties in the FCS component name attribute. The old way is supported in
// code at this time, but deprecated.

void FGFCSComponent::bind(Element* el)
{
  string tmp;
  if (Name.find("/") == string::npos)
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  else
    tmp = Name;

  bool node_exists = PropertyManager->HasNode(tmp);
  FGPropertyNode* node = PropertyManager->GetNode(tmp, true);

  if (node) {
    OutputNodes.push_back(node);
    // If the node has just been created then it must be initialized to a
    // sensible value since FGPropertyNode::GetNode() does not take care of
    // that.  If the node was already existing, its current value is kept
    // unchanged.
    if (!node_exists)
      node->setDoubleValue(Output);
  }
  else {
    cerr << el->ReadFrom()
         << "Could not get or create property " << tmp << endl;
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

void FGFCSComponent::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) {
      cout << endl << "    Loading Component \"" << Name
                   << "\" of type: " << Type << endl;

      if (clip) {
        cout << "      Minimum limit: " << ClipMin->GetName() << endl;
        cout << "      Maximum limit: " << ClipMax->GetName() << endl;
      }  
      if (delay > 0) cout <<"      Frame delay: " << delay
                                   << " frames (" << delay*dt << " sec)" << endl;
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
    }
  }
}
}
