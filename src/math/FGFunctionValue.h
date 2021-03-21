/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Header: FGFunctionValue.h
  Author: Bertrand Coconnier
  Date started: March 10 2018

  --------- Copyright (C) 2018  B. Coconnier (bcoconni@users.sf.net) -----------

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

  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  SENTRY
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFUNCTIONVALUE_H
#define FGFUNCTIONVALUE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "math/FGPropertyValue.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  FORWARD DECLARATIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  CLASS DOCUMENTATION
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Represents a property value on which a function is applied
    @author Bertrand Coconnier
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DECLARATION: FGFunctionValue
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFunctionValue : public FGPropertyValue
{
public:

  FGFunctionValue(FGPropertyNode* propNode, FGTemplateFunc_ptr f)
    :FGPropertyValue(propNode), function(f) {}
  FGFunctionValue(std::string propName, std::shared_ptr<FGPropertyManager> propertyManager,
                  FGTemplateFunc_ptr f, Element* el)
    :FGPropertyValue(propName, propertyManager, el), function(f) {}

  double GetValue(void) const override { return function->GetValue(GetNode()); }

  std::string GetName(void) const override {
    return function->GetName() + "(" + FGPropertyValue::GetName() + ")";
  }
  std::string GetNameWithSign(void) const override {
    return function->GetName() + "(" + FGPropertyValue::GetNameWithSign() + ")";
  }
  std::string GetPrintableName(void) const override {
    return function->GetName() + "(" + FGPropertyValue::GetPrintableName() + ")";
  }
  std::string GetFullyQualifiedName(void) const override {
    return function->GetName() + "(" + FGPropertyValue::GetFullyQualifiedName() + ")";
  }

private:
  FGTemplateFunc_ptr function;
};

} // namespace JSBSim

#endif
