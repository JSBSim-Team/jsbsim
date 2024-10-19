/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFilter.cpp
 Author:       Jon S. Berndt
 Date started: 11/2000

 ------------- Copyright (C) 2000 -------------

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

#include "FGFilter.h"
#include "models/FGFCS.h"
#include "math/FGParameterValue.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFilter::FGFilter(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element), DynamicFilter(false), Initialize(true)
{
  C[1] = C[2] = C[3] = C[4] = C[5] = C[6] = nullptr;

  CheckInputNodes(1, 1, element);

  auto PropertyManager = fcs->GetPropertyManager();
  for (int i=1; i<7; i++)
    ReadFilterCoefficients(element, i, PropertyManager);

  if      (Type == "LAG_FILTER")          FilterType = eLag        ;
  else if (Type == "LEAD_LAG_FILTER")     FilterType = eLeadLag    ;
  else if (Type == "SECOND_ORDER_FILTER") FilterType = eOrder2     ;
  else if (Type == "WASHOUT_FILTER")      FilterType = eWashout    ;
  else                                    FilterType = eUnknown    ;

  CalculateDynamicFilters();

  bind(element, PropertyManager.get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFilter::~FGFilter()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::ResetPastStates(void)
{
  FGFCSComponent::ResetPastStates();

  Input = 0.0; Initialize = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::ReadFilterCoefficients(Element* element, int index,
                                      std::shared_ptr<FGPropertyManager> PropertyManager)
{
  // index is known to be 1-7.
  // A stringstream would be overkill, but also trying to avoid sprintf
  string coefficient = "c0";
  coefficient[1] += index;

  if ( element->FindElement(coefficient) ) {
    C[index] = new FGParameterValue(element->FindElement(coefficient),
                                    PropertyManager);
    DynamicFilter |= !C[index]->IsConstant();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::CalculateDynamicFilters(void)
{
  double denom;

  switch (FilterType) {
    case eLag:
      denom = 2.0 + dt*C[1];
      ca = dt*C[1] / denom;
      cb = (2.0 - dt*C[1]) / denom;

      break;
    case eLeadLag:
      denom = 2.0*C[3] + dt*C[4];
      ca = (2.0*C[1] + dt*C[2]) / denom;
      cb = (dt*C[2] - 2.0*C[1]) / denom;
      cc = (2.0*C[3] - dt*C[4]) / denom;
      break;
    case eOrder2:
      denom = 4.0*C[4] + 2.0*C[5]*dt + C[6]*dt*dt;
      ca = (4.0*C[1] + 2.0*C[2]*dt + C[3]*dt*dt) / denom;
      cb = (2.0*C[3]*dt*dt - 8.0*C[1]) / denom;
      cc = (4.0*C[1] - 2.0*C[2]*dt + C[3]*dt*dt) / denom;
      cd = (2.0*C[6]*dt*dt - 8.0*C[4]) / denom;
      ce = (4.0*C[4] - 2.0*C[5]*dt + C[6]*dt*dt) / denom;
      break;
    case eWashout:
      denom = 2.0 + dt*C[1];
      ca = 2.0 / denom;
      cb = (2.0 - dt*C[1]) / denom;
      break;
    case eUnknown:
    {
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::ERROR);
      log << "Unknown filter type\n";
    }
    break;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFilter::Run(void)
{
  if (Initialize) {

    PreviousOutput2 = PreviousInput2 = PreviousOutput1 = PreviousInput1 = Output = Input;
    Initialize = false;

  } else {

    Input = InputNodes[0]->getDoubleValue();

    if (DynamicFilter) CalculateDynamicFilters();

    switch (FilterType) {
      case eLag:
        Output = (Input + PreviousInput1) * ca + PreviousOutput1 * cb;
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
      case eUnknown:
        break;
    }

  }

  PreviousOutput2 = PreviousOutput1;
  PreviousOutput1 = Output;
  PreviousInput2  = PreviousInput1;
  PreviousInput1  = Input;

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

void FGFilter::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
      log << "      INPUT: " << InputNodes[0]->GetName() << fixed << "\n";

      for (int i=1; i < 7; i++) {
        if (!C[i]) break;

        log << "      C[" << i << "]";
        if (!C[i]->IsConstant()) log << " is the value of property";
        log << ": "<< C[i]->GetName() << "\n";
      }

      for (auto node: OutputNodes)
        log << "      OUTPUT: " << node->getNameString() << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGFilter\n";
    if (from == 1) log << "Destroyed:    FGFilter\n";
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
