/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header:       FGLinearActuator.h
Author:       Adriano Bassignana
Date started: 2019-01-02

------------- Copyright (C) 2019 A. Bassignana -------------

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

#ifndef FGLINEARACTUATOR_H
#define FGLINEARACTUATOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {
    
class Element;
    
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 CLASS DOCUMENTATION
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
    
/** Models a flight control system summing component.

    Linear actuators typically operate by conversion of rotary motion into
    linear motion. The linear_actuator component is described in the Wikipedia
    link: https://en.wikipedia.org/wiki/Linear_actuator.
    
    Linear actuators converts a rotation into a linear movement or a multiturn
    rotation. For the conversion it is necessary to declare a module that is
    defined by the difference of the maximum and minimum value Input.

    List of parameters:
    - input: Value to be transformed
    - Output: Output value following the following rule it is the result of
              gain * (bias + Input + module*countSpin)
    - CountSpin is the number of complete rotations calculated by this device
    - gain: Apply a multiplication coefficient to the output value, the default
            value is 1.0
    - bias: Value that is added to the input value
    - module: Difference between the maximum and minimum value of Input. Default
              is 1.0.
    - hysteresis: Defines the sensitivity of the actuator according to the
                  input.

    For example, if the actuator has a module of 360 and if the hysteresis is
    5.0, the actuator, with gain = 1, will output a value equal to: 5, 10...355,
    360, 365, 370 etc.  This parameter allows to simulate stepper motors. The
    default value is 0.1 It is not advisable to have this parameter too small
    (less than 0.1) as it could make the CPU load heavier.

    - versus: Direction of rotation if fixed. The versus allows to obtain a
              mechanism similar to the tick of a clock. The default value
              is 0.
    
    If the value is zero, the verse is automatically obtained according to
    variation of the input data.

    If set to a value > 0.5 the verse is increasing, i.e. the output changes
    only if the Input value is greater than the previous one.

    If set to a value < -0.5 the output is changed only if the next value is
    lower than the previous one.

    With this parameter allows to easily obtain a "step counter".
    - rate: To define when the rotation is complete, the differential criterion
            is used.

    For example, if the rotation is clockwise and the module is 360, the
    revolution will be complete when the input value changes from 360 to 0.

    When this happens a counter increases the output of the value of the module.
    As a way the system keeps track of the number of rotations. Rate defines the
    sensitivity of the system.

    If a difficulty in determining the rotation is observed during the tests,
    this parameter must be modified with a positive value not exceeding 1.
    The default value is 0.3

    - set: If the absolute value is greater or equal 0.5, the output changes
           according to the input.

    If its value is lower 0.5, the output remains constant (the system stores
    the data). The use of this parameter allows to simulate the behavior of a
    servomechanism that is blocked, for example due to a power failure.

    - Reset: if the absolute value is greater or equal 0.5, the output returns
             to zero and reset internal data.
    - lag: Activate a lag filter on exit if the value is greater 0.0 the lag is
           active.

    Be very careful to use the lag filter in case you use two or more
    "linear_actuator" in cascade; it may happen that the smoothing effect due to
    the lag in the output value can mislead the rotation determination
    system. The effect is similar to that of a loose coupling of a rack and
    pinion.  Therefore, with these types of coupling, place lag only at the last
    stage.

   @code
   <linear_actuator name="{string}">
     <input> {property name} </input>
     <bias> {property name | value} </bias>
     <module> {value} </module>
     <hysteresis> {value} </hysteresis>
     <rate> {value} </rate>
     <versus> {property name | value} </versus>
     <gain> {value} </gain>
     <set> {property name | value} </set>
     <reset> {property name | value} </reset>
     <lag> {value} </lag>
     <output> {property name} </output>
   </linear_actuator>
   @endcode

   Mechanical counter:

   It is the typical mechanism used to count the kilometers traveled by a car,
   for example the value of the digit changes quickly at the beginning of the
   km. Module 10 indicates that the count goes from 0 to 9 after one complete
   revolution.

   @code

   <linear_actuator name="systems/gauges/PHI/indicator/digit3AW">
     <input>systems/gauges/PHI/indicator/digit3A</input>
     <module>10</module>
     <rate>0.2</rate>
     <lag>8.0</lag>
   </linear_actuator>

   @endcode

   The gyrocompass from a rotation with a value from 0 to 259 degrees. When it
   returns to zero, if it is made more realistic by means of an actuator, there
   is a jump of the disk which performs a complete rotation of 360 °. By
   activating a linear actuator followed by an actuator it is possible to obtain
   a very realistic result.

   @code

   <linear_actuator name="gyrocompass-magnetic-deg-linear">
     <input>gyrocompass-magnetic-deg</input>
     <module>360</module>
   </linear_actuator>

   <actuator name="gyrocompass-magnetic-deg-linear-actuator">
     <input>gyrocompass-magnetic-deg-linear</input>
     <lag>2.0</lag>
     <rate_limit>100</rate_limit>
     <bias>0.1</bias>
     <deadband_width>1</deadband_width>
     <hysteresis_width>0.5</hysteresis_width>
   </actuator>

   @endcode

   Count steps with memory:

   If you use a button or switch to advance a mechanism, we can build a step
   counter. In this case the module is 1 and the rate 1. The verse is positive
   (increasing). A pulse counter (for example the count of the switched-on
   states of a switch with values 1 and 0), the module must be 1 in that each
   step must advance its value by one unit. The verse is "1" because it has to
   accept only increasing values (as in a clock escapement). The gain is 0.5
   because, in similitude to an escapement of a clock, the gear makes two steps
   for a complete rotation of the pendulum.

   @code

   <linear_actuator name="systems/gauges/PHI/doppler/switch-increase-summer">
     <input>systems/gauges/PHI/doppler/switch-increase</input>
     <module>1</module>
     <rate>1</rate>
     <versus>1</versus>
     <gain>0.5</gain>
     <lag>0.0</lag>
     <reset>systems/gauges/PHI/doppler/test_reset_off</reset>
   </linear_actuator>

   @endcode

   @author Adriano Bassignana
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 CLASS DECLARATION
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGLinearActuator : public FGFCSComponent
{
public:
  /** Constructor.
      @param fcs a pointer to the parent FGFCS object.
      @param element a pointer to the configuration file node. */
  FGLinearActuator(FGFCS* fcs, Element* element);
  /// Destructor
  ~FGLinearActuator();

  /// The execution method for this FCS component.
  bool Run(void) override;
        
private:
  FGParameter_ptr ptrSet;
  bool set = true;
  FGParameter_ptr ptrReset;
  bool reset = false;
  int direction = 0;
  int countSpin = 0;
  int versus = 0;
  FGParameter_ptr ptrVersus;
  double bias = 0.0;
  FGParameter_ptr ptrBias;
  double inputLast = 0.0;
  double inputMem = 0.0;
  double module = 1.0;
  double hysteresis = 0.1;
  double input = 1.0;
  double rate = 0.3;
  double gain = 1.0;
  double lag = 0.0;
  double previousLagInput;
  double previousLagOutput;
  double ca; // lag filter coefficient "a"
  double cb; // lag filter coefficient "b"

  void Debug(int from) override;
};
}
#endif
