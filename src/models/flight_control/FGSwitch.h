/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSwitch.h
 Author:       Jon S. Berndt
 Date started: 12/23/2002

 ------------- Copyright (C)  -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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
#include "FGCondition.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SWITCH "$Id: FGSwitch.h,v 1.3 2005/06/13 16:59:19 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a switch for the flight control system.

The SWITCH component models a switch - either on/off or a multi-choice rotary
switch. The switch can represent a physical cockpit switch, or can represent a
logical switch, where several conditions might need to be satisfied before a
particular state is reached. The VALUE of the switch - the output value for the
component - is chosen depending on the state of the switch. Each switch is
comprised of two or more TESTs. Each TEST has a VALUE associated with it. The
first TEST that evaluates to TRUE will set the output value of the switch
according to the VALUE parameter belonging to that TEST. Each TEST contains one
or more CONDITIONS, which each must be logically related (if there are more than
one) given the value of the LOGIC parameter, and which takes the form:

  property conditional property|value

e.g.

  qbar GE 21.0

or,

  roll_rate < pitch_rate

Within a TEST, a CONDITION_GROUP can be specified. A CONDITION_GROUP allows for
complex groupings of logical comparisons. Each CONDITION_GROUP contains
additional conditions, as well as possibly additional CONDITION_GROUPs.

<pre>
\<COMPONENT NAME="switch1" TYPE="SWITCH"\>
  \<TEST LOGIC="{AND|OR|DEFAULT}" VALUE="{property|value}"\>
    {property} {conditional} {property|value}
    \<CONDITION_GROUP LOGIC="{AND|OR}"\>
      {property} {conditional} {property|value}
      ...
    \</CONDITION_GROUP\>
    ...
  \</TEST>
  \<TEST LOGIC="{AND|OR}" VALUE="{property|value}"\>
    {property} {conditional} {property|value}
    ...
  \</TEST\>
  ...
  [OUTPUT \<property>]
\</COMPONENT\>
</pre>

Here's an example:
<pre>
\<COMPONENT NAME="Roll A/P Autoswitch" TYPE="SWITCH">
  \<TEST LOGIC="DEFAULT" VALUE="0.0">
  \</TEST>
  \<TEST LOGIC="AND" VALUE="fcs/roll-ap-error-summer">
    ap/attitude_hold == 1
  \</TEST>
\</COMPONENT>
</pre>
The above example specifies that the default value of the component (i.e. the
output property of the component, addressed by the property, ap/roll-ap-autoswitch)
is 0.0.  If or when the attitude hold switch is selected (property
ap/attitude_hold takes the value 1), the value of the switch component will be
whatever value fcs/roll-ap-error-summer is.
@author Jon S. Berndt
@version $Id: FGSwitch.h,v 1.3 2005/06/13 16:59:19 ehofman Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSwitch  : public FGFCSComponent
{
public:
  FGSwitch(FGFCS* fcs, Element* element);
  ~FGSwitch();

  bool Run(void);

  enum eLogic {elUndef=0, eAND, eOR, eDefault};
  enum eComparison {ecUndef=0, eEQ, eNE, eGT, eGE, eLT, eLE};

private:
  FGFCS* fcs;

  struct test {
    vector <FGCondition> conditions;
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

  vector <test> tests;

  void Debug(int from);
};
}
#endif
