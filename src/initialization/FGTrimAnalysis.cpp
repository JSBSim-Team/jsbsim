/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrimAnalysis.cpp
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

//  !!!!!!! BEWARE ALL YE WHO ENTER HERE !!!!!!!

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdlib.h>

#include "FGFDMExec.h"
#include "models/FGAtmosphere.h"
#include "initialization/FGInitialCondition.h"
#include "FGTrimAnalysis.h"
#include "models/FGAircraft.h"
#include "models/FGMassBalance.h"
#include "models/FGGroundReactions.h"
#include "models/FGInertial.h"
#include "models/FGAerodynamics.h"
#include "math/FGColumnVector3.h"

#include "input_output/FGPropertyManager.h"
#include "input_output/FGXMLParse.h"

#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGPropeller.h"
#include "models/propulsion/FGTurbine.h"

#include "math/FGTable.h"

#if defined(sgi) && !defined(__GNUC__)
# include <math.h>
#else
# include <cmath>
#endif
#include <sstream>
#include <map>

#include "math/direct_search/SMDSearch.h"
#include "math/direct_search/NMSearch.h"
#include "math/direct_search/vec.h"

#if _MSC_VER
#pragma warning (disable : 4786 4788)
#endif

namespace JSBSim {

static const char *IdSrc = "$Id: FGTrimAnalysis.cpp,v 1.14 2011/03/23 11:58:29 jberndt Exp $";
static const char *IdHdr = ID_FGTRIMANALYSIS;


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
    //friend void find_CostFunctionFull(long vars, Vector<double> &v, double & f,
    void find_CostFunctionFull(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionFull(vars, v, f);
        success = true;
    }
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
    //friend void find_CostFunctionFullWingsLevel(long vars, Vector<double> &v, double & f,
    void find_CostFunctionFullWingsLevel(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionFullWingsLevel(vars, v, f);
        success = true;
    }
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
    //friend void find_CostFunctionLongitudinal(long vars, Vector<double> &v, double & f,
    void find_CostFunctionLongitudinal(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionLongitudinal(vars, v, f);
        success = true;
    }
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
    //friend void find_CostFunctionFullCoordinatedTurn(long vars, Vector<double> &v, double & f,
    void find_CostFunctionFullCoordinatedTurn(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionFullCoordinatedTurn(vars, v, f);
        success = true;
    }
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
    //fryyiend void find_CostFunctionFullTurn(long vars, Vector<double> &v, double & f,
    void find_CostFunctionFullTurn(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionFullTurn(vars, v, f);
        success = true;
    }
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
    //friend void find_CostFunctionPullUp(long vars, Vector<double> &v, double & f,
    void find_CostFunctionPullUp(long vars, Vector<double> &v, double & f,
                    bool & success, void* t_ptr)
    {
        (*(Objective*)t_ptr).CostFunctionPullUp(vars, v, f);
        success = true;
    }
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//public:

Objective::Objective(FGFDMExec* fdmex, FGTrimAnalysis* ta, double x) : _x(x), TrimAnalysis(ta), FDMExec(fdmex)
{
    // populate the map:type-of-trim/function-pointer
    mpCostFunctions[taFull           ] = find_CostFunctionFull;
    mpCostFunctions[taFullWingsLevel ] = find_CostFunctionFullWingsLevel;
    mpCostFunctions[taLongitudinal   ] = find_CostFunctionLongitudinal;
    mpCostFunctions[taTurn           ] = find_CostFunctionFullCoordinatedTurn;
    mpCostFunctions[taPullup         ] = find_CostFunctionPullUp;
}

void Objective::CostFunctionFull(long vars, Vector<double> &v, double & f)
{
  if (vars != 7) {
      cerr << "\nError: (Cost function for taFull mode) Dimension must be 7 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taFull){
      cerr << "\nError: must be taFull mode !!\n";
      exit(1);
  }

  f = myCostFunctionFull(v);
  return;
}

void Objective::CostFunctionFullWingsLevel(long vars, Vector<double> &v, double & f)
{
  if (vars != 6) {
      cerr << "\nError: (Cost function for taFullWingsLevel mode) Dimension must be 6 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taFullWingsLevel){
      cerr << "\nError: must be taFull mode !!\n";
      exit(1);
  }

  f = myCostFunctionFullWingsLevel(v);
  return;
}

void Objective::CostFunctionLongitudinal(long vars, Vector<double> &v, double & f)
{
  if (vars != 3) {
      cerr << "\nError: (Cost function for taLongitudinal mode) Dimension must be 3 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taLongitudinal){
      cerr << "\nError: trim mode must be taLongitudinal mode !!\n";
      exit(1);
  }

  f = myCostFunctionLongitudinal(v);
  return;
}

void Objective::CostFunctionFullCoordinatedTurn(long vars, Vector<double> &v, double & f)
{
  if (vars != 5) {
      cerr << "\nError: (Cost function for taTurn mode) Dimension must be 5 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taTurn){
      cerr << "\nError: trim mode must be taTurn mode !!\n";
      exit(1);
  }

  f = myCostFunctionFullCoordinatedTurn(v);
  return;
}

void Objective::CostFunctionFullTurn(long vars, Vector<double> &v, double & f)
{
  if (vars != 6) {
      cerr << "\nError: (Cost function for taTurn mode) Dimension must be 6 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taTurnFull){
      cerr << "\nError: trim mode must be taTurnFull ("<< (int)taTurnFull << ") mode !!\n";
      exit(1);
  }

  f = myCostFunctionFullTurn(v);
  return;
}

void Objective::CostFunctionPullUp(long vars, Vector<double> &v, double & f)
{
  if (vars != 5) {
      cerr << "\nError: (Cost function for taPullup mode) Dimension must be 5 !!\n";
      exit(1);
  }
  if (TrimAnalysis->GetMode()!=taPullup){
      cerr << "\nError: trim mode must be taPullup mode !!\n";
      exit(1);
  }

  f = myCostFunctionPullUp(v);
  return;
}

void Objective::Set_x_val(double new_x)
{
    _x = new_x;
}

//private:
// see the end of the file

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrimAnalysis::FGTrimAnalysis(FGFDMExec *FDMExec,TrimAnalysisMode tt)
{
  SetDebug(2);

  N=0;

  trim_failed = true;

  max_iterations=2500;
  stop_criterion="Stop-On-Delta";

  Debug=0;DebugLevel=0;

  fdmex=FDMExec;

  fgic=fdmex->GetIC();
  total_its=0;
  trimudot=true;
  gamma_fallback=true;
  ctrl_count=0;
  mode=tt;
  _targetNlf=1.0;
  _targetNlf=fgic->GetTargetNlfIC();

  _vtIC  = fgic->GetVtrueFpsIC();
  _hIC   = fgic->GetAltitudeFtIC();
  _gamma = fgic->GetFlightPathAngleRadIC();
  _rocIC = _vtIC*cos(_gamma);
  _vdownIC = _rocIC;

  // state variables
  _u       = fgic->GetUBodyFpsIC();
  _v       = fgic->GetVBodyFpsIC();
  _w       = fgic->GetWBodyFpsIC();
  _p       = fgic->GetPRadpsIC();
  _q       = fgic->GetQRadpsIC();
  _r       = fgic->GetRRadpsIC();
  _alpha   = fgic->GetAlphaRadIC();
  _beta    = fgic->GetBetaRadIC();
  _theta   = fgic->GetThetaRadIC();
  _phi     = fgic->GetPhiRadIC();
  _psiIC   = fgic->GetPsiRadIC();
  _psi     = _psiIC;
  _psigtIC = _psi;

  _phiW = _psiW = 0.0;

  _vgIC = _vtIC*cos(_gamma);
  _vnorthIC = _vgIC * cos(_psigtIC);
  _veastIC  = _vgIC * sin(_psigtIC);
  wnorthIC = _weastIC = _wdownIC = 0.;

  _udot=_vdot=_wdot=_pdot=_qdot=_rdot=0.;
  _psidot=_thetadot=0.;
  _psiWdot = _phiWdot = _gammadot = 0.;

  C1 = C2 = C3 = 1.0;
  _cbeta = cos(_beta);
  _sbeta = sin(_beta);
  _sphi = sin(_phi);

  SetMode(tt); // creates vTrimAnalysisControls
  fdmex->SetTrimMode( (int)tt );

  trim_id = "default-trim";

  // direct search stuff
  search_type = "Nelder-Mead";
  sigma_nm = 0.5; alpha_nm = 1.0; beta_nm = 0.5; gamma_nm = 2.0;
  initial_step = 0.01;
  tolerance = 1.0E-10; // 0.0000000001
  cost_function_value = 9999.0;

  rf_name = "";
  if (rf.is_open()) rf.close();

  Auxiliary    = fdmex->GetAuxiliary();
  Aerodynamics = fdmex->GetAerodynamics();
  Propulsion   = fdmex->GetPropulsion();
  FCS          = fdmex->GetFCS();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrimAnalysis::~FGTrimAnalysis(void) {

  ClearControls();

  vAlphaDeg.clear();
  vCL.clear(); vCD.clear(); vCm.clear();
  vThrottleCmd.clear(); vElevatorCmd.clear();
  vVn.clear(); vTn.clear();

  if (rf.is_open()) rf.close();

  fdmex->SetTrimStatus(false);
  fdmex->SetTrimMode( 99 );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::SetState(double u0, double v0, double w0, double p0, double q0, double r0,
                              double alpha0, double beta0, double phi0, double theta0, double psi0, double gamma0)
{
  _u=u0; _v=v0; _w=w0;
  _p=p0; _q=q0; _r=r0;
  _alpha=alpha0; _beta=beta0; _gamma=gamma0;
  _theta=theta0; _phi=phi0; _psi=psi0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::SetEulerAngles(double phi, double theta, double psi)
{
    // feed into private variables
    _phi = phi; _cphi = cos(_phi); _sphi = sin(_phi);
    _theta = theta; _ctheta = cos(_theta); _stheta = sin(_theta);
    _psi = psi; _cpsi = cos(_psi); _spsi = sin(_psi);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::SetDottedValues(double udot, double vdot, double wdot, double pdot, double qdot, double rdot)
{
    _udot=udot; _vdot=vdot; _wdot=wdot; _pdot=pdot; _qdot=qdot; _rdot=rdot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::Load(string fname, bool useStoredPath)
{
    string name="", type="";
    string trimDef;
    Element *element=0, *trimCfg=0, *search_element=0, *output_element=0;

    string sep = "/";
# ifdef macintosh
    sep = ";";
# endif

    if( useStoredPath ) {
        trimDef = fdmex->GetFullAircraftPath() + sep + fname + ".xml";
    } else {
        trimDef = fname;
    }

    document = this->LoadXMLDocument(trimDef);

    trimCfg = document->FindElement("trim_config");
    if (!trimCfg) {
        cerr << "File: " << trimDef << " does not contain a trim configuration tag" << endl;
        return false;
    }

    name = trimCfg->GetAttributeValue("name");
    trim_id = name;

    // First, find "search" element that specifies the type of cost function minimum search

    search_element = trimCfg->FindElement("search");
    if (!search_element) {
        cerr << "Using the Nelder-Mead search algorithm (default)." << endl;
    } else {
        type = search_element->GetAttributeValue("type");
        if (type.size() > 0) search_type = type; // if search type is not set, default is already Nelder-Mead
        if (search_type == "Nelder-Mead") {
            // Read settings from search
            // Note: all of these have defaults set above
            if ( search_element->FindElement("sigma_nm") )
                sigma_nm = search_element->FindElementValueAsNumber("sigma_nm");
            if ( search_element->FindElement("alpha_nm") )
                alpha_nm = search_element->FindElementValueAsNumber("alpha_nm");
            if ( search_element->FindElement("beta_nm") )
                beta_nm = search_element->FindElementValueAsNumber("beta_nm");
            if ( search_element->FindElement("gamma_nm") )
                gamma_nm = search_element->FindElementValueAsNumber("gamma_nm");
        }
        // ToDo: manage all the other possible choices here
        // if (search_type == "Sequential-Multiple-Nelder-Mead") { }
        // if (search_type == "Multicompass") { }
        // etc ...
        if ( search_element->FindElement("tolerance") ) {
            tolerance = search_element->FindElement("tolerance")->GetAttributeValueAsNumber("value");
        }
        if ( search_element->FindElement("max_iterations") ) {
            max_iterations = (unsigned int)search_element->FindElement("max_iterations")->GetAttributeValueAsNumber("value");
        }
        if ( search_element->FindElement("stop_criterion") ) {
            stop_criterion = search_element->FindElement("stop_criterion")->GetAttributeValue("type");
        }
    }

    // Initialize trim controls based on what is in the trim config file. This
    // includes initial trim values (or defaults from the IC file) and step
    // size values.

    element = trimCfg->FindElement("phi");
    InitializeTrimControl(fgic->GetPhiRadIC(), element, "RAD", JSBSim::taPhi);
    if ( ( fabs(_phi) < 89.5*(FGJSBBase::degtorad ) ) && ( mode == taTurn ))
                _targetNlf = 1./cos(_phi);

    element = trimCfg->FindElement("theta");
    InitializeTrimControl(fgic->GetThetaRadIC(), element, "RAD", JSBSim::taTheta);
    
    element = trimCfg->FindElement("psi");
    InitializeTrimControl(fgic->GetPsiRadIC(), element, "RAD", JSBSim::taHeading);

    element = trimCfg->FindElement("gamma");
    _gamma = fgic->GetFlightPathAngleRadIC();
    if (element)
      if (element->GetNumDataLines() > 0) _gamma = element->GetDataAsNumber();

    element = trimCfg->FindElement("nlf");
    if (element) {
      if (element->GetNumDataLines() > 0) _targetNlf = element->GetDataAsNumber();
      CalculatePhiWFromTargetNlfTurn(_targetNlf);
    }

    element = trimCfg->FindElement("throttle_cmd");
    if (element) InitializeTrimControl(0, element, "", JSBSim::taThrottle);

    element = trimCfg->FindElement("elevator_cmd");
    if (element) InitializeTrimControl(0, element, "", JSBSim::taElevator);

    element = trimCfg->FindElement("rudder_cmd");
    if (element) InitializeTrimControl(0, element, "", JSBSim::taRudder);

    element = trimCfg->FindElement("aileron_cmd");
    if (element) InitializeTrimControl(0, element, "", JSBSim::taAileron);

    output_element = trimCfg->FindElement("output_file");
    if (output_element) {
        rf_name = output_element->GetAttributeValue("name");
        if (rf_name.empty()) {
            cerr << "name must be specified in output_file \"name\" attribute."<< endl;
        } else {
            if ( !SetResultsFile(rf_name) )
                cerr << "Unable to use output file "<< rf_name << endl;
        }
    }
    return true;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::InitializeTrimControl(double default_value, Element* el,
                                           string unit, TaControl type)
{
  Element *step_size_element=0, *trim_config=0;
  double iv = 0.0;
  double step = 0.0; // default step value
  bool set_override = false;

  iv = default_value;

  if (el != 0) {
    string name = el->GetName();
    trim_config = el->GetParent();
    set_override = el->GetNumDataLines() != 0;
    if (set_override) {
      if (unit.empty()) iv = trim_config->FindElementValueAsNumber(name);
      else              iv = trim_config->FindElementValueAsNumberConvertTo(name, unit);
    }
    if (el->GetAttributeValueAsNumber("step_size") != HUGE_VAL)
      step = el->GetAttributeValueAsNumber("step_size");
  }

  for (unsigned int i=0; i<vTrimAnalysisControls.size(); i++) {
    if (vTrimAnalysisControls[i]->GetControlType() == type) {
      vTrimAnalysisControls[i]->SetControlInitialValue(iv);
      vTrimAnalysisControls[i]->SetControlStep(step);
      break;
    }
  }

  return set_override;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::TrimStats() {
  int run_sum=0;
  cout << endl << "  Trim Statistics: " << endl;
  cout << "    Total Iterations: " << total_its << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::Report(void) {

    cout << "---------------------------------------------------------------------\n";

    cout << "Trim report: " << endl;
      cout << "\tTrim algorithm terminated with the following values:" << endl;
      cout << "\tu, v, w        (ft/s): " << _u <<", "<< _v <<", "<< _w << endl
           << "\tp, q, r       (rad/s): " << _p <<", "<< _q <<", "<< _r << endl
           << "\talpha, beta     (deg): " << _alpha*57.3 <<", "<< _beta*57.3 << endl
           << "\tphi, theta, psi (deg): " << _phi*57.3 <<", "<< _theta*57.3 << ", " << _psi*57.3 << endl
           << "\tCost function value  : " << cost_function_value << endl
           << "\tCycles executed      : " << total_its << endl << endl;

      cout << "\tTrim variables adjusted:" << endl;
      for (unsigned int i=0; i<vTrimAnalysisControls.size();i++){
          cout << "\t\t" << vTrimAnalysisControls[i]->GetControlName() <<": ";
          cout << vTrimAnalysisControls[i]->GetControl() << endl;
      }
      //...

      cout << endl;

      cout << "\t** Initial -> Final Conditions **" << endl;
      cout << "\tAlpha IC: " << fgic->GetAlphaDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << Auxiliary->Getalpha()*57.3 << " Degrees" << endl;
      cout << "\tBeta  IC: " << fgic->GetBetaDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << Auxiliary->Getbeta()*57.3 << " Degrees" << endl;
      cout << "\tGamma IC: " << fgic->GetFlightPathAngleDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << Auxiliary->GetGamma()*57.3 << " Degrees" << endl;
      cout << "\tPhi IC  : " << fgic->GetPhiDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << fdmex->GetPropagate()->GetEuler(1)*57.3 << " Degrees" << endl;
      cout << "\tTheta IC: " << fgic->GetThetaDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << fdmex->GetPropagate()->GetEuler(2)*57.3 << " Degrees" << endl;
      cout << "\tPsi IC  : " << fgic->GetPsiDegIC() << " Degrees" << endl;
      cout << "\t   Final: " << fdmex->GetPropagate()->GetEuler(3)*57.3 << " Degrees" << endl;
      cout << endl;
      cout << "--------------------------------------------------------------------- \n\n";

      fdmex->EnableOutput();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::ClearControls(void) {

    FGTrimAnalysisControl* tac;

    mode=taCustom;
    vector<FGTrimAnalysisControl*>::iterator iControls;
    iControls = vTrimAnalysisControls.begin();
    while (iControls != vTrimAnalysisControls.end()) {
      tac=*iControls;
      delete tac;

      iControls++;
    }

    vTrimAnalysisControls.clear();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::AddControl( TaControl control ) {

  FGTrimAnalysisControl* tac;
  bool result=true;

  mode = taCustom;
  vector <FGTrimAnalysisControl*>::iterator iControls = vTrimAnalysisControls.begin();
  while (iControls != vTrimAnalysisControls.end()) {
      tac=*iControls;
      if( tac->GetControlType() == control )
        result=false;
      iControls++;
  }
  if(result) {
    vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,control));
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::RemoveControl( TaControl control ) {

  FGTrimAnalysisControl* tac;
  bool result=false;

  mode = taCustom;
  vector <FGTrimAnalysisControl*>::iterator iControls = vTrimAnalysisControls.begin();
  while (iControls != vTrimAnalysisControls.end()) {
      tac=*iControls;
      if( tac->GetControlType() == control ) {
        delete tac;
        vTrimAnalysisControls.erase(iControls);
        result=true;
        continue;
      }
      iControls++;
  }

  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::EditState( TaControl new_control, double new_initvalue, double new_step, double new_min, double new_max ) {
  FGTrimAnalysisControl* tac;
  bool result=false;

  mode = taCustom;
  vector <FGTrimAnalysisControl*>::iterator iControls = vTrimAnalysisControls.begin();
  while (iControls != vTrimAnalysisControls.end()) {
      tac=*iControls;
      if( tac->GetControlType() == new_control ) {
        vTrimAnalysisControls.insert(iControls,1,new FGTrimAnalysisControl(fdmex,fgic,new_control));
        delete tac;
        vTrimAnalysisControls.erase(iControls+1);
        result=true;
        break;
      }
      iControls++;
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::setupPullup() {
  double g,q,cgamma;
  g=fdmex->GetInertial()->gravity();
  cgamma=cos(fgic->GetFlightPathAngleRadIC());
  q=g*(_targetNlf-cgamma)/fgic->GetVtrueFpsIC();
  cout << _targetNlf << ", " << q << endl;
  fgic->SetQRadpsIC(q);
  updateRates();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::CalculatePhiWFromTargetNlfTurn(double nlf){
  if ( ( mode == taTurn ) || ( mode == taTurnFull ) ) {
     // target Nlf is given
     // set _phiW according to given Nlf
     _targetNlf = nlf;
     _phiW = atan2( sqrt(_targetNlf*_targetNlf-cos(_gamma)*cos(_gamma)), cos(_gamma) );
  }
}

//ToDo: check formulas for nonzero gamma
void FGTrimAnalysis::setupTurn(void){

  if ( mode == taTurn ) {
     // target Nlf is given
     // set _phiW according to given Nlf
     _phiW = atan2( sqrt(_targetNlf*_targetNlf-cos(_gamma)*cos(_gamma)), cos(_gamma) );
     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );
     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);

     // theta_underlined (see paper on Trim) for coordinated turn
     _theta = atan2( sin(_psi)*cos(_gamma)+cos(_psi)*sin(_gamma), cos(_gamma) );

      double g,V;
      V = sqrt( _u*_u +_v*_v + _w*_w );
      g=fdmex->GetInertial()->gravity();
      _psiWdot = (g/V)*tan(_phiW);
      updateRates();
  }
  if ( mode == taTurnFull ) {
     // target Nlf is given
     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );
     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);

      double g,V;
      V = sqrt( _u*_u +_v*_v + _w*_w );
      g=fdmex->GetInertial()->gravity();
      _psiWdot = (g/V)*tan(_phiW);
      updateRates();
  }
}

void FGTrimAnalysis::setupTurn(double phiW){
  if ( mode == taTurn ) {
     _phiW = phiW;
     // recalculate target Nlf
     _targetNlf =  sqrt( cos(_gamma)*cos(_gamma)*tan(_phiW)*tan(_phiW) + cos(_gamma)*cos(_gamma) );

     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );
     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);

     _cphi = cos(_phi);

     // theta_underlined (see paper on Trim) for coordinated turn
     //_theta = atan2( sin(_psi)*cos(_gamma)+cos(_psi)*sin(_gamma), cos(_gamma) );

      double g,V;
      V = sqrt( _u*_u +_v*_v + _w*_w );
      g=fdmex->GetInertial()->gravity();
      _psiWdot = (g/V)*tan(_phiW);
      updateRates();
  }
  if ( mode == taTurnFull ) {
     _phiW = phiW;
     // recalculate target Nlf
     _targetNlf =  sqrt( cos(_gamma)*cos(_gamma)*tan(_phiW)*tan(_phiW) + cos(_gamma)*cos(_gamma) );

     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );
     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);

     _cphi = cos(_phi);

     // theta_underlined (see paper on Trim) for coordinated turn
     //_theta = atan2( sin(_psi)*cos(_gamma)+cos(_psi)*sin(_gamma), cos(_gamma) );

      double g,V;
      V = sqrt( _u*_u +_v*_v + _w*_w );
      g=fdmex->GetInertial()->gravity();
      _psiWdot = (g/V)*tan(_phiW);
      updateRates();
  }

}

//ToDo: check formulas for nonzero gamma
void FGTrimAnalysis::setupTurnPhi(double psi, double theta){
  if ( mode == taTurn ) {
     _psi   = psi;   _cpsi   = cos(_psi);   _spsi   = sin(_psi);
     _theta = theta; _ctheta = cos(_theta); _stheta = sin(_theta);

     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );

     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);
     _cphi = cos(_phi);
  }
  if ( mode == taTurnFull ) {
     _psi   = psi;   _cpsi   = cos(_psi);   _spsi   = sin(_psi);
     _theta = theta; _ctheta = cos(_theta); _stheta = sin(_theta);

     C1 = cos(_phiW)*sin(_gamma)*cos(_theta)*sin(_psi)+
                   sin(_phiW)            *cos(_theta)*cos(_psi)    ;
     C2 = cos(_phiW)*cos(_gamma)*cos(_theta)*cos(_psi)+
                  cos(_phiW)*sin(_gamma)*sin(_theta)              ;
     C3 = sin(_phiW)            *sin(_theta)+
                  cos(_phiW)*cos(_gamma)*cos(_theta)*sin(_psi)    ;
     _cbeta = ( C1*sin(_phiW)*cos(_gamma)+C2*cos(_phiW) + C3*sin(_phiW)*sin(_gamma) )
                    /( sqrt(C1*C1 + C2*C2 + C3*C3) );
     _sbeta = sqrt(1.-_cbeta*_cbeta );

     _sphi = ( _cbeta*sin(_phiW) * cos(_gamma) - _sbeta*sin(_gamma) )/cos(_theta);
     _phi = asin(_sphi);
     _cphi = cos(_phi);

  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGTrimAnalysis::UpdateRatesTurn(double psi, double theta, double phi, double phiW){
      _psi = psi;
      setupTurn(phiW); // calls updateRates: updates _p,_q,_r
      return FGColumnVector3(_p,_q,_r);
}

FGColumnVector3 FGTrimAnalysis::UpdateRatesPullup(void){
      double g,cgamma;
      g=fdmex->GetInertial()->gravity();
      cgamma=cos(fgic->GetFlightPathAngleRadIC());
      _p = 0;
      _q = g*(_targetNlf-cgamma)/fgic->GetVtrueFpsIC();
      _r = 0.;

      fgic->SetQRadpsIC(_q);
      return FGColumnVector3(_p,_q,_r);
}

void FGTrimAnalysis::updateRates(void){
  if ( ( mode == taTurn ) || ( mode == taTurnFull ) ){
      // this is the result of a Matlab symbolic expression
      double cth2 = cos(_theta)*cos(_theta),
              sth2 = sin(_theta)*sin(_theta),
              scth = sin(_theta)*cos(_theta),
              cph2 = cos(_phiW)*cos(_phiW),
              sph2 = sin(_phiW)*sin(_phiW),
              scph = sin(_phiW)*cos(_phiW),
              cga2 = cos(_gamma)*cos(_gamma),
              scga = sin(_gamma)*cos(_gamma),
              cps2 = cos(_psi)*cos(_psi),
              scps = sin(_psi)*cos(_psi);
     _calpha =
        sqrt(
            1-cth2
            +cph2*cth2
            -2*scph*scth*cos(_gamma)*sin(_psi)+cph2*cga2*cth2
            +cph2*cga2*cth2*(1-cps2)
            +2*cph2*scga*scth*cos(_psi)
            -cga2*cph2
            -2*cph2*cth2*cps2
            +2*scph*sin(_gamma)*cth2*scps
            +cps2*cth2 ),
            _salpha = sqrt( 1. - _calpha*_calpha );

     _p = -_psiWdot*
          ( sin(_gamma)*_calpha*_cbeta
           +cos(_gamma)*sin(_phiW)*_calpha*_sbeta
           +cos(_gamma)*cos(_phiW)*_salpha
           );

     _q = -_psiWdot*
          ( sin(_gamma)*_sbeta
           -cos(_gamma)*sin(_phiW)*_cbeta
           );

     _r = -_psiWdot*
          ( sin(_gamma)*_salpha*_cbeta
           +cos(_gamma)*sin(_phiW)*_salpha*_sbeta
           -cos(_gamma)*cos(_phiW)*_calpha
           );

  } else if( mode == taPullup && fabs(_targetNlf-1) > 0.01) {

      double g,q,cgamma;
      g=fdmex->GetInertial()->gravity();
      cgamma=cos(fgic->GetFlightPathAngleRadIC());
      q=g*(_targetNlf-cgamma)/fgic->GetVtrueFpsIC();
      _q=q;
      fgic->SetQRadpsIC(q);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::setDebug(void) {
    Debug=0;
    return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrimAnalysis::SetMode(TrimAnalysisMode tt) {

    ClearControls();

    cout << "---------------------------------------------------------------------" << endl;
    cout << "Trim analysis performed: ";
    mode=tt;
    switch(tt) {
      case taLongitudinal:
        if (debug_lvl > 0)
          cout << "  Longitudinal Trim" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        break;
      case taFull:
        if (debug_lvl > 0)
          cout << "  Full Trim" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAileron ));  // TODO: taRollTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taRudder ));   // TODO: taYawTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taPhi ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taHeading ));
        break;
      case taFullWingsLevel:
        if (debug_lvl > 0)
          cout << "  Full Trim, Wings-Level" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAileron ));  // TODO: taRollTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taRudder ));   // TODO: taYawTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taHeading ));
        break;
      case taTurn:
          // ToDo: set target NLF here !!!
          // ToDo: assign psiDot here !!
        if (debug_lvl > 0)
          cout << "  Full Trim, Coordinated turn" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAileron ));  // TODO: taRollTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taRudder ));   // TODO: taYawTrim
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taPhi ));
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taHeading ));
        break;
      case taTurnFull:
        if (debug_lvl > 0)
          cout << "  Non-coordinated Turn Trim" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAileron ));  // TODO: taRollTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taRudder ));   // TODO: taYawTrim
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taPhi ));
        // calculate this from target nlf
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taHeading ));
        break;
      case taPullup:
          // ToDo: set target NLF here !!!
          // ToDo: assign qDot here !!
        if (debug_lvl > 0)
          cout << "  Full Trim, Pullup" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taThrottle ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taElevator )); // TODO: taPitchTrim
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAileron ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taRudder ));
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taPhi ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taHeading ));
        break;
      case taGround:
        if (debug_lvl > 0)
          cout << "  Ground Trim" << endl;
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taAltAGL ));
        vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taTheta ));
        //vTrimAnalysisControls.push_back(new FGTrimAnalysisControl(fdmex,fgic,taPhi ));
        break;
      case taCustom:
        // agodemar...
        // ...agodemar
      case taNone:
        break;
    }

    current_ctrl=0;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::SetResultsFile(string name)
{
    if ( rf.is_open() ) return false;

    rf_name = name;
    rf.open(rf_name.c_str(), ios::out);
    if ( !rf.is_open() ) {
       cerr << "Unable to open " << rf_name << endl;
       return false;
    }
    //rf << "# ... complete this " << endl;
    //rf << "# iteration, CostFunc, size, dT, dE, dA, dR, Psi (rad), Theta (rad), Phi (rad)\n";
    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::ensureRunning()
{
    bool success = false;

    if ( Propulsion == 0L ) return false;

    for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
    {
      if (!Propulsion->GetEngine(i)->GetRunning() ) {
          Propulsion->GetEngine(i)->SetStarter( true );
          if ( Propulsion->GetEngine(i)->GetType() == JSBSim::FGEngine::etPiston )
          {
              FGPiston * Piston = (FGPiston*)Propulsion->GetEngine(i);
              Piston->SetMagnetos(3);
          }
          else if ( Propulsion->GetEngine(i)->GetType() == FGEngine::etTurbine )
          {
              FGTurbine * Turbine = (FGTurbine*)Propulsion->GetEngine(i);
              Turbine->SetCutoff(false);
              Turbine->SetStarter(true);
              Turbine->Calculate();
          }
          Propulsion->GetEngine(i)->SetRunning( true );
          Propulsion->Run();
      } else {
          success = true; // at least one engine is found in a running state
          Propulsion->SetActiveEngine(i);
      }
    }
    return success;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::ensureRunning(unsigned int i)
{
    bool success = false;

    if ( Propulsion == 0L ) return false;

    if ( i < Propulsion->GetNumEngines() )
    {
      if (!Propulsion->GetEngine(i)->GetRunning() ) {
          Propulsion->GetEngine(i)->SetStarter( true );
          if ( Propulsion->GetEngine(i)->GetType() == JSBSim::FGEngine::etPiston )
          {
              FGPiston * Piston = (FGPiston*)Propulsion->GetEngine(i);
              Piston->SetMagnetos(3);
          }
          else if ( Propulsion->GetEngine(i)->GetType() == FGEngine::etTurbine )
          {
              FGTurbine * Turbine = (FGTurbine*)Propulsion->GetEngine(i);
              Turbine->SetCutoff(false);
              Turbine->SetStarter(true);
              Turbine->Calculate();
          }
          Propulsion->GetEngine(i)->SetRunning( true );
          Propulsion->Run();

      } else {
          success = true; // at least one engine is found in a running state
          Propulsion->SetActiveEngine(i);
      }
    }
    else success=false; // i not in range

    return success;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::runForAWhile(int nruns)
{
  bool result = fdmex->Run();  // MAKE AN INITIAL RUN

  int counter = 0;

  // *** CYCLIC EXECUTION LOOP, AND MESSAGE READING *** //
  while ( counter < nruns ) { // (result && (counter < nruns))
    counter++;
    result = fdmex->Run();
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool FGTrimAnalysis::populateVecAlphaDeg(double vmin, double vmax, int n)
{
    if ( !vAlphaDeg.empty() ) return false;
    for(int i=0; i<n; i++)
          vAlphaDeg.push_back( vmin + double(i)*(vmax - vmin)/double(n-1) );
    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool FGTrimAnalysis::populateVecThrottleCmd(double vmin, double vmax, int n)
{
    if ( !vThrottleCmd.empty() ) return false;
    for(int i=0; i<n; i++)
          vThrottleCmd.push_back( vmin + double(i)*(vmax - vmin)/double(n-1) );
    return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool FGTrimAnalysis::populateVecElevatorCmd(double vmin, double vmax, int n)
{
    if ( !vElevatorCmd.empty() ) return false;
    for(int i=0; i<n; i++)
          vElevatorCmd.push_back( vmin + double(i)*(vmax - vmin)/double(n-1) );
    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool FGTrimAnalysis::calculateAerodynamics(
                           double dE_cmd,
                           double Vt,
                           double alpha_deg,
                           double altitude,
                           double rho,
                           double S, double mac, double bw,
                           double& CL, double& CD, double& Cm)
{
    double qBar   = 0.5 * rho * Vt*Vt;
    double qBar_S = qBar * S;

    Auxiliary->SetVt( Vt );
    Auxiliary->Setalpha( alpha_deg*degtorad ); // ! note these are DEC converted to RAD!!

    Auxiliary->Setbeta( 0.0 );

    // ensure zero rates
    Auxiliary->Setadot( 0.0 );
    Auxiliary->Setbdot( 0.0 );
    Auxiliary->SetAeroPQR( FGColumnVector3(0.,0.,0.) ) ; // note assumes that trim_mode is triggered
    // note: do not Auxiliary->Run(), otherwise dotted values
    //       _and_ aerodynamic angles are recalculated !!!

    // ensure the desired dE before aerodynamics
    FCS->SetDeCmd( dE_cmd );
    FCS->Run();

    Aerodynamics->Run();

    // get the Aerodynamics internat data
    vector <FGFunction*> * Coeff = Aerodynamics->GetAeroFunctions();

    if ( Coeff->empty() ) return false;

    CL = 0.;
    unsigned int axis_ctr = 2; // means LIFT, 0=DRAG, 1=SIDE, 3,4,5=ROLL,PITCH,YAW
    for (unsigned int ctr=0; ctr < Coeff[axis_ctr].size(); ctr++)
      CL += Coeff[axis_ctr][ctr]->GetValue();
    CL /= qBar_S;

    CD = 0.;
    axis_ctr = 0; // means DRAG
    for (unsigned int ctr=0; ctr < Coeff[axis_ctr].size(); ctr++)
      CD += Coeff[axis_ctr][ctr]->GetValue();
    CD /= qBar_S;

    Cm=0.;
    axis_ctr = 4; // means PITCH 0=DRAG, 1=SIDE, 2=LIFT, 3=ROLL, 4=PITCH, 5=YAW
    for (unsigned int ctr=0; ctr < Coeff[axis_ctr].size(); ctr++)
      Cm += Coeff[axis_ctr][ctr]->GetValue();

    Cm /= ( qBar_S*mac );

    return true;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrimAnalysis::DoTrim(void) {

    // retrieve initial conditions
    fdmex->RunIC();

    cout << endl << "Numerical trim algorithm: constrained optimization of a cost function" << endl;

    Objective* obj_ptr = new Objective(this->fdmex, this, 999.0);

    fdmex->SetTrimStatus( true );

    //###################################
    // run for a while
    //###################################

    double tMin,tMax;
    double throttle = 1.0;
    for (unsigned i=0;i<Propulsion->GetNumEngines();i++)
    {
        tMin=Propulsion->GetEngine(i)->GetThrottleMin();
        tMax=Propulsion->GetEngine(i)->GetThrottleMax();
        FCS->SetThrottleCmd(i,tMin + throttle *(tMax-tMin));
        if (Propulsion->GetEngine(i)->GetType()==FGEngine::etPiston)
        {
            FCS->SetMixtureCmd(i,0.87);
        }
        if (Propulsion->GetEngine(i)->GetType()==FGEngine::etTurbine)
        {
            ((FGTurbine*)Propulsion->GetEngine(i))->SetCutoff(false);
            ((FGTurbine*)Propulsion->GetEngine(i))->SetPhase(FGTurbine::tpRun); // tpStart
        }

        FCS->Run(); // apply throttle change
    }
    Propulsion->GetSteadyState(); // GetSteadyState processes all engines

    //--------------------------------------------------------

    for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
    {
      int engineStartCount = 0;
      bool engine_started = false;

      int n_attempts = 100;
      if (Propulsion->GetEngine(i)->GetType()==FGEngine::etTurbine)
          n_attempts = 5000; // a turbine engine, e.g. 737's cfm56, takes a little longer to get running

      while ( !engine_started && (engineStartCount < n_attempts) )
      {
          engine_started = ensureRunning(i);
          fdmex->Run();
          engineStartCount++;
      }

    }

    //--------------------------------------------------------

    Propulsion->GetSteadyState();

    fdmex->SetDebugLevel(0); // 4

    //#########################################################################

    trim_failed=false;

    for(int i=0;i < fdmex->GetGroundReactions()->GetNumGearUnits();i++){
        fdmex->GetGroundReactions()->GetGearUnit(i)->SetReport(false);
    }

    fdmex->DisableOutput();

    fgic->SetPRadpsIC(0.0);
    fgic->SetQRadpsIC(0.0);
    fgic->SetRRadpsIC(0.0);

    if(mode == taPullup ) {
        setupPullup(); // also calls updateRates()
    } else if (mode == taTurn) {
        setupTurn(); // also calls updateRates()
    } else if (mode == taTurnFull) {
        setupTurn(); // also calls updateRates()
    } else {
        fgic->SetPRadpsIC(0.0);
        fgic->SetQRadpsIC(0.0);
        fgic->SetRRadpsIC(0.0);
        _p = _q = _r = 0.;
    }

  // ** DO HERE THE TRIM ** //

  //---------------------------------------------------------------------------
  // REMINDER:
  // n. of control variables for full trim (taFull): 7
  // ordering: the four commands first, then the three Euler angles,
  // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
  // [4] phi, [5] theta, [6] psi (alias taHeading)
  //-----------------------------------------------------------------------------------------------------------------

  // re-run ICs
  fdmex->RunIC();

  // write trim results on file, rf=results file
  if ( rf.is_open() )
    rf <<
      "# iteration, costf, dT, dE, dA, dR, Phi (rad), Theta (rad), Psi (rad), uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2), u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad), alphaDot (rad/s), betaDot (rad/s), Thrust"
      << endl;

  long n = vTrimAnalysisControls.size();  // number of variables (dimension of the search)

  /* we'll initialize an n-entry Vector whose value is startVal,
   * and use it as our starting point.
   */

  // a Vec, needed in the search constructor
  Vector<double>* minVec = new Vector<double>(n, 0.0);

  // a Vec needed to store the minimum point later
  Vector<double>* Sminimum;
  new_Vector_Init(Sminimum,*minVec);

  /*
   * now construct the search object:
   * NMS = Nelder-Mead Search algorithm;
   *       if n parameters have to be adjusted, then user has to provide
   *       n+1 initial combinations where the cost function is going to be sampled.
   *       The class has the capability of automatically initialize those points,
   *       but here I prefer giving them explicitly so that the initial steps in
   *       all parameter "directions" are well defined.
   */

  NMSearch NMS(
          n, *minVec, /* minVec, */
          sigma_nm, alpha_nm, beta_nm, gamma_nm,
          initial_step, tolerance,
          //find_CostFunctionFull, // gets the proper member function from the
                                   // object of class Objective (see main_obj.cc from DirectSearch examples)
          0L, // gets the proper member function from the
          (void *)obj_ptr
          );

  if ( GetMode()==taLongitudinal ) {
      NMS.SetFcnName(find_CostFunctionLongitudinal);
  }
  if ( GetMode()==taFull ) {
      NMS.SetFcnName(find_CostFunctionFull);
  }
  if ( GetMode()==taFullWingsLevel ) {
      NMS.SetFcnName(find_CostFunctionFullWingsLevel);
  }
  if ( GetMode()==taTurn ) {
      NMS.SetFcnName(find_CostFunctionFullCoordinatedTurn);
  }
  if ( GetMode()==taTurnFull ) {
      NMS.SetFcnName(find_CostFunctionFullTurn);
  }
  if ( GetMode()==taPullup ) {
      NMS.SetFcnName(find_CostFunctionPullUp);
  }

  //-----------------------------------------
  // initialize simplex (n+1 conditions)
  //-----------------------------------------
  // InitGeneralSimplex(plex) where...
  //    Matrix<double> *plex = NULL;
  //    new_Matrix(plex, n+1,n); // create one more row, see Dyn_alloc.h
  // ...

  //------------------------------------------------------------------------------
  // In this stringstream we I put the n+1 sequences, each of n values (n-ples)
  // The stream is then used to initialize the simplex by a call to the method
  // ReadInFile
  //------------------------------------------------------------------------------
  stringstream ss (stringstream::in | stringstream::out);

  // first feed the trial minimizer, zeroth point of the simplex
  for (unsigned int k=0; k<vTrimAnalysisControls.size();k++)
      ss << vTrimAnalysisControls[k]->GetControlInitialValue() << " ";

  // then the rest of n-ples
  for (unsigned int k=0; k<vTrimAnalysisControls.size();k++)
  {
    for(vector<FGTrimAnalysisControl*>::iterator vi=vTrimAnalysisControls.begin();
        vi!=vTrimAnalysisControls.end();vi++) {
            if ( (*vi)->GetControlType()==vTrimAnalysisControls[k]->GetControlType() )
                ss << (*vi)->GetControlInitialValue() + (*vi)->GetControlStep() << " ";
            else
                ss << (*vi)->GetControlInitialValue() << " ";
    }// this complets the k-th n-ple
  }// this completes the (N+1) n-ples

  NMS.ReadInFile(ss); // assign the initial combinations of parameters

  /*
  If Stop_on_std is set to false in an NMSearch, the standard-deviation test is used until that test is met.
  Then (and in subsequent iterations of the same search) the more expensive ``delta'' test will be used.
  We realize that there will be times when the standard deviation test will actually take more function calls than
  the ``delta'' test, but this is not typically the case, and we make this compromise for the sake of efficiency.
  */

  NMS.SetMaxCalls(max_iterations);
  //NMS.SetMaxCallsExact(20000);
  if ( stop_criterion=="Stop-On-Std" )
    NMS.Set_Stop_on_std();
  if ( stop_criterion=="Stop-On-Delta" )
    NMS.Set_Stop_on_delta();

  double SMinVal;
  long Scalls;

//  NMS.PrintDesign();

  fdmex->SetDebugLevel(0);

  /* start searching */
  NMS.BeginSearch();    // stops when the simplex shrinks to a point

  // WATCH this !!
  delete obj_ptr;

  /* recover information about the search */
  NMS.GetMinPoint(*Sminimum);
  NMS.GetMinVal(SMinVal);
  Scalls = NMS.GetFunctionCalls();

  // Apply the set of controls found by the minimization procedure

  if ( ( mode == taFull ) ||
       ( mode == taTurnFull ) )
  {
      FCS->SetDeCmd( (*Sminimum)[1] ); // elevator
      FCS->SetDaCmd( (*Sminimum)[2] ); // ailerons
      FCS->SetDrCmd( (*Sminimum)[3] ); // rudder
      double tMin,tMax;
      for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
      {
          tMin=Propulsion->GetEngine(i)->GetThrottleMin();
          tMax=Propulsion->GetEngine(i)->GetThrottleMax();
          FCS->SetThrottleCmd(i,tMin + (*Sminimum)[0] *(tMax-tMin));
          FCS->Run(); // apply throttle change
      }
      Propulsion->GetSteadyState(); // GetSteadyState processes all engines
      FCS->Run(); // apply throttle, yoke & pedal changes

      FGQuaternion quat( (*Sminimum)[4], (*Sminimum)[5], (*Sminimum)[6] ); // phi, theta, psi
      quat.Normalize();

      fgic->ResetIC(_u, _v, _w, _p, _q, _r, _alpha, _beta, _phi, _theta, _psi, _gamma);

      FGPropagate::VehicleState vstate = fdmex->GetPropagate()->GetVState();
      vstate.vQtrn = FGQuaternion(_phi,_theta,_psi);
      fdmex->GetPropagate()->SetVState(vstate);
      Auxiliary->Setalpha( _alpha ); // need to get Auxiliary updated
      Auxiliary->Setbeta ( _beta  );

      // NOTE: _do not_ fdmex->RunIC() here ! We just reset the state
      /*      */
      //fdmex->RunIC();
      fdmex->Run();

  }  // end taFull

  if ( mode == taLongitudinal )
  {
      FCS->SetDeCmd( (*Sminimum)[1] ); // elevator
      double tMin,tMax;
      for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
      {
          tMin=Propulsion->GetEngine(i)->GetThrottleMin();
          tMax=Propulsion->GetEngine(i)->GetThrottleMax();
          FCS->SetThrottleCmd(i,tMin + (*Sminimum)[0] *(tMax-tMin));
          FCS->Run(); // apply throttle change
      }
      Propulsion->GetSteadyState(); // GetSteadyState processes all engines
      FCS->Run(); // apply throttle, yoke & pedal changes

      FGQuaternion quat( 0, (*Sminimum)[2], fgic->GetPsiRadIC() ); // phi, theta, psi
      quat.Normalize();

      // enforce the current state to IC object

      fgic->ResetIC(_u, _v, _w, _p, _q, _r, _alpha, _beta, _phi, _theta, _psi, _gamma);

      // NOTE: _do not_ fdmex->RunIC() here ! We just reset the state
      //       which means that we populate IC private variables with our set of values
      //fdmex->RunIC();

      FGPropagate::VehicleState vstate = fdmex->GetPropagate()->GetVState();
      vstate.vQtrn = FGQuaternion(_phi,_theta,_psi);
      fdmex->GetPropagate()->SetVState(vstate);
      Auxiliary->Setalpha( _alpha ); // need to get Auxiliary updated
      Auxiliary->Setbeta ( _beta  );

      fdmex->Run();

  }  // end taLongitudinal

  if ( mode == taFullWingsLevel)
  {
      FCS->SetDeCmd( (*Sminimum)[1] ); // elevator
      FCS->SetDaCmd( (*Sminimum)[2] ); // ailerons
      FCS->SetDrCmd( (*Sminimum)[3] ); // rudder

      double tMin,tMax;
      for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
      {
          tMin=Propulsion->GetEngine(i)->GetThrottleMin();
          tMax=Propulsion->GetEngine(i)->GetThrottleMax();
          FCS->SetThrottleCmd(i,tMin + (*Sminimum)[0] *(tMax-tMin));
          FCS->Run(); // apply throttle change
      }
      Propulsion->GetSteadyState(); // GetSteadyState processes all engines
      FCS->Run(); // apply throttle, yoke & pedal changes

      FGQuaternion quat( 0, (*Sminimum)[2], fgic->GetPsiRadIC() ); // phi, theta, psi
      quat.Normalize();

      //...
      // enforce the current state to IC object

      fgic->ResetIC(_u, _v, _w, _p, _q, _r, _alpha, _beta, _phi, _theta, _psi, _gamma);

      // NOTE: _do not_ fdmex->RunIC() here ! We just reset the state
      //       which means that we populate IC private variables with our set of values
      //fdmex->RunIC();

      FGPropagate::VehicleState vstate = fdmex->GetPropagate()->GetVState();
      vstate.vQtrn = FGQuaternion(_phi,_theta,_psi);
      fdmex->GetPropagate()->SetVState(vstate);
      Auxiliary->Setalpha( _alpha ); // need to get Auxiliary updated
      Auxiliary->Setbeta ( _beta  );

      fdmex->Run();
  }// end taFullWingsLevel

  if ( mode == taTurn )
  {
      FCS->SetDeCmd( (*Sminimum)[1] ); // elevator
      FCS->SetDaCmd( (*Sminimum)[2] ); // ailerons
      FCS->SetDrCmd( (*Sminimum)[3] ); // rudder

      double tMin,tMax;
      for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
      {
          tMin=Propulsion->GetEngine(i)->GetThrottleMin();
          tMax=Propulsion->GetEngine(i)->GetThrottleMax();
          FCS->SetThrottleCmd(i,tMin + (*Sminimum)[0] *(tMax-tMin));
          FCS->Run(); // apply throttle change
      }
      Propulsion->GetSteadyState(); // GetSteadyState processes all engines
      FCS->Run(); // apply throttle, yoke & pedal changes

      FGQuaternion quat( fgic->GetPhiRadIC(), (*Sminimum)[2], fgic->GetPsiRadIC() ); // phi, theta, psi
      quat.Normalize();

      //...
      // enforce the current state to IC object

      fgic->ResetIC(_u, _v, _w, _p, _q, _r, _alpha, _beta, _phi, _theta, _psi, _gamma);

      // NOTE: _do not_ fdmex->RunIC() here ! We just reset the state
      //       which means that we populate IC private variables with our set of values
      //fdmex->RunIC();

      FGPropagate::VehicleState vstate = fdmex->GetPropagate()->GetVState();
      vstate.vQtrn = FGQuaternion(_phi,_theta,_psi);
      fdmex->GetPropagate()->SetVState(vstate);
      Auxiliary->Setalpha( _alpha ); // need to get Auxiliary updated
      Auxiliary->Setbeta ( 0.0  );
      Auxiliary->SetGamma( fgic->GetFlightPathAngleRadIC() );

      fdmex->Run();
  }// end ta turn

  if ( mode == taPullup )
  {
      FCS->SetDeCmd( (*Sminimum)[1] ); // elevator
      FCS->SetDaCmd( (*Sminimum)[2] ); // ailerons
      FCS->SetDrCmd( (*Sminimum)[3] ); // rudder

      double tMin,tMax;
      for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
      {
          tMin=Propulsion->GetEngine(i)->GetThrottleMin();
          tMax=Propulsion->GetEngine(i)->GetThrottleMax();
          FCS->SetThrottleCmd(i,tMin + (*Sminimum)[0] *(tMax-tMin));
          FCS->Run(); // apply throttle change
      }
      Propulsion->GetSteadyState(); // GetSteadyState processes all engines
      FCS->Run(); // apply throttle, yoke & pedal changes

      FGQuaternion quat( 0, (*Sminimum)[2], fgic->GetPRadpsIC() ); // phi, theta, psi
      quat.Normalize();

      //...
      // enforce the current state to IC object

      fgic->ResetIC(_u, _v, _w, _p, _q, _r, _alpha, _beta, _phi, _theta, _psi, _gamma);

      // NOTE: _do not_ fdmex->RunIC() here ! We just reset the state
      //       which means that we populate IC private variables with our set of values
      //fdmex->RunIC();

      FGPropagate::VehicleState vstate = fdmex->GetPropagate()->GetVState();
      vstate.vQtrn = FGQuaternion(_phi,_theta,_psi);
      fdmex->GetPropagate()->SetVState(vstate);
      Auxiliary->Setalpha( _alpha ); // need to get Auxiliary updated
      Auxiliary->Setbeta ( 0.0  );
      Auxiliary->SetGamma( fgic->GetFlightPathAngleRadIC() );

      fdmex->Run();
  }//end taPullup

  delete minVec;
  delete Sminimum;

  //-----------------------------------------------------------------------------------------------------------------

  total_its = NMS.GetFunctionCalls();

  if( !trim_failed ) {
    if (debug_lvl > 0) {
      cout << endl << "  Trim successful. (Cost function value: " << cost_function_value << ")" << endl;
    }
  } else {
    if (debug_lvl > 0)
        cout << endl << "  Trim failed" << endl;
  }


  for (unsigned int i=0; i<(unsigned int)fdmex->GetGroundReactions()->GetNumGearUnits();i++){
    fdmex->GetGroundReactions()->GetGearUnit(i)->SetReport(true);
  }

  return !trim_failed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double Objective::myCostFunctionFull(Vector<double> & x) // x variations come from the search algorithm
{

    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taFull, i.e. 7 independent variables to be adjusted
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R;

    double theta, phi, psi;

    double alpha = 0.0, beta = 0.0, gamma = 0.0;

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for full trim (taFull): 7
    // ordering: the four commands first, then the three Euler angles,
    // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
    // [4] phi, [5] theta, [6] psi (alias taHeading)
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    delta_cmd_A = x[2]; // aileron,  cmd
    delta_cmd_R = x[3]; // rudder,   cmd

    phi   = x[4]; // rad
    theta = x[5];
    psi   = x[6];

    TrimAnalysis->SetEulerAngles(phi, theta, psi);

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (delta_cmd_A <  -1) || (delta_cmd_A >  1) )
                 || ( (delta_cmd_R <  -1) || (delta_cmd_R >  1) )
                 || ( (psi   <    0.0     ) || (psi   >  2.0*M_PI) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) )
                 || ( (phi   <   -    M_PI) || (phi   >      M_PI) )
                 ;

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      +       vDot*vDot
      + 1.000*wDot*wDot
      + 0.010*pDot*pDot
      + 0.010*qDot*qDot
      + 0.010*rDot*rDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E); // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(delta_cmd_A); // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(delta_cmd_R); // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(phi);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl(psi);
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "<< delta_cmd_E <<", "<< delta_cmd_A <<", "<< delta_cmd_R <<", "
                                                                    // 3, 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n"; //26

        // this really goes into the integration process
        FDMExec->GetPropagate()->SetVState( VState );

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f);
    }
 
    return f;
}


double Objective::myCostFunctionFullWingsLevel(Vector<double> & x) // x variations come from the search algorithm
{

    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taFullWingsLevel, i.e. 6 independent variables to be adjusted
    // ___phi is set to zero because we want wings leveled___
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R;

    double theta, phi, psi;

    double alpha = 0.0, beta = 0.0, gamma = 0.0;

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for full trim (taFullWingsLevel): 6
    // ordering: the four commands first, then the three Euler angles,
    // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
    // [4] theta, [5] psi (alias taHeading)
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    delta_cmd_A = x[2]; // aileron,  cmd
    delta_cmd_R = x[3]; // rudder,   cmd

    phi   = 0.0 ; // rad, this enforces the wings-level condition
    theta = x[4];
    psi   = x[5];

    TrimAnalysis->SetEulerAngles(phi, theta, psi);

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (delta_cmd_A <  -1) || (delta_cmd_A >  1) )
                 || ( (delta_cmd_R <  -1) || (delta_cmd_R >  1) )
                 || ( (psi   <    0.0     ) || (psi   >  2.0*M_PI) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) )
                 // || ( (phi   <   -    M_PI) || (phi   >      M_PI) ) ...always zero here
                 ;

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {
        calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      +       vDot*vDot
      + 1.000*wDot*wDot
      + 0.010*pDot*pDot
      + 0.010*qDot*qDot
      + 0.010*rDot*rDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E); // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(delta_cmd_A); // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(delta_cmd_R); // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(phi);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl(psi);
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "<< delta_cmd_E <<", "<< delta_cmd_A <<", "<< delta_cmd_R <<", "
                                                                    // 3, 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n";  // 26

        // this influences really into the further integration process
        FDMExec->GetPropagate()->SetVState( VState );

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f);
    }

    return f;
}

double Objective::myCostFunctionLongitudinal(Vector<double> & x)
{
    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taLongitudinal, i.e. 3 independent variables to be adjusted
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //
    // A perfect longitudinal trim has to be possible!
    // (no asymmetric effect should be present)
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A = 0.0, delta_cmd_R = 0.0;

    double phi, theta, psi;

    double alpha = 0.0, beta = 0.0, gamma = 0.0;

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for longitudinal trim (taLongitudinal): 3
    // ordering: the two commands first, then the Euler angle theta,
    // [0] throttle cmd, [1] elevator cmd, [2] theta
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    theta       = x[2]; // pitch attitude

    double psiIC   = FDMExec->GetIC()->GetPsiRadIC();

    psi   = psiIC;
    phi   = 0.0;

    TrimAnalysis->SetEulerAngles(phi, theta, psi);

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) )
                 ;

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {
        calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      + 1.000*wDot*wDot
      + //1.0
        0.010
        *qDot*qDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E);    // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(0);              // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(0);              // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(0);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl( FDMExec->GetIC()->GetPsiRadIC());
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "                                   // 3
            << delta_cmd_E <<", " <<0.0 <<", "<< 0.0 <<", "         // 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n";  // 26

        FDMExec->GetPropagate()->SetVState( VState ); // ?? is this really necessary

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f); // update the current minimum
    }

    return f;
} // end of myCostFunctionLongitudinal

