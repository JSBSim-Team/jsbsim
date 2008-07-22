/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSummer.h
 Author:
 Date started:

 ------------- Copyright (C)  -------------

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

#ifndef FGSUMMER_H
#define FGSUMMER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>
#include <vector>
#include <string>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SUMMER "$Id: FGSummer.h,v 1.7 2008/07/22 02:44:42 jberndt Exp $"

using std::vector;
using std::string;

using std::vector;
using std::string;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a flight control system summing component.
    The Summer component sums two or more inputs. These can be pilot control
    inputs or state variables, and a bias can also be added in using the BIAS
    keyword.  The form of the summer component specification is:
@code
    <summer name="{string}">
      <input> {string} </input>
      <input> {string} </input>
      <bias> {number} </bias>
      <clipto>
         <min> {number} </min>
         <max> {number} </max>
      </clipto>
      <output> {string} </output>
    </summer>
@endcode

    Note that in the case of an input property the property name may be
    immediately preceded by a minus sign. Here's an example of a summer
    component specification:

@code
    <summer name="Roll A/P Error summer">
      <input> velocities/p-rad_sec </input>
      <input> -fcs/roll-ap-wing-leveler </input>
      <input> fcs/roll-ap-error-integrator </input>
      <clipto>
         <min> -1 </min>
         <max>  1 </max> 
      </clipto>
    </summer>
@endcode

<pre>
    Notes:

    There can be only one BIAS statement per component.

    There may be any number of inputs.
</pre>

    @author Jon S. Berndt
    @version $Id: FGSummer.h,v 1.7 2008/07/22 02:44:42 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSummer  : public FGFCSComponent
{
public:
  /** Constructor.
      @param fcs a pointer to the parent FGFCS object.
      @param element a pointer to the configuration file node. */
  FGSummer(FGFCS* fcs, Element* element);
  /// Destructor
  ~FGSummer();

  /// The execution method for this FCS component.
  bool Run(void);

private:
  double Bias;
  void Debug(int from);
};
}
#endif
