/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFCS_H
#define FGFCS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCS "$Header"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the Flight Control System (FCS) functionality.
    This class owns and contains the list of \URL[components]{FGFCSComponent.html}
    that define the control system for this aircraft. The config file for the
    aircraft contains a description of the control path that starts at an input
    or command and ends at an effector, e.g. an aerosurface. The FCS components
    which comprise the control laws for an axis are defined sequentially in
    the configuration file. For instance, for the X-15:
    
    <pre>
    &ltFLIGHT_CONTROL NAME="X-15 SAS"&gt

    &ltCOMPONENT NAME="Pitch Trim Sum" TYPE="SUMMER"&gt
      ID            0
      INPUT        FG_ELEVATOR_CMD
      INPUT        FG_PITCH_TRIM_CMD
      CLIPTO       -1 1
    &lt/COMPONENT&gt

    &ltCOMPONENT NAME="Pitch Command Scale" TYPE="AEROSURFACE_SCALE"&gt
      ID           1
      INPUT        0
      MIN         -50
      MAX          50
    &lt/COMPONENT&gt

    &ltCOMPONENT NAME="Pitch Gain 1" TYPE="PURE_GAIN"&gt
      ID           2
      INPUT        1
      GAIN         -0.36
    &lt/COMPONENT&gt

    &ltCOMPONENT NAME="Pitch Scheduled Gain 1" TYPE="SCHEDULED_GAIN"&gt
      ID           3
      INPUT        2
      GAIN         0.017
      SCHEDULED_BY FG_ELEVATOR_POS
      -0.35  -6.0
      -0.17  -3.0
       0.00  -2.0
       0.09  -3.0
       0.17  -5.0
       0.60 -12.0
    &lt/COMPONENT&gt

    ... etc.
    </pre>
    
    In the above case we can see the first few components of the pitch channel
    defined. The input to the first component, as can be seen in the "Pitch trim
    sum" component, is really the sum of two parameters: elevator command (from
    the stick - a pilot input), and pitch trim. The type of this component is
    "Summer". Its ID is 0 - the ID is used by other components to reference it.
    
    @author Jon S. Berndt
    @version $Id: FGFCS.h,v 1.18 2000/10/20 00:00:24 jsb Exp $
    @see FGFCSComponent
    @see FGConfigFile
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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
