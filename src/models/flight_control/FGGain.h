/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGain.h
 Author:       Jon Berndt
 Date started: 1998 ?

 ------------- Copyright (C) 1998 by Jon S. Berndt, jsb@hal-pc.org -------------

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

#ifndef FGGAIN_H
#define FGGAIN_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <string>
#include <input_output/FGXMLElement.h>
#include <math/FGTable.h>

using std::string;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_GAIN "$Id: FGGain.h,v 1.11 2008/07/22 02:42:18 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a gain component for the flight control system.
    The gain component merely multiplies the input by a gain.  The <b>pure gain</b> form
    of the component specification is:

    @code
    <pure_gain name="name">
      <input> {[-]property} </input>
      <gain> {property name | value} </gain>
      [<clipto>
        <min> {property name | value} </min>
        <max> {property name | value} </max>
      </clipto>]
      [<output> {property} </output>]
    </pure_gain>
    @endcode

    Example:

    @code
    <pure_gain name="Roll AP Wing Leveler">
      <input>fcs/attitude/sensor/phi-rad</input>
      <gain>2.0</gain>
      <clipto>
        <min>-0.255</min>
        <max>0.255</max>
      </clipto>
    </pure_gain>
    @endcode

    Note: the input property name may be immediately preceded by a minus sign to
    invert that signal.

    The <b>scheduled gain</b> component multiplies the input by a variable gain that is
    dependent on another property (such as qbar, altitude, etc.).  The lookup
    mapping is in the form of a table.  This kind of component might be used, for
    example, in a case where aerosurface deflection must only be commanded to
    acceptable settings - i.e at higher qbar the commanded elevator setting might
    be attenuated.  The form of the scheduled gain component specification is:

    @code
    <scheduled_gain name="name">
      <input> {[-]property} </input>
      <table>
        <tableData>
          ...
        </tableData>
      </table>
      [<clipto>
        <min> {[-]property name | value} </min>
        <max> {[-]property name | value} </max>
      </clipto>]
      [<gain> {property name | value} </gain>]
      [<output> {property} </output>]
    </scheduled_gain>
    @endcode

    Example:

    @code
    <scheduled_gain name="Scheduled Steer Pos Deg">
        <input>fcs/steer-cmd-norm</input>
        <table>
            <independentVar>velocities/vg-fps</independentVar>
            <tableData>
                10.0        80.0
                50.0        15.0
                150.0       2.0
            </tableData>
        </table>
        <gain>0.017</gain>
        <output>fcs/steer-pos-rad</output>
    </scheduled_gain>
    @endcode

    An overall GAIN may be supplied that is multiplicative with the scheduled gain.

    Note: the input property name may be immediately preceded by a minus sign to
    invert that signal.

    In the example above, we see the utility of the overall gain value in
    effecting a degrees-to-radians conversion.

    The <b>aerosurface scale</b> component is a modified version of the simple gain
    component.  The purpose for this component is to take control inputs from the
    domain minimum and maximum, as specified (or from -1 to +1 by default) and
    scale them to map to a specified range. This can be done, for instance, to match
    the component outputs to the expected inputs to a flight control system.

    The zero_centered element dictates whether the domain-to-range mapping is linear
    or centered about zero. For example, if zero_centered is false, and if the domain
    or range is not symmetric about zero, and an input value is zero, the output
    will not be zero. Let's say that the domain is min=-2 and max=+4, with a range
    of -1 to +1. If the input is 0.0, then the "normalized" input is calculated to
    be 33% of the way from the minimum to the maximum. That input would be mapped
    to an output of -0.33, which is 33% of the way from the range minimum to maximum.
    If zero_centered is set to true (or 1) then an input of 0.0 will be mapped to an
    output of 0.0, although if either the domain or range are unsymmetric about
    0.0, then the scales for the positive and negative portions of the input domain
    (above and below 0.0) will be different. The zero_centered element is true by
    default. Note that this feature may be important for some control surface mappings,
    where the maximum upper and lower deflections may be different, but where a zero
    setting is desired to be the "undeflected" value, and where full travel of the
    stick is desired to cause a full deflection of the control surface.

    The form of the aerosurface scaling component specification is:

    @code
    <aerosurface_scale name="name">
      <input> {[-]property name} </input>
      <domain>
        <min> {value} </min>   <!-- If omitted, default is -1.0 ->
        <max> {value} </max>   <!-- If omitted, default is  1.0 ->
      </domain>
      <range>
        <min> {value} </min>   <!-- If omitted, default is 0 ->
        <max> {value} </max>   <!-- If omitted, default is 0 ->
      </range>
      <zero_centered< value </zero_centered>
      [<clipto>
        <min> {[-]property name | value} </min>
        <max> {[-]property name | value} </max>
      </clipto>]
      [<gain> {property name | value} </gain>]
      [<output> {property} </output>]
    </aerosurface_scale>
    @endcode

    Note: the input property name may be immediately preceded by a minus sign to
    invert that signal.

    For instance, the normal and expected ability of a
    pilot to push or pull on a control stick is about 50 pounds.  The input to the
    pitch channel block diagram of a flight control system is often in units of pounds.
    Yet, the joystick control input usually defines a span from -1 to +1. The aerosurface_scale
    form of the gain component maps the inputs to the desired output range. The example
    below shoes a simple aerosurface_scale component that maps the joystick
    input to a range of +/- 50, which represents pilot stick force in pounds for the F-16.

    @code
    <aerosurface_scale name="Pilot input">
      <input>fcs/elevator-cmd-norm</input>
      <range>
        <min> -50 </min>   <!-- If omitted, default is 0 ->
        <max>  50 </max>   <!-- If omitted, default is 0 ->
      </range>
    </aerosurface_scale>
    @endcode

    @author Jon S. Berndt
    @version $Revision: 1.11 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGain  : public FGFCSComponent
{
public:
  FGGain(FGFCS* fcs, Element* element);
  ~FGGain();

  bool Run (void);

private:
  FGTable* Table;
  FGPropertyManager* GainPropertyNode;
  double GainPropertySign;
  double Gain;
  double InMin, InMax, OutMin, OutMax;
  int Rows;
  bool ZeroCentered;

  void Debug(int from);
};
}
#endif
