/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGExternalForce.h
 Author:       Jon Berndt, Dave Culp
 Date started: 9/21/07

 ------------- Copyright (C) 2007  Jon S. Berndt (jon@jsbsim.org) -------------

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
9/21/07  JB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGEXTERNALFORCE_H
#define FGEXTERNALFORCE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include "models/propulsion/FGForce.h"
#include "math/FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_EXTERNALFORCE "$Id: FGExternalForce.h,v 1.16 2017/05/28 19:01:49 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGParameter;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates code that models an individual arbitrary force.
    This class encapsulates an individual force applied at the specified
    location on the vehicle, and oriented as specified in one of three frames:

    - BODY frame is defined with the X axis positive forward, the Y axis
           positive out the right wing, and the Z axis completing the set
           positive downward out the belly of the aircraft.
    - LOCAL frame is a world-based frame, with X positive north, Y positive east
            and Z completing the right handed system positive down towards
            the center of the Earth.
    - WIND frame (rotated) has X negative into the wind vector (in other words
           drag is along the positive X axis), the Z axis is perpendicular to
           X and positive up (lift) but in the aircraft XZ plane, and Y
           completes the right handed system. This is modified from a normal
           wind frame definition, which is rotated about the Y axis 180 degrees
           from this WIND frame.

    Much of the substance of this class is located in the FGForce base class,
    from which this class is derived.

    Here is the XML definition of a force (optional items are in []):

    @code
    <force name="name" frame="BODY | LOCAL | WIND">

      [<function> ... </function>]

      <location unit="{IN | M}"> 
        <x> {number} </x>
        <y> {number} </y>
        <z> {number} </z>
      </location>
      [<direction> <!-- optional initial direction vector -->
        <x> {number} </x>
        <y> {number} </y>
        <z> {number} </z>
      </direction>]
    </force>
    @endcode

    The initial direction can optionally be set by specifying a unit vector
    in the chosen frame (body, local, or wind).

    As an example, a parachute can be defined oriented in the wind axis frame
    so the drag always acts in the drag direction - opposite the positive X
    axis. That does not include the effects of parachute oscillations, but
    those could be handled in the calling application.

    The force direction is not actually required to be specified as a unit
    vector, but prior to the force vector being calculated, the direction
    vector is normalized when initialized.

    The direction can be specified at runtime through setting any/all of the
    following properties:

    @code
    external_reactions/{force name}/x
    external_reactions/{force name}/y
    external_reactions/{force name}/z
    @endcode

    However in that case, the direction is no longer normalized.

    When no <function> has been provided in the definition, the force magnitude
    can be specified through the following property:

    @code
    external_reactions/{force name}/magnitude
    @endcode

    The location of the force vector, in structural coordinates, can be set at
    runtime through the following properties:

    @code
    external_reactions/{force name}/location-x-in
    external_reactions/{force name}/location-y-in
    external_reactions/{force name}/location-z-in
    @endcode

*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGExternalForce : public FGForce
{
public:
  /** Constructor.
      @param FDMExec pointer to the main executive class.
      @param el pointer to the XML element defining an individual force.
  */
  FGExternalForce(FGFDMExec *FDMExec, Element *el);

  /** Copy Constructor
      @param extForce a reference to an existing FGExternalForce object
  */
  FGExternalForce(const FGExternalForce& extForce);

  /// Destructor
  ~FGExternalForce();

  const FGColumnVector3& GetBodyForces(void);
  double GetDirectionX(void) const {return vDirection(eX);}
  double GetDirectionY(void) const {return vDirection(eY);}
  double GetDirectionZ(void) const {return vDirection(eZ);}
  void SetDirectionX(double x) {vDirection(eX) = x;}
  void SetDirectionY(double y) {vDirection(eY) = y;}
  void SetDirectionZ(double z) {vDirection(eZ) = z;}

private:

  std::string Name;
  FGParameter* Magnitude;
  FGColumnVector3 vDirection;
  void Debug(int from);
};
}
#endif
