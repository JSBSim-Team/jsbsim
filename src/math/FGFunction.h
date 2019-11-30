/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGFunction.h
Author: Jon Berndt
Date started: August 25 2004

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFUNCTION_H
#define FGFUNCTION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGParameter.h"
#include "input_output/FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;
class FGPropertyValue;
class FGFDMExec;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Represents a mathematical function.
The FGFunction class is a powerful and versatile resource that allows
algebraic functions to be defined in a JSBSim configuration file. It is
similar in concept to MathML (Mathematical Markup Language, www.w3.org/Math/),
but simpler and more terse.
A function definition consists of an operation, a value, a table, or a property
(which evaluates to a value). The currently supported operations are:
- sum (takes n args)
- difference (takes n args)
- product (takes n args)
- quotient (takes 2 args)
- pow (takes 2 args)
- sqrt (takes one argument)
- toradians (takes one argument)
- todegrees (takes one argument)
- exp (takes 2 args)
- log2 (takes 1 arg)
- ln (takes 1 arg)
- log10 (takes 1 arg)
- abs (takes 1 arg)
- sin (takes 1 arg)
- cos (takes 1 arg)
- tan (takes 1 arg)
- asin (takes 1 arg)
- acos (takes 1 arg)
- atan (takes 1 arg)
- atan2 (takes 2 args)
- min (takes n args)
- max (takes n args)
- avg (takes n args)
- fraction
- mod
- floor (takes 1 arg)
- ceil (takes 1 arg)
- fmod (takes 2 args)
- lt (less than, takes 2 args)
- le (less equal, takes 2 args)
- gt (greater than, takes 2 args)
- ge (greater than, takes 2 args)
- eq (equal, takes 2 args)
- nq (not equal, takes 2 args)
- and (takes n args)
- or (takes n args)
- not (takes 1 args)
- if-then (takes 2-3 args)
- switch (takes 2 or more args)
- random (Gaussian distribution random number)
- urandom (Uniform random number between -1 and +1)
- pi
- integer
- interpolate 1-dimensional (takes a minimum of five arguments, odd number)

An operation is defined in the configuration file as in the following example:

@code
  <sum>
    <value> 3.14159 </value>
    <property> velocities/qbar </property>
    <product>
      <value> 0.125 </value>
      <property> metrics/wingarea </property>
    </product>
  </sum>
@endcode

A full function definition, such as is used in the aerodynamics section of a
configuration file includes the function element, and other elements. It should
be noted that there can be only one non-optional (non-documentation) element -
that is, one operation element - in the top-level function definition.
Multiple value and/or property elements cannot be immediate child
members of the function element. Almost always, the first operation within the
function element will be a product or sum. For example:

@code
<function name="aero/moment/Clr">
    <description>Roll moment due to yaw rate</description>
    <product>
        <property>aero/qbar-area</property>
        <property>metrics/bw-ft</property>
        <property>aero/bi2vel</property>
        <property>velocities/r-aero-rad_sec</property>
        <table>
            <independentVar>aero/alpha-rad</independentVar>
            <tableData>
                 0.000  0.08
                 0.094  0.19
            </tableData>
        </table>
    </product>
</function>
@endcode

The "lowest level" in a function is always a value or a property, which cannot
itself contain another element. As shown, operations can contain values,
properties, tables, or other operations. In the first above example, the sum
element contains all three. What is evaluated is written algebraically as:

@code 3.14159 + qbar + (0.125 * wingarea) @endcode

Some operations can take only a single argument. That argument, however, can be
an operation (such as sum) which can contain other items. The point to keep in
mind is that it evaluates to a single value - which is just what the
trigonometric functions require (except atan2, which takes two arguments).

<h2>Specific Function Definitions</h2>

Note: In the definitions below, a "property" refers to a single property
specified within either the <property></property> tag or the shortcut tag,
\<p>\</p>. The keyword "value" refers to a single numeric value specified either
within the \<value>\</value> tag or the shortcut <v></v> tag. The keyword
"table" refers to a single table specified either within the \<table>\</table>
tag or the shortcut <t></t> tag. The plural form of any of the three words
refers to one or more instances of a property, value, or table.

- @b sum, sums the values of all immediate child elements:
    @code
    <sum>
      {properties, values, tables, or other function elements}
    </sum>

    Example: Mach + 0.01

    <sum>
      <p> velocities/mach </p>
      <v> 0.01 </v>
    </sum>
    @endcode
