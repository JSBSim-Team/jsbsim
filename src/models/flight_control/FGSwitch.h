/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSwitch.h
 Author:       Jon S. Berndt
 Date started: 12/23/2002

 ------------- Copyright (C) 2002 jsb@hal-pc.org  -------------

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

#ifndef FGSWITCH_H
#define FGSWITCH_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>
#include <math/FGCondition.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SWITCH "$Id: FGSwitch.h,v 1.11 2008/05/01 01:03:14 dpculp Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a switch for the flight control system.

The switch component models a switch - either on/off or a multi-choice rotary
switch. The switch can represent a physical cockpit switch, or can represent a
logical switch, where several conditions might need to be satisfied before a
particular state is reached. The value of the switch - the output value for the
component - is chosen depending on the state of the switch. Each switch is
comprised of one or more tests. Each test has a value associated with it. The
first test that evaluates to true will set the output value of the switch
according to the value parameter belonging to that test. Each test contains one
or more conditions, which each must be logically related (if there are more than
one) given the value of the logic attribute, and which takes the form:

  property conditional property|value

e.g.

  qbar ge 21.0

or,

  roll_rate == pitch_rate

Within a test, additional tests can be specified, which allows for
complex groupings of logical comparisons. Each test contains
additional conditions, as well as possibly additional tests.

@code
<switch name="switch1">
  <default value="{property|value}"/>
  <test logic="{AND|OR}" value="{property|value}">
    {property} {conditional} {property|value}
    <test logic="{AND|OR}">
      {property} {conditional} {property|value}
      ...
    </test>
    ...
  </test>
  <test logic="{AND|OR}" value="{property|value}">
    {property} {conditional} {property|value}
    ...
  </test>
  ...
  [<output> {property} </output>]
</switch>
@endcode

Here's an example:

@code
<switch name="roll a/p autoswitch">
  <default value="0.0"/>
  <test value="fcs/roll-ap-error-summer">
    ap/attitude_hold == 1
  </test>
</switch>
@endcode

Note: In the "logic" attribute, "AND" is the default logic, if none is supplied.

The above example specifies that the default value of the component (i.e. the
output property of the component, addressed by the property, ap/roll-ap-autoswitch)
is 0.0.  If or when the attitude hold switch is selected (property
ap/attitude_hold takes the value 1), the value of the switch component will be
whatever value fcs/roll-ap-error-summer is.

@author Jon S. Berndt
@version $Id: FGSwitch.h,v 1.11 2008/05/01 01:03:14 dpculp Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSwitch  : public FGFCSComponent
{
public:
  /** Constructor
      @param fcs a pointer to the parent FGFCS class
      @param element a pointer to the Element (from the config file XML tree)
             that represents this switch component */
  FGSwitch(FGFCS* fcs, Element* element);

  /// Destructor
  ~FGSwitch();

  /** Executes the switch logic.
      @return true - always*/
  bool Run(void);

  enum eLogic {elUndef=0, eAND, eOR, eDefault};
  enum eComparison {ecUndef=0, eEQ, eNE, eGT, eGE, eLT, eLE};

private:

  struct test {
    vector <FGCondition*> conditions;
    eLogic Logic;
    double OutputVal;
    FGPropertyManager *OutputProp;
    float sign;

    double GetValue(void) {
      if (OutputProp == 0L) return OutputVal;
      else                  return OutputProp->getDoubleValue()*sign;
    }

    test(void) { // constructor for the test structure
      Logic      = elUndef;
      OutputVal  = 0.0;
      OutputProp = 0L;
      sign       = 1.0;
    }

  };

  vector <test*> tests;

  void Debug(int from);
};
}
#endif
