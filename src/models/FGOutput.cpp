/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file, stdout or socket
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created
11/09/07   HDW   Added FlightGear Socket Interface

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGOutput.h"
#include "FGFDMExec.h"
#include "input_output/FGOutputSocket.h"
#include "input_output/FGOutputTextFile.h"
#include "input_output/FGOutputFG.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.70 2012/09/05 21:49:19 bcoconni Exp $";
static const char *IdHdr = ID_OUTPUT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  typedef int (FGOutput::*iOPV)(void) const;

  Name = "FGOutput";

  fdmex->GetPropertyManager()->Tie("simulation/force-output", this, (iOPV)0, &FGOutput::ForceOutput, false);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutput::~FGOutput()
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    delete (*it);

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::InitModel(void)
{
  bool ret = false;

  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    ret &= (*it)->InitModel();

  return ret;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;

  if (!Holding) {
    vector<FGOutputType*>::iterator it;
    for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
      (*it)->Run();
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::Print(void)
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    (*it)->Print();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SetStartNewOutput(void)
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    (*it)->SetStartNewOutput();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::Enable(void)
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    (*it)->Enable();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::Disable(void)
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    (*it)->Disable();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Toggle(int idx)
{
  if (idx >= (int)0 && idx < (int)OutputTypes.size())
    return OutputTypes[idx]->Toggle();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SetRate(double rate)
{
  vector<FGOutputType*>::iterator it;
  for (it = OutputTypes.begin(); it != OutputTypes.end(); ++it)
    (*it)->SetRate(rate);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::ForceOutput(int idx)
{
  if (idx >= (int)0 && idx < (int)OutputTypes.size())
    OutputTypes[idx]->Print();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::SetOutputName(unsigned int idx, const std::string& name)
{
  if (idx >= OutputTypes.size()) return false;

  OutputTypes[idx]->SetOutputName(name);
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGOutput::GetOutputName(unsigned int idx) const
{
  string name;

  if (idx < OutputTypes.size())
    name = OutputTypes[idx]->GetOutputName();
  return name;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::SetDirectivesFile(const std::string& fname)
{
  Element* document = LoadXMLDocument(fname);
  bool result = Load(document);

  ResetParser();

  if (!result)
    cerr << endl << "Aircraft output element has problems in file " << fname << endl;

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Load(int subSystems, std::string protocol, std::string type,
                    std::string port, std::string name, double outRate,
                    std::vector<FGPropertyManager *> & outputProperties)
{
  unsigned int idx = OutputTypes.size();
  FGOutputType* Output = 0;

  if (debug_lvl > 0) cout << endl << "  Output data set: " << idx << "  ";

  type = to_upper(type);

  if (type == "CSV") {
    Output = new FGOutputTextFile(FDMExec, ",", idx, subSystems, name, outRate,
                                  outputProperties);
  } else if (type == "TABULAR") {
    Output = new FGOutputTextFile(FDMExec, "\t", idx, subSystems, name, outRate,
                                  outputProperties);
  } else if (type == "SOCKET") {
    Output = new FGOutputSocket(FDMExec, idx, subSystems, protocol, port, name,
                                outRate, outputProperties);
  } else if (type == "FLIGHTGEAR") {
    Output = new FGOutputFG(FDMExec, idx, subSystems, protocol, port, name,
                            outRate, outputProperties);
  } else if (type == "TERMINAL") {
    // Not done yet
  } else if (type != string("NONE")) {
    cerr << "Unknown type of output specified in config file" << endl;
  }

  if (!Output) return false;

  OutputTypes.push_back(Output);

  Debug(2);
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Load(Element* document)
{
  if (!document) return false;

  unsigned int idx = OutputTypes.size();
  string type = document->GetAttributeValue("type");
  FGOutputType* Output = 0;

  if (debug_lvl > 0) cout << endl << "  Output data set: " << idx << "  ";

  type = to_upper(type);

  if (type == "CSV") {
    Output = new FGOutputTextFile(FDMExec, document, ",", idx);
  } else if (type == "TABULAR") {
    Output = new FGOutputTextFile(FDMExec, document, "\t", idx);
  } else if (type == "SOCKET") {
    Output = new FGOutputSocket(FDMExec, document, idx);
  } else if (type == "FLIGHTGEAR") {
    Output = new FGOutputFG(FDMExec, document, idx);
  } else if (type == "TERMINAL") {
    // Not done yet
  } else if (type != string("NONE")) {
    cerr << "Unknown type of output specified in config file" << endl;
  }

  if (!Output) return false;

  OutputTypes.push_back(Output);

  Debug(2);
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

void FGOutput::Debug(int from)
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
    if (from == 0) cout << "Instantiated: FGOutput" << endl;
    if (from == 1) cout << "Destroyed:    FGOutput" << endl;
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
