/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSensor.cpp
 Author:       Jon Berndt
 Date started: 9 July 2005

 ------------- Copyright (C) 2005 -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGSensor.h"
#include "input_output/FGXMLElement.h"
#include <iostream>
#include <cstdlib>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGSensor.cpp,v 1.23 2011/08/18 12:42:17 jberndt Exp $";
static const char *IdHdr = ID_SENSOR;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGSensor::FGSensor(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  double denom;

  // inputs are read from the base class constructor

  bits = quantized = divisions = 0;
  PreviousInput = PreviousOutput = 0.0;
  min = max = bias = gain = noise_variance = lag = drift_rate = drift = span = 0.0;
  granularity = 0.0;
  noise_type = 0;
  fail_low = fail_high = fail_stuck = false;

  Element* quantization_element = element->FindElement("quantization");
  if ( quantization_element) {
    if ( quantization_element->FindElement("bits") ) {
      bits = (int)quantization_element->FindElementValueAsNumber("bits");
    }
    divisions = (1<<bits);
    if ( quantization_element->FindElement("min") ) {
      min = quantization_element->FindElementValueAsNumber("min");
    }
    if ( quantization_element->FindElement("max") ) {
      max = quantization_element->FindElementValueAsNumber("max");
    }
    quant_property = quantization_element->GetAttributeValue("name");
    span = max - min;
    granularity = span/divisions;
  }
  if ( element->FindElement("bias") ) {
    bias = element->FindElementValueAsNumber("bias");
  }
  if ( element->FindElement("gain") ) {
    gain = element->FindElementValueAsNumber("gain");
  }
  if ( element->FindElement("drift_rate") ) {
    drift_rate = element->FindElementValueAsNumber("drift_rate");
  }
  if ( element->FindElement("lag") ) {
    lag = element->FindElementValueAsNumber("lag");
    denom = 2.00 + dt*lag;
    ca = dt*lag / denom;
    cb = (2.00 - dt*lag) / denom;
  }
  if ( element->FindElement("noise") ) {
    noise_variance = element->FindElementValueAsNumber("noise");
    string variation = element->FindElement("noise")->GetAttributeValue("variation");
    if (variation == "PERCENT") {
      NoiseType = ePercent;
    } else if (variation == "ABSOLUTE") {
      NoiseType = eAbsolute;
    } else {
      NoiseType = ePercent;
      cerr << "Unknown noise type in sensor: " << Name << endl;
      cerr << "  defaulting to PERCENT." << endl;
    }
    string distribution = element->FindElement("noise")->GetAttributeValue("distribution");
    if (distribution == "UNIFORM") {
      DistributionType = eUniform;
    } else if (distribution == "GAUSSIAN") {
      DistributionType = eGaussian;
    } else {
      DistributionType = eUniform;
      cerr << "Unknown random distribution type in sensor: " << Name << endl;
      cerr << "  defaulting to UNIFORM." << endl;
    }
  }

  FGFCSComponent::bind();
  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSensor::~FGSensor()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSensor::Run(void)
{
  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  ProcessSensorSignal();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::ProcessSensorSignal(void)
{
  Output = Input; // perfect sensor

  // Degrade signal as specified

  if (fail_stuck) {
    Output = PreviousOutput;
  } else {
    if (lag != 0.0)            Lag();       // models sensor lag and filter
    if (noise_variance != 0.0) Noise();     // models noise
    if (drift_rate != 0.0)     Drift();     // models drift over time
    if (gain != 0.0)           Gain();      // models a finite gain
    if (bias != 0.0)           Bias();      // models a finite bias

    if (delay != 0)            Delay();     // models system signal transport latencies

    if (fail_low)  Output = -HUGE_VAL;
    if (fail_high) Output =  HUGE_VAL;

    if (bits != 0)             Quantize();  // models quantization degradation

    Clip();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Noise(void)
{
  double random_value=0.0;

  if (DistributionType == eUniform) {
    random_value = 2.0*(((double)rand()/(double)RAND_MAX) - 0.5);
  } else {
    random_value = GaussianRandomNumber();
  }

  switch( NoiseType ) {
  case ePercent:
    Output *= (1.0 + noise_variance*random_value);
    break;

  case eAbsolute:
    Output += noise_variance*random_value;
    break;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Bias(void)
{
  Output += bias;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Gain(void)
{
  Output *= gain;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Drift(void)
{
  drift += drift_rate*dt;
  Output += drift;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Quantize(void)
{
  if (Output < min) Output = min;
  if (Output > max) Output = max;
  double portion = Output - min;
  quantized = (int)(portion/granularity);
  Output = quantized*granularity + min;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::Lag(void)
{
  // "Output" on the right side of the "=" is the current input
  Output = ca * (Output + PreviousInput) + PreviousOutput * cb;

  PreviousOutput = Output;
  PreviousInput  = Input;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::bind(void)
{
  string tmp = Name;
  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  }
  const string tmp_low = tmp + "/malfunction/fail_low";
  const string tmp_high = tmp + "/malfunction/fail_high";
  const string tmp_stuck = tmp + "/malfunction/fail_stuck";

  PropertyManager->Tie( tmp_low, this, &FGSensor::GetFailLow, &FGSensor::SetFailLow);
  PropertyManager->Tie( tmp_high, this, &FGSensor::GetFailHigh, &FGSensor::SetFailHigh);
  PropertyManager->Tie( tmp_stuck, this, &FGSensor::GetFailStuck, &FGSensor::SetFailStuck);
  
  if (!quant_property.empty()) {
    if (quant_property.find("/") == string::npos) { // not found
      string qprop = "fcs/" + PropertyManager->mkPropertyName(quant_property, true);
      PropertyManager->Tie(qprop, this, &FGSensor::GetQuantized);
    }
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGSensor::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (InputSigns.size() > 0) {
        if (InputSigns[0] < 0)
          cout << "      INPUT: -" << InputNodes[0]->GetName() << endl;
        else
          cout << "      INPUT: " << InputNodes[0]->GetName() << endl;
      }
      if (bits != 0) {
        if (quant_property.empty())
          cout << "      Quantized output" << endl;
        else
          cout << "      Quantized output (property: " << quant_property << ")" << endl;

        cout << "        Bits: " << bits << endl;
        cout << "        Min value: " << min << endl;
        cout << "        Max value: " << max << endl;
        cout << "          (span: " << span << ", granularity: " << granularity << ")" << endl;
      }
      if (bias != 0.0) cout << "      Bias: " << bias << endl;
      if (gain != 0.0) cout << "      Gain: " << gain << endl;
      if (drift_rate != 0) cout << "      Sensor drift rate: " << drift_rate << endl;
      if (lag != 0) cout << "      Sensor lag: " << lag << endl;
      if (noise_variance != 0) {
        if (NoiseType == eAbsolute) {
          cout << "      Noise variance (absolute): " << noise_variance << endl;
        } else if (NoiseType == ePercent) {
          cout << "      Noise variance (percent): " << noise_variance << endl;
        } else {
          cout << "      Noise variance type is invalid" << endl;
        }
        if (DistributionType == eUniform) {
          cout << "      Random noise is uniformly distributed." << endl;
        } else if (DistributionType == eGaussian) {
          cout << "      Random noise is gaussian distributed." << endl;
        }
      }
      if (IsOutput) {
        for (unsigned int i=0; i<OutputNodes.size(); i++)
          cout << "      OUTPUT: " << OutputNodes[i]->getName() << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGSensor" << endl;
    if (from == 1) cout << "Destroyed:    FGSensor" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
