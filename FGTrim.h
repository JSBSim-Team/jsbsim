/*******************************************************************************
 
 Header:       FGTrim.h
 Author:       Tony Peden
 Date started: 7/1/99
 
 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------
 
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
9/8/99   TP   Created
 
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
 
This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time 
scheme.  
 
 
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTRIM_H
#define FGTRIM_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGTrim.h"
#include "FGTrimAxis.h"

#include <vector.h>

#define ID_TRIM "$Header"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

typedef enum { tLongitudinal, tFull, tGround } TrimMode;

class FGTrim {
private:

  vector<FGTrimAxis*> TrimAxes;
  int current_axis;
  int N, Nsub;
  int NumAxes;
  TrimMode mode;
  int Debug;
  float Tolerance, A_Tolerance;
  float wdot,udot,qdot;
  float dth;
  float *sub_iterations;
  float *successful;
  bool *solution;
  int max_sub_iterations;
  int max_iterations;
  int total_its;
  bool trimudot;
  bool gamma_fallback;
  bool trim_failed;
  int axis_count;
  int solutionDomain;
  float xlo,xhi,alo,ahi;


  FGFDMExec* fdmex;
  FGInitialCondition* fgic;

  // returns false if there is no change in the current axis accel
  // between accel(control_min) and accel(control_max). if there is a
  // change, sets solutionDomain to:
  // 0 for no sign change,
  // -1 if sign change between accel(control_min) and accel(0)
  // 1 if sign between accel(0) and accel(control_max)
  bool solve(void);
  bool findInterval(void);
  bool checkLimits(void);

public:
  FGTrim(FGFDMExec *FDMExec, FGInitialCondition *FGIC, TrimMode tt);
  ~FGTrim(void);

  bool DoTrim(void);

  void Report(void);
  void ReportState(void);
  void TrimStats();

  inline void SetUdotTrim(bool bb) { trimudot=bb; }
  inline bool GetUdotTrim(void) { return trimudot; }

  inline void SetGammaFallback(bool bb) { gamma_fallback=true; }
  inline bool GetGammaFallback(void) { return gamma_fallback; }

  inline void SetMaxCycles(int ii) { max_iterations = ii; }
  inline void SetMaxCyclesPerAxis(int ii) { max_sub_iterations = ii; }
  inline void SetTolerance(float tt) {
    Tolerance = tt;
    A_Tolerance = tt / 10;
  }
  //Debug level 1 shows results of each top-level iteration
  //Debug level 2 shows level 1 & results of each per-axis iteration
  inline void SetDebug(int level) { Debug = level; }
  inline void ClearDebug(void) { Debug = 0; }

};


#endif









