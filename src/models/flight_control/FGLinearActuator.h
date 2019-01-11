/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * 
 * Header:  FGLinearActuator.h
 * Author:  Adriano Bassignana
 * Date started: 2019-01-02
 * 
 * ------------- Copyright (C)  -------------
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Further information about the GNU Lesser General Public License can also be found on
 * the world wide web at http://www.gnu.org.
 * 
 * HISTORY
 * --------------------------------------------------------------------------------
 * 
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * SENTRY
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGLINEARACTUATOR_H
#define FGLINEARACTUATOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * INCLUDES
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include "simgear/props/propertyObject.hxx"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * FORWARD DECLARATIONS
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {
    
    class Element;
    
    /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
     * CLASS DOCUMENTATION
     * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
    
    /** Models a flight control system summing component.
     *    linear actuators typically operate by conversion of rotary motion into linear motion.
     *    The linear_actuator component is decrobed in the Wikipedia link:
     *    https://en.wikipedia.org/wiki/Linear_actuator
     *    Converts a rotation into a linear movement or a multiturn rotation.
     *    For the conversion it is necessary to declare a module
     *    that is defined by the difference of the maximum and minimum value Input.
     *    List of parameters:
     * 
     *    input: Value to be transformed
     *    module: Difference of the maximum and minimum value Input, default is 1.0
     *    versus: Direction of rotation if fixed. The versus allows to obtain a kinematism
     *      similar to the escapement of a clock. The default value is 0.0 If the value
     *      is zero, the verse is automatically obtained according to variation of
     *      the input data.
     *      If set to a value> 0 the verse is increasing, ie the output changes only
     *      if the Input value is greater than the previous one.
     *      If set to a negative value, the output changes only if the next value
     *      is lower than the previous one.
     *      With this parameter allows to easily obtain a "step counter".
     *    rate: To define when the rotation is complete, the differential criterion is used.
     *      For example, if the rotation is clockwise and the module is 360, the revolution
     *      will be complete when the input value changes from 360 to 0.
     *      When this happens a counter increases the output of the value of the module.
     *      As a way the system keeps track of the number of rotations.
     *      Rate defines the sensitivity of the system.
     *      If a difficulty in determining the rotation is observed during the tests,
     *      this parameter must be modified with a positive value not exceeding 1.
     *      The default value is 0.3
     *    set: If the value is greater than zero, the output changes according to the input.
     *      If its value is zero, the output remains constant (the system stores the data).
     *      The use of this parameter allows to simulate the behavior of a servomechanism
     *      that is blocked, for example due to a power failure.
     *    Reset: When high, the output returns to zero.
     *    Lag: Activate a lag filter on exit.
     *    Gain: Apply a multiplication coefficient of the output value.
     *    Clipto: Clips the output. The clipping is applied after the gain and lag.
     *    Output: Output value following the following rule:
     *      Output = gain * (Input + module*countSpin)
     *      CountSpin is the number of complete rotations
     * 
     *    The form of the linear_actuator component specification is:
     * 
     * @code
     *    <linear_actuator name="{string}">
     *      <input> {property name} </input>
     *      <module> value </module>
     *      <versus> value|property </versus>
     *      <rate> value </rate>
     *      <set> {property name | value} </set>
     *      <reset> {property name | value} </reset>
     *      <lag> number </lag>
     *      <gain> value|property </gain>
     *      <clipto>
     *         <min> {value} </min>
     *         <max> {value} </max>
     *      </clipto>
     *      <output> {property name} </output>
     *    </linear_actuator>
     * @endcode
     *
     * Mechanical counter:
     * 
     * It is the typical mechanism used to count the kilometers traveled by a car,
     * for example the value of the digit changes quickly at the beginning of the km.
     * Module 10 indicates that the count goes from 0 to 9 after one complete revolution.
     * 
     * @code
     * 
     *   <linear_actuator name="systems/gauges/PHI/indicator/digit3AW">
     *      <input>systems/gauges/PHI/indicator/digit3A</input>
     *      <module>10</module>
     *      <rate>0.2</rate>
     *      <lag>8.0</lag>
     *  </linear_actuator>
     * 
     * @endcode
     * 
     * The gyrocompass from a rotation with a value from 0 to 259 degrees.
     * When it returns to zero, if it is made more realistic by means of an actuator,
     * there is a jump of the disk which performs a complete rotation of 360 °.
     * By activating a linear actuator followed by an actuator
     * it is possible to obtain a very realistic result.
     * 
     * @code
     * 
     * <linear_actuator name="gyrocompass-magnetic-deg-linear">
     *      <input>gyrocompass-magnetic-deg</input>
     *      <module>360</module>
     *  </linear_actuator>
     * 
     *  <actuator name="gyrocompass-magnetic-deg-linear-actuator">
     *      <input>gyrocompass-magnetic-deg-linear</input>
     *      <lag>2.0</lag>
     *      <rate_limit>100</rate_limit>
     *      <bias>0.1</bias>
     *      <deadband_width>1</deadband_width>
     *      <hysteresis_width>0.5</hysteresis_width>
     *  </actuator>
     * 
     * @endcode
     * 
     * Count steps with memory:
     * If you use a button or switch to advance a mechanism, we can build a step counter.
     * In this case the module is 1 and the rate 1. The verse is positive (increasing).
     *
     * @code
     * 
     *  <linear_actuator name="switch-increase-summer">
     *      <input>switch-increase-trigger</input>
     *      <set>switch-increase-operative</set>
     *      <module>1</module>
     *      <rate>1</rate>
     *      <versus>1</versus>
     *      <lag>8.0</lag>
     *      <gain>1.0</gain>
     *  </linear_actuator>
     *  
     * @endcode
     * 
     * 
     * <pre>
     *    Notes:
     * </pre>
     * 
     *    @author Adriano Bassignana
     */
    
    /*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
     * CLASS DECLARATION
     * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
    
    class FGLinear_Actuator : public FGFCSComponent
    {
    public:
        /** Constructor.
         *      @param fcs a pointer to the parent FGFCS object.
         *      @param element a pointer to the configuration file node. */
        FGLinear_Actuator(FGFCS* fcs, Element* element);
        /// Destructor
        ~FGLinear_Actuator();
        
        /// The execution method for this FCS component.
        bool Run(void);
        
    private:
        
        simgear::PropertyObject<double> setProperty;
        bool isSetProperty;
        double set;
        simgear::PropertyObject<double> resetProperty;
        bool isResetProperty;
        double reset;
        
        int initialized;
        int direction;
        int countSpin;
        
        double lagProperty;
        double lagPrevius;
        double versus;
        double input_prec;
        double module, rate, minRate, maxRate;
        double previousLagInput;
        double previousLagOutput;
        double gain;
        double lag;
        double ca; // lag filter coefficient "a"
        double cb; // lag filter coefficient "b"
        
        void Lag(void);
        void Debug(int from);
    };
}
#endif
