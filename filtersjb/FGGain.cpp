
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
#include "FGFCS.h"

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
  string type = AC_cfg->GetValue("TYPE");
  string token = "";
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
    if (token == "ID") {
    } else if (token == "QUEUE_ORDER") {
    } else if (token == "INPUT") {
    } else if (token == "GAIN") {
    } else if (token == "MIN") {
    } else if (token == "MAX") {
    } else if (token == "SCHEDULED_BY") {
    } else {
      lookup = new float[2];
      *AC_cfg >> lookup[0] >> lookup[1];
      Schedule.push_back(lookup);
    }
    AC_cfg->GetNextConfigLine();
  }

  if (type == "PURE_GAIN") {

  } else if (type == "SCHEDULED_GAIN") {

  } else if (type == "AEROSURFACE_SCALE") {

  }

}

// *****************************************************************************
//  Function:   Run
//  Purpose:
//  Parameters: void
//  Comments:

bool FGGain::Run(void ) 
{
  return true;
}

