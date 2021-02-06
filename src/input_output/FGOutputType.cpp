/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputType.cpp
 Author:       Bertrand Coconnier
 Date started: 09/10/11
 Purpose:      Manage output of sim parameters to file or stdout

  ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
09/10/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <ostream>

#include "FGFDMExec.h"
#include "FGOutputType.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"
#include "math/FGTemplateFunc.h"
#include "math/FGFunctionValue.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutputType::FGOutputType(FGFDMExec* fdmex) :
  FGModel(fdmex),
  SubSystems(0),
  enabled(true)
{
  Aerodynamics = FDMExec->GetAerodynamics();
  Auxiliary = FDMExec->GetAuxiliary();
  Aircraft = FDMExec->GetAircraft();
  Atmosphere = FDMExec->GetAtmosphere();
  Winds = FDMExec->GetWinds();
  Propulsion = FDMExec->GetPropulsion();
  MassBalance = FDMExec->GetMassBalance();
  Propagate = FDMExec->GetPropagate();
  Accelerations = FDMExec->GetAccelerations();
  FCS = FDMExec->GetFCS();
  GroundReactions = FDMExec->GetGroundReactions();
  ExternalReactions = FDMExec->GetExternalReactions();
  BuoyantForces = FDMExec->GetBuoyantForces();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutputType::~FGOutputType()
{
  for (auto param: OutputParameters)
    delete param;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputType::SetIdx(unsigned int idx)
{
  string outputProp = CreateIndexedPropertyName("simulation/output", idx);

  PropertyManager->Tie(outputProp + "/log_rate_hz", this, &FGOutputType::GetRateHz, &FGOutputType::SetRateHz);
  PropertyManager->Tie(outputProp + "/enabled", &enabled);
  OutputIdx = idx;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputType::Load(Element* element)
{
  if (element->FindElementValue("simulation") == string("ON"))
    SubSystems += ssSimulation;
  if (element->FindElementValue("aerosurfaces") == string("ON"))
    SubSystems += ssAerosurfaces;
  if (element->FindElementValue("rates") == string("ON"))
    SubSystems += ssRates;
  if (element->FindElementValue("velocities") == string("ON"))
    SubSystems += ssVelocities;
  if (element->FindElementValue("forces") == string("ON"))
    SubSystems += ssForces;
  if (element->FindElementValue("moments") == string("ON"))
    SubSystems += ssMoments;
  if (element->FindElementValue("atmosphere") == string("ON"))
    SubSystems += ssAtmosphere;
  if (element->FindElementValue("massprops") == string("ON"))
    SubSystems += ssMassProps;
  if (element->FindElementValue("position") == string("ON"))
    SubSystems += ssPropagate;
  if (element->FindElementValue("coefficients") == string("ON") || element->FindElementValue("aerodynamics") == string("ON"))
    SubSystems += ssAeroFunctions;
  if (element->FindElementValue("ground_reactions") == string("ON"))
    SubSystems += ssGroundReactions;
  if (element->FindElementValue("fcs") == string("ON"))
    SubSystems += ssFCS;
  if (element->FindElementValue("propulsion") == string("ON"))
    SubSystems += ssPropulsion;

  Element *property_element = element->FindElement("property");

  while (property_element) {
    string property_str = property_element->GetDataLine();
    FGPropertyNode* node = PropertyManager->GetNode(property_str);
    if (!node) {
      cerr << property_element->ReadFrom()
           << fgred << highint << endl << "  No property by the name "
           << property_str << " has been defined. This property will " << endl
           << "  not be logged. You should check your configuration file."
           << reset << endl;
    } else {
      if (property_element->HasAttribute("apply")) {
        string function_str = property_element->GetAttributeValue("apply");
        FGTemplateFunc* f = FDMExec->GetTemplateFunc(function_str);
        if (f)
          OutputParameters.push_back(new FGFunctionValue(node, f));
        else {
          cerr << property_element->ReadFrom()
               << fgred << highint << "  No function by the name "
               << function_str << " has been defined. This property will "
               << "not be logged. You should check your configuration file."
               << reset << endl;
        }
      }
      else
        OutputParameters.push_back(new FGPropertyValue(node));

      if (property_element->HasAttribute("caption"))
        OutputCaptions.push_back(property_element->GetAttributeValue("caption"));
      else
        OutputCaptions.push_back("");
    }
    property_element = element->FindNextElement("property");
  }

  double outRate = 1.0;
  if (element->HasAttribute("rate"))
    outRate = element->GetAttributeValueAsNumber("rate");

  SetRateHz(outRate);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputType::InitModel(void)
{
  bool ret = FGModel::InitModel();

  Debug(2);
  return ret;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputType::Run(void)
{
  if (FGModel::Run(false)) return true;
  if (!enabled) return true;

  RunPreFunctions();
  Print();
  RunPostFunctions();

  Debug(4);

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputType::SetRateHz(double rtHz)
{
  rtHz = rtHz>1000?1000:(rtHz<0?0:rtHz);
  if (rtHz > 0) {
    SetRate(0.5 + 1.0/(FDMExec->GetDeltaT()*rtHz));
    Enable();
  } else {
    SetRate(1);
    Disable();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGOutputType::GetRateHz(void) const
{
  return 1.0 / (rate * FDMExec->GetDeltaT());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputType::SetOutputProperties(vector<FGPropertyNode_ptr> & outputProperties)
{
  for (auto prop: outputProperties)
    OutputParameters.push_back(new FGPropertyValue(prop));
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

void FGOutputType::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
    if (from == 2) {
      if (SubSystems & ssSimulation)      cout << "    Simulation parameters logged" << endl;
      if (SubSystems & ssAerosurfaces)    cout << "    Aerosurface parameters logged" << endl;
      if (SubSystems & ssRates)           cout << "    Rate parameters logged" << endl;
      if (SubSystems & ssVelocities)      cout << "    Velocity parameters logged" << endl;
      if (SubSystems & ssForces)          cout << "    Force parameters logged" << endl;
      if (SubSystems & ssMoments)         cout << "    Moments parameters logged" << endl;
      if (SubSystems & ssAtmosphere)      cout << "    Atmosphere parameters logged" << endl;
      if (SubSystems & ssMassProps)       cout << "    Mass parameters logged" << endl;
      if (SubSystems & ssAeroFunctions)   cout << "    Coefficient parameters logged" << endl;
      if (SubSystems & ssPropagate)       cout << "    Propagate parameters logged" << endl;
      if (SubSystems & ssGroundReactions) cout << "    Ground parameters logged" << endl;
      if (SubSystems & ssFCS)             cout << "    FCS parameters logged" << endl;
      if (SubSystems & ssPropulsion)      cout << "    Propulsion parameters logged" << endl;
      if (!OutputParameters.empty())      cout << "    Properties logged:" << endl;
      for (auto param: OutputParameters)
        cout << "      - " << param->GetName() << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGOutputType" << endl;
    if (from == 1) cout << "Destroyed:    FGOutputType" << endl;
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
