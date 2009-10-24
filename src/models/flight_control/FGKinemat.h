/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGKinemat.h
 Author:       Tony Peden, for flight control system authored by Jon S. Berndt
 Date started: 12/02/01

 ------------- Copyright (C) Anthony K. Peden -------------

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

#ifndef FGKinemat_H
#define FGKinemat_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FLAPS "$Id: FGKinemat.h,v 1.10 2009/10/24 22:59:30 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a kinematic (mechanical) component for the flight control system.
This component models the action of a moving effector, such as an aerosurface or
other mechanized entity such as a landing gear strut for the purpose of effecting
vehicle control or configuration. The form of the component specification is:

@code
<kinematic name="Gear Control">
  <input> [-]property </input>
  <traverse>
    <setting>
      <position> number </position>
      <time> number </time>
    </setting>
    ...
  </traverse>
  [<clipto>
    <min> {[-]property name | value} </min>
    <max> {[-]property name | value} </max>
  </clipto>]
  [<gain> {property name | value} </gain>]
  [<output> {property} </output>]
</kinematic>
@endcode

The detent is the position that the component takes, and the lag is the time it
takes to get to that position from an adjacent setting. For example:

@code
<kinematic name="Gear Control">
  <input>gear/gear-cmd-norm</input>
  <traverse>
    <setting>
      <position>0</position>
      <time>0</time>
    </setting>
    <setting>
      <position>1</position>
      <time>5</time>
    </setting>
  </traverse>
  <output>gear/gear-pos-norm</output>
</kinematic>
@endcode

In this case, it takes 5 seconds to get to a 1 setting. As this is a software
mechanization of a servo-actuator, there should be an output specified.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGKinemat  : public FGFCSComponent {
public:
  /** Constructor.
      @param fcs A reference to the current flight control system.
      @param element reference to the current configuration file node.
   */
  FGKinemat(FGFCS* fcs, Element* element);

  /// Destructor.
  ~FGKinemat();

  /** Kinematic component output value.
      @return the current output of the kinematic object on the range of [0,1]. */
  double GetOutputPct() const { return OutputPct; }

  /** Run method, overrides FGModel::Run().
      @return false on success, true on failure.
      The routine doing the work.  */
  bool Run (void);

private:
  std::vector<double> Detents;
  std::vector<double> TransitionTimes;
  int NumDetents;
  double OutputPct;
  bool  DoScale;

  void Debug(int from);
};
}
#endif
