/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGActuator.cpp
 Author:       Jon Berndt
 Date started: 21 February 2006

 ------------- Copyright (C) 2007 Jon S. Berndt (jsb@hal-pc.org) -------------

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

#include "FGActuator.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGActuator.cpp,v 1.9 2009/05/14 01:54:00 jberndt Exp $";
static const char *IdHdr = ID_ACTUATOR;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGActuator::FGActuator(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  double denom;
  dt = fcs->GetDt();

  // inputs are read from the base class constructor

  PreviousOutput = 0.0;
  PreviousHystOutput = 0.0;
  PreviousRateLimOutput = 0.0;
  PreviousLagInput = PreviousLagOutput = 0.0;
  bias = lag = hysteresis_width = deadband_width = 0.0;
  rate_limit = 0.0; // no limit
  fail_zero = fail_hardover = fail_stuck = false;
  ca = cb = 0.0;

  if ( element->FindElement("deadband_width") ) {
    deadband_width = element->FindElementValueAsNumber("deadband_width");
  }
  if ( element->FindElement("hysteresis_width") ) {
    hysteresis_width = element->FindElementValueAsNumber("hysteresis_width");
  }
  if ( element->FindElement("rate_limit") ) {
    rate_limit = element->FindElementValueAsNumber("rate_limit");
  }
  if ( element->FindElement("bias") ) {
    bias = element->FindElementValueAsNumber("bias");
  }
  if ( element->FindElement("lag") ) {
    lag = element->FindElementValueAsNumber("lag");
    denom = 2.00 + dt*lag;
    ca = dt*lag / denom;
    cb = (2.00 - dt*lag) / denom;
  }

  FGFCSComponent::bind();
  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGActuator::~FGActuator()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGActuator::Run(void )
{
  dt = fcs->GetDt();

  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  if (fail_zero) Input = 0;
  if (fail_hardover) Input =  clipmax*sign(Input);

  Output = Input; // Perfect actuator. At this point, if no failures are present
                  // and no subsequent lag, limiting, etc. is done, the output
                  // is simply the input. If any further processing is done
                  // (below) such as lag, rate limiting, hysteresis, etc., then
                  // the Input will be further processed and the eventual Output
                  // will be overwritten from this perfect value.

  if (lag != 0.0)              Lag();        // models actuator lag
  if (rate_limit != 0)         RateLimit();  // limit the actuator rate
  if (deadband_width != 0.0)   Deadband();
  if (hysteresis_width != 0.0) Hysteresis();
  if (bias != 0.0)             Bias();       // models a finite bias

  if (fail_stuck) Output = PreviousOutput;
  PreviousOutput = Output; // previous value needed for "stuck" malfunction

  Clip();
  if (IsOutput) SetOutput();

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
  Output = ca * (input + PreviousLagInput) + PreviousLagOutput * cb;
  PreviousLagInput = input;
  PreviousLagOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Hysteresis(void)
{
  // Note: this function acts cumulatively on the "Output" parameter. So, "Output"
  // is - for the purposes of this Hysteresis method - really the input to the
  // method.
  double input = Output;
  
  if (input > PreviousHystOutput) {
    Output = max(PreviousHystOutput, input-0.5*hysteresis_width);
  } else if (input < PreviousHystOutput) {
    Output = min(PreviousHystOutput, input+0.5*hysteresis_width);
  }

  PreviousHystOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::RateLimit(void)
{
  // Note: this function acts cumulatively on the "Output" parameter. So, "Output"
  // is - for the purposes of this RateLimit method - really the input to the
  // method.
  double input = Output;
  if (dt > 0.0) {
    double rate = (input - PreviousRateLimOutput)/dt;
    if (fabs(rate) > rate_limit) {
      Output = PreviousRateLimOutput + (rate_limit*fabs(rate)/rate)*dt;
    }
  }
  PreviousRateLimOutput = Output;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::Deadband(void)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGActuator::bind(void)
{
  string tmp = Name;
  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  }
  const string tmp_zero = tmp + "/malfunction/fail_zero";
  const string tmp_hardover = tmp + "/malfunction/fail_hardover";
  const string tmp_stuck = tmp + "/malfunction/fail_stuck";

  PropertyManager->Tie( tmp_zero, this, &FGActuator::GetFailZero, &FGActuator::SetFailZero);
  PropertyManager->Tie( tmp_hardover, this, &FGActuator::GetFailHardover, &FGActuator::SetFailHardover);
  PropertyManager->Tie( tmp_stuck, this, &FGActuator::GetFailStuck, &FGActuator::SetFailStuck);
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

void FGActuator::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (InputSigns[0] < 0)
        cout << "      INPUT: -" << InputNodes[0]->getName() << endl;
      else
        cout << "      INPUT: " << InputNodes[0]->getName() << endl;

      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
      if (bias != 0.0) cout << "      Bias: " << bias << endl;
      if (rate_limit != 0) cout << "      Rate limit: " << rate_limit << endl;
      if (lag != 0) cout << "      Actuator lag: " << lag << endl;
      if (hysteresis_width != 0) cout << "      Hysteresis width: " << hysteresis_width << endl;
      if (deadband_width != 0) cout << "      Deadband width: " << deadband_width << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGActuator" << endl;
    if (from == 1) cout << "Destroyed:    FGActuator" << endl;
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
