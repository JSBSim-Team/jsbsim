/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGain.cpp
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

#include "FGGain.h"            

static const char *IdSrc = "$Id: FGGain.cpp,v 1.30 2001/11/14 23:53:27 jberndt Exp $";
static const char *IdHdr = ID_GAIN;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGain::FGGain(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                   AC_cfg(AC_cfg)
{
  string token;
  string strScheduledBy;

  State = fcs->GetState();

  Gain = 1.000;
  Rows = 0;
  Min = Max = 0.0;
  ScheduledBy = FG_UNDEF;

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
        InputIdx = State->GetParameterIndex(token);
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
    } else if (token == "ROWS") {
      *AC_cfg >> Rows;
      Table = new FGTable(Rows);
    } else if (token == "SCHEDULED_BY") {
      token = AC_cfg->GetValue("SCHEDULED_BY");
      if (token.find("FG_") != token.npos) {
        *AC_cfg >> strScheduledBy;
        ScheduledBy = State->GetParameterIndex(strScheduledBy);
      } else {
        *AC_cfg >> ScheduledBy;
      }
    } else if (token == "OUTPUT") {
      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputIdx = State->GetParameterIndex(sOutputIdx);
    } else {
      AC_cfg->ResetLineIndexToZero();
      *Table << *AC_cfg;
    }
  }

  if (debug_lvl > 0) {
    cout << "      ID: " << ID << endl;
    cout << "      INPUT: " << InputIdx << endl;
    cout << "      GAIN: " << Gain << endl;
    if (IsOutput) cout << "      OUTPUT: " << sOutputIdx << endl;
    cout << "      MIN: " << Min << endl;
    cout << "      MAX: " << Max << endl;
    if (ScheduledBy != FG_UNDEF) {
      cout << "      Scheduled by parameter: " << ScheduledBy << endl;
      Table->Print();
    }
  }

  if (debug_lvl & 2) cout << "Instantiated: FGGain" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGain::~FGGain()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGGain" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGain::Run(void )
{
  double SchedGain = 1.0;
  double LookupVal = 0;

  FGFCSComponent::Run(); // call the base class for initialization of Input

  if (Type == "PURE_GAIN") {
    Output = Gain * Input;
  } else if (Type == "SCHEDULED_GAIN") {
    LookupVal = State->GetParameter(ScheduledBy);
	  SchedGain = Table->GetValue(LookupVal);
    Output = Gain * SchedGain * Input;
  } else if (Type == "AEROSURFACE_SCALE") {
    if (Output >= 0.0) Output = Input * Max;
    else Output = Input * (-Min);
    Output *= Gain;
  }

  if (IsOutput) SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGain::Debug(void)
{
    //TODO: Add your source code here
}

