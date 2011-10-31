/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGExternalForce.h
 Author:       Jon Berndt, Dave Culp
 Date started: 9/21/07

 ------------- Copyright (C) 2007  Jon S. Berndt (jon@jsbsim.org) -------------

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
#include "FGFDMExec.h"
#include "FGJSBBase.h"
#include "models/propulsion/FGForce.h"
#include "input_output/FGPropertyManager.h"
#include "math/FGColumnVector3.h"
#include "math/FGFunction.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_EXTERNALFORCE "$Id: FGExternalForce.h,v 1.10 2011/10/31 14:54:41 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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

    Much of the substance of this class is located in the FGForce base class, from
    which this class is derived.
    
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
    in the chosen frame (body, local, or wind). The direction is specified
    at runtime through setting any/all of the following properties:
    
    @code
    external_reactions/{force name}/x
    external_reactions/{force name}/y
    external_reactions/{force name}/z
    @endcode
    
    As an example, a parachute can be defined oriented in the wind axis frame
    so the drag always acts in the drag direction - opposite the positive X
    axis. That does not include the effects of parachute oscillations, but
    those could be handled in the calling application.
     
    The force direction is not actually required to be specified as a unit
    vector, but prior to the force vector being calculated, the direction
    vector is normalized.
    
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGExternalForce : public FGForce
{
public:
  /** Constructor.
      @param FDMExec pointer to the main executive class.
  */
  FGExternalForce(FGFDMExec *FDMExec);

  /** Constructor.
      @param FDMExec pointer to the main executive class.
      @param el pointer to the XML element defining an individual force.
      @param index the position of this force object in the whole list.
  */
  FGExternalForce(FGFDMExec *FDMExec, Element *el, int index);

  /** Copy Constructor
      @param extForce a reference to an existing FGExternalForce object
  */
  FGExternalForce(const FGExternalForce& extForce);

  /// Destructor
  ~FGExternalForce();

  void SetMagnitude(double mag);
  void SetAzimuth(double az) {azimuth = az;}

  const FGColumnVector3& GetBodyForces(void);
  double GetMagnitude(void) const {return magnitude;}
  double GetAzimuth(void) const {return azimuth;}
  double GetX(void) const {return vDirection(eX);}
  double GetY(void) const {return vDirection(eY);}
  double GetZ(void) const {return vDirection(eZ);}
  void SetX(double x) {vDirection(eX) = x;}
  void SetY(double y) {vDirection(eY) = y;}
  void SetZ(double z) {vDirection(eZ) = z;}
  
private:

  string Frame;
  string Name;
  FGPropertyManager* PropertyManager;
  FGPropertyManager* Magnitude_Node;
  FGFunction* Magnitude_Function;
  string BasePropertyName;
  FGColumnVector3 vDirection;
  double magnitude;
  double azimuth;
  void unbind(FGPropertyManager *node);
  void Debug(int from);
};
}
#endif

