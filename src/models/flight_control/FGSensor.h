/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSensor.h
 Author:       Jon Berndt
 Date started: 9 July 2005

 ------------- Copyright (C) 2005 -------------

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

#ifndef FGSENSOR_H
#define FGSENSOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a Sensor component for the flight control system.

Syntax:

@code
<sensor name="name">
  <input> property </input>
  <lag> number </lag>
  <noise [variation="PERCENT|ABSOLUTE"] [distribution="UNIFORM|GAUSSIAN"]> number </noise>
  <quantization name="name">
    <bits> number </bits>
    <min> number </min>
    <max> number </max>
  </quantization>
  <drift_rate> number </drift_rate>
  <gain> number </gain>
  <bias> number </bias>
  <delay [type="time|frames"]> number < /delay>
</sensor>
@endcode

Example:

@code
<sensor name="aero/sensor/qbar">
  <input> aero/qbar </input>
  <lag> 0.5 </lag>
  <noise variation="PERCENT"> 2 </noise>
  <quantization name="aero/sensor/quantized/qbar">
    <bits> 12 </bits>
    <min> 0 </min>
    <max> 400 </max>
  </quantization>
  <bias> 0.5 </bias>
</sensor>
@endcode

The only required element in the sensor definition is the input element. In that
case, no degradation would be modeled, and the output would simply be the input.

Noise can be Gaussian or uniform, and the noise can be applied as a factor
(PERCENT) or additively (ABSOLUTE). The noise that can be applied at each frame
of the simulation execution is calculated as a random factor times a noise value
that is specified in the config file. When the noise distribution type is
Gaussian, the random number can be between roughly -3 and +3 for a span of six
sigma. When the distribution type is UNIFORM, the random value can be between
-1.0 and +1.0. This random value is multiplied against the specified noise to
arrive at a random noise value for the frame. If the noise type is PERCENT, then
random noise value is added to one, and that sum is then multiplied against the
input signal for the sensor. In this case, the specified noise value in the
config file would be expected to actually be a percent value, such as 0.05 (for
a 5% variance). If the noise type is ABSOLUTE, then the random noise value
specified in the config file is understood to be an absolute value of noise to
be added to the input signal instead of being added to 1.0 and having that sum
be multiplied against the input signal as in the PERCENT type. For the ABSOLUTE
noise case, the noise number specified in the config file could be any number.

If the type is ABSOLUTE, then the noise number times the random number is added
to the input signal instead of being multiplied against it as with the PERCENT
type of noise.

The delay element can specify a frame delay. The integer number provided is the
number of frames to delay the output signal.

@author Jon S. Berndt
@version $Revision: 1.24 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSensor  : public FGFCSComponent
{
public:
  FGSensor(FGFCS* fcs, Element* element);
  virtual ~FGSensor();

  void SetFailLow(double val) {if (val > 0.0) fail_low = true; else fail_low = false;}
  void SetFailHigh(double val) {if (val > 0.0) fail_high = true; else fail_high = false;}
  void SetFailStuck(double val) {if (val > 0.0) fail_stuck = true; else fail_stuck = false;}

  double GetFailLow(void) const {if (fail_low) return 1.0; else return 0.0;}
  double GetFailHigh(void) const {if (fail_high) return 1.0; else return 0.0;}
  double GetFailStuck(void) const {if (fail_stuck) return 1.0; else return 0.0;}
  int    GetQuantized(void) const {return quantized;}

  bool Run (void) override;
  void ResetPastStates(void) override;

protected:
  enum eNoiseType {ePercent=0, eAbsolute} NoiseType;
  enum eDistributionType {eUniform=0, eGaussian} DistributionType;
  double min, max;
  double span;
  double bias;
  double gain;
  double drift_rate;
  double drift;
  double noise_variance;
  double lag;
  double granularity;
  double ca; /// lag filter coefficient "a"
  double cb; /// lag filter coefficient "b"
  double PreviousOutput;
  double PreviousInput;
  int noise_type;
  int bits;
  int quantized;
  int divisions;
  bool fail_low;
  bool fail_high;
  bool fail_stuck;
  std::string quant_property;

  void ProcessSensorSignal(void);
  void Noise(void);
  void Bias(void);
  void Drift(void);
  void Quantize(void);
  void Lag(void);
  void Gain(void);

  void bind(Element* el, FGPropertyManager* pm) override;

private:
  std::shared_ptr<RandomNumberGenerator> generator;
  void Debug(int from) override;
};
}
#endif
