/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrimAnalysis.h
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

This class takes the given set of IC's and analyzes the possible trim states
of the aircraft, i.e. finds the aircraft state required to
maintain a specified flight condition.  This flight condition can be
steady-level, a steady turn, a pull-up or pushover.
It is implemented using an iterative, direct search of a cost function minimum.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRIMANAlYSIS_H
#define FGTRIMANAlYSIS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGJSBBase.h"
#include "FGTrimAnalysisControl.h"

#include "models/FGAtmosphere.h"
#include "initialization/FGInitialCondition.h"
#include "models/FGAircraft.h"
#include "models/FGMassBalance.h"
#include "models/FGGroundReactions.h"
#include "models/FGInertial.h"
#include "models/FGAerodynamics.h"
#include "math/FGColumnVector3.h"
#include "models/FGAuxiliary.h"
#include "models/FGPropulsion.h"

#include <vector>
#include <map>
#include <string>

#include "math/direct_search/vec.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FGTRIMANALYSIS "$Id: FGTrimAnalysis.h,v 1.8 2009/10/02 10:30:09 jberndt Exp $"

#if defined(_WIN32) && !defined(__CYGWIN__)
  #define snprintf _snprintf
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

typedef enum { taLongitudinal=0, taFull, taFullWingsLevel, taTurn, taPullup, taTurnFull,
                taGround, taCustom, taNone } TrimAnalysisMode;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** The trimming routine for JSBSim.

    Note that trims can (and do) fail for reasons that are completely outside
    the control of the trimming routine itself. The most common problem is the
    initial conditions: is the model capable of steady state flight
    at those conditions?  Check the speed, altitude, configuration (flaps,
    gear, etc.), weight, cg, and anything else that may be relevant.

    Example usage:
    @code
    FGFDMExec* FDMExec = new FGFDMExec();

    FGInitialCondition* fgic = new FGInitialCondition(FDMExec);
    FGTrimAnalysis fgta(FDMExec, fgic, taFull);
    fgic->SetVcaibratedKtsIC(100);
    fgic->SetAltitudeFtIC(1000);
    fgic->SetClimbRate(500);
    if( !fgta.DoTrim() ) {
      cout << "Trim Failed" << endl;
    }
    fgta.Report();
    @endcode

    @author Agostino De Marco
    @version "$Id: FGTrimAnalysis.h,v 1.8 2009/10/02 10:30:09 jberndt Exp $"
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTrimAnalysis;

class Objective {
public:
    /** Constructor
    */
    Objective(FGFDMExec* fdmex, FGTrimAnalysis* ta, double x);
    /** Destructor
    */
    ~Objective() {}

    /** Full Trim cost function
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionFull                (long vars, Vector<double> &v, double & f);
    /** Wings Level Trim cost function
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionFullWingsLevel      (long vars, Vector<double> &v, double & f);
    /** Longitudinal Trim cost function
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionLongitudinal        (long vars, Vector<double> &v, double & f);
    /** Steady Turn Trim cost function
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionFullTurn (long vars, Vector<double> &v, double & f);
    /** Steady Turn Trim cost function, NON-coordinated
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionFullCoordinatedTurn (long vars, Vector<double> &v, double & f);

    /** Pullup Trim cost function
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value

        Note: the effective cost function evaluation is performed in a private function
    */
    void CostFunctionPullUp              (long vars, Vector<double> &v, double & f);

    /** Wrapping function for the effective Full Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionFull(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    /** Wrapping function for the effective Wings Level Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionFullWingsLevel(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    /** Wrapping function for the effective Longitudinal Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionLongitudinal(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    /** Wrapping function for the effective Steady Turn Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionFullCoordinatedTurn(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    /** Wrapping function for the effective Steady Turn Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionFullTurn(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    /** Wrapping function for the effective Pullup Trim cost function, to be called by optimization method
        @param vars number ofcontrol varables
        @param v reference to a vector containing controls variables
        @param f function value
        @param success
        @param t_ptr
     * the following method is friend rather then member of FGTrimAnalysis because
     * we want our FGTrimAnalysis::DoTrim() to be able to pass pointers to it.
     *
     * Note that in the call masked by this methods, the void pointer
     * should be cast to a pointer of the class type.
     */
    friend void find_CostFunctionPullUp(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr);
    /* function definiation moved outside of class declaration */

