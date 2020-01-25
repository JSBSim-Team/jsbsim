/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPID.cpp
 Author:       Jon S. Berndt
 Date started: 6/17/2006

 ------------- Copyright (C) 2006 Jon S. Berndt (jon@jsbsim.org) -------------

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
Initial code 6/17/2006 JSB

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPID.h"
#include "math/FGParameterValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPID::FGPID(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Element *el;

  I_out_total = 0.0;
  Input_prev = Input_prev2 = 0.0;
  Trigger = nullptr;
  ProcessVariableDot = nullptr;
  IsStandard = false;
  IntType = eNone;       // No integrator initially defined.

  string pid_type = element->GetAttributeValue("type");

  if (pid_type == "standard") IsStandard = true;

  el = element->FindElement("kp");
  if (el)
    Kp = new FGParameterValue(el, PropertyManager);
  else
    Kp = new FGRealValue(0.0);

  el = element->FindElement("ki");
  if (el) {
    string integ_type = el->GetAttributeValue("type");
    if (integ_type == "rect") {            // Use rectangular integration
      IntType = eRectEuler;
    } else if (integ_type == "trap") {     // Use trapezoidal integration
      IntType = eTrapezoidal;
    } else if (integ_type == "ab2") {      // Use Adams Bashforth 2nd order integration
      IntType = eAdamsBashforth2;
    } else if (integ_type == "ab3") {      // Use Adams Bashforth 3rd order integration
      IntType = eAdamsBashforth3;
    } else {                               // Use default Adams Bashforth 2nd order integration
      IntType = eAdamsBashforth2;
    }

    Ki = new FGParameterValue(el, PropertyManager);
  }
  else
    Ki = new FGRealValue(0.0);


  el = element->FindElement("kd");
  if (el)
    Kd = new FGParameterValue(el, PropertyManager);
  else
    Kd = new FGRealValue(0.0);

  el = element->FindElement("pvdot");
  if (el)
    ProcessVariableDot = new FGPropertyValue(el->GetDataLine(), PropertyManager);

  el = element->FindElement("trigger");
  if (el)
    Trigger = new FGPropertyValue(el->GetDataLine(), PropertyManager);

  bind(el);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPID::bind(Element *el)
{
  FGFCSComponent::bind(el);

  string tmp;
  if (Name.find("/") == string::npos) {
    tmp = "fcs/" + PropertyManager->mkPropertyName(Name, true);
  } else {
    tmp = Name;
  }
  typedef double (FGPID::*PMF)(void) const;
  PropertyManager->Tie(tmp+"/initial-integrator-value", this, (PMF)nullptr,
                       &FGPID::SetInitialOutput);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPID::~FGPID()
{
  delete Kp;
  delete Ki;
  delete Kd;
  delete Trigger;
  delete ProcessVariableDot;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPID::ResetPastStates(void)
{
  FGFCSComponent::ResetPastStates();

  Input_prev = Input_prev2 = Output = I_out_total = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPID::Run(void )
{
  double I_out_delta = 0.0;
  double Dval = 0;

  Input = InputNodes[0]->getDoubleValue();

  if (ProcessVariableDot) {
    Dval = ProcessVariableDot->getDoubleValue();
  } else {
    Dval = (Input - Input_prev)/dt;
  }

  // Do not continue to integrate the input to the integrator if a wind-up
  // condition is sensed - that is, if the property pointed to by the trigger
  // element is non-zero. Reset the integrator to 0.0 if the Trigger value
  // is negative.

  double test = 0.0;
  if (Trigger) test = Trigger->getDoubleValue();

  if (fabs(test) < 0.000001) {
    switch(IntType) {
    case eRectEuler:
      I_out_delta = Input;                         // Normal rectangular integrator
      break;
    case eTrapezoidal:
      I_out_delta = 0.5 * (Input + Input_prev);    // Trapezoidal integrator
      break;
    case eAdamsBashforth2:
      I_out_delta = 1.5*Input - 0.5*Input_prev;  // 2nd order Adams Bashforth integrator
      break;
    case eAdamsBashforth3:                                   // 3rd order Adams Bashforth integrator
      I_out_delta = (23.0*Input - 16.0*Input_prev + 5.0*Input_prev2) / 12.0;
      break;
    case eNone:
      // No integrator is defined or used.
      I_out_delta = 0.0;
      break;
    }
  }

  if (test < 0.0) I_out_total = 0.0;  // Reset integrator to 0.0
  
  I_out_total += Ki->GetValue() * dt * I_out_delta;

  if (IsStandard)
    Output = Kp->GetValue() * (Input + I_out_total + Kd->GetValue()*Dval);
  else
    Output = Kp->GetValue()*Input + I_out_total + Kd->GetValue()*Dval;

  Input_prev2 = test < 0.0 ? 0.0:Input_prev;
  Input_prev = Input;

  Clip();
  SetOutput();

  return true;
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

void FGPID::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      INPUT: " << InputNodes[0]->GetNameWithSign() << endl;

      for (auto node: OutputNodes)
        cout << "      OUTPUT: " << node->getName() << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPID" << endl;
    if (from == 1) cout << "Destroyed:    FGPID" << endl;
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