double Objective::myCostFunctionFullCoordinatedTurn(Vector<double> & x)
{
    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taTurn, i.e. 4 independent variables to be adjusted
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R;

    double phi, theta, psi;

    double alpha = 0.0, beta = 0.0;
    double gamma = 0.0;

    // for coordinated turns with nonzero rate of climb
    gamma = FDMExec->GetIC()->GetFlightPathAngleRadIC();

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for longitudinal trim (taTurn): 5
    // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
    // [4] psi
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    delta_cmd_A = x[2]; // aileron,  cmd
    delta_cmd_R = x[3]; // rudder,   cmd

    psi         = x[4];

    // theta_underlined (see paper on Trim) for coordinated turn
    theta = atan2( sin(psi)*cos(gamma)+cos(psi)*sin(gamma), cos(gamma) );

    TrimAnalysis->setupTurnPhi(psi,theta); // calculates also phi

    double psiIC   = FDMExec->GetIC()->GetPsiRadIC();
    double phiIC   = FDMExec->GetIC()->GetPhiRadIC();

    phi   = TrimAnalysis->GetPhiRad(); // this one is calculated by setupTurnPhi() above

    TrimAnalysis->SetEulerAngles(phi, theta, psi); // recalculates sin and cosines

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (delta_cmd_A <  -1) || (delta_cmd_A >  1) )
                 || ( (delta_cmd_R <  -1) || (delta_cmd_R >  1) )
                 || ( (psi   <    0.0     ) || (psi   >  2.0*M_PI) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) ) // may go out of range
                 ;

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {
        calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      +       vDot*vDot
      + 1.000*wDot*wDot
      + 0.010*pDot*pDot
      + 0.010*qDot*qDot
      + 0.010*rDot*rDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E);    // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(delta_cmd_A);              // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(delta_cmd_R);              // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(phi);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl(psi);
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "                                   // 3
            << delta_cmd_E <<", " << delta_cmd_A <<", "<< delta_cmd_R <<", " // 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n";  // 26

        FDMExec->GetPropagate()->SetVState( VState ); // ?? is this really necessary

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f); // update the current minimum
    }

    return f;
} // end of myCostFunctionFullCoordinatedTurn

