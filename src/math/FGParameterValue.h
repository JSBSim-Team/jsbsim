/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Header: FGParameterValue.h
  Author: Bertrand Coconnier
  Date started: December 09 2018

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

#ifndef FGPARAMETERVALUE_H
#define FGPARAMETERVALUE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "math/FGRealValue.h"
#include "math/FGPropertyValue.h"
#include "input_output/FGXMLElement.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  FORWARD DECLARATIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGPropertyManager;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  CLASS DOCUMENTATION
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Represents a either a real value or a property value
    @author Bertrand Coconnier
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DECLARATION: FGParameterValue
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGParameterValue : public FGParameter
{
public:
  FGParameterValue(Element* el, std::shared_ptr<FGPropertyManager> pm)
  : FGParameterValue(el->GetDataLine(), pm, el)
  {
    string value = el->GetDataLine();

    if (el->GetNumDataLines() != 1 || value.empty()) {
      cerr << el->ReadFrom()
           << "The element <" << el->GetName()
           << "> must either contain a value number or a property name."
           << endl;
      throw BaseException("FGParameterValue: Illegal argument defining: " + el->GetName());
    }
  }

  FGParameterValue(const std::string& value, std::shared_ptr<FGPropertyManager> pm,
                   Element* el) {
    if (is_number(value)) {
      param = new FGRealValue(atof(value.c_str()));
    } else {
      // "value" must be a property if execution passes to here.
      param = new FGPropertyValue(value, pm, el);
    }
  }

  double GetValue(void) const override { return param->GetValue(); }
  bool IsConstant(void) const override { return param->IsConstant(); }

  std::string GetName(void) const override {
    FGPropertyValue* v = dynamic_cast<FGPropertyValue*>(param.ptr());
    if (v)
      return v->GetNameWithSign();
    else
      return param->GetName();
  }

  bool IsLateBound(void) const {
    FGPropertyValue* v = dynamic_cast<FGPropertyValue*>(param.ptr());
    return v != nullptr && v->IsLateBound();
  }
private:
  FGParameter_ptr param;
};

typedef SGSharedPtr<FGParameterValue> FGParameterValue_ptr;

} // namespace JSBSim

#endif
