/*******************************************************************************
 
 Module:       FGFlap.cpp
 Author:       Tony Peden, for flight control system authored by Jon S. Berndt
 Date started: 5/11/00
 
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
 
********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
 
********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFlaps.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

// *****************************************************************************
//  Function:   Constructor
//  Purpose:
//  Parameters: void
//  Comments:

FGFlaps::FGFlaps(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
AC_cfg(AC_cfg) {
  string token;
  float tmpDetent;
  float tmpTime;

  Detents.clear();
  TransitionTimes.clear();

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
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
      }
    } else if ( token == "DETENTS" ) {
      *AC_cfg >> NumDetents;
      cout << "      DETENTS: " << NumDetents << endl;
      for(int i=0;i<NumDetents;i++) {
        *AC_cfg >> tmpDetent;
        *AC_cfg >> tmpTime;
        Detents.push_back(tmpDetent);
        TransitionTimes.push_back(tmpTime);
        cout << "        " << Detents[i] << " " << TransitionTimes[i] << endl;
      }
    } else if (token == "OUTPUT") {

      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      cout << "      OUTPUT: " <<sOutputIdx << endl;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
    }
  }
}

// *****************************************************************************
//  Function:   Run
//  Purpose:
//  Parameters: void
//  Comments:

bool FGFlaps::Run(void ) {
  float dt=fcs->GetState()->Getdt();
  float flap_transit_rate=0;

  FGFCSComponent::Run(); // call the base class for initialization of Input
  Flap_Handle = Input*Detents[NumDetents-1];
  Flap_Position = fcs->GetState()->GetParameter(OutputIdx);

  if(Flap_Handle < Detents[0]) {
    fi=0;
    Flap_Handle=Detents[0];
    lastFlapHandle=Flap_Handle;
    Flap_Position=Detents[0];
    Output=Flap_Position;
  } else if(Flap_Handle > Detents[NumDetents-1]) {
    fi=NumDetents-1;
    Flap_Handle=Detents[fi];
    lastFlapHandle=Flap_Handle;
    Flap_Position=Detents[fi];
    Output=Flap_Position;
  } else {
    //cout << "FGFlaps::Run Handle: " << Flap_Handle << " Position: " << Flap_Position << endl;
    if(dt <= 0)
      Flap_Position=Flap_Handle;
    else {
      if(Flap_Handle != lastFlapHandle) {

        Flaps_In_Transit=1;
      }
      if(Flaps_In_Transit) {

        //fprintf(stderr,"Flap_Handle: %g, Flap_Position: %g\n",Flap_Handle,Flap_Position);
        fi=0;
        while(Detents[fi] < Flap_Handle) {
          fi++;
        }
        if(Flap_Position < Flap_Handle) {
          if(TransitionTimes[fi] > 0)
            flap_transit_rate=(Detents[fi] - Detents[fi-1])/TransitionTimes[fi];
          else
            flap_transit_rate=(Detents[fi] - Detents[fi-1])/5;
        } else {
          if(TransitionTimes[fi+1] > 0)
            flap_transit_rate=(Detents[fi] - Detents[fi+1])/TransitionTimes[fi+1];
          else
            flap_transit_rate=(Detents[fi] - Detents[fi+1])/5;
        }
        if(fabs(Flap_Position - Flap_Handle) > dt*flap_transit_rate)
          Flap_Position+=flap_transit_rate*dt;
        else {
          Flaps_In_Transit=0;
          Flap_Position=Flap_Handle;
        }
      }
    }
    lastFlapHandle=Flap_Handle;
    Output=Flap_Position;
  }
  //cout << "FGFlaps::Run Handle: " << Flap_Handle << " Position: " << Flap_Position << " Output: " << Output << endl;
  if (IsOutput) {
    //cout << "Calling SetOutput()" << endl;
    SetOutput();
  }
  //cout << "Out FGFlap::Run" << endl;
  return true;
}

