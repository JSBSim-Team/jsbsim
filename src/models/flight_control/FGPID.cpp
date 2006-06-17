/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPID.cpp
 Author:       Jon S. Berndt
 Date started: 6/17/2006

 ------------- Copyright (C) 2006 Jon S. Berndt (jsb@hal-pc.org) -------------

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
Initial code 6/17/2006 JSB

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPID.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGPID.cpp,v 1.1 2006/06/17 17:03:00 jberndt Exp $";
static const char *IdHdr = ID_PID;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPID::FGPID(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  dt = fcs->GetState()->Getdt();

  Kp = Ki = Kd = 0.0;
  P_out = D_out = I_out = 0.0;
  Input_prev = Input_prev2 = 0.0;
  Trigger = 0;

  if (element->FindElement("kp")) Kp = element->FindElementValueAsNumber("kp");
  if (element->FindElement("ki")) Ki = element->FindElementValueAsNumber("ki");
  if (element->FindElement("kd")) Kd = element->FindElementValueAsNumber("kd");
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
  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  P_out = Kp * (Input - Input_prev);
  I_out = Ki * dt * Input;
  D_out = (Kd / dt) * (Input - 2*Input_prev + Input_prev2);

  if (Trigger != 0) {
    double test = Trigger->getDoubleValue();
    if (fabs(test) > 0.000001) {
      I_out = 0.0;
    }
  }

  Input_prev = Input;
  Input_prev2 = Input_prev;

  Output += P_out + I_out + D_out;

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
