/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGActuator.cpp
 Author:       Jon Berndt
 Date started: 21 February 2006

 ------------- Copyright (C) 2007 Jon S. Berndt (jon@jsbsim.org) -------------

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

#include "FGActuator.h"
#include "input_output/FGXMLElement.h"
#include "math/FGParameterValue.h"
#include "models/FGFCS.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGActuator::FGActuator(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element)
{
  // inputs are read from the base class constructor

  PreviousOutput = 0.0;
  PreviousHystOutput = 0.0;
  PreviousRateLimOutput = 0.0;
  PreviousLagInput = PreviousLagOutput = 0.0;
  bias = hysteresis_width = deadband_width = 0.0;
  lag = nullptr;
  rate_limit_incr = rate_limit_decr = 0; // no limit
  fail_zero = fail_hardover = fail_stuck = false;
  ca = cb = 0.0;
  initialized = 0;
  saturated = false;

  CheckInputNodes(1, 1, element);

  if ( element->FindElement("deadband_width") ) {
    deadband_width = element->FindElementValueAsNumber("deadband_width");
  }
  if ( element->FindElement("hysteresis_width") ) {
    hysteresis_width = element->FindElementValueAsNumber("hysteresis_width");
  }

  // There can be a single rate limit specified, or increasing and
  // decreasing rate limits specified, and rate limits can be numeric, or
  // a property.
  auto PropertyManager = fcs->GetPropertyManager();
  Element* ratelim_el = element->FindElement("rate_limit");
  while ( ratelim_el ) {
    string rate_limit_str = ratelim_el->GetDataLine();
    FGParameter* rate_limit = new FGParameterValue(rate_limit_str,
                                                   PropertyManager, ratelim_el);

    if (ratelim_el->HasAttribute("sense")) {
      string sense = ratelim_el->GetAttributeValue("sense");
      if (sense.substr(0,4) == "incr")
        rate_limit_incr = rate_limit;
      else if (sense.substr(0,4) == "decr")
        rate_limit_decr = rate_limit;
    } else {
      rate_limit_incr = rate_limit;
      rate_limit_decr = rate_limit;
    }
    ratelim_el = element->FindNextElement("rate_limit");
  }

  if ( element->FindElement("bias") ) {
    bias = element->FindElementValueAsNumber("bias");
  }

  // Lag if specified can be numeric or a property
  Element* lag_el = element->FindElement("lag");
  if ( lag_el ) {
    string lag_str = lag_el->GetDataLine();
    lag = new FGParameterValue(lag_str, PropertyManager, lag_el);
    InitializeLagCoefficients();
  }

  bind(element, PropertyManager.get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGActuator::~FGActuator()
{
  delete rate_limit_incr;
  if (rate_limit_decr != rate_limit_incr)
    delete rate_limit_decr;

  delete lag;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::ResetPastStates(void)
{
  FGFCSComponent::ResetPastStates();

  PreviousOutput = PreviousHystOutput = PreviousRateLimOutput
    = PreviousLagInput = PreviousLagOutput = Output = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGActuator::Run(void )
{
  Input = InputNodes[0]->getDoubleValue();

  if( fcs->GetTrimStatus() ) initialized = 0;

  if (fail_zero) Input = 0;
  if (fail_hardover) Input = Input < 0.0 ? ClipMin->GetValue() : ClipMax->GetValue();

  Output = Input; // Perfect actuator. At this point, if no failures are present
                  // and no subsequent lag, limiting, etc. is done, the output
                  // is simply the input. If any further processing is done
                  // (below) such as lag, rate limiting, hysteresis, etc., then
                  // the Input will be further processed and the eventual Output
                  // will be overwritten from this perfect value.

  if (fail_stuck) {
    Output = PreviousOutput;
  } else {
    if (lag)                Lag();        // models actuator lag
    if (rate_limit_incr != 0 || rate_limit_decr != 0) RateLimit();  // limit the actuator rate
    if (deadband_width != 0.0)   Deadband();
    if (hysteresis_width != 0.0) Hysteresis();
    if (bias != 0.0)             Bias();       // models a finite bias
    if (delay != 0)              Delay();      // Model transport latency
  }

  PreviousOutput = Output; // previous value needed for "stuck" malfunction

  initialized = 1;

  Clip();

  if (clip) {
    double clipmax = ClipMax->GetValue();
    saturated = false;

    if (Output >= clipmax && clipmax != 0)
      saturated = true;
    else{
      double clipmin = ClipMin->GetValue();
      if (Output <= clipmin && clipmin != 0)
        saturated = true;
    }
  }

  SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Bias(void)
{
  Output += bias;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Lag(void)
{
  // "Output" on the right side of the "=" is the current frame input
  // for this Lag filter
  double input = Output;

  if (initialized) {
    // Check if lag value has changed via dynamic property
    if (lagVal != lag->GetValue())
      InitializeLagCoefficients();
    Output = ca * (input + PreviousLagInput) + PreviousLagOutput * cb;
  }

  PreviousLagInput = input;
  PreviousLagOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Hysteresis(void)
{
  // Note: this function acts cumulatively on the "Output" parameter. So,
  // "Output" is - for the purposes of this Hysteresis method - really the input
  // to the method.
  double input = Output;

  if ( initialized ) {
    if (input > PreviousHystOutput)
      Output = max(PreviousHystOutput, input-0.5*hysteresis_width);
    else if (input < PreviousHystOutput)
      Output = min(PreviousHystOutput, input+0.5*hysteresis_width);
  }

  PreviousHystOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::RateLimit(void)
{
  // Note: this function acts cumulatively on the "Output" parameter. So,
  // "Output" is - for the purposes of this RateLimit method - really the input
  // to the method.
  double input = Output;
  if ( initialized ) {
    double delta = input - PreviousRateLimOutput;
    if (rate_limit_incr) {
      double rate_limit = rate_limit_incr->GetValue();
      if (delta > dt * rate_limit)
        Output = PreviousRateLimOutput + rate_limit * dt;
    }
    if (rate_limit_decr) {
      double rate_limit = -rate_limit_decr->GetValue();
      if (delta < dt * rate_limit)
        Output = PreviousRateLimOutput + rate_limit * dt;
    }
  }
  PreviousRateLimOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Deadband(void)
{
  // Note: this function acts cumulatively on the "Output" parameter. So,
  // "Output" is - for the purposes of this Deadband method - really the input
  // to the method.
  double input = Output;

  if (input < -deadband_width/2.0) {
    Output = (input + deadband_width/2.0);
  } else if (input > deadband_width/2.0) {
    Output = (input - deadband_width/2.0);
  } else {
    Output = 0.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::bind(Element* el, FGPropertyManager* PropertyManager)
{
  string tmp = Name;

  FGFCSComponent::bind(el, PropertyManager);

  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  }
  const string tmp_zero = tmp + "/malfunction/fail_zero";
  const string tmp_hardover = tmp + "/malfunction/fail_hardover";
  const string tmp_stuck = tmp + "/malfunction/fail_stuck";
  const string tmp_sat = tmp + "/saturated";

  PropertyManager->Tie( tmp_zero, this, &FGActuator::GetFailZero, &FGActuator::SetFailZero);
  PropertyManager->Tie( tmp_hardover, this, &FGActuator::GetFailHardover, &FGActuator::SetFailHardover);
  PropertyManager->Tie( tmp_stuck, this, &FGActuator::GetFailStuck, &FGActuator::SetFailStuck);
  PropertyManager->Tie( tmp_sat, this, &FGActuator::IsSaturated);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::InitializeLagCoefficients()
{
  lagVal = lag->GetValue();
  double denom = 2.00 + dt * lagVal;
  ca = dt * lagVal / denom;
  cb = (2.00 - dt * lagVal) / denom;
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

void FGActuator::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
      log << "      INPUT: " << InputNodes[0]->GetNameWithSign() << fixed
          << setprecision(4) << "\n";

      if (!OutputNodes.empty()) {
        for (auto node: OutputNodes)
          log << "      OUTPUT: " << node->getNameString() << "\n";
      }
      if (bias != 0.0) log << "      Bias: " << bias << "\n";
      if (rate_limit_incr != 0) {
        log << "      Increasing rate limit: " << rate_limit_incr->GetName() << "\n";
      }
      if (rate_limit_decr != 0) {
        log << "      Decreasing rate limit: " << rate_limit_decr->GetName() << "\n";
      }
      if (lag != 0) log << "      Actuator lag: " << lag->GetName() << "\n";
      if (hysteresis_width != 0) log << "      Hysteresis width: " << hysteresis_width << "\n";
      if (deadband_width != 0) log << "      Deadband width: " << deadband_width << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGActuator\n";
    if (from == 1) log << "Destroyed:    FGActuator\n";
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
