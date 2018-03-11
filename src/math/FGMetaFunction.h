/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  Header: FGMetaFunction.h
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

#ifndef FGMETAFUNCTION_H
#define FGMETAFUNCTION_H

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
  DECLARATION: FGMetaFunction
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMetaFunction : public FGFunction
{
public:

  FGMetaFunction(FGPropertyManager* PropertyManager, Element* element)
    : FGFunction(), var(0L) {
    Load(PropertyManager, element, &var);
    // Since 'var' is a member of FGMetafunction, we don't want SGSharedPtr to
    // destroy 'var' when it would no longer be referenced by any shared
    // pointers. In order to avoid this, the reference counter is increased an
    // extra time to make sure that it never reaches 0 when all shared pointers
    // referencing 'var' are destroyed.
    get(&var);
  }

  virtual ~FGMetaFunction() {
    // Since the FGMetaFunction is a derived class of FGFunction, its destructor
    // is called first and its member 'var' will be destroyed before the
    // FGFunction destructor is called. This will cause the program to segfault
    // when the FGFunction destructor will try to access the reference counter
    // of 'var' in the attempt to destroy 'Parameters'.
    //
    // In order to prevent that, we must destroy the content of 'Parameters'
    // before the FGFunction destructor is called.
    //
    // Since we manually increased the reference counter during the construction
    // of FGMetaFunction, 'var' will not be destroyed in the process.
    Parameters.clear();
  }

  double GetValue(FGPropertyNode* node) {
    var.SetNode(node);
    return FGFunction::GetValue();
  }

private:
  /** FGMetaFunctions must not be bound to the property manager. The bind method
      is therefore overloaded as a no-op */
  virtual void bind(Element*, FGPropertyManager*) {}
  FGPropertyValue var;
};
} // namespace JSBSim

#endif
