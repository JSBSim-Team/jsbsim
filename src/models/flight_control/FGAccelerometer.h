/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAccelerometer.h
 Author:       Jon Berndt
 Date started: May 2009

 ------------- Copyright (C) 2009 -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGACCELEROMETER_H
#define FGACCELEROMETER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGSensorOrientation.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class FGPropagate;
class FGAccelerations;
class FGMassBalance;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a Accelerometer component for the flight control system.

Syntax:

@code
<accelerometer name="name">
  <location unit="{IN | M}">
    <x> number </x>
    <y> number </y>
    <z> number </z>
  </location>
  <orientation unit="{RAD | DEG}">
    <pitch> {number} </pitch>
    <roll> {number} </roll>
    <yaw> {number} </yaw>
  </orientation>
  <axis> {X | Y | Z} </axis>
  <lag> number </lag>
  <noise variation="PERCENT|ABSOLUTE"> number </noise>
  <quantization name="name">
    <bits> number </bits>
    <min> number </min>
    <max> number </max>
  </quantization>
  <drift_rate> number </drift_rate>
  <gain> number </gain>
  <bias> number </bias>
  <output> { output_property } </output>
</accelerometer>
@endcode

Example:

@code
<accelerometer name="aero/accelerometer/right_tip_wing">
  <location unit="IN">
    <x> 43.2 </x>
    <y> 214. </y>
    <z> 59.4 </z>
  </location>
  <axis> Z </axis>
  <lag> 0.5 </lag>
  <noise variation="PERCENT"> 2 </noise>
  <quantization name="aero/accelerometer/quantized/right_tip_wing">
    <bits> 12 </bits>
    <min> 0 </min>
    <max> 400 </max>
  </quantization>
  <bias> 0.5 </bias>
  <output> aero/accelerometer/right_tip_wing </output>
</accelerometer>
@endcode

The only required element in the accelerometer definition is the input element.
In that case, no degradation would be modeled, and the output would simply be
the input.

For noise, if the type is PERCENT, then the value supplied is understood to be a
percentage variance. That is, if the number given is 0.05, the variance is
understood to be +/-0.05 percent maximum variance. So, the actual value for the
accelerometer will be *anywhere* from 0.95 to 1.05 of the actual "perfect" value
at any time - even varying all the way from 0.95 to 1.05 in adjacent frames -
whatever the delta time.

@author Jon S. Berndt
@version $Revision: 1.9 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAccelerometer  : public FGSensor, public FGSensorOrientation
{
public:
  FGAccelerometer(FGFCS* fcs, Element* element);
  ~FGAccelerometer();

  bool Run (void) override;

private:
  std::shared_ptr<FGPropagate> Propagate;
  std::shared_ptr<FGAccelerations> Accelerations;
  std::shared_ptr<FGMassBalance> MassBalance;
  FGColumnVector3 vLocation;
  FGColumnVector3 vRadius;
  FGColumnVector3 vAccel;
  
  void Debug(int from) override;
};
}
#endif
