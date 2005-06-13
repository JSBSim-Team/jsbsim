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

#define ID_FILTER "$Id: FGFilter.h,v 1.3 2005/06/13 16:59:19 ehofman Exp $"

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
\<COMPONENT NAME="name" TYPE="type">
  INPUT \<property>
  C1  \<value>
  [C2 \<value>]
  [C3 \<value>]
  [C4 \<value>]
  [C5 \<value>]
  [C6 \<value>]
  [OUTPUT \<property>]
\</COMPONENT>
</pre>

For a lag filter of the form,
<pre>
  C1
------
s + C1 
</pre>
the corresponding filter definition is:
<pre>
\<COMPONENT NAME="name" TYPE="LAG_FILTER">
  INPUT \<property>
  C1 \<value>
  [OUTPUT \<property>]
\</COMPONENT>
</pre>
As an example, for the specific filter:
<pre>
  600
------
s + 600 
</pre>
the corresponding filter definition could be:
<pre>
\<COMPONENT NAME="LAG_1" TYPE="LAG_FILTER">
  INPUT aileron_cmd
  C1 600
\</COMPONENT>
</pre>
For a lead-lag filter of the form:
<pre>
C1*s + C2
---------
C3*s + C4 
</pre>
The corresponding filter definition is:
<pre>
\<COMPONENT NAME="name" TYPE="LEAD_LAG_FILTER">
  INPUT \<property>
  C1 \<value>
  C2 \<value>
  C3 \<value>
  C4 \<value>
  [OUTPUT \<property>]
\</COMPONENT>
</pre>
For a washout filter of the form:
<pre>
  s
------
s + C1 
</pre>
The corresponding filter definition is:
<pre>
\<COMPONENT NAME="name" TYPE="WASHOUT_FILTER">
  INPUT \<property>
  C1 \<value>
  [OUTPUT \<property>]
\</COMPONENT>
</pre>
For a second order filter of the form:
<pre>
C1*s^2 + C2*s + C3
------------------
C4*s^2 + C5*s + C6
</pre>
The corresponding filter definition is:
<pre>
\<COMPONENT NAME="name" TYPE="SECOND_ORDER_FILTER">
  INPUT \<property>
  C1 \<value>
  C2 \<value>
  C3 \<value>
  C4 \<value>
  C5 \<value>
  C6 \<value>
  [OUTPUT \<property>]
\</COMPONENT>
</pre>
For an integrator of the form:
<pre>
 C1
 ---
  s
</pre>
The corresponding filter definition is:
<pre>
\<COMPONENT NAME="name" TYPE="INTEGRATOR">
  INPUT \<property>
  C1 \<value>
  [OUTPUT \<property>]
  [TRIGGER \<property>]
\</COMPONENT>
</pre>
For the integrator, the TRIGGER features the following behavior, if the TRIGGER
property value is:
  - -1 (or simply less than zero), all previous inputs and outputs are set to 0.0
  - 0, no action is taken - the output is calculated normally
  - +1 (or simply greater than zero), all previous outputs (only) will be set to 0.0

In all the filter specifications above, an [OUTPUT] keyword is also seen.  This
is so that the last component in a "string" can copy its value to the appropriate
output, such as the elevator, or speedbrake, etc.

@author Jon S. Berndt
@version $Id: FGFilter.h,v 1.3 2005/06/13 16:59:19 ehofman Exp $
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

