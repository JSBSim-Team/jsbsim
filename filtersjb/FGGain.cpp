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

static const char *IdSrc = "$Id: FGGain.cpp,v 1.27 2001/03/29 22:26:06 jberndt Exp $";
static const char *IdHdr = ID_GAIN;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGain::FGGain(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                   AC_cfg(AC_cfg)
{
  string token;
  string strScheduledBy;

  lookup = NULL;
  Schedule.clear();
  Gain = 1.000;
  Min = Max = 0;
  ScheduledBy = FG_UNDEF;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {
    *AC_cfg >> token;
    if (token == "ID") {
      *AC_cfg >> ID;
      cout << "      ID: " << ID << endl;
    } else if (token == "INPUT") {
      token = AC_cfg->GetValue("INPUT");
      cout << "      INPUT: " << token << endl;
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
      cout << "      GAIN: " << Gain << endl;
    } else if (token == "MIN") {
      *AC_cfg >> Min;
      cout << "      MIN: " << Min << endl;
    } else if (token == "MAX") {
      *AC_cfg >> Max;
      cout << "      MAX: " << Max << endl;
    } else if (token == "SCHEDULED_BY") {
      token = AC_cfg->GetValue("SCHEDULED_BY");
      if (token.find("FG_") != token.npos) {
        *AC_cfg >> strScheduledBy;
        ScheduledBy = fcs->GetState()->GetParameterIndex(strScheduledBy);
        cout << "      Scheduled by parameter: " << token << endl;
      } else {
        *AC_cfg >> ScheduledBy;
        cout << "      Scheduled by FCS output: " << ScheduledBy << endl;
      }
    } else if (token == "OUTPUT") {
      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
      cout << "      OUTPUT: " << sOutputIdx << endl;
    } else {
      AC_cfg->ResetLineIndexToZero();
      lookup = new float[2];
      *AC_cfg >> lookup[0] >> lookup[1];
      cout << "        " << lookup[0] << "  " << lookup[1] << endl;
      Schedule.push_back(lookup);
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
  float SchedGain = 1.0;

  FGFCSComponent::Run(); // call the base class for initialization of Input

  if (Type == "PURE_GAIN") {

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {

    float LookupVal = fcs->GetState()->GetParameter(ScheduledBy);
    unsigned int last = Schedule.size()-1;
    float lowVal = Schedule[0][0], hiVal = Schedule[last][0];
    float factor = 1.0;

    if (LookupVal <= lowVal) Output = Gain * Schedule[0][1] * Input;
    else if (LookupVal >= hiVal) Output = Gain * Schedule[last][1] * Input;
    else {
      for (unsigned int ctr = 1; ctr < last; ctr++) {
        if (LookupVal < Schedule[ctr][0]) {
          hiVal = Schedule[ctr][0];
          lowVal = Schedule[ctr-1][0];
          factor = (LookupVal - lowVal) / (hiVal - lowVal);
          SchedGain = Schedule[ctr-1][1] + factor*(Schedule[ctr][1] - Schedule[ctr-1][1]);
          Output = Gain * SchedGain * Input;
          break;
        }
      }
    }

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

