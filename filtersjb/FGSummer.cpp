/*******************************************************************************

 Module:       FGSummer.cpp
 Author:       
 Date started: 
 
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

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGSummer.h"            

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

// *****************************************************************************
//  Function:   Constructor
//  Purpose:
//  Parameters: void
//  Comments:

FGSummer::FGSummer(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                       AC_cfg(AC_cfg)
{
  string token;
  int tmpInputIndex;

  clip = false;
  InputIndices.clear();
  InputTypes.clear();

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
    *AC_cfg >> token;

    if (token == "ID") {

      *AC_cfg >> ID;
      cout << "      ID: " << ID << endl;

    } else if (token == "INPUT") {

      token = AC_cfg->GetValue("INPUT");
      cout << "      INPUT: " << token << endl;
      if (token.find("FG_") != token.npos) {
        *AC_cfg >> token;
        tmpInputIndex = fcs->GetState()->GetParameterIndex(token);
        InputIndices.push_back(tmpInputIndex);
        InputTypes.push_back(itPilotAC);
      } else {
        *AC_cfg >> tmpInputIndex;
        InputIndices.push_back(tmpInputIndex);
        InputTypes.push_back(itFCS);
      }

    } else if (token == "CLIPTO") {

      *AC_cfg >> clipmin >> clipmax;
      if (clipmax > clipmin) {
        clip = true;
        cout << "      CLIPTO: " << clipmin << ", " << clipmax << endl;
      }

    } else if (token == "OUTPUT") {

      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      cout << "      OUTPUT: " <<sOutputIdx <<  endl;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
    }
  }
}

// *****************************************************************************
//  Function:   Run
//  Purpose:
//  Parameters: void
//  Comments:

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
    }
  }

  if (clip) {
    if (Output > clipmax)      Output = clipmax;
    else if (Output < clipmin) Output = clipmin;
  }

  if (IsOutput) SetOutput();

  return true;
}

