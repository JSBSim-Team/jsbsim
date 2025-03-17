/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGDistributor.h
 Author:       Jon S. Berndt
 Date started: 09/27/2013

 ------------- Copyright (C) 2013 jon@jsbsim.org  -------------

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

#include "FGFCSComponent.h"
#include "math/FGCondition.h"
#include "math/FGParameterValue.h"

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

@code{.xml}
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

If the distributor type is exclusive no further <case> components are evaluated
once a case <test> condition has been found to be true.

If the distributor type is inclusive all the <case> components are evaluated
no matter how many <case> conditions are true.

Whether the distributor type is inclusive or exclusive the <case> component
without <test> is always executed.

Here's an example that evaluate the sign of the property test/number and sets
test/default to the value of test/reference.

@code{.xml}
<distributor>
  <case>
    <test>
      test/number lt 0.0
    </test>
    <property value="-1.0"> test/sign </property>
  </case>
  <case>
    <test>
      test/number ge 0.0
    </test>
    <property value="1.0"> test/sign </property>
  </case>
  <!-- default case -->
  <case>
    <property value="test/reference"> test/default </property>
  </case>
</distributor>
@endcode

Note: In the "logic" attribute, "AND" is the default logic, if none is supplied.

@author Jon S. Berndt
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

  /** Executes the distributor logic.
      @return true - always*/
  bool Run(void) override;

private:

  enum eType {eInclusive=0, eExclusive} Type;
  template<typename T> using vector_of_unique_ptr = std::vector<std::unique_ptr<T>>;

  class PropValPair {
  public:
    PropValPair(const std::string& prop, const std::string& val,
                std::shared_ptr<FGPropertyManager> propMan, Element* el)
      : Prop(new FGPropertyValue(prop, propMan, el)),
        Val(new FGParameterValue(val, propMan, el)) {}

    void SetPropToValue() {
      try {
        Prop->SetValue(Val->GetValue());
      }
      catch(...) {
        throw BaseException(Prop->GetName()+" in distributor component is not known");
      }
    }

    std::string GetPropName() const { return Prop->GetName(); }
    std::string GetValString() const { return Val->GetName(); }
    bool GetLateBoundProp() const { return Prop->IsLateBound(); }
    bool GetLateBoundValue() const { return Val->IsLateBound(); }
  private:
    FGPropertyValue_ptr Prop;
    FGParameterValue_ptr Val;
  };

  class Case {
  public:
    Case() : Test(nullptr) {}

    void SetTest(Element* test_element, std::shared_ptr<FGPropertyManager> propMan) {
      Test = std::make_unique<FGCondition>(test_element, propMan);
    }
    const FGCondition& GetTest(void) const noexcept { return *Test; }
    void AddPropValPair(const std::string& property, const std::string& value,
                        std::shared_ptr<FGPropertyManager> propManager, Element* prop_val_el) {
      PropValPairs.push_back(std::make_unique<PropValPair>(property, value, propManager, prop_val_el));
    }
    void SetPropValPairs() {
      for (auto& pair: PropValPairs) pair->SetPropToValue();
    }
    vector_of_unique_ptr<PropValPair>::const_iterator begin(void) const
    { return PropValPairs.cbegin(); }
    vector_of_unique_ptr<PropValPair>::const_iterator end(void) const
    { return PropValPairs.cend(); }
    bool HasTest() const noexcept { return Test != nullptr; }
    bool GetTestResult() const { return Test->Evaluate(); }

  private:
    std::unique_ptr<FGCondition> Test;
    vector_of_unique_ptr<PropValPair> PropValPairs;
  };

  vector_of_unique_ptr<Case> Cases;

  void Debug(int from) override;
};
}
#endif