- @b difference, subtracts the values of all immediate child elements from the
                 value of the first child element:
    @code
    <difference>
      {properties, values, tables, or other function elements}
    </difference>

    Example: Mach - 0.01

    <difference>
      <p> velocities/mach </p>
      <v> 0.01 </v>
    </difference>
    @endcode
- @b product multiplies together the values of all immediate child elements:
    @code
    <product>
      {properties, values, tables, or other function elements}
    </product>

    Example: qbar*S*beta*CY_beta

    <product>
      <property> aero/qbar-psf            </property>
      <property> metrics/Sw-sqft          </property>
      <property> aero/beta-rad            </property>
      <property> aero/coefficient/CY_beta </property>
    </product>
    @endcode
- @b quotient, divides the value of the first immediate child element by the
               second immediate child element:
    @code
    <quotient>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </quotient>

    Example: (2*GM)/R

    <quotient>
      <product>
        <v> 2.0 </v>
        <p> guidance/executive/gm </p>
      </product>
      <p> position/radius-to-vehicle-ft </p>
    </quotient>
    @endcode
- @b pow, raises the value of the first immediate child element to the power of
          the value of the second immediate child element:
    @code
    <pow>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </pow>

    Example: Mach^2

    <pow>
      <p> velocities/mach </p>
      <v> 2.0 </v>
    </pow>
    @endcode
- @b sqrt, takes the square root of the value of the immediate child element:
    @code
    <sqrt>
      {property, value, table, or other function element}
    </sqrt>

    Example: square root of 25

    <sqrt> <v> 25.0 </v> </sqrt>
    @endcode
- @b toradians, converts a presumed argument in degrees to radians by
                multiplying the value of the immediate child element by pi/180:
    @code
    <toradians>
      {property, value, table, or other function element}
    </toradians>

    Example: convert 45 degrees to radians

    <toradians> <v> 45 </v> </toradians>
    @endcode
- @b todegrees, converts a presumed argument in radians to degrees by
                multiplying the value of the immediate child element by 180/pi:
    @code
    <todegrees>
      {property, value, table, or other function element}
    </todegrees>

    Example: convert 0.5*pi radians to degrees

    <todegrees>
      <product> <v> 0.5 </v> <pi/> </product>
    </todegrees>
    @endcode
- @b exp, raises "e" to the power of the immediate child element:
    @code
    <exp>
      {property, value, table, or other function element}
    </exp>

    Example: raise "e" to the 1.5 power, e^1.5

    <exp> <v> 1.5 </v> </exp>
    @endcode
- @b log2, calculates the log base 2 value of the immediate child element:
    @code
    <log2>
      {property, value, table, or other function element} 
    </log2>

    Example:
    <log2> <v> 128 </v> </log2>
    @endcode
- @b ln, calculates the natural logarithm of the value of the immediate child
         element:
    @code
    <ln>
      {property, value, table, or other function element}
    </ln>
    
    Example: ln(128)

    <ln> <v> 200 </v> </ln>
    @endcode
- @b log10, calculates the base 10 logarithm of the value of the immediate child
            element
    @code
    <log10>
      {property, value, table, or other function element}
    </log10>

    Example log(Mach)

    <log10> <p> velocities/mach </p> </log10>
    @endcode
- @b abs calculates the absolute value of the immediate child element
    @code
    <abs>
      {property, value, table, or other function element}
    </abs>

    Example:

    <abs> <p> flight-path/gamma-rad </p> </abs>
    @endcode
- @b sin, calculates the sine of the value of the immediate child element (the
          argument is expected to be in radians)
    @code
    <sin>
      {property, value, table, or other function element}
    </sin>

    Example:

    <sin> <toradians> <p> fcs/heading-true-degrees </p> </toradians> </sin>
    @endcode
- @b cos, calculates the cosine of the value of the immediate child element (the
          argument is expected to be in radians)
    @code
    <cos>
      {property, value, table, or other function element}
    </cos>

    Example:

    <cos> <toradians> <p> fcs/heading-true-degrees </p> </toradians> </cos>
    @endcode
- @b tan, calculates the tangent of the value of the immediate child element
          (the argument is expected to be in radians)
    @code
    <tan>
      {property, value, table, or other function element}
    </tan>

    Example:

    <tan> <toradians> <p> fcs/heading-true-degrees </p> </toradians> </tan>
    @endcode
- @b asin, calculates the arcsine (inverse sine) of the value of the immediate
           child element. The value provided should be in the range from -1 to
           +1. The value returned will be expressed in radians, and will be in
           the range from -pi/2 to +pi/2.
    @code
    <asin>
      {property, value, table, or other function element}
    </asin>

    Example:

    <asin> <v> 0.5 </v> </asin>
    @endcode
