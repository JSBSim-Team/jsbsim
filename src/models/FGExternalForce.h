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

#include "models/propulsion/FGForce.h"
#include "input_output/FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGParameter;
class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropertyVector3
{
public:
  FGPropertyVector3(void) {}
  FGPropertyVector3(FGPropertyManager* pm, const std::string& baseName,
                    const std::string& xcmp, const std::string& ycmp,
                    const std::string& zcmp);

  FGPropertyVector3& operator=(const FGColumnVector3& v) {
    data[1]->setDoubleValue(v(2));
    data[0]->setDoubleValue(v(1));
    data[2]->setDoubleValue(v(3));

    return *this;
  }

  operator FGColumnVector3() const {
    return FGColumnVector3(data[0]->getDoubleValue(), data[1]->getDoubleValue(),
                           data[2]->getDoubleValue());
  }

  FGColumnVector3 operator*(double a) const {
    return FGColumnVector3(a * data[0]->getDoubleValue(),
                           a * data[1]->getDoubleValue(),
                           a * data[2]->getDoubleValue());
  }

private:
  FGPropertyNode_ptr data[3];
};

inline FGColumnVector3 operator*(double a, const FGPropertyVector3& v) {
  return v*a;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates code that models an individual arbitrary force, moment or a
    combination thereof.
    This class encapsulates an individual reaction applied at the specified
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

    The location of the force vector, in structural coordinates, can be set at
    runtime through the following properties:

    @code
    external_reactions/{force name}/location-x-in
    external_reactions/{force name}/location-y-in
    external_reactions/{force name}/location-z-in
    @endcode

    The XML definition of a moment (optional items are in []) is a bit simpler
    because you do not need to specify the location:

    @code
    <moment name="name" frame="BODY | LOCAL | WIND">

      [<function> ... </function>]

      [<direction> <!-- optional initial direction vector -->
        <x> {number} </x>
        <y> {number} </y>
        <z> {number} </z>
      </direction>]
    </moment>
    @endcode

    The initial direction can optionally be set by specifying a unit vector
    in the chosen frame (body, local, or wind).

    As an example, a parachute can be defined oriented in the wind axis frame
    so the drag always acts in the drag direction - opposite the positive X
    axis. That does not include the effects of parachute oscillations, but
    those could be handled in the calling application.

    The force (or moment) direction is not actually required to be specified as
    a unit vector, but prior to the force (or moment) vector being calculated,
    the direction vector is normalized when initialized.

    The force direction can be specified at runtime through setting any/all of
    the following properties:

    @code
    external_reactions/{force name}/x
    external_reactions/{force name}/y
    external_reactions/{force name}/z
    @endcode

    The moment direction can be specified at runtime through setting any/all of
    the following properties:

    @code
    external_reactions/{moment name}/l
    external_reactions/{moment name}/m
    external_reactions/{moment name}/n
    @endcode

    However in that case, the direction is no longer normalized.

    When no <function> has been provided in the force definition, its magnitude
    can be specified through the following property:

    @code
    external_reactions/{force name}/magnitude
    @endcode

    When no <function> has been provided in the moment definition, its magnitude
    can be specified through the following property:

    @code
    external_reactions/{moment name}/magnitude-lbsft
    @endcode
*/

class FGExternalForce : public FGForce
{
public:
  /** Constructor.
      @param FDMExec pointer to the main executive class.
      @param el pointer to the XML element defining an individual force.
  */
  explicit FGExternalForce(FGFDMExec *FDMExec)
    : FGForce(FDMExec), forceMagnitude(NULL), momentMagnitude(NULL)
  { Debug(0); }

  /** Copy Constructor
      @param extForce a reference to an existing FGExternalForce object
  */
  FGExternalForce(const FGExternalForce& extForce);

  /// Destructor
  ~FGExternalForce() override;

  void setForce(Element* el);
  void setMoment(Element* el);
  const FGColumnVector3& GetBodyForces(void) override;

private:
  FGParameter* bind(Element* el, const std::string& baseName,
                    FGPropertyVector3& v);

  std::string Name;
  FGParameter *forceMagnitude, *momentMagnitude;
  FGPropertyVector3 forceDirection, momentDirection;
  void Debug(int from);
};
}
#endif
