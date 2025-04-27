/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGPropertyValue.cpp
Author: Jon Berndt
Date started: 12/10/2004
Purpose: Stores property values

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------
 ------ Copyright (C) 2010 - 2011  Anders Gidenstam (anders(at)gidenstam.org) -

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <assert.h>

#include "FGPropertyValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropertyValue::FGPropertyValue(const std::string& propName,
                                 std::shared_ptr<FGPropertyManager> propertyManager, Element* el)
  : PropertyManager(propertyManager), PropertyNode(nullptr), XML_def(el),
    PropertyName(propName), Sign(1.0)
{
  if (PropertyName[0] == '-') {
    PropertyName.erase(0,1);
    Sign = -1.0;
  }

  if (PropertyManager->HasNode(PropertyName)) {
    PropertyNode = PropertyManager->GetNode(PropertyName);

    assert(PropertyNode);
    XML_def = nullptr; // Now that the property is bound, we no longer need that.
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPropertyNode* FGPropertyValue::GetNode(void) const
{
  if (PropertyNode) return PropertyNode;

  // Manage late binding.
  PropertyNode = PropertyManager->GetNode(PropertyName);
  if (!PropertyNode) {
    if (XML_def)
      cerr << XML_def->ReadFrom()
           << "Property " << PropertyName << " does not exist" << endl;
    throw BaseException("FGPropertyValue::GetValue() The property " +
                        PropertyName + " does not exist.");
  }

  XML_def = nullptr; // Now that the property is bound, we no longer need that.

  return PropertyNode;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropertyValue::GetValue(void) const
{
  return GetNode()->getDoubleValue()*Sign;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropertyValue::SetValue(double value)
{
  // SetValue() ignores the Sign flag. So make sure it is never called with a
  // negative sign.
  assert(Sign == 1);
  GetNode()->setDoubleValue(value);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPropertyValue::GetName(void) const
{
  if (PropertyNode)
    return PropertyNode->getNameString();
  else
    return PropertyName;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPropertyValue::GetNameWithSign(void) const
{
  string name;

  if (Sign < 0.0) name ="-";

  name += GetName();

  return name;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPropertyValue::GetFullyQualifiedName(void) const
{
  if (PropertyNode)
    return JSBSim::GetFullyQualifiedName(PropertyNode);
  else
    return PropertyName;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPropertyValue::GetPrintableName(void) const
{
  if (PropertyNode)
    return JSBSim::GetPrintableName(PropertyNode);
  else
    return PropertyName;
}

}