- @b acos, calculates the arccosine (inverse cosine) of the value of the
           immediate child element. The value provided should be in the range
           from -1 to +1. The value returned will be expressed in radians, and
           will be in the range from 0 to pi.
    @code
    <acos>
      {property, value, table, or other function element}
    </acos>

    Example:

    <acos> <v> 0.5 </v> </acos>
    @endcode
- @b atan, calculates the inverse tangent of the value of the immediate child
           element. The value returned will be expressed in radians, and will
           be in the range from -pi/2 to +pi/2.
    @code
    <atan>
      {property, value, table, or other function element}
    </atan>

    Example:

    <atan> <v> 0.5 </v> </atan>
    @endcode
- @b atan2 calculates the inverse tangent of the value of the immediate child
           elements, Y/X (in that order). It even works for X values near zero.
           The value returned will be expressed in radians, and in the range
           -pi to +pi.
    @code
    <atan2>
      {property, value, table, or other function element} {property, value, table, or other function element}
    </atan2>

    Example: inverse tangent of 0.5/0.25, evaluates to: 1.107 radians

    <atan2> <v> 0.5 </<v> <v> 0.25 </v> </atan2>
    @endcode
- @b min returns the smallest value from all the immediate child elements
    @code
    <min>
      {properties, values, tables, or other function elements}
    </min>
    
    Example: returns the lesser of velocity and 2500

    <min>
      <p> velocities/eci-velocity-mag-fps </p>
      <v> 2500.0 </v>
    </min>
    @endcode
- @b max returns the largest value from all the immediate child elements
    @code
    <max>
      {properties, values, tables, or other function elements}
    </max>
    
    Example: returns the greater of velocity and 15000

    <max>
      <p> velocities/eci-velocity-mag-fps </p>
      <v> 15000.0 </v>
    </max>
    @endcode
- @b avg returns the average value of all the immediate child elements
    @code
    <avg>
      {properties, values, tables, or other function elements} 
    </avg>

    Example: returns the average of the four numbers below, evaluates to 0.50.

    <avg>
      <v> 0.25 </v>
      <v> 0.50 </v>
      <v> 0.75 </v>
      <v> 0.50 </v>
    </avg>
    @endcode
- @b fraction, returns the fractional part of the value of the immediate child
               element
    @code
    <fraction>
      {property, value, table, or other function element}
    </fraction>

    Example: returns the fractional part of pi - or, roughly, 0.1415926...

    <fraction> <pi/> </fraction>
    @endcode
- @b integer, returns the integer portion of the value of the immediate child
              element
    @code
    <integer>
      {property, value, table, or other function element}
    </integer>
    @endcode
- @b mod returns the remainder from the integer division of the value of the
         first immediate child element by the second immediate child element,
         X/Y (X modulo Y). The value returned is the value X-I*Y, for the
         largest  integer I such  that if Y is nonzero, the result has the
         same sign as X and magnitude less than the magnitude of Y. For
         instance, the expression "5 mod 2" would evaluate to 1 because 5
         divided by 2 leaves a quotient of 2 and a remainder of 1, while
         "9 mod 3" would evaluate to 0 because the division of 9 by 3 has a
         quotient of 3 and leaves a remainder of 0.
    @code
    <mod>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </mod>

    Example: 5 mod 2, evaluates to 1

    <mod> <v> 5 </v> <v> 2 </v> </mod>
    @endcode
- @b floor returns the largest integral value that is not greater than X.
    @code
    <floor>
      {property, value, table, or other function element}
    </floor>
    @endcode
    Examples: floor(2.3) evaluates to 2.0 while floor(-2.3) evaluates to -3.0
- @b ceil returns the smallest integral value that is not less than X.
    @code
    <ceil>
      {property, value, table, or other function element}
    </ceil>
    @endcode
    Examples: ceil(2.3) evaluates to 3.0 while ceil(-2.3) evaluates to -2.0
- @b fmod returns the floating-point remainder of X/Y (rounded towards zero)
    @code
    <fmod>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </fmod>
    @endcode
    Example: fmod(18.5, 4.2) evaluates to 1.7
- @b lt returns a 1 if the value of the first immediate child element is less
        than the value of the second immediate child element, returns 0
        otherwise
    @code
    <lt>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </lt>

    Example: returns 1 if thrust is less than 10,000, returns 0 otherwise

    <lt>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 10000.0 </v>
    </lt>
    @endcode