    void Set_x_val(double new_x);
    double Get_x_val() const {return _x;}

    /** Pointer to cost function implementation
    */
    typedef void (*PF)(long vars, Vector<double> &v, double & f, bool & success, void* t_ptr);

    map<TrimAnalysisMode,PF> mpCostFunctions; // primordial...

private:
    /**
        Calculate state variables derivatives (udot, vdot, ...) as a function of control variables
    */
    void calculateDottedStates(double delta_cmd_T, double delta_cmd_E, double delta_cmd_A, double delta_cmd_R,
                               double phi, double theta, double psi,
                               TrimAnalysisMode trimMode,
                               double& alpha, double& beta, double& gamma,
                               //FGColumnVector3& vUVW, FGColumnVector3& vPQR,
                               FGPropagate::VehicleState& VState,
                               FGColumnVector3& vUVWdot, FGColumnVector3& vPQRdot );

    /** Calculate cost function for Full Trim
    */
    double myCostFunctionFull                  (Vector<double> & vec1);
    /** Calculate cost function for Wings Level Trim
    */
    double myCostFunctionFullWingsLevel        (Vector<double> & vec1);
    /** Calculate cost function for Longitudinal Trim
    */
    double myCostFunctionLongitudinal          (Vector<double> & vec1);
    /** Calculate cost function for Steady Turn Trim
    */
    double myCostFunctionFullCoordinatedTurn   (Vector<double> & vec1);
    /** Calculate cost function for Steady Turn Trim, NON-Coordinated
    */
    double myCostFunctionFullTurn              (Vector<double> & vec1);
    /** Calculate cost function for Pullup Trim
    */
    double myCostFunctionPullUp                (Vector<double> & vec1);

    double _x;
    FGFDMExec* FDMExec;
    FGTrimAnalysis* TrimAnalysis;
};


class FGTrimAnalysis : public FGJSBBase, public FGXMLFileRead
{
private:
  vector<FGTrimAnalysisControl*> vTrimAnalysisControls;
  double cost_function_value;
  unsigned int current_ctrl;
  int N, Nsub;
  TrimAnalysisMode mode;
  int DebugLevel, Debug;

  double dth;
  bool trimudot;
  bool gamma_fallback;

  bool trim_failed;
  unsigned int ctrl_count;

  FGFDMExec* fdmex;
  FGInitialCondition * fgic; // IC;

  FGAuxiliary *        Auxiliary;
  FGAerodynamics *     Aerodynamics;
  FGPropulsion *       Propulsion;
  FGFCS *              FCS;

  vector<double> vAlphaDeg, vAlphaTrimDeg; // DEG !!!
  vector<double> vCL, vCD, vCm;
  vector<double> vCLE, vCDE, vCmE;
  vector<vector <double> > mCL, mCD, mCm, mTa;
  vector<double> vThrottleCmd, vElevatorCmd;
  vector<double> vVn, vTn, vTa, vVnE, vTE;
  vector<double> vCt, vCq, vRPM;
  vector<vector <double> > mCt, mCq, mRPM;
  vector<bool> hasThrustTrim, hasCmTrim;

  string trim_id;

  // direct search stuff
  string search_type;
  double sigma_nm, alpha_nm, beta_nm, gamma_nm;
  double initial_step;
  double tolerance;
  string stop_criterion;
  int max_iterations;
  int total_its;

  // results file
  mutable ofstream rf; // see Stroustrup 10.2.7.2; alternative: use a cache struct
  string rf_name;

  double _u,_v,_w;
  double _p,_q,_r;
  //double uw,vw,ww;
  //double vnorth,veast,vdown;
  //double wnorth,weast,wdown;
  double _alpha, _beta, _theta, _phi, _psi, _psiW, _gamma, _phiW;

