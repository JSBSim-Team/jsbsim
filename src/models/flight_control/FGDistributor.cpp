/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGDistributor.cpp
 Author:       Jon S. Berndt
 Date started: 9/2013

 ------------- Copyright (C) 2013 -------------

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

Also, see the header file (FGDistributor.h) for further details.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
 
#include "FGDistributor.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

IDENT(IdSrc,"$Id: FGDistributor.cpp,v 1.7 2015/02/27 20:46:01 bcoconni Exp $");
IDENT(IdHdr,ID_DISTRIBUTOR);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGDistributor::FGDistributor(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Element *case_element=0;
  Element* test_element=0;
  Element* prop_val_element=0;
  string type_string;
  Case* current_case=0;

  FGFCSComponent::bind(); // Bind() this component here in case it is used
                          // in its own definition for a sample-and-hold

  type_string = element->GetAttributeValue("type");
  if (type_string == "inclusive") Type = eInclusive;
  else if (type_string == "exclusive") Type = eExclusive;
  else {
    throw("Not a known Distributor type, "+type_string);
  }

  case_element = element->FindElement("case");
  while (case_element) {
    current_case = new Case;
    test_element = case_element->FindElement("test");
    if (test_element) current_case->SetTest(new FGCondition(test_element, PropertyManager));
    prop_val_element = case_element->FindElement("property");
    while (prop_val_element) {
      string value_string = prop_val_element->GetAttributeValue("value");
      string property_string = prop_val_element->GetDataLine();
      current_case->AddPropValPair(new PropValPair(property_string, value_string, PropertyManager));
      prop_val_element = case_element->FindNextElement("property");
    }
    Cases.push_back(current_case);
    case_element = element->FindNextElement("case");
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGDistributor::~FGDistributor()
{

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGDistributor::Run(void )
{
  bool completed = false;
  for (unsigned int ctr=0; ctr<Cases.size(); ctr++) { // Loop through all Cases
    if (Cases[ctr]->HasTest()) {                      
      if (Cases[ctr]->GetTestResult() && !((Type == eExclusive) && completed)) {
        Cases[ctr]->SetPropValPairs();
        completed = true;
      }
    } else {                                          // If no test present, execute always
      Cases[ctr]->SetPropValPairs();
    }
  }

//  if (delay != 0) Delay();
//  Clip();
//  if (IsOutput) SetOutput();

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

void FGDistributor::Debug(int from)
{
  string comp, scratch;
  string indent = "        ";
  //bool first = false;

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      for (unsigned int ctr=0; ctr < Cases.size(); ctr++) {
        std::cout << "      Case: " << ctr << endl;
        if (Cases[ctr]->GetTest() != 0) {
          Cases[ctr]->GetTest()->PrintCondition("        ");
        } else {
          std::cout << "        Set these properties by default: " << std::endl;
        }
        std::cout << std::endl;
        for (unsigned int propCtr=0; propCtr < Cases[ctr]->GetNumPropValPairs(); propCtr++) {
          std::cout << "        Set property " << Cases[ctr]->GetPropValPair(propCtr)->GetPropName();
          if (Cases[ctr]->GetPropValPair(propCtr)->GetPropNode() == 0) std::cout << " (late bound)";
          std::cout << " to " << Cases[ctr]->GetPropValPair(propCtr)->GetValString();
          if (Cases[ctr]->GetPropValPair(propCtr)->GetLateBoundValue()) std::cout << " (late bound)";
          std::cout << std::endl;
        }
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGDistributor" << endl;
    if (from == 1) cout << "Destroyed:    FGDistributor" << endl;
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

} //namespace JSBSim

