/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       FGTrimAxis.h
 Author:       Tony Peden
 Date started: 7/3/00
 
 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.
 
 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.
 
 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.
 
 HISTORY
--------------------------------------------------------------------------------
7/3/00  TP   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRIMAXIS_H
#define FGTRIMAXIS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>

#include "FGFDMExec.h"
#include "FGJSBBase.h"
#include "FGInitialCondition.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TRIMAXIS "$Id: FGTrimAxis.h,v 1.5 2010/09/07 18:36:29 andgi Exp $"

#define DEFAULT_TOLERANCE 0.001

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

const string StateNames[] =   { "all","udot","vdot","wdot","qdot","pdot","rdot",
                                "hmgt","nlf" 
                              };
const string ControlNames[] =  { "Throttle","Sideslip","Angle of Attack",
                                 "Elevator","Ailerons","Rudder",
                                 "Altitude AGL", "Pitch Angle",
                                 "Roll Angle", "Flight Path Angle", 
                                 "Pitch Trim", "Roll Trim", "Yaw Trim",
                                 "Heading"
                               };

class FGInitialCondition;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models an aircraft axis for purposes of trimming.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

enum State { tAll,tUdot,tVdot,tWdot,tQdot,tPdot,tRdot,tHmgt,tNlf };
enum Control { tThrottle, tBeta, tAlpha, tElevator, tAileron, tRudder, tAltAGL,
               tTheta, tPhi, tGamma, tPitchTrim, tRollTrim, tYawTrim, tHeading };

class FGTrimAxis : public FGJSBBase
{
public:
  /**  Constructor for Trim Axis class.
       @param fdmex FGFDMExec pointer
       @param IC pointer to initial conditions instance
       @param state a State type (enum)
       @param control a Control type (enum) */
  FGTrimAxis(FGFDMExec* fdmex, 
             FGInitialCondition *IC, 
             State state,
             Control control );
  /// Destructor
  ~FGTrimAxis();

  /** This function iterates through a call to the FGFDMExec::RunIC() 
      function until the desired trimming condition falls inside a tolerance.*/
  void Run(void);
 
  double GetState(void) { getState(); return state_value; }
  //Accels are not settable
  inline void SetControl(double value ) { control_value=value; }
  inline double GetControl(void) { return control_value; }

  inline State GetStateType(void) { return state; }
  inline Control GetControlType(void) { return control; }

  inline string GetStateName(void) { return StateNames[state]; }
  inline string GetControlName(void) { return ControlNames[control]; }

  inline double GetControlMin(void) { return control_min; }
  inline double GetControlMax(void) { return control_max; }

  inline void SetControlToMin(void) { control_value=control_min; }
  inline void SetControlToMax(void) { control_value=control_max; }
  
  inline void SetControlLimits(double min, double max) { 
      control_min=min;
      control_max=max;
  }    

  inline void  SetTolerance(double ff) { tolerance=ff;}
  inline double GetTolerance(void) { return tolerance; }

  inline double GetSolverEps(void) { return solver_eps; }
  inline void SetSolverEps(double ff) { solver_eps=ff; }

  inline int  GetIterationLimit(void) { return max_iterations; }
  inline void SetIterationLimit(int ii) { max_iterations=ii; }

  inline int GetStability(void) { return its_to_stable_value; }
  inline int GetRunCount(void) { return total_stability_iterations; }
  double GetAvgStability( void );
  
  void SetThetaOnGround(double ff);
  void SetPhiOnGround(double ff);
  
  inline void SetStateTarget(double target) { state_target=target; }
  inline double GetStateTarget(void) { return state_target; }
  
  bool initTheta(void);
  
  void AxisReport(void);
  
  bool InTolerance(void) { getState(); return (fabs(state_value) <= tolerance); }

private:
  FGFDMExec *fdmex;
  FGInitialCondition *fgic;

  State   state;
  Control control;
  
  double state_target;
  
  double state_value;
  double control_value;

  double control_min;
  double control_max;

  double tolerance;

  double solver_eps;

  double state_convert;
  double control_convert;

  int max_iterations;

  int its_to_stable_value;
  int total_stability_iterations;
  int total_iterations;

  void setThrottlesPct(void);

  void getState(void);
  void getControl(void);
  void setControl(void);
  
  double computeHmgt(void);
  
  void Debug(int from);
};
}
#endif
