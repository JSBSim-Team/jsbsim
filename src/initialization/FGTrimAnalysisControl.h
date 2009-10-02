/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrimAnalysisControl.h
 Author:       Agostino De Marco
 Date started: Dec/14/2006

 ------------- Copyright (C) 2006  Agostino De Marco (agodemar@unina.it) -------

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
12/14/06   ADM   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRIMANALYSISCONTROL_H
#define FGTRIMANALYSISCONTROL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>

#include "FGFDMExec.h"
#include "FGJSBBase.h"
#include "initialization/FGInitialCondition.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TRIMANALYSISCONTROL "$Id: FGTrimAnalysisControl.h,v 1.2 2009/10/02 10:30:09 jberndt Exp $"

#define DEFAULT_TRIM_ANALYSIS_TOLERANCE 0.00000001

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGInitialCondition;

/**
    Available target state
*/
enum TaState { taAll,taUdot,taVdot,taWdot,taQdot,taPdot,taRdot,taHmgt,taNlf };
/**
    Available controls
*/
enum TaControl { taThrottle = 0,
                 taPitchTrim, taRollTrim, taYawTrim,
                 taElevator,  taAileron,  taRudder,
                 taPhi, taTheta, taHeading,
                 taGamma, taAltAGL, taBeta, taAlpha};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models an aircraft control variables for purposes of trimming.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTrimAnalysisControl : public FGJSBBase
{
public:
  /**  Constructor for Trim Analysis Control class.
       @param fdmex FGFDMExec pointer
       @param IC pointer to initial conditions instance
       @param control a Control type (enum) */
  FGTrimAnalysisControl(FGFDMExec* fdmex,
                        FGInitialCondition *IC,
                        //State state,
                        TaControl control );
  /// Destructor
  ~FGTrimAnalysisControl();

  /** This function iterates through a call to the FGFDMExec::RunIC()
      function until the desired trimming condition falls inside a tolerance.*/
  void Run(void);

  //double GetState(void) { getState(); return state_value; }
  //Accels are not settable

  /** Sets the control value
      @param value
  */
  inline void SetControl(double value ) { control_value=value; }

  /** Gets the control value
      @return value
  */
  inline double GetControl(void) { return control_value; }

  //inline State GetStateType(void) { return state; }
  /** Return the control type
      @return TaControl
  */
  inline TaControl GetControlType(void) { return control; }

  //inline string GetStateName(void) { return StateNames[state]; }

  /** Gets the control name
      @return control name
  */
  inline string GetControlName(void) { return control_name; }

  /** Gets the control minimum value
      @return control min value
  */
  inline double GetControlMin(void) { return control_min; }

  /** Gets the control maximum value
      @return control nax value
  */inline double GetControlMax(void) { return control_max; }

  /** Set control step
      @param value of control step
  */
  inline void SetControlStep(double value) { control_step = value; }
  /** Get control step
      @return value of control step
  */
  inline double GetControlStep(void) { return control_step; }

  /** Set control initial value
      @param value of control initial value
  */
  inline void SetControlInitialValue(double value) { control_initial_value = value; }

  /** Get control step
      @return value of control initial value
  */
  inline double GetControlInitialValue(void) { return control_initial_value; }

  /** Set control value to minimum
  */
  inline void SetControlToMin(void) { control_value=control_min; }

  /** Set control value to maximum
  */
  inline void SetControlToMax(void) { control_value=control_max; }

  /** Set both control limits
      @param max control max
      @param min control min
  */
  inline void SetControlLimits(double min, double max) {
      control_min=min;
      control_max=max;
  }

  /** Set control tolerance
      @param ff value of control tolerance
  */
  inline void  SetTolerance(double ff) { control_tolerance=ff;}

  /** Get control tolerance
      @return value of control tolerance
  */
  inline double GetTolerance(void) { return control_tolerance; }

  /** Set theta value on ground for trim
      @param ff
  */
  void SetThetaOnGround(double ff);

  /** Set phi value on ground for trim
      @param ff
  */
  void SetPhiOnGround(double ff);

  /** Set target state value for trim
      @param target
  */
  inline void SetStateTarget(double target) { state_target=target; }

  /** Get target state value for trim
      @return state target
  */
  inline double GetStateTarget(void) { return state_target; }

  /** Calculate steady state thetas value on ground
      @return true if successful
  */
  bool initTheta(void);

private:
  FGFDMExec *fdmex;
  FGInitialCondition *fgic;

  TaState   state;
  TaControl control;

  string control_name;

  double state_target;

  double state_value;
  double control_value;

  double control_min;
  double control_max;

  double control_initial_value;
  double control_step;
  double control_tolerance;

  double state_convert;
  double control_convert;

  void setThrottlesPct(void);

  void getState(void);
  void getControl(void);
  void setControl(void);

  double computeHmgt(void);

  void Debug(int from);
};
}
#endif
