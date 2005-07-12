/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSensor.h
 Author:
 Date started:

 ------------- Copyright (C)  -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSENSOR_H
#define FGSENSOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SENSOR "$Id: FGSensor.h,v 1.1 2005/07/12 04:44:04 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a Sensor component for the flight control system.

Syntax:

<sensor name=”name” rate_group=”name”>
  <input> property </input>
  <lag> number </lag>
  <noise variation=”PERCENT|ABSOLUTE”> number </noise>
  <quantization>
    <bits> number </bits>
    <min> number </min>
    <max> number </max>
  </quantization>
  <drift_rate> number </drift_rate>
  <bias> number </bias>
</sensor>

Example:

<sensor name=”aero/sensor/qbar” rate_group=”HFCS”>
  <input> aero/qbar </input>
  <lag> 0.5 </lag>
  <noise variation=”PERCENT”> 2 </noise>
  <quantization>
    <bits> 12 </bits>
    <min> 0 </min>
    <max> 400 </max>
 </quantization>
  <bias> 0.5 </bias>
</sensor>

The only required element in the sensor definition is the input element. In that
case, no degradation would be modeled, and the output would simply be the input.

For noise, if the type is PERCENT, then the value supplied is understood to be a
percentage variance. That is, if the number given is 0.05, the the variance is
understood to be +/-0.05 percent maximum variance. So, the actual value for the sensor
will be *anywhere* from 0.95 to 1.05 of the actual "perfect" value at any time -
even varying all the way from 0.95 to 1.05 in adjacent frames - whatever the delta
time.

  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSensor  : public FGFCSComponent
{
public:
  FGSensor(FGFCS* fcs, Element* element);
  ~FGSensor();

  bool Run (void);

private:
  enum eNoiseType {ePercent=0, eAbsolute} NoiseType;
  double dt;
  double min, max;
  double span;
  double bias;
  double drift_rate;
  double drift;
  double noise_variance;
  double lag;
  double granularity;
  int noise_type;
  int bits;
  int quantized;
  int divisions;

  void Noise(void);
  void Bias(void);
  void Drift(void);
  void Quantize(void);
  void Lag(void);

  void Debug(int from);
};
}
#endif
