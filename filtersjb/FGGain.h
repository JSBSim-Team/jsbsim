/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGain.h
 Author:
 Date started:

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

#ifndef FGGAIN_H
#define FGGAIN_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
   SG_USING_STD(string);
#else
#  include <string>
#endif

#include "FGFCSComponent.h"
#include "../FGConfigFile.h"
#include "../FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_GAIN "$Id: FGGain.h,v 1.26 2004/05/04 12:22:45 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a gain component for the flight control system.
    The gain component merely multiplies the input by a gain.  The form of the
    gain component specification is:
    <pre>
    \<COMPONENT NAME="name" TYPE="PURE_GAIN">
      INPUT \<property>
      GAIN  \<value>
      [OUTPUT \<property>]
    \</COMPONENT>
    </pre>
    Note: as is the case with the Summer component, the input property name may be
    immediately preceded by a minus sign to invert that signal.

    The scheduled gain component multiplies the input by a variable gain that is
    dependent on another property (such as qbar, altitude, etc.).  The lookup
    mapping is in the form of a table.  This kind of component might be used, for
    example, in a case where aerosurface deflection must only be commanded to
    acceptable settings - i.e at higher qbar the commanded elevator setting might
    be attenuated.  The form of the scheduled gain component specification is:
    <pre>
    \<COMPONENT NAME="name" TYPE="SCHEDULED_GAIN">
      INPUT \<property>
      [GAIN  \<value>]
      SCHEDULED_BY \<property>
      ROWS \<number_of_rows>
      \<lookup_value  gain_value>
      ?
      [CLIPTO \<min> \<max> 1]
      [OUTPUT \<property>]
    \</COMPONENT>
    </pre>
    An overall GAIN may be supplied that is multiplicative with the scheduled gain.

    Note: as is the case with the Summer component, the input property name may
    be immediately preceded by a minus sign to invert that signal.

    Here is an example of a scheduled gain component specification:
    <pre>
    \<COMPONENT NAME="Pitch Scheduled Gain 1" TYPE="SCHEDULED_GAIN">
      INPUT        fcs/pitch-gain-1
      GAIN         0.017
      SCHEDULED_BY fcs/elevator-pos-rad
      ROWS         22
      -0.68  -26.548
      -0.595 -20.513
      -0.51  -15.328
      -0.425 -10.993
      -0.34   -7.508
      -0.255  -4.873
      -0.17   -3.088
      -0.085  -2.153
       0      -2.068
       0.085  -2.833
       0.102  -3.088
       0.119  -3.377
       0.136  -3.7
       0.153  -4.057
       0.17   -4.448
       0.187  -4.873
       0.272  -7.508
       0.357 -10.993
       0.442 -15.328
       0.527 -20.513
       0.612 -26.548
       0.697 -33.433
    \</COMPONENT>
    </pre>
    In the example above, we see the utility of the overall GAIN value in
    effecting a degrees-to-radians conversion.

    The aerosurface scale component is a modified version of the simple gain
    component.  The normal purpose
    for this component is to take control inputs that range from -1 to +1 or
    from 0 to +1 and scale them to match the expected inputs to a flight control
    system.  For instance, the normal and expected ability of a pilot to push or
    pull on a control stick is about 50 pounds.  The input to the pitch channelb
    lock diagram of a flight control system is in units of pounds.  Yet, the
    joystick control input is usually in a range from -1 to +1.  The form of the
    aerosurface scaling component specification is:
<pre>
    \<COMPONENT NAME="name" TYPE="AEROSURFACE_SCALE">
      INPUT \<property>
      MIN \<value>
      MAX \<value>
      [GAIN  \<value>]
      [OUTPUT \<property>]
    \</COMPONENT>
</pre>
    Note: as is the case with the Summer component, the input property name may be
    immediately preceded by a minus sign to invert that signal.

    @author Jon S. Berndt
    @version $Id: FGGain.h,v 1.26 2004/05/04 12:22:45 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGain  : public FGFCSComponent
{
public:
  FGGain(FGFCS* fcs, FGConfigFile* AC_cfg);
  ~FGGain();

  double GetOutputPct() const { return OutputPct; }

  bool Run (void);

private:
  FGConfigFile* AC_cfg;
  FGTable* Table;
  FGState* State;
  double Gain;
  double Min, Max;
  double clipmin, clipmax;
  double OutputPct;
  bool invert, clip;
  int Rows;
  FGPropertyManager* ScheduledBy;

  void Debug(int from);
};
}
#endif
