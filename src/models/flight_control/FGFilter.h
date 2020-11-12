/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFilter.h
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 Jon S. Berndt jon@jsbsim.org -------------

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

#ifndef FGFILTER_H
#define FGFILTER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

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

The numerical integration of filters is made by a Runge-Kutta scheme of order
2 except for the second order filter which uses an RK scheme of order 3.

For a lag filter of the form \f$\frac{C_1}{s+C_1}\f$, the corresponding filter
definition is:

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

As an example, for the specific filter \f$\frac{600}{s+600}\f$ the corresponding
filter definition could be:

@code
<lag_filter name="Heading Roll Error Lag">
  <input> fcs/heading-command </input>
  <c1> 600 </c1>
</lag_filter>
@endcode

For a lead-lag filter of the form \f$\frac{C_1s+C_2}{C_3s+C_4}\f$, the
corresponding filter definition is:

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

For a washout filter of the form \f$\frac{s}{s+C_1}\f$, the corresponding filter
definition is:

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

For a second order filter of the form
\f$\frac{C_1s^2+C_2s+C_3}{C_4s^2+C_5s+C_6}\f$, the corresponding filter
definition is:

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

For an integrator of the form \f$\frac{C_1}{s}\f$, the corresponding filter
definition is:

@code
<integrator name="{string}">
  <input> {property} </input>
  <c1 type="rect|trap|ab2|ab3"> {[-]property | number} </c1>
  [<trigger> {property} </trigger>]
  [<clipto>
    <min> {[-]property | number} </min>
    <max> {[-]property | number} </max>
  </clipto>]
  [<output> {property} </output>]
</integrator>
@endcode

For the integrator, the trigger features the following behavior. If the trigger
property value is:
  - 0: no action is taken - the output is calculated normally
  - not 0: (or simply greater than zero), all current and previous inputs will
           be set to 0.0

By default, the integration scheme is the trapezoidal scheme.

An integrator is equivalent to a PID with the following parameters:
@code
<pid name="{string}">
  <input> {[-]property} </input>
  <kp> 0.0 </kp>
  <ki type="rect|trap|ab2|ab3"> {number|[-]property} </ki>
  <kd> 0.0 </kd>
  <trigger> {property} </trigger>
  [<clipto>
  <min> {[-]property | value} </min>
  <max> {[-]property | value} </max>
  </clipto>]
  [<output> {property} </output>]
</pid>
@endcode

As a consequence, JSBSim internally uses PID controllers to simulate INTEGRATOR
filters.

In all the filter specifications above, an \<output> element is also seen.  This
is so that the last component in a "string" can copy its value to the
appropriate output, such as the elevator, or speedbrake, etc.

@author Jon S. Berndt

*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFilter  : public FGFCSComponent
{
public:
  FGFilter(FGFCS* fcs, Element* element);
  ~FGFilter();

  bool Run (void) override;

  void ResetPastStates(void) override;

private:
  bool DynamicFilter;
  /** When true, causes previous values to be set to current values. This
      is particularly useful for first pass. */
  bool Initialize;
  double ca, cb, cc, cd, ce;
  FGParameter_ptr C[7]; // There are 6 coefficients, indexing is "1" based.
  double PreviousInput1, PreviousInput2;
  double PreviousOutput1, PreviousOutput2;

  enum {eLag, eLeadLag, eOrder2, eWashout, eUnknown} FilterType;

  void CalculateDynamicFilters(void);
  void ReadFilterCoefficients(Element* el, int index,
                              std::shared_ptr<FGPropertyManager> pm);
  void Debug(int from) override;
};
}
#endif

