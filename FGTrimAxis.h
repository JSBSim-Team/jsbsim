/*******************************************************************************
 
 Header:       FGTrimAxis.h
 Author:       Tony Peden
 Date started: 7/3/00
 
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
7/3/00  TP   Created
 
 
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTRIMAXIS_H
#define FGTRIMAXIS_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include <string>

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

#define ID_TRIMAXIS "$Header"

const string AccelNames[6]=   { "udot","vdot","wdot","qdot","pdot","rdot" };
const string ControlNames[13]= { "Throttle","Sideslip","Angle of Attack",
                                 "Elevator","Ailerons","Rudder",
                                 "Altitude AGL", "Pitch Angle",
                                 "Roll Angle", "Flight Path Angle", 
                                 "Pitch Trim", "Roll Trim", "Yaw Trim"
                               };
/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

enum Accel { tUdot,tVdot,tWdot,tQdot,tPdot,tRdot };
enum Control { tThrottle, tBeta, tAlpha, tElevator, tAileron, tRudder, tAltAGL,
               tTheta, tPhi, tGamma, tPitchTrim, tRollTrim, tYawTrim };

class FGTrimAxis {
public:
  FGTrimAxis(FGFDMExec* fdmex, FGInitialCondition *ic, Accel acc,
             Control ctrl, float tolerance);
  ~FGTrimAxis();

  void Run(void);

  float GetAccel(void) { getAccel(); return accel_value; }
  //Accels are not settable
  inline void SetControl(float value ) { control_value=value; }
  inline float GetControl(void) { return control_value; }

  inline Accel GetAccelType(void) { return accel; }
  inline Control GetControlType(void) { return control; }

  inline string GetAccelName(void) { return AccelNames[accel]; }
  inline string GetControlName(void) { return ControlNames[control]; }

  inline float GetControlMin(void) { return control_min; }
  inline float GetControlMax(void) { return control_max; }

  inline void SetControlToMin(void) { control_value=control_min; }
  inline void SetControlToMax(void) { control_value=control_max; }

  inline void  SetTolerance(float ff) { tolerance=ff;}
  inline float GetTolerance(void) { return tolerance; }

  inline float GetSolverEps(void) { return solver_eps; }
  inline void SetSolverEps(float ff) { solver_eps=ff; }

  inline int  GetIterationLimit(void) { return max_iterations; }
  inline void SetIterationLimit(int ii) { max_iterations=ii; }

  inline int GetStability(void) { return its_to_stable_value; }
  inline int GetRunCount(void) { return total_stability_iterations; }
  float GetAvgStability( void );
  
  void SetThetaOnGround(float ff);
  void SetPhiOnGround(float ff);

  void AxisReport(void);
  
  bool InTolerance(void) { getAccel(); return (fabs(accel_value) <= tolerance); }

private:
  FGFDMExec *fdmex;
  FGInitialCondition *fgic;


  Accel   accel;
  Control control;

  float accel_value;
  float control_value;

  float control_min;
  float control_max;

  float tolerance;

  float solver_eps;

  float accel_convert;
  float control_convert;

  int max_iterations;

  int its_to_stable_value;
  int total_stability_iterations;
  int total_iterations;


  void setThrottlesPct(void);

  void getAccel(void);
  void getControl(void);
  void setControl(void);

};

#endif
