/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGKinemat.cpp
 Author:       Tony Peden, for flight control system authored by Jon S. Berndt
 Date started: 12/02/01

 ------------- Copyright (C) 2000 Anthony K. Peden -------------

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

#include "FGKinemat.h"
#include "input_output/FGXMLElement.h"
#include "models/FGFCS.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGKinemat::FGKinemat(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element)
{
  CheckInputNodes(1, 1, element);

  Output = 0;
  DoScale = true;

  if (element->FindElement("noscale")) DoScale = false;

  Element* traverse_element = element->FindElement("traverse");
  Element* setting_element = traverse_element->FindElement("setting");
  while (setting_element) {
    double tmpDetent = setting_element->FindElementValueAsNumber("position");
    double tmpTime = setting_element->FindElementValueAsNumber("time");
    Detents.push_back(tmpDetent);
    TransitionTimes.push_back(tmpTime);
    setting_element = traverse_element->FindNextElement("setting");
  }

  if (Detents.size() <= 1) {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "\nKinematic component " << Name
        << " must have more than 1 setting element\n";
    throw err;
  }

  bind(element, fcs->GetPropertyManager().get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGKinemat::~FGKinemat()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGKinemat::Run(void )
{
  double dt0 = dt;

  Input = InputNodes[0]->getDoubleValue();

  if (DoScale) Input *= Detents.back();

  if (!OutputNodes.empty())
    Output = OutputNodes[0]->getDoubleValue();

  Input = Constrain(Detents.front(), Input, Detents.back());

  if (fcs->GetTrimStatus())
    // When trimming the output must be reached in one step
    Output = Input;
  else {
    // Process all detent intervals the movement traverses until either the
    // final value is reached or the time interval has finished.
    while ( dt0 > 0.0 && !EqualToRoundoff(Input, Output) ) {

      // Find the area where Output is in
      unsigned int ind;
      for (ind = 1; (Input < Output) ? Detents[ind] < Output : Detents[ind] <= Output ; ++ind)
        if (ind >= Detents.size())
          break;

      // A transition time of 0.0 means an infinite rate.
      // The output is reached in one step
      if (TransitionTimes[ind] <= 0.0) {
        Output = Input;
        break;
      } else {
        // Compute the rate in this area
        double Rate = (Detents[ind] - Detents[ind-1])/TransitionTimes[ind];
        // Compute the maximum input value inside this area
        double ThisInput = Constrain(Detents[ind-1], Input, Detents[ind]);
        // Compute the time to reach the value in ThisInput
        double ThisDt = fabs((ThisInput-Output)/Rate);

        // and clip to the timestep size
        if (dt0 < ThisDt) {
          ThisDt = dt0;
          if (Output < Input)
            Output += ThisDt*Rate;
          else
            Output -= ThisDt*Rate;
        } else
          // Handle this case separate to make shure the termination condition
          // is met even in inexact arithmetics ...
          Output = ThisInput;

        dt0 -= ThisDt;
      }
    }
  }

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

void FGKinemat::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) { // Constructor
      log << "      INPUT: " << InputNodes[0]->GetName() << "\n";
      log << "      DETENTS: " << Detents.size() << fixed << setprecision(4) << "\n";
      for (unsigned int i=0;i<Detents.size();i++) {
        log << "        " << Detents[i] << " " << TransitionTimes[i] << "\n";
      }
      for (auto node: OutputNodes)
          log << "      OUTPUT: " << node->getNameString() << "\n";
      if (!DoScale) log << "      NOSCALE\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGKinemat\n";
    if (from == 1) log << "Destroyed:    FGKinemat\n";
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