  double _stheta, _sphi, _spsi;
  double _ctheta, _cphi, _cpsi;

  double _vtIC, _hIC, _gammaIC, _rocIC, _vdownIC, _psiIC, _psigtIC, _vgIC,
         _vnorthIC, _veastIC, wnorthIC, _weastIC, _wdownIC;

  double _udot, _vdot, _wdot, _pdot, _qdot, _rdot;
  double _targetNlf;
  double _psiWdot, _phiWdot, _gammadot, _psidot, _thetadot;

  double C1, C2, C3, _calpha, _salpha, _cbeta, _sbeta;

  void setupPullup(void);
  void setupTurn(void);
  void setupTurn(double phiW); // recalculate target Nlf
  void setupTurnPhi(double psi, double theta); // recalculate only phi

  void updateRates(void);

  void setDebug(void);

  bool ensureRunning(void);
  bool ensureRunning(unsigned int i);
  bool runForAWhile(int nruns);

  bool populateVecAlphaDeg(double vmin, double vmax, int n);
  bool populateVecThrottleCmd(double vmin, double vmax, int n);
  bool populateVecElevatorCmd(double vmin, double vmax, int n);

  bool calculateAerodynamics(
                           double dE_cmd,
                           double Vt,
                           double alpha_deg,
                           double altitude,
                           double rho,
                           double S, double mac, double bw,
                           double& CL, double& CD, double& Cm);


  bool getSteadyState(int nrepeat);
  bool InitializeTrimControl(double default_value, Element* el,
                                           string unit, TaControl type);

public:
  /** Initializes the trimming class
      @param FDMExec pointer to a JSBSim executive object.
      @param tam trim mode
  */
  FGTrimAnalysis(FGFDMExec *FDMExec, TrimAnalysisMode tam=taFull );

  /// Destructor
  ~FGTrimAnalysis(void);

  friend class Objective;

  /** Loads the trim configuration from file.
      @param fname The name of a trim configuration file
      @param useStoredPath true if the stored path to the trim config file should be used
      @return true if successful */
  bool Load(string fname, bool useStoredPath = true );

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

   /** Set the file where trim analysis results are written,
      open and get ready
      @param name the file name
      @return true if file open is successful
  */
  bool SetResultsFile(string name);

  /** Get the pointer to the file where trim analysis results are written,
      @return non null pointer if file is open
  */
  ofstream* GetResultsFile() const { if (rf.is_open()) return &rf; else return 0; } // const_cast<ofstream*>(&rf) (if rf is not mutable)

  /** Set the value of the cost function
      @param value the cost function value
  */
  inline void SetCostFunctionValue(double value){cost_function_value = value;}

  /** @return The current cost function value
  */
  inline double GetCostFunctionValue() const { return cost_function_value;}

  /** Clear all controls and set a predefined trim mode
      (Note: controls are intended here as those variables to be
             adjusted for attaining convergence of the trimming algorithm)
      @param tam the set of axes to trim. Can be:
             taLongitudinal, taFull, taGround, taCustom, or taNone
  */
  void SetMode(TrimAnalysisMode tam);

  /** @return The Trim Analysis mode
             taLongitudinal, taFull, taGround, taCustom, or taNone
  */
  inline TrimAnalysisMode GetMode() const { return mode;};

  inline vector<FGTrimAnalysisControl*>* GetControls(){return &vTrimAnalysisControls;}

  /** Clear all controls from the current configuration.
      The trimming routine must have at least one control
      configured to be useful
      (Note: controls are intended here as those variables to be
             adjusted for attaining convergence of the trimming algorithm)
  */
  void ClearControls(void);

  /** Add a control to the current configuration. See the enums
      TaControl in FGTrimAnalysisControl.h for the available options.
      (Note: controls are intended here as those variables to be
             adjusted for attaining convergence of the trimming algorithm)
      Will fail if the given state is already configured.
      @param control the control to be adjusted in the trimming algorithm
      @return true if add is successful
  */
  bool AddControl( TaControl control );

  /** Remove a specific control from the current configuration
      @param control the state to remove
      @return true if removal is successful
  */
  bool RemoveControl( TaControl control );

