
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

FGGain::FGGain(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                   AC_cfg(AC_cfg)
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
      AC_cfg->ResetLineIndexToZero();
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
  FGFCSComponent::Run(); // call the base class for initialization of Input

  if (Type == "PURE_GAIN") {

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {

    float LookupVal = fcs->GetState()->GetParameter(ScheduledBy);
    unsigned int last = Schedule.size()-1;
    float lowVal = Schedule[0][0], hiVal = Schedule[last][0];
    float factor = 1.0;

    if (LookupVal <= lowVal) Output = Schedule[0][1];
    else if (LookupVal >= hiVal) Output = Schedule[last][1];
    else {
      for (unsigned int ctr = 1; ctr < last; ctr++) {
        if (LookupVal < Schedule[ctr][0]) {
          hiVal = Schedule[ctr][0];
          lowVal = Schedule[ctr-1][0];
          factor = (LookupVal - lowVal) / (hiVal - lowVal);
          Gain = Schedule[ctr-1][1] + factor*(Schedule[ctr][1] - Schedule[ctr-1][1]);
          Output = Gain * Input;
          break;
        }
      }
    }

  } else if (Type == "AEROSURFACE_SCALE") {

    if (Output >= 0.0) Output = Gain * Max;
    else Output = Gain * (-Min);
  }

  return true;
}

