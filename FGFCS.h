/*******************************************************************************
 
 Header:       FGGFCS.h
 Author:       Jon S. Berndt
 Date started: 12/12/98
 
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
 
HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFCS_H
#define FGFCS_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
#endif

#include <string>
#include "filtersjb/FGFCSComponent.h"
#include "FGModel.h"
#include "FGConfigFile.h"

#define ID_FCS "$Header"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFCS : public FGModel {
private:
  float DaCmd, DeCmd, DrCmd, DfCmd, DsbCmd, DspCmd;
  float DaPos, DePos, DrPos, DfPos, DsbPos, DspPos;
  float PTrimCmd;
  float ThrottleCmd[MAX_ENGINES];       // Needs to be changed: no limit
  float ThrottlePos[MAX_ENGINES];       // Needs to be changed: no limit

  vector <FGFCSComponent*> Components;

public:
  FGFCS(FGFDMExec*);
  ~FGFCS(void);

  bool Run(void);

  inline float GetDaCmd(void) { return DaCmd; }
  inline float GetDeCmd(void) { return DeCmd; }
  inline float GetDrCmd(void) { return DrCmd; }
  inline float GetDfCmd(void) { return DfCmd; }
  inline float GetDsbCmd(void) { return DsbCmd; }
  inline float GetDspCmd(void) { return DspCmd; }
  inline float GetThrottleCmd(int ii) { return ThrottleCmd[ii]; }
  inline float GetPitchTrimCmd(void) { return PTrimCmd; }

  inline float GetDaPos(void) { return DaPos; }
  inline float GetDePos(void) { return DePos; }
  inline float GetDrPos(void) { return DrPos; }
  inline float GetDfPos(void) { return DfPos; }
  inline float GetDsbPos(void) { return DsbPos; }
  inline float GetDspPos(void) { return DspPos; }

  inline float GetThrottlePos(int ii) { return ThrottlePos[ii]; }
  inline FGState* GetState(void) { return State; }
  float GetComponentOutput(eParam idx);
  string GetComponentName(int idx);

  inline void SetDaCmd(float tt) { DaCmd = tt; }
  inline void SetDeCmd(float tt) { DeCmd = tt; }
  inline void SetDrCmd(float tt) { DrCmd = tt; }
  inline void SetDfCmd(float tt) { DfCmd = tt; }
  inline void SetDsbCmd(float tt) { DsbCmd = tt; }
  inline void SetDspCmd(float tt) { DspCmd = tt; }
  inline void SetPitchTrimCmd(float tt) { PTrimCmd = tt; }

  void SetThrottleCmd(int ii, float tt);

  inline void SetDaPos(float tt) { DaPos = tt; }
  inline void SetDePos(float tt) { DePos = tt; }
  inline void SetDrPos(float tt) { DrPos = tt; }
  inline void SetDfPos(float tt) { DfPos = tt; }
  inline void SetDsbPos(float tt) { DsbPos = tt; }
  inline void SetDspPos(float tt) { DspPos = tt; }

  void SetLBrake(float);
  void SetRBrake(float);
  void SetCBrake(float);

  void SetThrottlePos(int ii, float tt);

  bool LoadFCS(FGConfigFile* AC_cfg);
  string FCSName;
};

#include "FGState.h"

#endif
