/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFilter.cpp
 Author:       Jon S. Berndt
 Date started: 11/2000

 ------------- Copyright (C) 2000 -------------

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

#include "FGFilter.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"

#include <iostream>
#include <string>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGFilter.cpp,v 1.17 2011/04/18 08:51:12 andgi Exp $";
static const char *IdHdr = ID_FILTER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFilter::FGFilter(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Trigger = 0;
  DynamicFilter = false;

  C[1] = C[2] = C[3] = C[4] = C[5] = C[6] = 0.0;
  for (int i=1; i<7; i++) {
    PropertySign[i] = 1.0;
    PropertyNode[i] = 0L;
    ReadFilterCoefficients(element, i);
  }

  if      (Type == "LAG_FILTER")          FilterType = eLag        ;
  else if (Type == "LEAD_LAG_FILTER")     FilterType = eLeadLag    ;
  else if (Type == "SECOND_ORDER_FILTER") FilterType = eOrder2     ;
  else if (Type == "WASHOUT_FILTER")      FilterType = eWashout    ;
  else if (Type == "INTEGRATOR")          FilterType = eIntegrator ;
  else                                    FilterType = eUnknown    ;

  if (element->FindElement("trigger")) {
    Trigger =  PropertyManager->GetNode(element->FindElementValue("trigger"));
  }

  Initialize = true;

  CalculateDynamicFilters();

  FGFCSComponent::bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFilter::~FGFilter()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::ReadFilterCoefficients(Element* element, int index)
{
  // index is known to be 1-7. 
  // A stringstream would be overkill, but also trying to avoid sprintf
  string coefficient = "c0";
  coefficient[1] += index;
  
  if ( element->FindElement(coefficient) ) {
    string property_string = element->FindElementValue(coefficient);
    if (!is_number(property_string)) { // property
      if (property_string[0] == '-') {
       PropertySign[index] = -1.0;
       property_string.erase(0,1);
      } else {
       PropertySign[index] = 1.0;
      }
      PropertyNode[index] = PropertyManager->GetNode(property_string);
      DynamicFilter = true;
    } else {
      C[index] = element->FindElementValueAsNumber(coefficient);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::CalculateDynamicFilters(void)
{
  double denom;

  switch (FilterType) {
    case eLag:
      if (PropertyNode[1] != 0L) C[1] = PropertyNode[1]->getDoubleValue()*PropertySign[1];
      denom = 2.00 + dt*C[1];
      ca = dt*C[1] / denom;
      cb = (2.00 - dt*C[1]) / denom;

      break;
    case eLeadLag:
      if (PropertyNode[1] != 0L) C[1] = PropertyNode[1]->getDoubleValue()*PropertySign[1];
      if (PropertyNode[2] != 0L) C[2] = PropertyNode[2]->getDoubleValue()*PropertySign[2];
      if (PropertyNode[3] != 0L) C[3] = PropertyNode[3]->getDoubleValue()*PropertySign[3];
      if (PropertyNode[4] != 0L) C[4] = PropertyNode[4]->getDoubleValue()*PropertySign[4];
      denom = 2.00*C[3] + dt*C[4];
      ca = (2.00*C[1] + dt*C[2]) / denom;
      cb = (dt*C[2] - 2.00*C[1]) / denom;
      cc = (2.00*C[3] - dt*C[4]) / denom;
      break;
    case eOrder2:
      if (PropertyNode[1] != 0L) C[1] = PropertyNode[1]->getDoubleValue()*PropertySign[1];
      if (PropertyNode[2] != 0L) C[2] = PropertyNode[2]->getDoubleValue()*PropertySign[2];
      if (PropertyNode[3] != 0L) C[3] = PropertyNode[3]->getDoubleValue()*PropertySign[3];
      if (PropertyNode[4] != 0L) C[4] = PropertyNode[4]->getDoubleValue()*PropertySign[4];
      if (PropertyNode[5] != 0L) C[5] = PropertyNode[5]->getDoubleValue()*PropertySign[5];
      if (PropertyNode[6] != 0L) C[6] = PropertyNode[6]->getDoubleValue()*PropertySign[6];
      denom = 4.0*C[4] + 2.0*C[5]*dt + C[6]*dt*dt;
      ca = (4.0*C[1] + 2.0*C[2]*dt + C[3]*dt*dt) / denom;
      cb = (2.0*C[3]*dt*dt - 8.0*C[1]) / denom;
      cc = (4.0*C[1] - 2.0*C[2]*dt + C[3]*dt*dt) / denom;
      cd = (2.0*C[6]*dt*dt - 8.0*C[4]) / denom;
      ce = (4.0*C[4] - 2.0*C[5]*dt + C[6]*dt*dt) / denom;
      break;
    case eWashout:
      if (PropertyNode[1] != 0L) C[1] = PropertyNode[1]->getDoubleValue()*PropertySign[1];
      denom = 2.00 + dt*C[1];
      ca = 2.00 / denom;
      cb = (2.00 - dt*C[1]) / denom;
      break;
    case eIntegrator:
      if (PropertyNode[1] != 0L) C[1] = PropertyNode[1]->getDoubleValue()*PropertySign[1];
      ca = dt*C[1] / 2.00;
      break;
    case eUnknown:
      cerr << "Unknown filter type" << endl;
    break;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFilter::Run(void)
{
  double test = 0.0;

  if (Initialize) {

    PreviousOutput1 = PreviousInput1 = Output = Input;
    Initialize = false;

  } else {

    Input = InputNodes[0]->getDoubleValue() * InputSigns[0];
    
    if (DynamicFilter) CalculateDynamicFilters();
    
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
        if (Trigger != 0) {
          test = Trigger->getDoubleValue();
          if (fabs(test) > 0.000001) {
            Input  = PreviousInput1 = PreviousInput2 = 0.0;
          }
        }
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
  string sgn="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      INPUT: " << InputNodes[0]->GetName() << endl;
        switch (FilterType) {
        case eLag:
          if (PropertySign[1] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[1] == 0L) cout << "      C[1]: " << C[1] << endl;
          else cout << "      C[1] is the value of property: " << sgn << PropertyNode[1]->GetName() << endl;
          break;
        case eLeadLag:
          if (PropertySign[1] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[1] == 0L) cout << "      C[1]: " << C[1] << endl;
          else cout << "      C[1] is the value of property: " << sgn << PropertyNode[1]->GetName() << endl;
          if (PropertySign[2] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[2] == 0L) cout << "      C[2]: " << C[2] << endl;
          else cout << "      C[2] is the value of property: " << sgn << PropertyNode[2]->GetName() << endl;
          if (PropertySign[3] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[3] == 0L) cout << "      C[3]: " << C[3] << endl;
          else cout << "      C[3] is the value of property: " << sgn << PropertyNode[3]->GetName() << endl;
          if (PropertySign[4] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[4] == 0L) cout << "      C[4]: " << C[4] << endl;
          else cout << "      C[4] is the value of property: " << sgn << PropertyNode[4]->GetName() << endl;
          break;
        case eOrder2:
          if (PropertySign[1] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[1] == 0L) cout << "      C[1]: " << C[1] << endl;
          else cout << "      C[1] is the value of property: " << sgn << PropertyNode[1]->GetName() << endl;
          if (PropertySign[2] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[2] == 0L) cout << "      C[2]: " << C[2] << endl;
          else cout << "      C[2] is the value of property: " << sgn << PropertyNode[2]->GetName() << endl;
          if (PropertySign[3] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[3] == 0L) cout << "      C[3]: " << C[3] << endl;
          else cout << "      C[3] is the value of property: " << sgn << PropertyNode[3]->GetName() << endl;
          if (PropertySign[4] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[4] == 0L) cout << "      C[4]: " << C[4] << endl;
          else cout << "      C[4] is the value of property: " << sgn << PropertyNode[4]->GetName() << endl;
          if (PropertySign[5] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[5] == 0L) cout << "      C[5]: " << C[5] << endl;
          else cout << "      C[5] is the value of property: " << sgn << PropertyNode[5]->GetName() << endl;
          if (PropertySign[6] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[6] == 0L) cout << "      C[6]: " << C[6] << endl;
          else cout << "      C[6] is the value of property: " << sgn << PropertyNode[6]->GetName() << endl;
          break;
        case eWashout:
          if (PropertySign[1] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[1] == 0L) cout << "      C[1]: " << C[1] << endl;
          else cout << "      C[1] is the value of property: " << sgn << PropertyNode[1]->GetName() << endl;
          break;
        case eIntegrator:
          if (PropertySign[1] < 0.0) sgn="-";
          else sgn = "";
          if (PropertyNode[1] == 0L) cout << "      C[1]: " << C[1] << endl;
          else cout << "      C[1] is the value of property: " << sgn << PropertyNode[1]->GetName() << endl;
          break;
        case eUnknown:
          break;
       } 
      if (IsOutput) {
        for (unsigned int i=0; i<OutputNodes.size(); i++)
          cout << "      OUTPUT: " << OutputNodes[i]->getName() << endl;
      }
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
