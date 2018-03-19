/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Header: FGTemplateFunc.h
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

#ifndef FGTEMPLATEFUNC_H
#define FGTEMPLATEFUNC_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "math/FGFunction.h"
#include "math/FGPropertyValue.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  FORWARD DECLARATIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  CLASS DOCUMENTATION
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DECLARATION: FGTemplateFunc
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTemplateFunc : public FGFunction
{
public:

  FGTemplateFunc(FGPropertyManager* PropertyManager, Element* element)
    : var(0L)
  {
    Load(PropertyManager, element, &var);
    // Since 'var' is a member of FGTemplateFunc, we don't want SGSharedPtr to
    // destroy 'var' when it would no longer be referenced by any shared
    // pointers. In order to avoid this, the reference counter is increased an
    // extra time to make sure that it never reaches 0 when all shared pointers
    // referencing 'var' are destroyed.
    get(&var);
  }

  double GetValue(FGPropertyNode* node) {
    var.SetNode(node);
    return FGFunction::GetValue();
  }

private:
  /** FGTemplateFunc must not be bound to the property manager. The bind method
      is therefore overloaded as a no-op */
  virtual void bind(Element*, FGPropertyManager*) {}
  FGPropertyValue var;
};

typedef SGSharedPtr<FGTemplateFunc> FGTemplateFunc_ptr;

} // namespace JSBSim

#endif