  /** Change the control settings previously configured
      @param new_control the control used to zero the state
      @param new_initvalue a new initial value
      @param new_step a new adjusting step
      @param new_min, new_max a new range
      @return true if editing is successful
  */
  bool EditState( TaControl new_control, double new_initvalue, double new_step, double new_min, double new_max );


  /** Return the current flight path angle in TrimAnalysis object
      @return value of gamma
  */
  inline double GetGamma() { return _gamma; }

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

  /** Set the tolerance for declaring a state trimmed.
      (In the evaluation of the cost function, squares of angular accels are
      devided by 100)
      The default is 1e-8.
      @param tt user defined tolerance
  */
  inline void SetTolerance(double tt) {
    tolerance = tt;
  }
  /** Get the tolerance for declaring a state trimmed.
      @return tolerance.
  */
  inline double GetTolerance(void) {return tolerance; }

  /** Sets trim result status
      @param tf (boolean)
  */
  inline void SetTrimFailed(bool tf) { trim_failed = tf; }
  /** Gets trim result status
      @return trim_failed (boolean)
  */
  inline bool GetTrimFailed(void) { return trim_failed; }
  inline void SetTrimSuccessfull() { trim_failed = false; }


  /** Sets state variables
      @param u0
      @param v0
      @param w0
      @param p0
      @param q0
      @param r0
      @param alpha0
      @param beta0
      @param phi0
      @param psi0
      @param theta0
      @param gamma0
  **/
  void SetState(double u0, double v0, double w0, double p0, double q0, double r0,
                double alpha0, double beta0, double phi0, double theta0, double psi0, double gamma0);

  /** Sets Euler angles
      @param phi0, theta0, psi0 **/
  void SetEulerAngles(double phi0, double theta0, double psi0);

  /** Gets Euler angle phi
      @return phi [rad]
  */
  double GetPhiRad  (){ return _phi;}
  /** Gets Euler angle theta
      @return phi [rad]
  */
  double GetThetaRad(){ return _theta;}
  /** Gets Euler angle psi
      @return phi [rad]
  */
  double GetPsiRad  (){ return _psi;}

  /** Gets Euler angle phiW (wind axes)
      @return phiW [rad]
  */
  double GetPhiWRad  (){ return _phiW;}
  /** Gets flight path angle
      @return gamma [rad]
  */
  double GetGammaRad (){ return _gamma;}
  /** Gets true speed [fps] from IC
      @return Vt [fps]
  */
  double GetVtFps    (){ return _vtIC;}

  /** Calculate the wind axis bank angle from a given Nlf (sets also the target Nlf)
      @param nlf
  */
  void CalculatePhiWFromTargetNlfTurn(double nlf);

  /** Updates angular rates for turn trim according to turning trim constraints
      @param psi, theta, phi
      @return p, q, r
  */
  FGColumnVector3 UpdateRatesTurn(double psi, double theta, double phi, double phiW);

  /** Updates angular rates for pull-up trim
      @return p, q, r
  */
  FGColumnVector3 UpdateRatesPullup(void);

  /** Sets Dotted values
      @param udot, vdot, wdot, pdot, qdot, rdot **/
  void SetDottedValues(double udot, double vdot, double wdot, double pdot, double qdot, double rdot);

  /**
    Debug level 1 shows results of each top-level iteration
    Debug level 2 shows level 1 & results of each per-axis iteration
  */
  inline void SetDebug(int level) { DebugLevel = level; }
  inline void ClearDebug(void) { DebugLevel = 0; }

  /** Sets target normal load factor in steady turn
      @param nlf target normal load factor **/
  inline void SetTargetNlf(double nlf) { _targetNlf=nlf; }

  /** Gets target normal load factor in steady turn
      @return _targetNlf
  */
  inline double GetTargetNlf(void) { return _targetNlf; }

  //void eom_costf (long vars, Vector<double> &x, double & f, bool& flag, void* an_obj);
  //inline FGFDMExec* GetFDMExec() const { return fdmex; }

};
}

#endif
