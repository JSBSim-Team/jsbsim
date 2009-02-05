/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrim.h
 Author:       Tony Peden
 Date started: 7/1/99

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
9/8/99   TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class takes the given set of IC's and finds the aircraft state required to
maintain a specified flight condition.  This flight condition can be
steady-level with non-zero sideslip, a steady turn, a pull-up or pushover.
On-ground conditions can be trimmed as well, but this is currently limited to
adjusting altitude and pitch angle only. It is implemented using an iterative,
one-axis-at-a-time scheme.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRIM_H
#define FGTRIM_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGJSBBase.h"
#include "FGTrimAxis.h"

#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TRIM "$Id: FGTrim.h,v 1.6 2009/02/05 10:22:49 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

typedef enum { tLongitudinal=0, tFull, tGround, tPullup,
               tCustom, tTurn, tNone } TrimMode;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** The trimming routine for JSBSim.
    FGTrim finds the aircraft attitude and control settings needed to maintain
    the steady state described by the FGInitialCondition object .  It does this
    iteratively by assigning a control to each state and adjusting that control
    until the state is within a specified tolerance of zero. States include the
    recti-linear accelerations udot, vdot, and wdot, the angular accelerations
    qdot, pdot, and rdot, and the difference between heading and ground track.
    Controls include the usual flight deck controls available to the pilot plus
    angle of attack (alpha), sideslip angle(beta), flight path angle (gamma),
    pitch attitude(theta), roll attitude(phi), and altitude above ground.  The
    last three are used for on-ground trimming. The state-control pairs used in
    a given trim are completely user configurable and several pre-defined modes
    are provided as well. They are:
    - tLongitudinal: Trim wdot with alpha, udot with thrust, qdot with elevator
    - tFull: tLongitudinal + vdot with phi, pdot with aileron, rdot with rudder
             and heading minus ground track (hmgt) with beta
    - tPullup: tLongitudinal but adjust alpha to achieve load factor input
               with SetTargetNlf()
    - tGround: wdot with altitude, qdot with theta, and pdot with phi

    The remaining modes include <b>tCustom</b>, which is completely user defined and
    <b>tNone</b>.

    Note that trims can (and do) fail for reasons that are completely outside
    the control of the trimming routine itself. The most common problem is the
    initial conditions: is the model capable of steady state flight
    at those conditions?  Check the speed, altitude, configuration (flaps,
    gear, etc.), weight, cg, and anything else that may be relevant.

    Example usage:
    @code
    FGFDMExec* FDMExec = new FGFDMExec();

    FGInitialCondition* fgic = new FGInitialCondition(FDMExec);
    FGTrim fgt(FDMExec, fgic, tFull);
    fgic->SetVcaibratedKtsIC(100);
    fgic->SetAltitudeFtIC(1000);
    fgic->SetClimbRate(500);
    if( !fgt.DoTrim() ) {
      cout << "Trim Failed" << endl;
    }
    fgt.Report();
    @endcode
    
    @author Tony Peden
    @version "$Id: FGTrim.h,v 1.6 2009/02/05 10:22:49 jberndt Exp $"
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTrim : public FGJSBBase
{
private:

  vector<FGTrimAxis*> TrimAxes;
  unsigned int current_axis;
  int N, Nsub;
  TrimMode mode;
  int DebugLevel, Debug;
  double Tolerance, A_Tolerance;
  double wdot,udot,qdot;
  double dth;
  double *sub_iterations;
  double *successful;
  bool *solution;
  int max_sub_iterations;
  int max_iterations;
  int total_its;
  bool trimudot;
  bool gamma_fallback;
  bool trim_failed;
  unsigned int axis_count;
  int solutionDomain;
  double xlo,xhi,alo,ahi;
  double targetNlf;
  int debug_axis;

  double psidot,thetadot;

  FGFDMExec* fdmex;
  FGInitialCondition* fgic;

  bool solve(void);

  /** @return false if there is no change in the current axis accel
      between accel(control_min) and accel(control_max). If there is a
      change, sets solutionDomain to:
      0 for no sign change,
     -1 if sign change between accel(control_min) and accel(0)
      1 if sign between accel(0) and accel(control_max)
  */
  bool findInterval(void);

  bool checkLimits(void);

  void setupPullup(void);
  void setupTurn(void);

  void updateRates(void);

  void setDebug(void);

public:
  /** Initializes the trimming class
      @param FDMExec pointer to a JSBSim executive object.
      @param tm trim mode
  */
  FGTrim(FGFDMExec *FDMExec, TrimMode tm=tGround );

  ~FGTrim(void);

  /** Execute the trim
  */
  bool DoTrim(void);

  /** Print the results of the trim. For each axis trimmed, this
      includes the final state value, control value, and tolerance
      used.
      @return true if trim succeeds
  */
  void Report(void);

  /** Iteration statistics
  */
  void TrimStats();

  /** Clear all state-control pairs and set a predefined trim mode
      @param tm the set of axes to trim. Can be:
             tLongitudinal, tFull, tGround, tCustom, or tNone
  */
  void SetMode(TrimMode tm);

  /** Clear all state-control pairs from the current configuration.
      The trimming routine must have at least one state-control pair
      configured to be useful
  */
  void ClearStates(void);

  /** Add a state-control pair to the current configuration. See the enums
      State and Control in FGTrimAxis.h for the available options.
      Will fail if the given state is already configured.
      @param state the accel or other condition to zero
      @param control the control used to zero the state
      @return true if add is successful
  */
  bool AddState( State state, Control control );

  /** Remove a specific state-control pair from the current configuration
      @param state the state to remove
      @return true if removal is successful
  */
  bool RemoveState( State state );

  /** Change the control used to zero a state previously configured
      @param state the accel or other condition to zero
      @param new_control the control used to zero the state
  */
  bool EditState( State state, Control new_control );

  /** automatically switch to trimming longitudinal acceleration with
      flight path angle (gamma) once it becomes apparent that there
      is not enough/too much thrust.
      @param bb true to enable fallback
  */
  inline void SetGammaFallback(bool bb) { gamma_fallback=bb; }

  /** query the fallback state
      @return true if fallback is enabled.
  */
  inline bool GetGammaFallback(void) { return gamma_fallback; }

  /** Set the iteration limit. DoTrim() will return false if limit
      iterations are reached before trim is achieved.  The default
      is 60.  This does not ordinarily need to be changed.
      @param ii integer iteration limit
  */
  inline void SetMaxCycles(int ii) { max_iterations = ii; }

  /** Set the per-axis iteration limit.  Attempt to zero each state
      by iterating limit times before moving on to the next. The
      default limit is 100 and also does not ordinarily need to
      be changed.
      @param ii integer iteration limit
  */
  inline void SetMaxCyclesPerAxis(int ii) { max_sub_iterations = ii; }

  /** Set the tolerance for declaring a state trimmed. Angular accels are
      held to a tolerance of 1/10th of the given.  The default is
      0.001 for the recti-linear accelerations and 0.0001 for the angular.
  */
  inline void SetTolerance(double tt) {
    Tolerance = tt;
    A_Tolerance = tt / 10;
  }

  /**
    Debug level 1 shows results of each top-level iteration
    Debug level 2 shows level 1 & results of each per-axis iteration
  */
  inline void SetDebug(int level) { DebugLevel = level; }
  inline void ClearDebug(void) { DebugLevel = 0; }

  /**
    Output debug data for one of the axes
    The State enum is defined in FGTrimAxis.h
  */
  inline void DebugState(State state) { debug_axis=state; }

  inline void SetTargetNlf(double nlf) { targetNlf=nlf; }
  inline double GetTargetNlf(void) { return targetNlf; }

};
}

#endif
