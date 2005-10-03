/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFilter.h
 Author:       Jon S. Berndt
 Date started: 4/2000

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

#ifndef FGFILTER_H
#define FGFILTER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FILTER "$Id: FGFilter.h,v 1.4 2005/10/03 03:12:37 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a filter for the flight control system.
The filter component can simulate any filter up to second order. The
Tustin substitution is used to take filter definitions from LaPlace space to the
time domain. The general format for a filter specification is:

<pre>
\<component name="name" type="type">
  \<input> property \</input>
  \<c1> value \<c/1>
  [\<c2> value \<c/2>]
  [\<c3> value \<c/3>]
  [\<c4> value \<c/4>]
  [\<c5> value \<c/5>]
  [\<c6> value \<c/6>]
  [\<output> property \<output>]
\</component>
</pre>

For a lag filter of the form,
<pre>
  C1
------
s + C1
</pre>
the corresponding filter definition is:
<pre>
\<component name="name" type="LAG_FILTER">
  \<input> property \</input>
  \<c1> value \<c/1>
  [\<output> property \<output>]
\</component>
</pre>
As an example, for the specific filter:
<pre>
  600
------
s + 600
</pre>
the corresponding filter definition could be:
<pre>
\<component name="Heading Roll Error Lag" type="LAG_FILTER">
  \<input> fcs/heading-command \</input>
  \<c1> 600 \</c1>
\</component>
</pre>
For a lead-lag filter of the form:
<pre>
C1*s + C2
---------
C3*s + C4
</pre>
The corresponding filter definition is:
<pre>
\<component name="name" type="LEAD_LAG_FILTER">
  \<input> property \</input>
  \<c1> value \<c/1>
  \<c2> value \<c/2>
  \<c3> value \<c/3>
  \<c4> value \<c/4>
  [\<output> property \<output>]
\</component>
</pre>
For a washout filter of the form:
<pre>
  s
------
s + C1
</pre>
The corresponding filter definition is:
<pre>
\<component name="name" type="WASHOUT_FILTER">
  \<input> property \</input>
  \<c1> value \</c1>
  [\<output> property \<output>]
\</component>
</pre>
For a second order filter of the form:
<pre>
C1*s^2 + C2*s + C3
------------------
C4*s^2 + C5*s + C6
</pre>
The corresponding filter definition is:
<pre>
\<component name="name" type="SECOND_ORDER_FILTER">
  \<input> property \</input>
  \<c1> value \<c/1>
  \<c2> value \<c/2>
  \<c3> value \<c/3>
  \<c4> value \<c/4>
  \<c5> value \<c/5>
  \<c6> value \<c/6>
  [\<output> property \<output>]
\</component>
</pre>
For an integrator of the form:
<pre>
 C1
 ---
  s
</pre>
The corresponding filter definition is:
<pre>
\<component name="name" type="INTEGRATOR">
  \<input> property \</input>
  \<c1> value \<c/1>
  [\<trigger> property \</trigger>]
  [\<output> property \<output>]
\</component>
</pre>
For the integrator, the trigger features the following behavior. If the trigger
property value is:
  - 0: no action is taken - the output is calculated normally
  - not 0: (or simply greater than zero), all current and previous inputs will
           be set to 0.0

In all the filter specifications above, an \<output> element is also seen.  This
is so that the last component in a "string" can copy its value to the appropriate
output, such as the elevator, or speedbrake, etc.

@author Jon S. Berndt
@version $Id: FGFilter.h,v 1.4 2005/10/03 03:12:37 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFilter  : public FGFCSComponent
{
public:
  FGFilter(FGFCS* fcs, Element* element);
  ~FGFilter();

  bool Run (void);

  /** When true, causes previous values to be set to current values. This
      is particularly useful for first pass. */
  bool Initialize;

  enum {eLag, eLeadLag, eOrder2, eWashout, eIntegrator, eUnknown} FilterType;

private:
  double dt;
  double ca;
  double cb;
  double cc;
  double cd;
  double ce;
  double C1;
  double C2;
  double C3;
  double C4;
  double C5;
  double C6;
  double PreviousInput1;
  double PreviousInput2;
  double PreviousOutput1;
  double PreviousOutput2;
  FGPropertyManager* Trigger;
  void Debug(int from);
};
}
#endif

