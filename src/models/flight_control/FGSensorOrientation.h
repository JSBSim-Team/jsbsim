/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSensorOrientation.h
 Author:       Jon Berndt
 Date started: September 2009

 ------------- Copyright (C) 2009 -------------

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

#ifndef FGSENSORORIENTATION_H
#define FGSENSORORIENTATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGSensor.h"
#include "input_output/FGXMLElement.h"
#include "math/FGColumnVector3.h"
#include "math/FGMatrix33.h"

#include <iostream>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a SensorOrientation capability for a sensor.

Syntax:

@author Jon S. Berndt
@version $Revision: 1.6 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSensorOrientation  : public FGJSBBase
{
public:
  FGSensorOrientation(Element* element)
  {
    Element* orient_element = element->FindElement("orientation");
    if (orient_element) vOrient = orient_element->FindElementTripletConvertTo("RAD");

    axis = 0;

    Element* axis_element = element->FindElement("axis");
    if (axis_element) {
      std::string sAxis = element->FindElementValue("axis");
      if (sAxis == "X" || sAxis == "x") {
        axis = 1;
      } else if (sAxis == "Y" || sAxis == "y") {
        axis = 2;
      } else if (sAxis == "Z" || sAxis == "z") {
        axis = 3;
      }
    }

    if (!axis) {
      std::cerr << "  Incorrect/no axis specified for this sensor; assuming X axis" << std::endl;
      axis = 1;
    }

    CalculateTransformMatrix();
  }

//  ~FGSensorOrientation();

protected:
  FGColumnVector3 vOrient;
  FGMatrix33 mT;
  int axis;
  void CalculateTransformMatrix(void)
  {
    double cp,sp,cr,sr,cy,sy;

    cp=cos(vOrient(ePitch)); sp=sin(vOrient(ePitch));
    cr=cos(vOrient(eRoll));  sr=sin(vOrient(eRoll));
    cy=cos(vOrient(eYaw));   sy=sin(vOrient(eYaw));

    mT(1,1) =  cp*cy;
    mT(1,2) =  cp*sy;
    mT(1,3) = -sp;

    mT(2,1) = sr*sp*cy - cr*sy;
    mT(2,2) = sr*sp*sy + cr*cy;
    mT(2,3) = sr*cp;

    mT(3,1) = cr*sp*cy + sr*sy;
    mT(3,2) = cr*sp*sy - sr*cy;
    mT(3,3) = cr*cp;

    // This transform is different than for FGForce, where we want a native nozzle
    // force in body frame. Here we calculate the body frame accel and want it in
    // the transformed accelerometer frame. So, the next line is commented out.
    // mT = mT.Inverse();
  }

private:
  void Debug(int from);
};
}
#endif
