/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFilter.cpp
 Author:       Jon S. Berndt
 Date started: 11/2000

 ------------- Copyright (C) 2000 -------------

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

#include "FGFilter.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGFilter.cpp,v 1.2 2005/06/13 00:54:45 jberndt Exp $";
static const char *IdHdr = ID_FILTER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFilter::FGFilter(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  double denom;

  dt = fcs->GetState()->Getdt();
  Trigger = 0;

  C1 = C2 = C3 = C4 = C5 = C6 = 0.0;

  if      (Type == "LAG_FILTER")          FilterType = eLag        ;
  else if (Type == "LEAD_LAG_FILTER")     FilterType = eLeadLag    ;
  else if (Type == "SECOND_ORDER_FILTER") FilterType = eOrder2     ;
  else if (Type == "WASHOUT_FILTER")      FilterType = eWashout    ;
  else if (Type == "INTEGRATOR")          FilterType = eIntegrator ;
  else                                    FilterType = eUnknown    ;

  if (element->FindElement("c1")) C1 = element->FindElementValueAsNumber("c1");
  if (element->FindElement("c2")) C2 = element->FindElementValueAsNumber("c2");
  if (element->FindElement("c3")) C3 = element->FindElementValueAsNumber("c3");
  if (element->FindElement("c4")) C4 = element->FindElementValueAsNumber("c4");
  if (element->FindElement("c5")) C5 = element->FindElementValueAsNumber("c5");
  if (element->FindElement("c6")) C6 = element->FindElementValueAsNumber("c6");
  if (element->FindElement("trigger")) {
    Trigger =  resolveSymbol(element->FindElementValue("trigger"));
  }

  Initialize = true;

  switch (FilterType) {
    case eLag:
      denom = 2.00 + dt*C1;
      ca = dt*C1 / denom;
      cb = (2.00 - dt*C1) / denom;
      break;
    case eLeadLag:
      denom = 2.00*C3 + dt*C4;
      ca = (2.00*C1 + dt*C2) / denom;
      cb = (dt*C2 - 2.00*C1) / denom;
      cc = (2.00*C3 - dt*C4) / denom;
      break;
    case eOrder2:
      denom = 4.0*C4 + 2.0*C5*dt + C6*dt*dt;
      ca = (4.0*C1 + 2.0*C2*dt + C3*dt*dt) / denom;
      cb = (2.0*C3*dt*dt - 8.0*C1) / denom;
      cc = (4.0*C1 - 2.0*C2*dt + C3*dt*dt) / denom;
      cd = (2.0*C6*dt*dt - 8.0*C4) / denom;
      ce = (4.0*C4 - 2.0*C5*dt + C6*dt*dt) / denom;
      break;
    case eWashout:
      denom = 2.00 + dt*C1;
      ca = 2.00 / denom;
      cb = (2.00 - dt*C1) / denom;
      break;
    case eIntegrator:
      ca = dt*C1 / 2.00;
      break;
    case eUnknown:
      cerr << "Unknown filter type" << endl;
    break;
  }
  FGFCSComponent::bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFilter::~FGFilter()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFilter::Run(void)
{
  int test = 0;

  if (Initialize) {

    PreviousOutput1 = PreviousInput1 = Output = Input;
    Initialize = false;

  } else if (Trigger != 0) {
    test = Trigger->getIntValue();
    if (test < 0) {
      Input  = PreviousInput1 = PreviousInput2 = 0.0;
    } else {
      Output = PreviousOutput1 = PreviousOutput2 = 0.0;
    }

  } else {

    Input = InputNodes[0]->getDoubleValue() * InputSigns[0];
    switch (FilterType) {
      case eLag:
        Output = Input * ca + PreviousInput1 * ca + PreviousOutput1 * cb;
        break;
      case eLeadLag:
        Output = Input * ca + PreviousInput1 * cb + PreviousOutput1 * cc;
        break;
      case eOrder2:
        Output = Input * ca + PreviousInput1 * cb + PreviousInput2 * cc
                            - PreviousOutput1 * cd - PreviousOutput2 * ce;
        break;
      case eWashout:
        Output = Input * ca - PreviousInput1 * ca + PreviousOutput1 * cb;
        break;
      case eIntegrator:
        Output = Input * ca + PreviousInput1 * ca + PreviousOutput1;
        break;
      case eUnknown:
        break;
    }

  }

  PreviousOutput2 = PreviousOutput1;
  PreviousOutput1 = Output;
  PreviousInput2  = PreviousInput1;
  PreviousInput1  = Input;

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

void FGFilter::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      INPUT: " << InputNodes[0]->getName() << endl;
      cout << "      C1: " << C1 << endl;
      cout << "      C2: " << C2 << endl;
      cout << "      C3: " << C3 << endl;
      cout << "      C4: " << C4 << endl;
      cout << "      C5: " << C5 << endl;
      cout << "      C6: " << C6 << endl;
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFilter" << endl;
    if (from == 1) cout << "Destroyed:    FGFilter" << endl;
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