double Objective::myCostFunctionFullTurn(Vector<double> & x)
{
    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taTurnFull, i.e. 5 independent variables to be adjusted
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R;

    double phi, theta, psi;

    double alpha = 0.0, beta = 0.0;
    double gamma = 0.0;

    // for coordinated turns with nonzero rate of climb
    gamma = FDMExec->GetIC()->GetFlightPathAngleRadIC();

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for longitudinal trim (taTurn): 5
    // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
    // [4] psi, [5] theta
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    delta_cmd_A = x[2]; // aileron,  cmd
    delta_cmd_R = x[3]; // rudder,   cmd

    psi         = x[4];
    theta       = x[5];

    TrimAnalysis->setupTurn();

    double psiIC   = FDMExec->GetIC()->GetPsiRadIC();
    double phiIC   = FDMExec->GetIC()->GetPhiRadIC();

    phi   = TrimAnalysis->GetPhiRad(); // this one is calculated by sutupTurnPhi() above

    TrimAnalysis->SetEulerAngles(phi, theta, psi); // recalculates sin and cosines

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (delta_cmd_A <  -1) || (delta_cmd_A >  1) )
                 || ( (delta_cmd_R <  -1) || (delta_cmd_R >  1) )
                 || ( (psi   <    0.0     ) || (psi   >  2.0*M_PI) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) ) // may go out of range
                 ;

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {
        calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      +       vDot*vDot
      + 1.000*wDot*wDot
      + 0.010*pDot*pDot
      + 0.010*qDot*qDot
      + 0.010*rDot*rDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E);    // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(delta_cmd_A);              // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(delta_cmd_R);              // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(phi);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl(psi);
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "                                   // 3
            << delta_cmd_E <<", " << delta_cmd_A <<", "<< delta_cmd_R <<", " // 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n";  // 

        FDMExec->GetPropagate()->SetVState( VState ); // ?? is this really necessary

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f); // update the current minimum
    }

    return f;
} // end of myCostFunctionFullCoordinatedTurn


