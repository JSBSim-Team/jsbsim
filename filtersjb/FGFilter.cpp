/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFilter.cpp
 Author:       Jon S. Berndt
 Date started: 11/2000
 
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

#include "FGFilter.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/filtersjb/Attic/FGFilter.cpp,v 1.15 2001/03/19 23:53:47 jberndt Exp $";
static const char *IdHdr = ID_FILTER;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFilter::FGFilter(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                       AC_cfg(AC_cfg)
{
  string token;
  float denom;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();
  dt = fcs->GetState()->Getdt();

  C1 = C2 = C3 = C4 = C5 = C6 = 0.0;

  if      (Type == "LAG_FILTER")          FilterType = eLag        ;
  else if (Type == "LEAD_LAG_FILTER")     FilterType = eLeadLag    ;
  else if (Type == "SECOND_ORDER_FILTER") FilterType = eOrder2     ;
  else if (Type == "WASHOUT_FILTER")      FilterType = eWashout    ;
  else if (Type == "INTEGRATOR")          FilterType = eIntegrator ;
  else                                    FilterType = eUnknown    ;

  while ((token = AC_cfg->GetValue()) != "/COMPONENT") {
    *AC_cfg >> token;
    if (token == "ID") {
      *AC_cfg >> ID;
      cout << "      ID: " << ID << endl;
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
      cout << "      INPUT: " << token << endl;
    } else if (token == "C1") {
      *AC_cfg >> C1;
      cout << "      C1: " << C1 << endl;
    } else if (token == "C2") {
      *AC_cfg >> C2;
      cout << "      C2: " << C2 << endl;
    } else if (token == "C3") {
      *AC_cfg >> C3;
      cout << "      C3: " << C3 << endl;
    } else if (token == "C4") {
      *AC_cfg >> C4;
      cout << "      C4: " << C4 << endl;
    } else if (token == "C5") {
      *AC_cfg >> C5;
      cout << "      C5: " << C5 << endl;
    } else if (token == "C6") {
      *AC_cfg >> C6;
      cout << "      C6: " << C6 << endl;
    } else if (token == "OUTPUT") {
      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputIdx = fcs->GetState()->GetParameterIndex(sOutputIdx);
      cout << "      OUTPUT: " << sOutputIdx << endl;
    }
  }

  Initialize = true;

  switch (FilterType) {
    case eLag:
      denom = 2.00 + dt*C1;
      ca = dt*C1 / denom;
      cb = (2.00 - dt*C1) / denom;
      break;
    case eLeadLag:
      denom = 2.00*C3 + dt*C4;
      ca = (2.00*C1 + dt*C2) / denom;
      cb = (dt*C2 - 2.00*C1) / denom;
      cc = (2.00*C3 - dt*C2) / denom;
      break;
    case eOrder2:
      break;
    case eWashout:
      denom = 2.00 + dt*C1;
      ca = 2.00 / denom;
      cb = (2.00 - dt*C1) / denom;
      break;
    case eIntegrator:
      ca = dt*C1 / 2.00;
      break;
  }

  if (debug_lvl & 2) cout << "Instantiated: FGFilter" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFilter::~FGFilter()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGFilter" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFilter::Run(void)
{
  FGFCSComponent::Run(); // call the base class for initialization of Input

  if (Initialize) {

    PreviousOutput1 = PreviousInput1 = Output = Input;
    Initialize = false;

  } else {

    switch (FilterType) {
      case eLag:
        Output = Input * ca + PreviousOutput1 * cb;
        break;
      case eLeadLag:
        Output = Input * ca + PreviousInput1 * cb + PreviousOutput1 * cc;
        break;
      case eOrder2:
        break;
      case eWashout:
        Output = Input * ca - PreviousInput1 * ca + PreviousOutput1 * cb;
        break;
      case eIntegrator:
        Output = Input * ca + PreviousInput1 * ca + PreviousOutput1;
        break;
    }

  }

  PreviousOutput2 = PreviousOutput1;
  PreviousOutput1 = Output;
  PreviousInput2  = PreviousInput1;
  PreviousInput1  = Input;

  if (IsOutput) SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFilter::Debug(void)
{
    //TODO: Add your source code here
}

