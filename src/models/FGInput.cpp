/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGInput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage input of sim parameters from socket
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
12/02/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGInput.h"
#include "FGFDMExec.h"
#include "input_output/FGUDPInputSocket.h"
#include "input_output/FGXMLFileRead.h"
#include "input_output/FGModelLoader.h"
#include "input_output/FGLog.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGInput::FGInput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGInput";
  enabled = true;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInput::~FGInput()
{
  vector<FGInputType*>::iterator it;
  for (it = InputTypes.begin(); it != InputTypes.end(); ++it)
    delete (*it);

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::Load(Element* el)
{
  // Unlike the other FGModel classes, properties listed in the <input> section
  // are not intended to create new properties. For that reason, FGInput
  // cannot load its XML directives with FGModel::Load().
  // Instead FGModelLoader::Open() and FGModel::PreLoad() must be explicitely
  // called.
  FGModelLoader ModelLoader(this);
  Element* element = ModelLoader.Open(el);

  if (!element) return false;

  FGModel::PreLoad(element, FDMExec);

  size_t idx = InputTypes.size();
  string type = element->GetAttributeValue("type");
  FGInputType* Input = 0;

  if (debug_lvl > 0) {
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    log << endl << "  Input data set: " << idx << "  " << endl;
  }

  type = to_upper(type);

  if (type.empty() || type == "SOCKET") {
    Input = new FGInputSocket(FDMExec);
  } else if (type == "QTJSBSIM") {
    Input = new FGUDPInputSocket(FDMExec);
  } else if (type != string("NONE")) {
    FGXMLLogging log(FDMExec->GetLogger(), element, LogLevel::ERROR);
    log << "Unknown type of input specified in config file" << endl;
  }

  if (!Input) return false;

  Input->SetIdx(idx);
  Input->Load(element);
  PostLoad(element, FDMExec);

  InputTypes.push_back(Input);

  Debug(2);
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::InitModel(void)
{
  bool ret = false;

  if (!FGModel::InitModel()) return false;

  vector<FGInputType*>::iterator it;
  for (it = InputTypes.begin(); it != InputTypes.end(); ++it)
    ret &= (*it)->InitModel();

  return ret;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::Run(bool Holding)
{
  if (FDMExec->GetTrimStatus()) return true;
  if (FGModel::Run(Holding)) return true;
  if (!enabled) return true;

  vector<FGInputType*>::iterator it;
  for (it = InputTypes.begin(); it != InputTypes.end(); ++it)
    (*it)->Run(Holding);

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::SetDirectivesFile(const SGPath& fname)
{
  FGXMLFileRead XMLFile;
  Element* document = XMLFile.LoadXMLDocument(fname);
  if (!document) {
    LogException err(FDMExec->GetLogger());
    err << "Could not read directive file: " << fname << endl;
    throw err;
  }
  bool result = Load(document);

  if (!result) {
    FGLogging log(FDMExec->GetLogger(), LogLevel::ERROR);
    log << endl << "Aircraft input element has problems in file " << fname << endl;
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::Toggle(int idx)
{
  if (idx >= (int)0 && idx < (int)InputTypes.size())
    return InputTypes[idx]->Toggle();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInput::SetInputName(unsigned int idx, const std::string& name)
{
  if (idx >= InputTypes.size()) return false;

  InputTypes[idx]->SetInputName(name);
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGInput::GetInputName(unsigned int idx) const
{
  string name;

  if (idx < InputTypes.size())
    name = InputTypes[idx]->GetInputName();
  return name;
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

void FGInput::Debug(int from)
{
  string scratch="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
    if (from == 2) {
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGInput" << endl;
    if (from == 1) log << "Destroyed:    FGInput" << endl;
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