- @b le, returns a 1 if the value of the first immediate child element is less
         than or equal to the value of the second immediate child element,
         returns 0 otherwise
    @code
    <le>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </le>

    Example: returns 1 if thrust is less than or equal to 10,000, returns 0 otherwise

    <le>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 10000.0 </v>
    </le>
    @endcode
- @b gt returns a 1 if the value of the first immediate child element is greater
        than the value of the second immediate child element, returns 0
        otherwise
    @code
    <gt>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </gt>

    Example: returns 1 if thrust is greater than 10,000, returns 0 otherwise

    <gt>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 10000.0 </v>
    </gt>
    @endcode
- @b ge, returns a 1 if the value of the first immediate child element is
         greater than or equal to the value of the second immediate child
         element, returns 0 otherwise
    @code
    <ge>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </ge>

    Example: returns 1 if thrust is greater than or equal to 10,000, returns 0
    otherwise

    <ge>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 10000.0 </v>
    </ge>
    @endcode
- @b eq returns a 1 if the value of the first immediate child element is 
        equal to the second immediate child element, returns 0
        otherwise
    @code
    <eq>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </eq>

    Example: returns 1 if thrust is equal to 10,000, returns 0 otherwise

    <eq>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 10000.0 </v>
    </eq>
    @endcode
- @b nq returns a 1 if the value of the first immediate child element is not
        equal to the value of the second immediate child element, returns 0
        otherwise
    @code
    <nq>
      {property, value, table, or other function element}
      {property, value, table, or other function element}
    </nq>

    Example: returns 1 if thrust is not 0, returns 0 otherwise

    <nq>
      <p> propulsion/engine[2]/thrust-lbs </p>
      <v> 0.0 </v>
    </nq>
    @endcode
- @b and returns a 1 if the values of the immediate child elements are all 1,
         returns 0 otherwise. Values provided are expected to be either 1 or 0
         within machine precision.
    @code
    <and>
      {properties, values, tables, or other function elements}
    </and>

    Example: returns 1 if the specified flags are all 1

    <and>
      <p> guidance/first-stage-flight-flag </p>
      <p> control/engines-running-flag </p>
    </and>
    @endcode
- @b or returns a 1 if the values of any of the immediate child elements 1,
         returns 0 otherwise. Values provided are expected to be either 1 or 0
         within machine precision.
    @code
    <or>
      {properties, values, tables, or other function elements}
    </or>

    Example: returns 1 if any of the specified flags are 1

    <or>
      <p> guidance/first-stage-flight-flag </p>
      <p> control/engines-running-flag </p>
    </or>
    @endcode
- @b not, returns the inverse of the value of the supplied immediate child
          element (e.g., returns 1 if supplied a 0)
    @code
    <not>
      {property, value, table, or other function element} 
    </not>

    Example: returns 0 if the value of the supplied flag is 1

    <not> <p> guidance/first-stage-flight-flag </p> </not>
    @endcode
- @b ifthen if the value of the first immediate child element is 1, then the
             value of the second immediate child element is returned, otherwise
             the value of the third child element is returned
     @code
     <ifthen>
       {property, value, table, or other function element}
       {property, value, table, or other function element}
       {property, value, table, or other function element}
     </ifthen>

     Example: if flight-mode is greater than 2, then a value of 0.00 is
              returned, otherwise the value of the property control/pitch-lag is
              returned.

     <ifthen>
       <gt> <p> executive/flight-mode </p> <v> 2 </v> </gt>
       <v> 0.00 </v>
       <p> control/pitch-lag </p>
     </ifthen>
     @endcode
- @b switch uses the integer value of the first immediate child element as an
            index to select one of the subsequent immediate child elements to
            return the value of
     @code
     <switch>
       {property, value, table, or other function element}
       {property, value, table, or other function element}
       {property, value, table, or other function element}
       ...
     </switch>

     Example: if flight-mode is 2, the switch function returns 0.50
     <switch>
       <p> executive/flight-mode </p>
       <v> 0.25 </v>
       <v> 0.50 </v>
       <v> 0.75 </v>
       <v> 1.00 </v>
     </switch>
     @endcode
- @b random Returns a normal distributed random number.
            The function, without parameters, returns a normal distributed 
            random value with a distribution defined by the parameters
            mean = 0.0 and standard deviation (stddev) = 1.0
            The Mean of the distribution (its expected value, μ).
            Which coincides with the location of its peak.
            Standard deviation (σ): The square root of variance,
            representing the dispersion of values from the distribution mean.
            This shall be a positive value (σ>0).
    @code
    <random/> 
    <random seed="1234"/>
    <random seed="time_now"/>
    <random seed="time_now" mean="0.0" stddev="1.0"/>
    @endcode