double Objective::myCostFunctionPullUp(Vector<double> & x)
{
    //----------------------------------------------------------------------------
    // NOTE:
    // when the program flow arrives here, it is assumed that a successful
    // instantiation of a TrimAnalysis object took place with a mode set
    // to taTurn, i.e. 6 independent variables to be adjusted
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    // NOTE:
    // rely on the fact that IC quantities have been fed at least once into
    // their correspondent data structure in TrimAnalysis->fdmex tree
    //----------------------------------------------------------------------------

    double delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R;

    double phi, theta, psi;

    double alpha = 0.0, beta = 0.0, gamma = 0.0;

    FGQuaternion Quat;
    FGColumnVector3 vPQRdot(0.0,0.0,0.0), vUVWdot(0.0,0.0,0.0);

    FGPropagate::VehicleState VState;
    VState.vLocation = FGColumnVector3(0.0,0.0,0.0);
    VState.vUVW      = FGColumnVector3(0.0,0.0,0.0);
    VState.vPQR      = FGColumnVector3(0.0,0.0,0.0);
    VState.vQtrn     = FGQuaternion   (0.0,0.0,0.0);

    //----------------------------------------------------------------------------
    // Minimization control vector, i.e. independent variables
    //----------------------------------------------------------------------------

    // CHECK the trim mode

    //-----------------------------------------------------------------------------------------------------------------
    // REMINDER:
    // n. of control variables for longitudinal trim (taPullup): 5
    // [0] throttle cmd, [1] elevator cmd, 2) aileron cmd, [3] rudder cmd
    // [4] theta
    //-----------------------------------------------------------------------------------------------------------------

    delta_cmd_T = x[0]; // throttle, cmd
    delta_cmd_E = x[1]; // elevator, cmd
    delta_cmd_A = x[2]; // aileron,  cmd
    delta_cmd_R = x[3]; // rudder,   cmd

    theta       = x[4];

    double psiIC   = FDMExec->GetIC()->GetPsiRadIC();

    phi   = 0.0; // assume a wing level pull-up
    psi   = psiIC;

    TrimAnalysis->SetEulerAngles(phi, theta, psi);

    //----------------------------------------------------------------------------
    // parameter bound check
    // ToDo: manage bounds via the FGTrimAnalysisControl class
    //----------------------------------------------------------------------------
    bool penalty =  ( (delta_cmd_T <   0) || (delta_cmd_T >  1) )
                 || ( (delta_cmd_E <  -1) || (delta_cmd_E >  1) )
                 || ( (delta_cmd_A <  -1) || (delta_cmd_A >  1) )
                 || ( (delta_cmd_R <  -1) || (delta_cmd_R >  1) )
                 || ( (theta <   -0.5*M_PI) || (theta >  0.5*M_PI) );

    if ( penalty ) {
        //-------------------------------------------------
        // just return an "infinite" value
        return HUGE_VAL;
        //-------------------------------------------------
    } else {
        calculateDottedStates(delta_cmd_T, delta_cmd_E, delta_cmd_A, delta_cmd_R,
                              phi, theta, psi,
                              TrimAnalysis->GetMode(),
                              alpha, beta, gamma,
                              VState,
                              vUVWdot, vPQRdot );
    }

    double u   , v   , w   ,
           p   , q   , r   ,
           uDot, vDot, wDot,
           pDot, qDot, rDot;
    u     = VState.vUVW   (1);  v     = VState.vUVW   (2);  w     = VState.vUVW   (3);
    p     = VState.vPQR   (1);  q     = VState.vPQR   (2);  r     = VState.vPQR   (3);
    uDot  =        vUVWdot(1);  vDot  =        vUVWdot(2);  wDot  =        vUVWdot(3);
    pDot  =        vPQRdot(1);  qDot  =        vPQRdot(2);  rDot  =        vPQRdot(3);

    double
    f = 1.000*uDot*uDot
      +       vDot*vDot
      + 1.000*wDot*wDot
      + 0.010*pDot*pDot
      + 0.010*qDot*qDot
      + 0.010*rDot*rDot;

    static int count = 0;
    count++;

    if ( f < TrimAnalysis->GetCostFunctionValue() )
    {
        // feed into the vector of TrimAnalysis Controls the current values
        vector<FGTrimAnalysisControl*>::iterator vi;
        for(vi=TrimAnalysis->GetControls()->begin();
            vi!=TrimAnalysis->GetControls()->end();vi++)
        {
            if ( (*vi)->GetControlType()==JSBSim::taThrottle  ) (*vi)->SetControl(delta_cmd_T);
            if ( (*vi)->GetControlType()==JSBSim::taElevator  ) (*vi)->SetControl(delta_cmd_E);    // TODO: taPitchTrim
            if ( (*vi)->GetControlType()==JSBSim::taAileron   ) (*vi)->SetControl(delta_cmd_A);    // TODO: taRollTrim
            if ( (*vi)->GetControlType()==JSBSim::taRudder    ) (*vi)->SetControl(delta_cmd_R);    // TODO: taYawTrim
            if ( (*vi)->GetControlType()==JSBSim::taPhi       ) (*vi)->SetControl(phi);
            if ( (*vi)->GetControlType()==JSBSim::taTheta     ) (*vi)->SetControl(theta);
            if ( (*vi)->GetControlType()==JSBSim::taHeading   ) (*vi)->SetControl(psi);
        }

        if ( f<=TrimAnalysis->GetTolerance() ) TrimAnalysis->SetTrimSuccessfull(); // SetTrimFailed(false);

        // write on file, if open

        // "# iteration, costf,
        //    dT, dE, dA, dR, dPsi, dTheta, dPhi, Psi (rad), Theta (rad), Phi (rad),
        //    uDot (fps2), vDot (fps2), wDot (fps2), pDot (rad/s2), qDot (rad/s2), rDot (rad/s2),
        //    u (fps), v (fps), w (fps), p (rad/s), q (rad/s), r (rad/s), alpha (rad), beta (rad),
        //    alphaDot (rad/s), betaDot (rad/s), Thrust"

        ofstream* rfp = TrimAnalysis->GetResultsFile();
        if (rfp)
            (*rfp)
            << count <<", "<< f <<", "                              // 1, 2
            << delta_cmd_T <<", "                                   // 3
            << delta_cmd_E <<", " <<delta_cmd_A <<", "<< delta_cmd_R <<", "         // 4, 5, 6
            << phi <<", "<< theta <<", "<< psi <<", "               // 7, 8, 9
            << uDot <<", "<< vDot <<", "<< wDot <<", "              // 10, 11, 12
            << pDot <<", "<< qDot <<", "<< rDot <<", "              // 13, 14, 15
            << u <<", "<< v <<", "<< w << ", "                      // 16, 17, 18
            << VState.vPQR(1) <<", "<< VState.vPQR(2) <<", "<< VState.vPQR(3) << ", "
                                                                    // 19, 20, 21
            << FDMExec->GetAuxiliary()->Getalpha() << ", "          // 22
            << FDMExec->GetAuxiliary()->Getbeta() << ", "           // 23
            << FDMExec->GetAuxiliary()->Getadot() << ", "           // 24
            << FDMExec->GetAuxiliary()->Getbdot() << ", "           // 25

            << FDMExec->GetPropulsion()->GetEngine(0)->GetThrust() << "\n";  // 26

        FDMExec->GetPropagate()->SetVState( VState ); // ?? is this really necessary

        // update in TrimAnalysis some private variables, i.e. the state _u, _v, _w, ...
        TrimAnalysis->SetState(u, v, w, p, q, r, alpha, beta, phi, theta, psi, gamma);

        TrimAnalysis->SetCostFunctionValue(f); // update the current minimum
    }

    return f;
} // end of myCostFunctionPullUp


