/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropertyReader.cpp
 Author:       Bertrand Coconnier
 Date started: 12/30/13
 Purpose:      Read and manage properties from XML data

  ------------- Copyright (C) 2013 Bertrand Coconnier -------------

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
This class reads and manages properties defined in XML data

HISTORY
--------------------------------------------------------------------------------
12/30/13   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropertyReader.h"
#include "FGPropertyManager.h"
#include "FGXMLElement.h"
#include "FGJSBBase.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropertyReader::ResetToIC(void)
{
  for (auto v: interface_prop_initial_value) {
    SGPropertyNode* node = v.first;
    if (!node->getAttribute(SGPropertyNode::PRESERVE))
      node->setDoubleValue(v.second);
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyReader::Load(Element* el, FGPropertyManager* PM, bool override_props)
{
  Element *property_element = el->FindElement("property");
  if (property_element && FGJSBBase::debug_lvl > 0) {
    FGLogging log(LogLevel::DEBUG);
    log << "\n    ";
    if (override_props)
      log << "Overriding";
    else
      log << "Declared";
    log << " properties\n\n";
  }

  while (property_element) {
    SGPropertyNode* node = nullptr;
    double value=0.0;
    bool has_value_attribute = !property_element->GetAttributeValue("value").empty();

    if (has_value_attribute)
      value = property_element->GetAttributeValueAsNumber("value");

    string interface_property_string = property_element->GetDataLine();
    if (PM->HasNode(interface_property_string)) {
      node = PM->GetNode(interface_property_string);
      if (override_props) {

        if (FGJSBBase::debug_lvl > 0) {
          FGXMLLogging log(property_element, LogLevel::DEBUG);
          if (interface_prop_initial_value.find(node) == interface_prop_initial_value.end()) {
            log << "  The following property will be overridden but it has not been\n"
                << "  defined in the current model '" << el->GetName() << "'\n";
          }

          log << "      " << "Overriding value for property " << interface_property_string
              << "\n       (old value: " << node->getDoubleValue()
              << "  new value: " << value << ")\n\n";
        }

        node->setDoubleValue(value);
      }
      else {
        if (has_value_attribute) {
          FGXMLLogging log(property_element, LogLevel::WARN);
          log << "      Property " << interface_property_string
              << " is already defined.\n"
              << "      Its value (" << node->getDoubleValue() << ") will not"
              << " be overridden.\n";
        }
        property_element = el->FindNextElement("property");
        continue;
      }
    } else {
      node = PM->GetNode(interface_property_string, true);
      if (node) {
        node->setDoubleValue(value);

        if (FGJSBBase::debug_lvl > 0) {
          FGLogging log(LogLevel::DEBUG);
          log << "      " << interface_property_string << " (initial value: "
              << value << ")\n\n";
        }
      }
      else {
        FGXMLLogging log(property_element, LogLevel::ERROR);
        log << "Could not create property " << interface_property_string << "\n";
        property_element = el->FindNextElement("property");
        continue;
      }
    }
    interface_prop_initial_value[node] = value;
    if (property_element->GetAttributeValue("persistent") == string("true"))
      node->setAttribute(SGPropertyNode::PRESERVE, true);

    property_element = el->FindNextElement("property");
  }

  // End of interface property loading logic
}
}
