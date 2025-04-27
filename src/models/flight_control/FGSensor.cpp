/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSensor.cpp
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
#include "models/FGFCS.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGSensor::FGSensor(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element), generator(fcs->GetExec()->GetRandomGenerator())
{
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
    double denom = 2.00 + dt*lag;
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
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::ERROR);
      log << "Unknown noise type in sensor: " << Name
        << "\n  defaulting to PERCENT.\n";
    }
    string distribution = element->FindElement("noise")->GetAttributeValue("distribution");
    if (distribution == "UNIFORM") {
      DistributionType = eUniform;
    } else if (distribution == "GAUSSIAN") {
      DistributionType = eGaussian;
    } else {
      DistributionType = eUniform;
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::ERROR);
      log << "Unknown random distribution type in sensor: " << Name
        << "\n  defaulting to UNIFORM.\n";
    }
  }

  bind(element, fcs->GetPropertyManager().get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSensor::~FGSensor()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::ResetPastStates(void)
{
  FGFCSComponent::ResetPastStates();

  PreviousOutput = PreviousInput = Output = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSensor::Run(void)
{
  Input = InputNodes[0]->getDoubleValue();

  ProcessSensorSignal();

  SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGSensor::ProcessSensorSignal(void)
{
  // Degrade signal as specified

  if (!fail_stuck) {
    Output = Input; // perfect sensor

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

  if (DistributionType == eUniform)
    random_value = generator->GetUniformRandomNumber();
  else
    random_value = generator->GetNormalRandomNumber();

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

void FGSensor::bind(Element* el, FGPropertyManager* PropertyManager)
{
  string tmp = Name;

  FGFCSComponent::bind(el, PropertyManager);

  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  }
  const string tmp_low = tmp + "/malfunction/fail_low";
  const string tmp_high = tmp + "/malfunction/fail_high";
  const string tmp_stuck = tmp + "/malfunction/fail_stuck";
  const string tmp_randomseed = tmp + "/randomseed";

  PropertyManager->Tie( tmp_low, this, &FGSensor::GetFailLow, &FGSensor::SetFailLow);
  PropertyManager->Tie( tmp_high, this, &FGSensor::GetFailHigh, &FGSensor::SetFailHigh);
  PropertyManager->Tie( tmp_stuck, this, &FGSensor::GetFailStuck, &FGSensor::SetFailStuck);
  PropertyManager->Tie( tmp_randomseed, this, &FGSensor::GetNoiseRandomSeed, &FGSensor::SetNoiseRandomSeed);

  if (!quant_property.empty()) {
    if (quant_property.find("/") == string::npos) { // not found
      string qprop = "fcs/" + PropertyManager->mkPropertyName(quant_property, true);
      SGPropertyNode* node = PropertyManager->GetNode(qprop, true);
      if (node->isTied()) {
        XMLLogException err(fcs->GetExec()->GetLogger(), el);
        err << "Property " << tmp << " has already been successfully bound (late).\n";
        throw err;
      }
      else
        PropertyManager->Tie(qprop, this, &FGSensor::GetQuantized);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// User is supplying a random seed specifically for this sensor to override the
// random seed used by FGFDMExec.

void FGSensor::SetNoiseRandomSeed(int sr)
{
  RandomSeed = sr;
  generator = std::make_shared<RandomNumberGenerator>(*RandomSeed);
}

int FGSensor::GetNoiseRandomSeed(void) const
{
  if (RandomSeed)
    return *RandomSeed;
  else
    return fcs->GetExec()->SRand();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicitly requests the normal JSBSim
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
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) { // Constructor
      if (!InputNodes.empty())
        log << "      INPUT: " << InputNodes[0]->GetNameWithSign() << fixed
            << setprecision(4) << "\n";
      if (bits != 0) {
        if (quant_property.empty())
          log << "      Quantized output\n";
        else
          log << "      Quantized output (property: " << quant_property << ")\n";

        log << "        Bits: " << bits << "\n";
        log << "        Min value: " << min << "\n";
        log << "        Max value: " << max << "\n";
        log << "          (span: " << span << ", granularity: " << granularity << ")\n";
      }
      if (bias != 0.0) log << "      Bias: " << bias << " \n";
      if (gain != 0.0) log << "      Gain: " << gain << " \n";
      if (drift_rate != 0) log << "      Sensor drift rate: " << drift_rate << " \n";
      if (lag != 0) log << "      Sensor lag: " << lag << " \n";
      if (noise_variance != 0) {
        if (NoiseType == eAbsolute) {
          log << "      Noise variance (absolute): " << noise_variance << " \n";
        } else if (NoiseType == ePercent) {
          log << "      Noise variance (percent): " << noise_variance << " \n";
        } else {
          log << "      Noise variance type is invalid\n";
        }
        if (DistributionType == eUniform) {
          log << "      Random noise is uniformly distributed.\n";
        } else if (DistributionType == eGaussian) {
          log << "      Random noise is gaussian distributed.\n";
        }
      }
      for (auto node: OutputNodes)
        log << "      OUTPUT: " << node->getNameString() << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGSensor\n";
    if (from == 1) log << "Destroyed:    FGSensor\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