- @b urandom Returns a uniformly distributed random number.
             The function, without parameters, returns a random value 
             between the minimum value -1.0 and the maximum value of 1.0
             The two maximum and minimum values can be modified using the 
             lower and upper parameters.
    @code
    <urandom/>
    <random seed="1234"/>
    <random seed="time_now"/>
    <random seed="time_now" lower="-1.0" upper="1.0"/>
    @endcode
- @b pi Takes no argument and returns the value of Pi
    @code<pi/>@endcode
- @b interpolate1d returns the result from a 1-dimensional interpolation of the
                   supplied values, with the value of the first immediate child
                   element representing the lookup value into the table, and the
                   following pairs of values representing the independent and
                   dependent values. The first provided child element is
                   expected to be a property. The interpolation does not
                   extrapolate, but holds the highest value if the provided
                   lookup value goes outside of the provided range.
     @code
     <interpolate1d>
       {property, value, table, or other function element}
       {property, value, table, or other function element} {property, value, table, or other function element}
       ...
     </interpolate1d>

     Example: If mach is 0.4, the interpolation will return 0.375. If mach is
              1.5, the interpolation will return 0.60.

     <interpolate1d>
       <p> velocities/mach </p>
       <v> 0.00 </v>  <v> 0.25 </v>
       <v> 0.80 </v>  <v> 0.50 </v>
       <v> 0.90 </v>  <v> 0.60 </v>
     </interpolate1d>
     @endcode
@author Jon Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGFunction
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Todo: Does this class need a copy constructor, like FGLGear?

class FGFunction : public FGParameter, public FGJSBBase
{
public:
  /// Default constructor.
  FGFunction()
    : cached(false), cachedValue(-HUGE_VAL), PropertyManager(nullptr),
      pNode(nullptr), pCopyTo(nullptr) {}

  explicit FGFunction(FGPropertyManager* pm)
    : FGFunction()
  { PropertyManager = pm; }

  /** Constructor.
    When this constructor is called, the XML element pointed to in memory by the
    element argument is traversed. If other FGParameter-derived objects (values,
    functions, properties, or tables) are encountered, this instance of the
    FGFunction object will store a pointer to the found object and pass the
    relevant Element pointer to the constructor for the new object. In other
    words, each FGFunction object maintains a list of "child"
    FGParameter-derived objects which in turn may each contain its own list, and
    so on. At runtime, each object evaluates its child parameters, which each
    may have its own child parameters to evaluate.

    @param PropertyManager a pointer to the property manager instance.
    @param element a pointer to the Element object containing the function
                   definition.
    @param prefix an optional prefix to prepend to the name given to the
                  property that represents this function (if given).
*/
  FGFunction(FGFDMExec* fdmex, Element* element, const std::string& prefix="",
             FGPropertyValue* var=0L);

  /** Destructor
      Make sure the function is untied before destruction.
   */
  ~FGFunction(void) override;

/** Retrieves the value of the function object.
    @return the total value of the function. */
  double GetValue(void) const override;

/** The value that the function evaluates to, as a string.
  @return the value of the function as a string. */
  std::string GetValueAsString(void) const;

/// Retrieves the name of the function.
  std::string GetName(void) const override {return Name;}

/** Does the function always return the same result (i.e. does it apply to
    constant parameters) ? */
  bool IsConstant(void) const override;

/** Specifies whether to cache the value of the function, so it is calculated
    only once per frame.
    If shouldCache is true, then the value of the function is calculated, and a
    flag is set so further calculations done this frame will use the cached
    value.  In order to turn off caching, cacheValue must be called with a false
    argument.
    @param shouldCache specifies whether the function should cache the computed
    value. */
  void cacheValue(bool shouldCache);

  enum class OddEven {Either, Odd, Even};

protected:
  bool cached;
  double cachedValue;
  std::vector <FGParameter_ptr> Parameters;
  FGPropertyManager* PropertyManager;
  FGPropertyNode_ptr pNode;

  void Load(Element* element, FGPropertyValue* var, FGFDMExec* fdmex,
            const std::string& prefix="");
  virtual void bind(Element*, const std::string&);
  void CheckMinArguments(Element* el, unsigned int _min);
  void CheckMaxArguments(Element* el, unsigned int _max);
  void CheckOddOrEvenArguments(Element* el, OddEven odd_even);
  std::string CreateOutputNode(Element* el, const string& Prefix);

private:
  std::string Name;
  FGPropertyNode_ptr pCopyTo; // Property node for CopyTo property string

  void Debug(int from);
};

} // namespace JSBSim

#endif
