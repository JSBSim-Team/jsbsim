/*******************************************************************************

 Module:       FGFCSComponent.cpp
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

#include "FGFCSComponent.h"    				

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

// *****************************************************************************
//  Function:   Constructor
//  Purpose:
//  Parameters: void
//  Comments:

FGFCSComponent::FGFCSComponent(FGFCS* _fcs) : fcs(_fcs)
{
  Type       = "";
  ID         = 0;
  Input      = 0.0;
  InputIdx   = 0;
  Output     = 0.0;
  sOutputIdx  = "";
  OutputIdx   = 0;
  IsOutput   = false;
}


void FGFCSComponent::SetOutput(void)
{
  fcs->GetState()->SetParameter(OutputIdx, Output);
}


bool FGFCSComponent::Run(void)
{
  switch(InputType) {
  case itPilotAC:
    Input = fcs->GetState()->GetParameter(InputIdx);
    break;
  case itFCS:
    Input = fcs->GetComponentOutput(InputIdx);
    break;
  case itAP:
    // implement autopilot input mechanism
    break;
  }

  return true;
}
