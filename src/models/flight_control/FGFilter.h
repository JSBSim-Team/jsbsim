/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFilter.h
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 Jon S. Berndt jon@jsbsim.org -------------

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

#ifndef FGFILTER_H
#define FGFILTER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FILTER "$Id: FGFilter.h,v 1.12 2009/10/24 22:59:30 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;
class FGPropertyManager;
class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a filter for the flight control system.
The filter component can simulate any first or second order filter. The
Tustin substitution is used to take filter definitions from LaPlace space to the
time domain. The general format for a filter specification is:

@code
<typename name="name">
  <input> property </input>
  <c1> value|property </c1>
  [<c2> value|property </c2>]
  [<c3> value|property </c3>]
  [<c4> value|property </c4>]
  [<c5> value|property </c5>]
  [<c6> value|property </c6>]
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property </output>]
</typename>
@endcode

For a lag filter of the form,

@code
  C1
------
s + C1
@endcode

the corresponding filter definition is:

@code
<lag_filter name="name">
  <input> property </input>
  <c1> value|property </c1>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property <output>]
</lag_filter>
@endcode

As an example, for the specific filter:

@code
  600
------
s + 600
@endcode

the corresponding filter definition could be:

@code
<lag_filter name="Heading Roll Error Lag">
  <input> fcs/heading-command </input>
  <c1> 600 </c1>
</lag_filter>
@endcode

For a lead-lag filter of the form:

@code
C1*s + C2
---------
C3*s + C4
@endcode

The corresponding filter definition is:

@code
<lead_lag_filter name="name">
  <input> property </input>
  <c1> value|property <c/1>
  <c2> value|property <c/2>
  <c3> value|property <c/3>
  <c4> value|property <c/4>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property </output>]
</lead_lag_filter>
@endcode

For a washout filter of the form:

@code
  s
------
s + C1
@endcode

The corresponding filter definition is:

@code
<washout_filter name="name">
  <input> property </input>
  <c1> value </c1>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property </output>]
</washout_filter>
@endcode

For a second order filter of the form:

@code
C1*s^2 + C2*s + C3
------------------
C4*s^2 + C5*s + C6
@endcode

The corresponding filter definition is:

@code
<second_order_filter name="name">
  <input> property </input>
  <c1> value|property </c1>
  <c2> value|property </c2>
  <c3> value|property </c3>
  <c4> value|property </c4>
  <c5> value|property </c5>
  <c6> value|property </c6>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property </output>]
</second_order_filter>
@endcode

For an integrator of the form:

@code
 C1
 ---
  s
@endcode

The corresponding filter definition is:

@code
<integrator name="name">
  <input> property </input>
  <c1> value|property </c1>
  [<trigger> property </trigger>]
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> property </output>]
</integrator>
@endcode

For the integrator, the trigger features the following behavior. If the trigger
property value is:
  - 0: no action is taken - the output is calculated normally
  - not 0: (or simply greater than zero), all current and previous inputs will
           be set to 0.0

In all the filter specifications above, an \<output> element is also seen.  This
is so that the last component in a "string" can copy its value to the appropriate
output, such as the elevator, or speedbrake, etc.

@author Jon S. Berndt
@version $Revision: 1.12 $

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
  void ResetPastStates(void) {Input = 0.0; Initialize = true;}
  
  enum {eLag, eLeadLag, eOrder2, eWashout, eIntegrator, eUnknown} FilterType;

private:
  double ca;
  double cb;
  double cc;
  double cd;
  double ce;
  double C[7]; // There are 6 coefficients, indexing is "1" based.
  double PropertySign[7];
  double PreviousInput1;
  double PreviousInput2;
  double PreviousOutput1;
  double PreviousOutput2;
  FGPropertyManager* Trigger;
  FGPropertyManager* PropertyNode[7];
  void CalculateDynamicFilters(void);
  void ReadFilterCoefficients(Element* el, int index);
  bool DynamicFilter;
  void Debug(int from);
};
}
#endif

