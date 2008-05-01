/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFCSFunction.h
 Author:       Jon Berndt
 Date started: 2005

 ------------- Copyright (C) 2005 Jon S. Berndt -------------

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

#ifndef FGFCSFUNCTION_H
#define FGFCSFUNCTION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>
#include <math/FGFunction.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCSFUNCTION "$Id: FGFCSFunction.h,v 1.6 2008/05/01 01:03:14 dpculp Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a FCSFunction object.
    @author Jon S. Berndt

One of the most recent additions to the FCS component set is the FCS Function
component. This component allows a function to be created when no other component
is suitable. Available mathematical operations are described in the FGFunction class.  
The function component is defined as follows:

@code
<fcs_function name="Windup Trigger">
  [<input> [-]property </input>]
  <function>
    ...
  </function>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<output> {property} </output>]
</ fcs_function >
@endcode

The function definition itself can include a nested series of products, sums,
quotients, etc. as well as trig and other math functions. Here's an example of
a function (from an aero specification):

@code
<function name="aero/coefficient/CDo">
    <description>Drag_at_zero_lift</description>
    <product>
        <property>aero/qbar-psf</property>
        <property>metrics/Sw-sqft</property>
        <table>
            <independentVar>velocities/mach</independentVar>
            <tableData>
                0.0000	0.0220
                0.2000	0.0200
                0.6500	0.0220
                0.9000	0.0240
                0.9700	0.0500
            </tableData>
        </table>
    </product>
</function>
@endcode

    @version $Id: FGFCSFunction.h,v 1.6 2008/05/01 01:03:14 dpculp Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFCSFunction  : public FGFCSComponent
{
public:
  FGFCSFunction(FGFCS* fcs, Element* element);
  ~FGFCSFunction();

  bool Run(void);

private:
  FGFunction* function;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
