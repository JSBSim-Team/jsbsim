/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGSummer.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000
 
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

#include "FGSummer.h"            

static const char *IdSrc = "$Id: FGSummer.cpp,v 1.30 2002/02/14 23:41:14 jberndt Exp $";
static const char *IdHdr = ID_SUMMER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGSummer::FGSummer(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                       AC_cfg(AC_cfg)
{
  string token;
  eParam tmpInputIndex;

  clip = false;
  Bias = 0.0;
  InputIndices.clear();
  InputTypes.clear();

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {
    *AC_cfg >> token;

    if (token == "ID") {
      *AC_cfg >> ID;
    } else if (token == "INPUT") {
      token = AC_cfg->GetValue("INPUT");
      if (token.find("FG_") != token.npos) {
        *AC_cfg >> token;
        tmpInputIndex = fcs->GetState()->GetParameterIndex(token);
        InputIndices.push_back(tmpInputIndex);
        InputTypes.push_back(itPilotAC);
      } else if (token.find(".") != token.npos) { // bias
        *AC_cfg >> Bias;
        InputIndices.push_back((eParam)0);
        InputTypes.push_back(itBias);
      } else {
        *AC_cfg >> tmpInputIndex;
        InputIndices.push_back(tmpInputIndex);
        InputTypes.push_back(itFCS);
      }
    } else if (token == "CLIPTO") {
      *AC_cfg >> clipmin >> clipmax;
      if (clipmax > clipmin) {
        clip = true;
      }
    } else if (token == "OUTPUT") {
      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
    }
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGSummer::~FGSummer()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGSummer::Run(void )
{
  unsigned int idx;

  // The Summer takes several inputs, so do not call the base class Run()
  // FGFCSComponent::Run();

  Output = 0.0;

  for (idx=0; idx<InputIndices.size(); idx++) {
    switch (InputTypes[idx]) {
    case itPilotAC:
      Output += fcs->GetState()->GetParameter(InputIndices[idx]);
      break;
    case itFCS:
      Output += fcs->GetComponentOutput(InputIndices[idx]);
      break;
    case itBias:
      Output += Bias;
      break;
    }
  }

  if (clip) {
    if (Output > clipmax)      Output = clipmax;
    else if (Output < clipmin) Output = clipmin;
  }

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

void FGSummer::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      ID: " << ID << endl;
      cout << "      INPUTS: " << endl;
      for (unsigned i=0;i<InputIndices.size();i++) {
        switch (InputTypes[i]) {
        case itPilotAC:
          cout << "       " << fcs->GetState()->GetParameterName(InputIndices[i]) << endl;
          break;
        case itFCS:
          cout << "        FCS Component " << InputIndices[i] << " (" << 
                              fcs->GetComponentName(InputIndices[i]) << ")" << endl;
          break;
        case itBias:
          cout << "        " << "Bias of " << Bias << endl;
          break;
        }
      }
      if (clipmax > clipmin) cout << "      CLIPTO: " << clipmin 
                                  << ", " << clipmax << endl;
      if (IsOutput) cout << "      OUTPUT: " <<sOutputIdx <<  endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGSummer" << endl;
    if (from == 1) cout << "Destroyed:    FGSummer" << endl;
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

