/*******************************************************************************

 Module:       FGFilter.cpp
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

#include "FGFilter.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

// *****************************************************************************
//  Function:   constructor
//  Purpose:
//  Parameters: void
//  Comments:

FGFilter::FGFilter(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                       AC_cfg(AC_cfg)
{
  string token;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  C1 = C2 = C3 = C4 = C5 = C6 = 0.0;

  if      (Type == "LAG_FILTER")          FilterType = eLag        ;
  else if (Type == "RECT_LAG_FILTER")     FilterType = eRectLag    ;
  else if (Type == "LEAD_LAG_FILTER")     FilterType = eLeadLag    ;
  else if (Type == "SECOND_ORDER_FILTER") FilterType = eOrder2     ;
  else if (Type == "WASHOUT_FILTER")      FilterType = eWashout    ;
  else if (Type == "INTEGRATOR")          FilterType = eIntegrator ;
  else                                    FilterType = eUnknown    ;

  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
    *AC_cfg >> token;
    if (token == "ID") {
      *AC_cfg >> ID;
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
    } else if (token == "C1") {
      *AC_cfg >> C1;
    } else if (token == "C2") {
      *AC_cfg >> C2;
    } else if (token == "C3") {
      *AC_cfg >> C3;
    } else if (token == "C4") {
      *AC_cfg >> C4;
    } else if (token == "C5") {
      *AC_cfg >> C5;
    } else if (token == "C6") {
      *AC_cfg >> C6;
    }
  }

  switch (FilterType) {
    case eLag:
      ca = dt*C1 / (2.00 + dt*C1);
      cb = (2.00 - dt*C1) / (2.00 + dt*C1);
      break;
    case eRectLag:
      break;
    case eLeadLag:
      break;
    case eOrder2:
      break;
    case eWashout:
      break;
    case eIntegrator:
      break;
  }

}

// *****************************************************************************
//  Function:   Run
//  Purpose:
//  Parameters: void
//  Comments:

bool FGFilter::Run(void)
{
  FGFCSComponent::Run(); // call the base class for initialization of Input

  switch (FilterType) {
    case eLag:
      ca = dt*C1 / (2.00 + dt*C1);
      cb = (2.00 - dt*C1) / (2.00 + dt*C1);
      break;
    case eRectLag:
      break;
    case eLeadLag:
      break;
    case eOrder2:
      break;
    case eWashout:
      break;
    case eIntegrator:
      break;
  }

  PreviousOutput2 = PreviousOutput1;
  PreviousOutput1 = Output;
  PreviousInput2  = PreviousInput1;
  PreviousInput1  = Input;

  return true;
}

