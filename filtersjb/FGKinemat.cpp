/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGKinemat.cpp
 Author:       Tony Peden, for flight control system authored by Jon S. Berndt
 Date started: 12/02/01
 
 ------------- Copyright (C) 2000 Anthony K. Peden -------------
 
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

#include "FGKinemat.h"

static const char *IdSrc = "$Id: FGKinemat.cpp,v 1.1 2001/12/02 16:02:09 apeden Exp $";
static const char *IdHdr = ID_FLAPS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGKinemat::FGKinemat(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
AC_cfg(AC_cfg) {
  string token;
  double tmpDetent;
  double tmpTime;

  Detents.clear();
  TransitionTimes.clear();

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
        InputIdx = fcs->GetState()->GetParameterIndex(token);
        InputType = itPilotAC;
      }
    } else if ( token == "DETENTS" ) {
      *AC_cfg >> NumDetents;
      for(int i=0;i<NumDetents;i++) {
        *AC_cfg >> tmpDetent;
        *AC_cfg >> tmpTime;
        Detents.push_back(tmpDetent);
        TransitionTimes.push_back(tmpTime);
      }
    } else if (token == "OUTPUT") {

      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
    }
  }

  if (debug_lvl > 1) {
    cout << "      ID: " << ID << endl;
    cout << "      INPUT: " << InputIdx << endl;
    cout << "      DETENTS: " << NumDetents << endl;
    for(int i=0;i<NumDetents;i++) {
      cout << "        " << Detents[i] << " " << TransitionTimes[i] << endl;
    }
    if (IsOutput) cout << "      OUTPUT: " <<sOutputIdx << endl;
  }

  if (debug_lvl & 2) cout << "Instantiated: FGKinemat" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGKinemat::~FGKinemat()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGKinemat" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGKinemat::Run(void ) {
  double dt=fcs->GetState()->Getdt();
  double output_transit_rate=0;

  FGFCSComponent::Run(); // call the base class for initialization of Input
  InputCmd = Input*Detents[NumDetents-1];
  OutputPos = fcs->GetState()->GetParameter(OutputIdx);

  if(InputCmd < Detents[0]) {
    fi=0;
    InputCmd=Detents[0];
    lastInputCmd=InputCmd;
    OutputPos=Detents[0];
    Output=OutputPos;
  } else if(InputCmd > Detents[NumDetents-1]) {
    fi=NumDetents-1;
    InputCmd=Detents[fi];
    lastInputCmd=InputCmd;
    OutputPos=Detents[fi];
    Output=OutputPos;
  } else {
    //cout << "FGKinemat::Run Handle: " << InputCmd << " Position: " << OutputPos << endl;
    if(dt <= 0)
      OutputPos=InputCmd;
    else {
      if(InputCmd != lastInputCmd) {

        InTransit=1;
      }
      if(InTransit) {

        //fprintf(stderr,"InputCmd: %g, OutputPos: %g\n",InputCmd,OutputPos);
        fi=0;
        while(Detents[fi] < InputCmd) {
          fi++;
        }
        if(OutputPos < InputCmd) {
          if(TransitionTimes[fi] > 0)
            output_transit_rate=(Detents[fi] - Detents[fi-1])/TransitionTimes[fi];
          else
            output_transit_rate=(Detents[fi] - Detents[fi-1])/5;
        } else {
          if(TransitionTimes[fi+1] > 0)
            output_transit_rate=(Detents[fi] - Detents[fi+1])/TransitionTimes[fi+1];
          else
            output_transit_rate=(Detents[fi] - Detents[fi+1])/5;
        }
        if(fabs(OutputPos - InputCmd) > dt*output_transit_rate)
          OutputPos+=output_transit_rate*dt;
        else {
          InTransit=0;
          OutputPos=InputCmd;
        }
      }
    }
    lastInputCmd=InputCmd;
    Output=OutputPos;
  }
  //cout << "FGKinemat::Run Handle: " << InputCmd << " Position: " << OutputPos << " Output: " << Output << endl;
  if (IsOutput) {
    //cout << "Calling SetOutput()" << endl;
    SetOutput();
  }
  //cout << "Out FGKinemat::Run" << endl;
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGKinemat::Debug(void)
{
    //TODO: Add your source code here
}

