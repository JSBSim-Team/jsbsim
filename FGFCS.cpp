/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGDefs.h"

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

#include "filtersjb/FGFilter.h"
#include "filtersjb/FGDeadBand.h"
#include "filtersjb/FGGain.h"
#include "filtersjb/FGGradient.h"
#include "filtersjb/FGSwitch.h"
#include "filtersjb/FGSummer.h"
#include "filtersjb/FGFlaps.h"

static const char *IdSrc = "$Id: FGFCS.cpp,v 1.49 2001/06/26 00:21:31 jberndt Exp $";
static const char *IdHdr = ID_FCS;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCS::FGFCS(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGFCS";

  DaCmd = DeCmd = DrCmd = DfCmd = DsbCmd = DspCmd = PTrimCmd = 0.0;
  DaPos = DePos = DrPos = DfPos = DsbPos = DspPos = 0.0;
  LeftBrake = RightBrake = CenterBrake = 0.0;

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFCS::~FGFCS()
{
  ThrottleCmd.clear();
  ThrottlePos.clear();

  unsigned int i;

  for(i=0;i<Components.size();i++) delete Components[i];
  if (debug_lvl & 2) cout << "Destroyed:    FGFCS" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Run(void)
{
  unsigned int i;

  if (!FGModel::Run()) {
    for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = ThrottleCmd[i];
    for (i=0; i<Components.size(); i++)  Components[i]->Run();
  } else {
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetThrottleCmd(int engineNum, float setting)
{
  unsigned int ctr;

  if (engineNum < 0) {
    for (ctr=0;ctr<ThrottleCmd.size();ctr++) ThrottleCmd[ctr] = setting;
  } else {
    ThrottleCmd[engineNum] = setting;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetThrottlePos(int engineNum, float setting)
{
  unsigned int ctr;

  if (engineNum < 0) {
    for (ctr=0;ctr<=ThrottleCmd.size();ctr++) ThrottlePos[ctr] = ThrottleCmd[ctr];
  } else {
    ThrottlePos[engineNum] = setting;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Load(FGConfigFile* AC_cfg)
{
  string token;

  Name = AC_cfg->GetValue("NAME");
  if (debug_lvl > 0) cout << "    Control System Name: " << Name << endl;
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != "/FLIGHT_CONTROL") {
    if (token == "COMPONENT") {
      token = AC_cfg->GetValue("TYPE");
      if (debug_lvl > 0) cout << "    Loading Component \""
                              << AC_cfg->GetValue("NAME")
			                        << "\" of type: " << token << endl;
      if ((token == "LAG_FILTER") ||
          (token == "LEAD_LAG_FILTER") ||
          (token == "SECOND_ORDER_FILTER") ||
          (token == "WASHOUT_FILTER") ||
          (token == "INTEGRATOR") ) {
        Components.push_back(new FGFilter(this, AC_cfg));
      } else if ((token == "PURE_GAIN") ||
                 (token == "SCHEDULED_GAIN") ||
                 (token == "AEROSURFACE_SCALE") ) {

        Components.push_back(new FGGain(this, AC_cfg));

      } else if (token == "SUMMER") {
        Components.push_back(new FGSummer(this, AC_cfg));
      } else if (token == "DEADBAND") {
        Components.push_back(new FGDeadBand(this, AC_cfg));
      } else if (token == "GRADIENT") {
        Components.push_back(new FGGradient(this, AC_cfg));
      } else if (token == "SWITCH") {
        Components.push_back(new FGSwitch(this, AC_cfg));
      } else if (token == "FLAPS") {
        Components.push_back(new FGFlaps(this, AC_cfg));
      } else {
        cerr << "Unknown token [" << token << "] in FCS portion of config file" << endl;
        return false;
      }
      AC_cfg->GetNextConfigLine();
    }
  }
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGFCS::GetComponentOutput(eParam idx) {
  return Components[idx]->GetOutput();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentName(int idx) {
  return Components[idx]->GetName();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGFCS::GetBrake(FGLGear::BrakeGroup bg) {
  switch (bg) {
  case FGLGear::bgLeft:
    return LeftBrake;
  case FGLGear::bgRight:
    return RightBrake;
  case FGLGear::bgCenter:
    return CenterBrake;
  default:
    cerr << "GetBrake asked to return a bogus brake value" << endl;
  }
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentStrings(void)
{
  unsigned int comp;

  string CompStrings = "";
  bool firstime = true;

  for (comp = 0; comp < Components.size(); comp++) {
    if (firstime) firstime = false;
    else          CompStrings += ", ";

    CompStrings += Components[comp]->GetName();
  }

  return CompStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentValues(void)
{
  unsigned int comp;

  string CompValues = "";
  char buffer[10];
  bool firstime = true;

  for (comp = 0; comp < Components.size(); comp++) {
    if (firstime) firstime = false;
    else          CompValues += ", ";

    sprintf(buffer, "%9.6f", Components[comp]->GetOutput());
    CompValues += string(buffer);
  }

  return CompValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::AddThrottle(void)
{
  ThrottleCmd.push_back(0.0);
  ThrottlePos.push_back(0.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::Debug(void)
{
    //TODO: Add your source code here
}

