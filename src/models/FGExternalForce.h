/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGExternalForce.h
 Author:       Jon Berndt, Dave Culp
 Date started: 9/21/07

 ------------- Copyright (C) 2007  Jon S. Berndt (jsb@hal-pc.org) -------------

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

#include <FGFDMExec.h>
#include <FGJSBBase.h>
#include <models/propulsion/FGForce.h>
#include <string>
#include <input_output/FGPropertyManager.h>
#include <math/FGColumnVector3.h>
#include <math/FGFunction.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_EXTERNALFORCE "$Id: FGExternalForce.h,v 1.2 2008/01/08 12:57:02 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** 
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGExternalForce : public FGForce
{
public:
  /// Constructor
  FGExternalForce(FGFDMExec *FDMExec);
  FGExternalForce(FGFDMExec *FDMExec, Element *el, int index);
  /** Constructor
      @param lgear a reference to an existing FGLGear object     */
  FGExternalForce(const FGExternalForce& extForce);

  /// Destructor
  ~FGExternalForce();

  void SetMagnitude(double mag);
  void SetAzimuth(double az) {azimuth = az;}

  FGColumnVector3& GetBodyForces(void);
  double GetMagnitude(void) const {return magnitude;}
  double GetAzimuth(void) const {return azimuth;}
  double GetX(void) const {return vDirection(0);}
  double GetY(void) const {return vDirection(1);}
  double GetZ(void) const {return vDirection(2);}
  void SetX(double x) {vDirection(0) = x;}
  void SetY(double y) {vDirection(1) = y;}
  void SetZ(double z) {vDirection(2) = z;}
  
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

