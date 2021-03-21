/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGMagnetometer.h
 Author:       Matthew Chave
 Date started: August 2009

 ------------- Copyright (C) 2009 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMAGNETOMETER_H
#define FGMAGNETOMETER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGSensor.h"
#include "FGSensorOrientation.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class FGPropagate;
class FGMassBalance;
class FGInertial;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a magnetometer component for the flight control system.

Syntax:

@code
<magnetometer name="name">
  <axis> {X|Y|Z} </axis>
  <lag> number </lag>
  <orientation unit=DEG">
    <x> number </x>
    <y> number </y>
    <z> number </z>
  </orientation>
  <noise variation="PERCENT|ABSOLUTE"> number </noise>
  <quantization name="name">
    <bits> number </bits>
    <min> number </min>
    <max> number </max>
  </quantization>
  <drift_rate> number </drift_rate>
  <bias> number </bias>
  <gain> number </gain>
</magnetometer>
@endcode

Example:

@code
<magnetometer name="aero/magnetometer/X">
  <axis> X </axis>
  <lag> 0.5 </lag>
  <noise variation="PERCENT"> 2 </noise>
  <quantization name="aero/magnetometer/quantized/qbar">
    <bits> 12 </bits>
    <min> 0 </min>
    <max> 400 </max>
  </quantization>
  <bias> 0.5 </bias>
  <gain> 0.5 </gain>
</magnetometer>
@endcode

The only required element in the magnetometer definition is the axis element. In
the default case, no degradation would be modeled, and the output would simply
be the input.

For noise, if the type is PERCENT, then the value supplied is understood to be a
percentage variance. That is, if the number given is 0.05, the the variance is
understood to be +/-0.05 percent maximum variance. So, the actual value for the
magnetometer will be *anywhere* from 0.95 to 1.05 of the actual "perfect" value
at any time - even varying all the way from 0.95 to 1.05 in adjacent frames -
whatever the delta time.

@author Jon S. Berndt
@version $Revision: 1.5 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMagnetometer  : public FGSensor, public FGSensorOrientation
{
public:
  FGMagnetometer(FGFCS* fcs, Element* element);
  ~FGMagnetometer();

  bool Run (void) override;
  void ResetPastStates(void) override;

private:
  std::shared_ptr<FGPropagate> Propagate;
  std::shared_ptr<FGMassBalance> MassBalance;
  std::shared_ptr<FGInertial> Inertial;
  FGColumnVector3 vLocation;
  FGColumnVector3 vRadius;
  FGColumnVector3 vMag;
  void updateInertialMag(void);
  double field[6];
  double usedLat;
  double usedLon;
  double usedAlt;
  unsigned long int date;
  unsigned int counter;
  const unsigned int INERTIAL_UPDATE_RATE;

  void Debug(int from) override;
};
}
#endif
