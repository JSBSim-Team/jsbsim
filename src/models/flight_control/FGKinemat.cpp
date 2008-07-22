/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGKinemat.cpp
 Author:       Tony Peden, for flight control system authored by Jon S. Berndt
 Date started: 12/02/01

 ------------- Copyright (C) 2000 Anthony K. Peden -------------

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

#include "FGKinemat.h"
#include <math.h>
#include <float.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGKinemat.cpp,v 1.6 2008/07/22 02:42:18 jberndt Exp $";
static const char *IdHdr = ID_FLAPS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGKinemat::FGKinemat(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Element *traverse_element, *setting_element;
  double tmpDetent;
  double tmpTime;

  Detents.clear();
  TransitionTimes.clear();

  Output = OutputPct = 0;
  DoScale = true;

  if (element->FindElement("noscale")) DoScale = false;

  traverse_element = element->FindElement("traverse");
  setting_element = traverse_element->FindElement("setting");
  while (setting_element) {
    tmpDetent = setting_element->FindElementValueAsNumber("position");
    tmpTime = setting_element->FindElementValueAsNumber("time");
    Detents.push_back(tmpDetent);
    TransitionTimes.push_back(tmpTime);
    setting_element = traverse_element->FindNextElement("setting");
  }
  NumDetents = Detents.size();

  if (NumDetents <= 1) {
    cerr << "Kinematic component " << Name
         << " must have more than 1 setting element" << endl;
    exit(-1);
  }

  FGFCSComponent::bind();
//  treenode->Tie("output-norm", this, &FGKinemat::GetOutputPct );

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
  double dt = fcs->GetState()->Getdt();

  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  if (DoScale) Input *= Detents[NumDetents-1];

  if (IsOutput) Output = OutputNode->getDoubleValue();

  if (Input < Detents[0])
    Input = Detents[0];
  else if (Detents[NumDetents-1] < Input)
    Input = Detents[NumDetents-1];

  // Process all detent intervals the movement traverses until either the
  // final value is reached or the time interval has finished.
  while ( 0.0 < dt && !EqualToRoundoff(Input, Output) ) {

    // Find the area where Output is in
    int ind;
    for (ind = 1; (Input < Output) ? Detents[ind] < Output : Detents[ind] <= Output ; ++ind)
      if (NumDetents <= ind)
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
      double ThisInput = Input;
      if (ThisInput < Detents[ind-1])   ThisInput = Detents[ind-1];
      if (Detents[ind] < ThisInput)     ThisInput = Detents[ind];
      // Compute the time to reach the value in ThisInput
      double ThisDt = fabs((ThisInput-Output)/Rate);

      // and clip to the timestep size
      if (dt < ThisDt) {
        ThisDt = dt;
        if (Output < Input)
          Output += ThisDt*Rate;
        else
          Output -= ThisDt*Rate;
      } else
        // Handle this case separate to make shure the termination condition
        // is met even in inexact arithmetics ...
        Output = ThisInput;

      dt -= ThisDt;
    }
  }

  OutputPct = (Output-Detents[0])/(Detents[NumDetents-1]-Detents[0]);

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

void FGKinemat::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      INPUT: " << InputNodes[0]->getName() << endl;
      cout << "      DETENTS: " << NumDetents << endl;
      for (int i=0;i<NumDetents;i++) {
        cout << "        " << Detents[i] << " " << TransitionTimes[i] << endl;
      }
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
      if (!DoScale) cout << "      NOSCALE" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGKinemat" << endl;
    if (from == 1) cout << "Destroyed:    FGKinemat" << endl;
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
