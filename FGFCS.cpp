/*******************************************************************************

 Module:       FGFCS.cpp
 Author:       Jon Berndt
 Date started: 12/12/98
 Purpose:      Model the flight controls
 Called by:    FDMExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
This class models the flight controls for a specific airplane

HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFCS.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

// test
#include "filtersjb/FGfcsComponent.h"
// end test

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGFCS::FGFCS(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGFCS";
}


FGFCS::~FGFCS(void)
{
}


bool FGFCS::Run(void)
{
  if (!FGModel::Run()) {

//    for (int i=0;i<Components.size();i++) Components[i].Run();

  } else {
  }
  return false;
}


void FGFCS::SetThrottle(int engineNum, float setting)
{
  if (engineNum < 0) {
    for (int ctr=0;ctr<Aircraft->GetNumEngines();ctr++) Throttle[ctr] = setting;
  } else {
    Throttle[engineNum] = setting;
  }
}


FGFCS::LoadFCS(FGConfigFile* AC_cfg)
{
  string token;

  FCSName = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != "/FLIGHT_CONTROL") {
    if (token == "COMPONENT") {

      // FCS COMPONENT CREATION LOGIC HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
      //
      // ** Example ** reference implementation:
      //
      // Supports the following filters:
      //
      // LAG_FILTER
      // RECT_LAG_FILTER
      // LEAD_LAG_FILTER
      // SECOND_ORDER_FILTER
      // WASHOUT_FILTER
      // INTEGRATOR
      // PURE_GAIN
      // SCHEDULED_GAIN
      // AEROSURFACE_SCALE
      // SUMMER
      // DEADBAND
      // GRADIENT
      // SWITCH
      //
      // if ((token = GetValue("TYPE") == "LAG_FILTER") ||
      //     (token = GetValue("TYPE") == "RECT_LAG_FILTER") ||
      //     (token = GetValue("TYPE") == "LEAD_LAG_FILTER") ||
      //     (token = GetValue("TYPE") == "SECOND_ORDER_FILTER") ||
      //     (token = GetValue("TYPE") == "WASHOUT_FILTER") ||
      //     (token = GetValue("TYPE") == "INTEGRATOR") )
      // {
      //   Components.push_back(new FGFilter(this, AC_cfg));
      // } else if ((token == "PURE_GAIN") ||
      //            (token == "SCHEDULED_GAIN") ||
      //            (token == "AEROSURFACE_SCALE") )
      // {
      //   Components.push_back(new FGGain(this, AC_cfg));
      // } else if (token == "SUMMER") {
      //   Components.push_back(new FGSummer(this, AC_cfg));
      // } else if (token == "DEADBAND") {
      //   Components.push_back(new FGDeadBand(this, AC_cfg));
      // } else if (token == "GRADIENT") {
      //   Components.push_back(new FGGradient(this, AC_cfg));
      // } else if (token == "SWITCH") {
      //   Components.push_back(new FGSwitch(this, AC_cfg));
      // }
      //
      // remove lines below after component creation logic added ********* START

      while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
        AC_cfg->GetNextConfigLine();
      }

      // remove above                                            *********** END
      // FCS COMPONENT CREATION LOGIC END  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

      AC_cfg->GetNextConfigLine();
    }
  }
}

