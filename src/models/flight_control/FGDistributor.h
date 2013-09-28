/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGDistributor.h
 Author:       Jon S. Berndt
 Date started: 09/27/2013

 ------------- Copyright (C) 2013 jon@jsbsim.org  -------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGDISTRIBUTOR_H
#define FGDISTRIBUTOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <cstdlib>
#include "FGFCSComponent.h"
#include "input_output/FGXMLElement.h"
#include "math/FGCondition.h"
#include "math/FGPropertyValue.h"
#include "math/FGRealValue.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_DISTRIBUTOR "$Id: FGDistributor.h,v 1.2 2013/09/27 20:07:10 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a distributor for the flight control system.

The distributor component models a distributor - 

Within a test, additional tests can be specified, which allows for
complex groupings of logical comparisons. Each test contains
additional conditions, as well as possibly additional tests.

@code
<distributor name="name/is/irrelevant" type="exclusive|inclusive">

  <case>
    [<test logic="{AND|OR}" value="{property|value}">
      {property} {conditional} {property|value}
      <test logic="{AND|OR}">
        {property} {conditional} {property|value}
        ...
      </test>
      ...
    </test>] <!-- Optional <test> one time. A <case> without a <test> is always executed -->
    <property value="number|property"> property_name </property>
    ...
  </case>

  ... <!-- Additional cases -->

</distributor>
@endcode

Here's an example:

@code

@endcode

Note: In the "logic" attribute, "AND" is the default logic, if none is supplied.

@author Jon S. Berndt
@version $Id: FGDistributor.h,v 1.2 2013/09/27 20:07:10 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGDistributor  : public FGFCSComponent
{
public:
  /** Constructor
      @param fcs a pointer to the parent FGFCS class
      @param element a pointer to the Element (from the config file XML tree)
             that represents this distributor component */
  FGDistributor(FGFCS* fcs, Element* element);

  /// Destructor
  ~FGDistributor();

  /** Executes the distributor logic.
      @return true - always*/
  bool Run(void);

private:

  enum eType {eInclusive=0, eExclusive} Type;

  class PropValPair {
  public:
    PropValPair(string prop, string val, FGPropertyManager* propMan) {
      PropMan = propMan;
      Val = 0;
      ValString = val;
      FGPropertyNode *node = propMan->GetNode(prop, false);
      if (node) PropNode = node;
      else PropNode = 0;
      PropName = prop;
      if (is_number(ValString)) {
        Val = new FGRealValue(atof(ValString.c_str()));
      } else {
        // "value" must be a property if execution passes to here.
        if (ValString[0] == '-') {
          sign = -1;
          ValString.erase(0,1);
        } else {
          sign = 1;
        }
        node = propMan->GetNode(ValString, false);
        if (node) Val = new FGPropertyValue(node);
      }
    }

    ~PropValPair() {
      delete PropNode;
      delete PropMan;
    }
    
    void SetPropToValue() {
      if (PropNode == 0) {
        if (PropMan->HasNode(PropName)) {
          PropNode = PropMan->GetNode(PropName);
        } else {
          throw(PropName+" in distributor component is not known");
        }
      }
      if (Val == 0) {
        if (PropMan->HasNode(ValString)) {
          FGPropertyNode* node = PropMan->GetNode(ValString, false);
          if (node) Val = new FGPropertyValue(node);
        }
      }
      PropNode->setDoubleValue(Val->GetValue()*sign);
    }

  private:
    string PropName;
    FGPropertyNode* PropNode;
    FGPropertyManager* PropMan;
    FGParameter* Val;
    string ValString;
    int sign;
  };

  class Case {
  public:
    Case() {
      Test = 0;
    }

    ~Case() {
      for (unsigned int i=0; i<PropValPairs.size(); i++) delete PropValPairs[i];
      PropValPairs.clear();
    }

    void SetTest(FGCondition* test) {Test = test;}
    void AddPropValPair(PropValPair* pvPair) {PropValPairs.push_back(pvPair);}
    void SetPropValPairs() {
      for (unsigned int i=0; i<PropValPairs.size(); i++) PropValPairs[i]->SetPropToValue();
    }
    bool HasTest() {return (Test != 0);}
    bool GetTestResult() { return Test->Evaluate(); }

  private:
    FGCondition* Test;
    vector <PropValPair*> PropValPairs;
  };

  vector <Case*> Cases;

  void Debug(int from);
};
}
#endif
