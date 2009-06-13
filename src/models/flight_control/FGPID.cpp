/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPID.cpp
 Author:       Jon S. Berndt
 Date started: 6/17/2006

 ------------- Copyright (C) 2006 Jon S. Berndt (jsb@hal-pc.org) -------------

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
Initial code 6/17/2006 JSB

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPID.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGPID.cpp,v 1.11 2009/06/13 02:41:59 jberndt Exp $";
static const char *IdHdr = ID_PID;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPID::FGPID(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  string kp_string, ki_string, kd_string;
  dt = fcs->GetState()->Getdt();

  Kp = Ki = Kd = 0.0;
  KpPropertyNode = 0;
  KiPropertyNode = 0;
  KdPropertyNode = 0;
  KpPropertySign = 1.0;
  KiPropertySign = 1.0;
  KdPropertySign = 1.0;
  I_out_total = 0.0;
  Input_prev = Input_prev2 = 0.0;
  Trigger = 0;

  if ( element->FindElement("kp") ) {
    kp_string = element->FindElementValue("kp");
    if (!is_number(kp_string)) { // property
      if (kp_string[0] == '-') {
       KpPropertySign = -1.0;
       kp_string.erase(0,1);
      }
      KpPropertyNode = PropertyManager->GetNode(kp_string);
    } else {
      Kp = element->FindElementValueAsNumber("kp");
    }
  }

  if ( element->FindElement("ki") ) {
    ki_string = element->FindElementValue("ki");
    if (!is_number(ki_string)) { // property
      if (ki_string[0] == '-') {
       KiPropertySign = -1.0;
       ki_string.erase(0,1);
      }
      KiPropertyNode = PropertyManager->GetNode(ki_string);
    } else {
      Ki = element->FindElementValueAsNumber("ki");
    }
  }

  if ( element->FindElement("kd") ) {
    kd_string = element->FindElementValue("kd");
    if (!is_number(kd_string)) { // property
      if (kd_string[0] == '-') {
       KdPropertySign = -1.0;
       kd_string.erase(0,1);
      }
      KdPropertyNode = PropertyManager->GetNode(kd_string);
    } else {
      Kd = element->FindElementValueAsNumber("kd");
    }
  }

  if (element->FindElement("trigger")) {
    Trigger =  PropertyManager->GetNode(element->FindElementValue("trigger"));
  }

  FGFCSComponent::bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPID::~FGPID()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPID::Run(void )
{
  double I_out_delta = 0.0;
  double P_out, D_out;

  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  if (KpPropertyNode != 0) Kp = KpPropertyNode->getDoubleValue() * KpPropertySign;
  if (KiPropertyNode != 0) Ki = KiPropertyNode->getDoubleValue() * KiPropertySign;
  if (KdPropertyNode != 0) Kd = KdPropertyNode->getDoubleValue() * KdPropertySign;

  P_out = Kp * Input;
  D_out = (Kd / dt) * (Input - Input_prev);

  // Do not continue to integrate the input to the integrator if a wind-up
  // condition is sensed - that is, if the property pointed to by the trigger
  // element is non-zero. Reset the integrator to 0.0 if the Trigger value
  // is negative.

  if (Trigger != 0) {
    double test = Trigger->getDoubleValue();
    if (fabs(test) < 0.000001) I_out_delta = Ki * dt * Input;  // Normal
    if (test < 0.0)            I_out_total = 0.0;  // Reset integrator to 0.0
  } else { // no anti-wind-up trigger defined
    I_out_delta = Ki * dt * Input;
  }
  
  I_out_total += I_out_delta;

  Output = P_out + I_out_total + D_out;
  
  Input_prev = Input;
  Input_prev2 = Input_prev;

  Clip();
  if (IsOutput) SetOutput();

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
      if (InputSigns[0] < 0)
        cout << "      INPUT: -" << InputNodes[0]->getName() << endl;
      else
        cout << "      INPUT: " << InputNodes[0]->getName() << endl;

      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