void Objective::calculateDottedStates(double delta_cmd_T, double delta_cmd_E, double delta_cmd_A, double delta_cmd_R,
                                      double phi, double theta, double psi,
                                      TrimAnalysisMode trimMode,
                                      double& alpha, double& beta, double& gamma,
                                      FGPropagate::VehicleState& VState,
                                      FGColumnVector3& vUVWdot, FGColumnVector3& vPQRdot )
{
    double stheta,sphi,spsi;
    double ctheta,cphi,cpsi;
    double phiW = 0.;
    FGPropulsion* Propulsion = FDMExec->GetPropulsion();
    FGFCS* FCS = FDMExec->GetFCS();
    FGAuxiliary* Auxiliary = FDMExec->GetAuxiliary();

    if ( ( trimMode == taTurn ) || ( trimMode == taTurnFull ) )//... Coordinated turn: p,q,r not zero, beta=0, gamma=0
    {
        // setupTurn has already made its job
        phiW = TrimAnalysis->GetPhiWRad();
    }

    cphi   = cos(phi);   sphi   = sin(phi);   // phi, rad
    ctheta = cos(theta); stheta = sin(theta); // theta, rad
    cpsi   = cos(psi);   spsi   = sin(psi);   // psi, rad

    TrimAnalysis->SetEulerAngles(phi, theta, psi);

    //-------------------------------------------------
    // apply controls
    //-------------------------------------------------

    // make sure the engines are running
    for (unsigned int i=0; i<Propulsion->GetNumEngines(); i++) {
       Propulsion->GetEngine(i)->SetRunning(true);
    }

    double tMin,tMax;
    for(unsigned i=0;i<Propulsion->GetNumEngines();i++)
    {
      tMin=Propulsion->GetEngine(i)->GetThrottleMin();
      tMax=Propulsion->GetEngine(i)->GetThrottleMax();
      FCS->SetThrottleCmd(i,tMin + delta_cmd_T *(tMax-tMin));
      FCS->Run(); // apply throttle change
    }
    Propulsion->GetSteadyState(); // GetSteadyState processes all engines

    // apply commands
    // ToDo: apply aerosurface deflections,
    // to override control system authority

    FCS->SetDeCmd( delta_cmd_E ); // elevator
    FCS->SetDaCmd( delta_cmd_A ); // ailerons
    FCS->SetDrCmd( delta_cmd_R ); // rudder

    FCS->Run(); // apply yoke & pedal changes

    //................................................
    // set also euler angles
    //................................................

    // new euler angles -> new quaternion Quat

    FGQuaternion Quat1( phi, theta, psi ); // values coming fro search algorithm

    VState.vQtrn = Quat1;
    VState.vQtrn.Normalize();

    //------------------------------------------
    // reconstruct NED velocity components from
    // initial conditions
    //------------------------------------------

    // initial altitude (never changed)
    double hIC = FDMExec->GetIC()->GetAltitudeFtIC();

    // re-apply the desired altitude;
    // FGAtmosphere wants the altitude from FGPropagate
    // (currently there's no Seth method in FGAtmosphere)
    FDMExec->GetPropagate()->Seth( hIC );
    FDMExec->GetAtmosphere()->Run();

    // initial speed (never changed)
    double vtIC = FDMExec->GetIC()->GetVtrueFpsIC();

    // initial flight-path angle (never changed)
    double gammaIC = FDMExec->GetIC()->GetFlightPathAngleRadIC();

    // initial climb-rate (never changed)
    // ???
    double rocIC = FDMExec->GetIC()->GetClimbRateFpsIC();
    // ???
    gammaIC = TrimAnalysis->GetGamma(); // from file of from IC
    rocIC = vtIC * tan(gammaIC);
    gamma = gammaIC; // <--------------------------------------------- this goes out to the caller

    double vdownIC = - rocIC;

    if ( ( trimMode == taTurn ) || ( trimMode == taTurnFull ) ) //... turn: p,q,r, gamma not zero
    {
        gamma   = TrimAnalysis->GetGammaRad(); //0.0;
        vdownIC = TrimAnalysis->GetVtFps() * tan(gamma); //0.0;
    }
    Auxiliary->SetGamma(gamma);

    // euler angles from the IC
    double psiIC   = FDMExec->GetIC()->GetPsiRadIC(); // this is the desired ground track direction

    // assume that the desired ground-track heading coincides with the
    // aircraft initial heading given in IC
    double psigtIC = psiIC;

    // NED velocity components
    //double vgIC = FDMExec->GetIC()->GetVgroundFpsIC();
    double vgIC = vtIC * cos(gammaIC);

    double vnorthIC = vgIC * cos(psigtIC);
    double veastIC  = vgIC * sin(psigtIC);

    // Wind components in NED frame
    double wnorthIC = FDMExec->GetIC()->GetWindNFpsIC();
    double weastIC  = FDMExec->GetIC()->GetWindEFpsIC();
    double wdownIC  = FDMExec->GetIC()->GetWindDFpsIC();

    // Velocity components in body-frame (from NED)
    double u, v, w;

    u=vnorthIC*ctheta*cpsi +
       veastIC*ctheta*spsi -
       vdownIC*stheta;
    v=vnorthIC*( sphi*stheta*cpsi - cphi*spsi ) +
       veastIC*( sphi*stheta*spsi + cphi*cpsi ) +
       vdownIC*sphi*ctheta;
    w=vnorthIC*( cphi*stheta*cpsi + sphi*spsi ) +
       veastIC*( cphi*stheta*spsi - sphi*cpsi ) +
       vdownIC*cphi*ctheta;

    // Wind domponents in body-frame (from NED)
    double uw, vw, ww;

    uw=wnorthIC*ctheta*cpsi +
        weastIC*ctheta*spsi -
        wdownIC*stheta;
    vw=wnorthIC*( sphi*stheta*cpsi - cphi*spsi ) +
        weastIC*( sphi*stheta*spsi + cphi*cpsi ) +
        wdownIC*sphi*ctheta;
    ww=wnorthIC*(cphi*stheta*cpsi + sphi*spsi) +
        weastIC*(cphi*stheta*spsi - sphi*cpsi) +
        wdownIC*cphi*ctheta;

    /**********************************************************************************************
                                                                 P R O P A G A T I O N  start . . .
     **********************************************************************************************/

    // now that we have the velocities we can imitate the chain
    // in FDMexec->Run()... Propagate::Run()

    //++++++++++++++++++++++++++++++++++++++++++++
    // recalculate some important auxiliary data
    // imitate here Auxiliary::Run()
    //++++++++++++++++++++++++++++++++++++++++++++

    double ua, va, wa;
    double adot, bdot;

    Auxiliary->SetVt(vtIC); // re-apply the desired velocity

    if ((trimMode==taTurn)||(trimMode==taPullup))
    {
        v=0.0; // no sideslip
    }

    ua = u + uw; va = v + vw; wa = w + ww;

    // from FGAuxiliary::Run()
    if ( vtIC > 0.05) {
      if ( wa != 0.0 ) alpha = ua*ua > 0.0 ? atan2(wa, ua) : 0.0;                    // this goes out to the caller
      if ( va != 0.0 ) beta = ua*ua+wa*wa > 0.0 ? atan2(va,sqrt(ua*ua+wa*wa)) : 0.0; // this goes out to the caller
      double mUW = (ua*ua + wa*wa);
      double signU=1;
      if (ua != 0.0) signU = ua/fabs(ua);
      adot = bdot = 0; // enforce zero aerodyn. angle rates
    } else {
      alpha = beta = adot = bdot = 0;                                                // this goes out to the caller
    }

    //------------------------------------------------------------------------------------
    // APPLY constraints to (p,q,r)
    //------------------------------------------------------------------------------------

    // constraint applied to p,q,r
    double p, q, r;

    // for NON TURNING straight FLIGHT, these will remain zero
    p = q = r = 0.0;

    if ( ( trimMode == taTurn ) || ( trimMode == taTurnFull ) ) //... Coordinated turn: p,q,r not zero
    {
      double g=FDMExec->GetInertial()->gravity();
      //double cgamma=cos(FDMExec->GetIC()->GetFlightPathAngleRadIC());

      // assume that the given phiIC is the desired phi_Wind
      // phiW  ... see above at the beginning
      double turnRate = g*tan(phiW) / FDMExec->GetIC()->GetVtrueFpsIC(); //FDMExec->GetIC()->GetUBodyFpsIC();
      double pW = 0.0;
      double qW = turnRate*sin(phiW);
      double rW = turnRate*cos(phiW);

      FGColumnVector3 pqr = TrimAnalysis->UpdateRatesTurn(psi, theta, phi, phiW);

      // finally calculate the body frame angular rates
      p = pqr(1);
      q = pqr(2);
      r = pqr(3);

      Auxiliary->SetGamma(0.);

    }
    if ( trimMode == taPullup ) //... then p,q,r not zero
    {
        double g=FDMExec->GetInertial()->gravity();
        double cgamma=cos(FDMExec->GetIC()->GetFlightPathAngleRadIC());
        //double targetNlf=FDMExec->GetIC()->GetTargetNlfIC();
        double targetNlf = TrimAnalysis->GetTargetNlf();

        FGColumnVector3 pqr = TrimAnalysis->UpdateRatesPullup();
        q = pqr(2);

        // assume levelled wings
    }
    //if ( trimMode == taRoll ) ... then p,q,r not zero
    //    may assign theta rate (=q if wings are leveled)

    double qbar   = 0.5*FDMExec->GetAtmosphere()->GetDensity()*vtIC*vtIC;
    double qbarUW = 0.5*FDMExec->GetAtmosphere()->GetDensity()*(ua*ua + wa*wa);
    double qbarUV = 0.5*FDMExec->GetAtmosphere()->GetDensity()*(ua*ua + va*va);
    double Mach = vtIC / FDMExec->GetAtmosphere()->GetSoundSpeed();

    double MachU = ua / FDMExec->GetAtmosphere()->GetSoundSpeed();
    double MachV = va / FDMExec->GetAtmosphere()->GetSoundSpeed();
    double MachW = wa / FDMExec->GetAtmosphere()->GetSoundSpeed();

    //++++++++++++++++++++++++++++++++++++++++++++
    // now feed the above values into Auxiliary data structure
    //++++++++++++++++++++++++++++++++++++++++++++

    Auxiliary->Setalpha( alpha );
    Auxiliary->Setbeta ( beta  );

    if ((trimMode==taTurn)||(trimMode==taPullup)) Auxiliary->Setbeta( 0.0 );

    // ensure zero rates
    Auxiliary->Setadot( 0.0 );
    Auxiliary->Setbdot( 0.0 );

    // ToDo: take into account here the wind and other desired trim conditions
    Auxiliary->SetAeroPQR( FGColumnVector3(p,q,r)) ;

    // note assumes that trim_mode is triggered
    // assumes that p=q=r=0, or set by the appropriate taTrimMode context

    if ((trimMode==taTurn)||(trimMode==taPullup))
    {
        v=0.0; // no sideslip
    }

    JSBSim::FGColumnVector3 vUVWAero( ua,va,wa );
    Auxiliary->SetAeroUVW( vUVWAero );

    Auxiliary->Setqbar  ( qbar    );
    Auxiliary->SetqbarUV( qbarUV  );
    Auxiliary->SetqbarUW( qbarUW  );

    Auxiliary->SetVt    ( vtIC    );

    Auxiliary->SetMach  ( Mach    );
    Auxiliary->SetGamma ( gammaIC );

    // note: do not Auxiliary->Run(), otherwise dotted values
    //       _and_ aerodynamic angles are recalculated !!!

    //++++++++++++++++++++++++++++++++++++++++++++
    // Recalculate aerodynamics by taking into
    // account the above changes
    //++++++++++++++++++++++++++++++++++++++++++++
    FDMExec->GetAerodynamics()->Run();

    //++++++++++++++++++++++++++++++++++++++++++++
    // Recalculate propulsion forces & moments
    //++++++++++++++++++++++++++++++++++++++++++++
    //Propulsion->Run();
    Propulsion->GetSteadyState();

    //++++++++++++++++++++++++++++++++++++++++++++
    // Recalculate GroundReaction forces & moments
    //++++++++++++++++++++++++++++++++++++++++++++
    FDMExec->GetGroundReactions()->Run();

    //++++++++++++++++++++++++++++++++++++++++++++
    // Recalculate TOTAL Forces & Moments
    //++++++++++++++++++++++++++++++++++++++++++++
    FDMExec->GetAircraft()->Run();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // now we have the forces & moments
    // we need some Propagate-like statements ... i.e. replicate what Propagate::Run() does
    // to finally get the "dotted" state
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    VState.vLocation = JSBSim::FGLocation( FDMExec->GetPropagate()->GetLongitude(),            // location
                                           FDMExec->GetPropagate()->GetLatitude(),
                                           hIC + FDMExec->GetIC()->GetSeaLevelRadiusFtIC() );

    // _do not_ feed into Propagate at this level!!
    //FDMExec->GetPropagate()->SetLocation( VState.vLocation );
    // but pass to the caller the calculated state and dotted state

    VState.vUVW(1) = u; // u,v,w
    VState.vUVW(2) = v;
    VState.vUVW(3) = w;

    VState.vPQR(1) = p; // p,q,r
    VState.vPQR(2) = q;
    VState.vPQR(3) = r;

    //VState.vQtrn     = Quat;    // already fed above

    const FGColumnVector3 omega( 0.0, 0.0, FDMExec->GetInertial()->omega() ); // earth rotation
    const FGColumnVector3& vForces = FDMExec->GetAircraft()->GetForces();     // current forces
    const FGColumnVector3& vMoments = FDMExec->GetAircraft()->GetMoments();   // current moments

    double mass = FDMExec->GetMassBalance()->GetMass();                       // mass
    const FGMatrix33& J = FDMExec->GetMassBalance()->GetJ();                  // inertia matrix
    const FGMatrix33& Jinv = FDMExec->GetMassBalance()->GetJinv();            // inertia matrix inverse

    double rd = FDMExec->GetPropagate()->GetRadius();                         // radius
    if (rd == 0.0) {cerr << "radius = 0 !" << endl; rd = 1e-16;}              // radius check

    double rdInv = 1.0/rd;
    FGColumnVector3 gAccel( 0.0, 0.0, FDMExec->GetInertial()->GetGAccel(rd) );

    // The rotation matrices:
    const FGMatrix33& Tl2b  = VState.vQtrn.GetT();          // local to body frame
    const FGMatrix33& Tb2l  = VState.vQtrn.GetTInv();       // body to local frame
    const FGMatrix33& Tec2l = VState.vLocation.GetTec2l();  // earth centered to local frame
    const FGMatrix33& Tl2ec = VState.vLocation.GetTl2ec();  // local to earth centered frame

    // Inertial angular velocity measured in the body frame.
    //... const FGColumnVector3 pqri = VState.vPQR + Tl2b*(Tec2l*omega);

    // NOTE:   the trim is valid in flat-earth hypothesis,
    //         do not take into account the motion relative to the e.c.,
    //         consider only the motion wrt local frame

    const FGColumnVector3 pqri = VState.vPQR;

    const FGColumnVector3 vVel = Tb2l * VState.vUVW;

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Finally compute the time derivatives of the vehicle state values:
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // Compute body frame rotational accelerations based on the current body moments
    vPQRdot = Jinv*(vMoments - pqri*(J*pqri)); // <------------------------ this goes out to the caller

    // Compute body frame accelerations based on the current body forces
    vUVWdot = VState.vUVW*VState.vPQR + vForces/mass; // <----------------- this goes out to the caller

    // Coriolis acceleration.
    FGColumnVector3 ecVel = Tl2ec*vVel;
    FGColumnVector3 ace = 2.0*omega*ecVel;
    //... vUVWdot -= Tl2b*(Tec2l*ace);

    if (!FDMExec->GetGroundReactions()->GetWOW()) {
        // Centrifugal acceleration.
        FGColumnVector3 aeec = omega*(omega*VState.vLocation);
    }

    // Gravitation accel
    vUVWdot += Tl2b*gAccel;

    // Compute vehicle velocity wrt EC frame, expressed in EC frame
    FGColumnVector3 vLocationDot = Tl2ec * vVel;

    FGColumnVector3 omegaLocal( rdInv*vVel(2), // East
                               -rdInv*vVel(1), // North
                               -rdInv*vVel(2)*VState.vLocation.GetTanLatitude() );

    // Compute quaternion orientation derivative on current body rates
    //... FGQuaternion vQtrndot = VState.vQtrn.GetQDot( VState.vPQR - Tl2b*omegaLocal );
    FGQuaternion vQtrndot = VState.vQtrn.GetQDot( VState.vPQR );

    /**********************************************************************************************
                                                               end of....     P R O P A G A T I O N
     **********************************************************************************************/

}

bool FGTrimAnalysis::getSteadyState(int nrepeat )
{
  double currentThrust = 0, lastThrust=-1;
  int steady_count=0;
  bool steady=false;

  while ( !steady && steady_count <= nrepeat )
  {
      steady_count++;
      steady = Propulsion->GetSteadyState();
  }
  return steady;
}

//................................................................................
} // JSBSim namespace
