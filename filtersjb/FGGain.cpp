
/*******************************************************************************

 Module:       FGGain.cpp
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

#include "FGGain.h"    				

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

// *****************************************************************************
//  Function:   Constructor
//  Purpose:    Builds a Gain-type of FCS component.
//  Parameters: void
//  Comments:   Types are PURE_GAIN, SCHEDULED_GAIN, and AEROSURFACE_SCALE

FGGain::FGGain(FGFCS* fcs, FGConfigFile* AC_cfg) : fcs(fcs), AC_cfg(AC_cfg)
{
  Type = AC_cfg->GetValue("TYPE");
  AC_cfg->GetNextConfigLine();
  string token;

  Gain = 0;
  Min = Max = 0;
  ScheduledBy = 0;

  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
    *AC_cfg >> token;
    if (token == "ID") {
      *AC_cfg >> ID;
    } else if (token == "QUEUE_ORDER") {
      *AC_cfg >> QueueOrder;
    } else if (token == "INPUT") {
      token = AC_cfg->GetValue("INPUT");
      if (token.find("FG_") != token.npos) {
        *AC_cfg >> token;
        InputIdx = fcs->GetState()->GetParameterIndex(token);
        InputType = itPilotAC;
      } else {
        *AC_cfg >> InputIdx;
        InputType = itFCS;
      }
    } else if (token == "GAIN") {
      *AC_cfg >> Gain;
    } else if (token == "MIN") {
      *AC_cfg >> Min;
    } else if (token == "MAX") {
      *AC_cfg >> Max;
    } else if (token == "SCHEDULED_BY") {
      *AC_cfg >> ScheduledBy;
    } else {
      lookup = new float[2];
      *AC_cfg >> lookup[0] >> lookup[1];
      Schedule.push_back(lookup);
    }
  }
}

// *****************************************************************************
//  Function:   Run
//  Purpose:
//  Parameters: void
//  Comments:

bool FGGain::Run(void ) 
{
  if (Type == "PURE_GAIN") {
    switch(InputType) {
    case itPilotAC:
      Output = Gain * fcs->GetState()->GetParameter(InputIdx);
      break;
    case itFCS:
      Output = Gain * fcs->GetComponentOutput(InputIdx);
      break;
    case itAP:
      break;
    }
  } else if (Type == "SCHEDULED_GAIN") {
    // implement me
  } else if (Type == "AEROSURFACE_SCALE") {
    switch(InputType) {
    case itPilotAC:
      Output = fcs->GetState()->GetParameter(InputIdx);
      break;
    case itFCS:
      Output = fcs->GetComponentOutput(InputIdx);
      break;
    case itAP:
      break;
    }

    if (Output >= 0.0) Output *= Max;
    else Output *= Min;
  }

  return true;
}

