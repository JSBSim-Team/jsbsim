/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGDistributor.cpp
 Author:       Jon S. Berndt
 Date started: 9/2013

 ------------- Copyright (C) 2013 -------------

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

Also, see the header file (FGDistributor.h) for further details.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGDistributor.h"
#include "models/FGFCS.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGDistributor::FGDistributor(FGFCS* fcs, Element* element)
  : FGFCSComponent(fcs, element)
{
  auto PropertyManager = fcs->GetPropertyManager();

  bind(element, PropertyManager.get()); // Bind() this component here in case it is used in its own
                                        // definition for a sample-and-hold

  string type_string = element->GetAttributeValue("type");
  if (type_string == "inclusive") Type = eInclusive;
  else if (type_string == "exclusive") Type = eExclusive;
  else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "Distributor type should be \"inclusive\" or \"exclusive\""
        << " but got \"" << type_string << "\" instead.\n";
    throw err;
  }

  Element* case_element = element->FindElement("case");
  while (case_element) {
    auto current_case = make_unique<Case>();
    Element* test_element = case_element->FindElement("test");
    try {
      if (test_element) current_case->SetTest(test_element, PropertyManager);
    } catch (XMLLogException&) {
      throw;
    } catch (LogException& e) {
      throw XMLLogException(e, test_element);
    } catch (const BaseException& e) {
      XMLLogException err(fcs->GetExec()->GetLogger(), test_element);
      err << LogFormat::RED << e.what() << LogFormat::RESET << "\n\n";
      throw err;
    }
    Element* prop_val_element = case_element->FindElement("property");
    while (prop_val_element) {
      string value_string = prop_val_element->GetAttributeValue("value");
      string property_string = prop_val_element->GetDataLine();
      current_case->AddPropValPair(property_string, value_string, PropertyManager,
                                   prop_val_element);
      prop_val_element = case_element->FindNextElement("property");
    }
    Cases.push_back(std::move(current_case));
    case_element = element->FindNextElement("case");
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGDistributor::Run(void )
{
  bool completed = false;
  for (auto& Case: Cases) { // Loop through all Cases
    if (Case->HasTest()) {
      if (Case->GetTestResult() && !((Type == eExclusive) && completed)) {
        Case->SetPropValPairs();
        completed = true;
      }
    } else { // If no test present, execute always
      Case->SetPropValPairs();
    }
  }

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

void FGDistributor::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
      unsigned int ctr=0;
      for (const auto& Case: Cases) {
        log << "      Case: " << fixed << ctr << "\n";
        if (Case->HasTest()) {
          Case->GetTest().PrintCondition("        ");
        } else {
          log << "        Set these properties by default: \n";
        }
        log << "\n";
        for (const auto& propVal: *Case) {
          log << "        Set property " << propVal->GetPropName();
          if (propVal->GetLateBoundProp()) log << " (late bound)";
          log << " to " << propVal->GetValString();
          if (propVal->GetLateBoundValue()) log << " (late bound)";
          log << "\n";
        }
        ctr++;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGDistributor\n";
    if (from == 1) log << "Destroyed:    FGDistributor\n";
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

} //namespace JSBSim
