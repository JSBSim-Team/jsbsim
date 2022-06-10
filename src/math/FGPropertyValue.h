/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGPropertyValue.h
Author: Jon Berndt
Date started: December 10 2004

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
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPERTYVALUE_H
#define FGPROPERTYVALUE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGParameter.h"
#include "input_output/FGPropertyManager.h"
#include "input_output/FGXMLElement.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

  /** Represents a property value which can use late binding.
      @author Jon Berndt, Anders Gidenstam
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGPropertyValue
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGPropertyValue : public FGParameter
{
public:

  explicit FGPropertyValue(FGPropertyNode* propNode)
    : PropertyManager(nullptr), PropertyNode(propNode), Sign(1.0) {}
  FGPropertyValue(const std::string& propName,
                  std::shared_ptr<FGPropertyManager> propertyManager, Element* el);

  double GetValue(void) const override;
  bool IsConstant(void) const override {
    return PropertyNode && (!PropertyNode->isTied()
                         && !PropertyNode->getAttribute(SGPropertyNode::WRITE));
  }
  void SetNode(FGPropertyNode* node) {PropertyNode = node;}
  void SetValue(double value);
  bool IsLateBound(void) const { return PropertyNode == nullptr; }

  std::string GetName(void) const override;
  virtual std::string GetNameWithSign(void) const;
  virtual std::string GetFullyQualifiedName(void) const;
  virtual std::string GetPrintableName(void) const;

protected:
  FGPropertyNode* GetNode(void) const;

private:
  std::shared_ptr<FGPropertyManager> PropertyManager; // Property root used to do late binding.
  mutable FGPropertyNode_ptr PropertyNode;
  mutable Element_ptr XML_def;
  std::string PropertyName;
  double Sign;
};

typedef SGSharedPtr<FGPropertyValue> FGPropertyValue_ptr;

} // namespace JSBSim

#endif
