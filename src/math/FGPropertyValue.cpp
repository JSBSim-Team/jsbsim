/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGPropertyValue.cpp
Author: Jon Berndt
Date started: 12/10/2004
Purpose: Stores property values

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------
 ------ Copyright (C) 2010 - 2011  Anders Gidenstam (anders(at)gidenstam.org) -

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropertyValue.h"
#include "input_output/FGPropertyManager.h"

namespace JSBSim {

IDENT(IdSrc,"$Id: FGPropertyValue.cpp,v 1.11 2014/01/13 10:46:03 ehofman Exp $");
IDENT(IdHdr,ID_PROPERTYVALUE);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropertyValue::FGPropertyValue(FGPropertyNode* propNode)
  : PropertyManager(0L), PropertyNode(propNode)
{
  Sign = 1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropertyValue::FGPropertyValue(std::string propName, FGPropertyManager* propertyManager)
  : PropertyManager(propertyManager), PropertyNode(0L)
{
  Sign = 1;
  if (propName[0] == '-') {
    propName.erase(0,1);
    Sign = -1;
  }
  PropertyName = propName;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropertyValue::GetValue(void) const
{
  FGPropertyNode* node = PropertyNode;

  if (!PropertyNode) {
    // The node cannot be cached since this is a const method.
    node = PropertyManager->GetNode(PropertyName);
    
    if (!node) {
      throw(std::string("FGPropertyValue::GetValue() The property " +
                        PropertyName + " does not exist."));
    }
  }

  return node->getDoubleValue()*Sign;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGPropertyValue::GetName(void) const
{
  if (PropertyNode) {
    return PropertyNode->GetName();
  } else {
    return PropertyName;
  }
}

}
