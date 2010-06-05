/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRotor.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the rotor object

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
08/24/00  JSB  Created
01/01/10  T.Kreitler test implementation

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>

#include "FGRotor.h"

#include "models/FGPropagate.h"
#include "models/FGAtmosphere.h"
#include "models/FGAuxiliary.h"
#include "models/FGMassBalance.h"

#include "input_output/FGXMLElement.h"

#include "math/FGRungeKutta.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGRotor.cpp,v 1.9 2010/06/05 12:12:34 jberndt Exp $";
static const char *IdHdr = ID_ROTOR;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MISC
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static int dump_req; // debug schwafel flag

static inline double sqr(double x) { return x*x; }

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// starting with 'inner' rotor, FGRotor constructor is further down

FGRotor::rotor::~rotor() { }

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// hmm, not a real alternative to a pretty long initializer list

void FGRotor::rotor::zero() {
  FGColumnVector3 zero_vec(0.0, 0.0, 0.0);

  flags               = 0;
  parent              = NULL  ;

  reports             = 0;

  // configuration
  Radius              = 0.0 ;
  BladeNum            = 0   ;
  RelDistance_xhub    = 0.0 ;
  RelShift_yhub       = 0.0 ;
  RelHeight_zhub      = 0.0 ;
  NominalRPM          = 0.0 ;
  MinRPM              = 0.0 ;
  BladeChord          = 0.0 ;
  LiftCurveSlope      = 0.0 ;
  BladeFlappingMoment = 0.0 ;
  BladeTwist          = 0.0 ;
  BladeMassMoment     = 0.0 ;
  TipLossB            = 0.0 ;
  PolarMoment         = 0.0 ;
  InflowLag           = 0.0 ;
  ShaftTilt           = 0.0 ;
  HingeOffset         = 0.0 ;
  HingeOffset_hover   = 0.0 ;
  CantAngleD3         = 0.0 ;

  theta_shaft         = 0.0 ;
  phi_shaft           = 0.0 ;

  // derived parameters
  LockNumberByRho     = 0.0 ;
  solidity            = 0.0 ;
  RpmRatio            = 0.0 ;

  for (int i=0; i<5; i++) R[i] = 0.0;
  for (int i=0; i<6; i++) B[i] = 0.0;

  BodyToShaft.InitMatrix();
  ShaftToBody.InitMatrix();

  // dynamic values
  ActualRPM           = 0.0 ;
  Omega               = 0.0 ;
  beta_orient         = 0.0 ;
  a0                  = 0.0 ;
  a_1 = b_1 = a_dw    = 0.0 ;
  a1s = b1s           = 0.0 ;
  H_drag = J_side     = 0.0 ;

  Torque              = 0.0 ;
  Thrust              = 0.0 ;
  Ct                  = 0.0 ;
  lambda              = 0.0 ;
  mu                  = 0.0 ;
  nu                  = 0.0 ;
  v_induced           = 0.0 ;

  force      = zero_vec ;
  moment     = zero_vec ;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 5in1: value-fetch-convert-default-return function 

double FGRotor::rotor::cnf_elem(  const string& ename, double default_val, 
                                  const string& unit, bool tell)
{

  Element *e = NULL;
  double val=default_val;

  std::string pname = "*No parent element*";

  if (parent) {
    e = parent->FindElement(ename);
    pname = parent->GetName();
  }

  if (e) {
    if (unit.empty()) {
      // val = e->FindElementValueAsNumber(ename); 
      // yields to: Attempting to get single data value from multiple lines
      val = parent->FindElementValueAsNumber(ename);
    } else {
      // val = e->FindElementValueAsNumberConvertTo(ename,unit); 
      // yields to: Attempting to get non-existent element diameter + crash, why ?
      val = parent->FindElementValueAsNumberConvertTo(ename,unit);
    }
  } else {
    if (tell) {
      cerr << pname << ": missing element '" << ename <<"' using estimated value: " << default_val << endl;
    }
  }

  return val;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRotor::rotor::cnf_elem(const string& ename, double default_val, bool tell)
{
  return cnf_elem(ename, default_val, "", tell);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 1. read configuration and try to fill holes, ymmv
// 2. calculate derived parameters and transforms
void FGRotor::rotor::configure(int f, const rotor *xmain)
{

  double estimate;
  const bool yell   = true;
  const bool silent = false;

  flags = f;

  estimate = (xmain) ? 2.0 * xmain->Radius * 0.2 : 42.0;
  Radius = 0.5 * cnf_elem("diameter", estimate, "FT", yell);

  estimate = (xmain) ? xmain->BladeNum  : 2.0;
  estimate = Constrain(1.0,estimate,4.0);
  BladeNum = (int) cnf_elem("numblades", estimate, yell);

  estimate = (xmain) ? - xmain->Radius * 1.05 - Radius : - 0.025 * Radius ; 
  RelDistance_xhub = cnf_elem("xhub", estimate, "FT", yell);

  RelShift_yhub = cnf_elem("yhub", 0.0, "FT", silent);
  
  estimate = - 0.1 * Radius - 4.0;
  RelHeight_zhub = cnf_elem("zhub", estimate, "FT", yell);
  
  // make sure that v_tip (omega*r) is below 0.7mach ~ 750ft/s
  estimate = (750.0/Radius)/(2.0*M_PI) * 60.0;  // 7160/Radius
  NominalRPM = cnf_elem("nominalrpm", estimate, yell);

  MinRPM = cnf_elem("minrpm", 1.0, silent);
  MinRPM = Constrain(1.0, MinRPM, NominalRPM-1.0);

  estimate = (xmain) ? 0.12 : 0.07;  // guess solidity
  estimate = estimate * M_PI*Radius/BladeNum;
  BladeChord = cnf_elem("chord", estimate, "FT", yell);

  LiftCurveSlope = cnf_elem("liftcurveslope", 6.0, yell); // "1/RAD"

  estimate = sqr(BladeChord) * sqr(Radius) * 0.57;
  BladeFlappingMoment = cnf_elem("flappingmoment", estimate, "SLUG*FT2", yell);   
  BladeFlappingMoment = Constrain(0.1, BladeFlappingMoment, 1e9);

  BladeTwist = cnf_elem("twist", -0.17, "RAD", yell);

  estimate = sqr(BladeChord) * BladeChord * 15.66; // might be really wrong!
  BladeMassMoment = cnf_elem("massmoment", estimate, yell); // slug-ft
  BladeMassMoment = Constrain(0.1, BladeMassMoment, 1e9);

  TipLossB = cnf_elem("tiplossfactor", 0.98, silent);

  estimate = 1.1 * BladeFlappingMoment * BladeNum;
  PolarMoment = cnf_elem("polarmoment", estimate, "SLUG*FT2", silent);
  PolarMoment = Constrain(0.1, PolarMoment, 1e9);

  InflowLag = cnf_elem("inflowlag", 0.2, silent); // fixme, depends on size

  estimate = (xmain) ? 0.0 : -0.06;  
  ShaftTilt = cnf_elem("shafttilt", estimate, "RAD", silent);

  // ignore differences between teeter/hingeless/fully-articulated constructions
  estimate = 0.05 * Radius;
  HingeOffset = cnf_elem("hingeoffset", estimate, "FT", (xmain) ? silent : yell);

  CantAngleD3 = cnf_elem("cantangle", 0.0, "RAD", silent);  

  // derived parameters

  // precalc often used powers
  R[0]=1.0; R[1]=Radius;   R[2]=R[1]*R[1]; R[3]=R[2]*R[1]; R[4]=R[3]*R[1];
  B[0]=1.0; B[1]=TipLossB; B[2]=B[1]*B[1]; B[3]=B[2]*B[1]; B[4]=B[3]*B[1]; B[5]=B[4]*B[1];

  LockNumberByRho = LiftCurveSlope * BladeChord * R[4] / BladeFlappingMoment;
  solidity = BladeNum * BladeChord / (M_PI * Radius);

  // use simple orientations at the moment
  if (flags & eTail) { // axis parallel to Y_body
    theta_shaft = 0.0; // no tilt
    phi_shaft = 0.5*M_PI;

    // opposite direction if main rotor is spinning CW
    if (xmain && (xmain->flags & eRotCW) ) { 
      phi_shaft = -phi_shaft; 
    }
  } else {  // more or less upright
    theta_shaft = ShaftTilt;
    phi_shaft = 0.0; 
  }

  // setup Shaft-Body transforms, see /SH79/ eqn(17,18)
  double st = sin(theta_shaft);
  double ct = cos(theta_shaft);
  double sp = sin(phi_shaft);
  double cp = cos(phi_shaft);

  ShaftToBody.InitMatrix(    ct, st*sp, st*cp,
                            0.0,    cp,   -sp,
                            -st, ct*sp, ct*cp  );

  BodyToShaft  =  ShaftToBody.Inverse();

  // misc defaults
  nu = 0.001; // help the flow solver by providing some moving molecules
  
  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// calculate control-axes components of total airspeed at the hub.
// sets rotor orientation angle (beta) as side effect. /SH79/ eqn(19-22)

FGColumnVector3 FGRotor::rotor::hub_vel_body2ca( const FGColumnVector3 &uvw, 
                                                 const FGColumnVector3 &pqr,
                                                 double a_ic, double b_ic)
{

  FGColumnVector3  v_r, v_shaft, v_w;
  FGColumnVector3  pos(RelDistance_xhub,0.0,RelHeight_zhub);

  v_r = uvw + pqr*pos;
  v_shaft = BodyToShaft * v_r;

  beta_orient = atan2(v_shaft(eV),v_shaft(eU));

  v_w(eU) = v_shaft(eU)*cos(beta_orient) + v_shaft(eV)*sin(beta_orient);
  v_w(eV) = 0.0;
  v_w(eW) = v_shaft(eW) - b_ic*v_shaft(eU) - a_ic*v_shaft(eV);

  return v_w;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// express fuselage angular velocity in control axes /SH79/ eqn(30,31)

FGColumnVector3 FGRotor::rotor::fus_angvel_body2ca( const FGColumnVector3 &pqr)
{
  FGColumnVector3 av_s_fus, av_w_fus;    

  av_s_fus = BodyToShaft * pqr;

  av_w_fus(eP)=   av_s_fus(eP)*cos(beta_orient) + av_s_fus(eQ)*sin(beta_orient);
  av_w_fus(eQ)= - av_s_fus(eP)*sin(beta_orient) + av_s_fus(eQ)*cos(beta_orient);
  av_w_fus(eR)=   av_s_fus(eR);
         
  return av_w_fus;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// problem function passed to rk solver

  double FGRotor::rotor::dnuFunction::pFunc(double x, double nu) {
    double d_nu;
    d_nu =  k_sat * (ct_lambda * (k_wor - nu) + k_theta) / 
                     (2.0 * sqrt( mu2 + sqr(k_wor - nu)) ); 
    d_nu = d_nu * k_flowscale - nu;
    return  d_nu; 
  }; 

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  // merge params to keep the equation short
  void FGRotor::rotor::dnuFunction::update_params(rotor *r, double ct_t01, double fs, double w) {
    k_sat       = 0.5* r->solidity * r->LiftCurveSlope;
    ct_lambda   = 1.0/2.0*r->B[2] + 1.0/4.0 * r->mu*r->mu;
    k_wor       = w/(r->Omega*r->Radius);
    k_theta     = ct_t01;
    mu2         = r->mu * r->mu;
    k_flowscale = fs;
  };


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Calculate rotor thrust and inflow-ratio (lambda), this is achieved by
// approximating a solution for the differential equation:
// 
//  dnu/dt = 1/tau ( Ct / (2*sqrt(mu^2+lambda^2))  -  nu )  , /SH79/ eqn(26).
// 
// Propper calculation of the inflow-ratio (lambda) is vital for the
// following calculations. Simple implementations (i.e. Newton-Raphson w/o
// checking) tend to oscillate or overshoot in the low speed region,
// therefore a more expensive solver is used.
//
// The flow_scale parameter is used to approximate a reduction of inflow
// if the helicopter is close to the ground, yielding to higher thrust,
// see /TA77/ eqn(10a). Doing the ground effect calculation here seems
// more favorable then to code it in the fdm_config.

void FGRotor::rotor::calc_flow_and_thrust(    
                           double dt, double rho, double theta_0,
                           double Uw, double Ww, double flow_scale)
{

  double ct_over_sigma = 0.0;
  double ct_l, ct_t0, ct_t1;
  double nu_ret = 0.0;

  mu = Uw/(Omega*Radius); // /SH79/ eqn(24)

  ct_t0 = (1.0/3.0*B[3] + 1.0/2.0 * TipLossB*mu*mu - 4.0/(9.0*M_PI) * mu*mu*mu )*theta_0;
  ct_t1 = (1.0/4.0*B[4] + 1.0/4.0 * B[2]*mu*mu)*BladeTwist;

  // merge params together
  flowEquation.update_params(this, ct_t0+ct_t1, flow_scale, Ww);
  
  nu_ret = rk.evolve(nu, &flowEquation);

  if (rk.getStatus() != FGRungeKutta::eNoError) { // never observed so far
    cerr << "# IEHHHH [" << flags << "]: Solver Error - resetting!" << endl;
    rk.clearStatus();
    nu_ret = nu; // use old value and keep fingers crossed.
  }

  // keep an eye on the solver, but be quiet after a hundred messages
  if (reports < 100 && rk.getIterations()>6) {
    cerr << "# LOOK [" << flags << "]: Solver took " 
         << rk.getIterations() << " rounds." << endl;
    reports++;
    if (reports==100) {
      cerr << "# stopped babbling after 100 notifications." << endl;
    }
  }

  // now from nu to lambda, Ct, and Thrust

  nu = nu_ret;
  lambda = Ww/(Omega*Radius) - nu; // /SH79/ eqn(25)

  ct_l  = (1.0/2.0*B[2] + 1.0/4.0 * mu*mu)*lambda;  
  ct_over_sigma = (LiftCurveSlope/2.0)*(ct_l + ct_t0 + ct_t1); // /SH79/ eqn(27)

  Thrust = BladeNum*BladeChord*Radius*rho*sqr(Omega*Radius) * ct_over_sigma;

  Ct = ct_over_sigma * solidity;
  v_induced = nu * (Omega*Radius);

  if (dump_req && (flags & eMain) ) {
    printf("# mu %f : nu %f lambda %f vi %f\n", mu, nu, lambda, v_induced);
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// this is the most arcane part in the calculation chain the
// constants used should be reverted to more general parameters.
// otoh: it works also for smaller rotors, sigh!
// See /SH79/ eqn(36), and /BA41/ for a richer set of equations.

void FGRotor::rotor::calc_torque(double rho, double theta_0)
{
  double Qa0;
  double cq_s_m[5], cq_over_sigma;
  double l,m,t075; // shortcuts

  t075 = theta_0 + 0.75 * BladeTwist;

  m = mu;
  l = lambda;

  cq_s_m[0] = 0.00109 - 0.0036*l - 0.0027*t075 - 1.10*sqr(l) - 0.545*l*t075 + 0.122*sqr(t075);
  cq_s_m[2] = ( 0.00109 - 0.0027*t075 - 3.13*sqr(l) - 6.35*l*t075 - 1.93*sqr(t075) ) * sqr(m);
  cq_s_m[3] = - 0.133*l*t075 * sqr(m)*m;
  cq_s_m[4] = ( - 0.976*sqr(l) - 6.38*l*t075 - 5.26*sqr(t075) ) * sqr(m)*sqr(m);

  cq_over_sigma = cq_s_m[0] + cq_s_m[2] + cq_s_m[3] + cq_s_m[4];
  // guess an a (LiftCurveSlope) is included in eqn above, so check if there is a large
  // influence when  a_'other-model'/ a_'ch54' diverts from 1.0.

  Qa0 = BladeNum * BladeChord * R[2] * rho * sqr(Omega*Radius);

// TODO: figure out how to handle negative cq_over_sigma/torque

  Torque =  Qa0 *  cq_over_sigma;

  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// The coning angle doesn't apply for teetering rotors, but calculating
// doesn't hurt. /SH79/ eqn(29)

void FGRotor::rotor::calc_coning_angle(double rho, double theta_0)
{
  double lock_gamma = LockNumberByRho * rho;

  double a0_l  = (1.0/6.0  * B[3] + 0.04 * mu*mu*mu) * lambda;
  double a0_t0 = (1.0/8.0  * B[4] + 1.0/8.0  * B[2]*mu*mu) * theta_0;
  double a0_t1 = (1.0/10.0 * B[5] + 1.0/12.0 * B[3]*mu*mu) * BladeTwist;
  a0 = lock_gamma * ( a0_l + a0_t0 + a0_t1);
  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Flapping angles relative to control axes /SH79/ eqn(32)

void FGRotor::rotor::calc_flapping_angles(  double rho, double theta_0, 
                                            const FGColumnVector3 &pqr_fus_w)
{
  double lock_gamma = LockNumberByRho * rho;

  double mu2_2B2 = sqr(mu)/(2.0*B[2]);
  double t075 = theta_0 + 0.75 * BladeTwist;  // common approximation for rectangular blades
  
  a_1 = 1.0/(1.0 - mu2_2B2) * (
                                 (2.0*lambda + (8.0/3.0)*t075)*mu
                               + pqr_fus_w(eP)/Omega
                               - 16.0 * pqr_fus_w(eQ)/(B[4]*lock_gamma*Omega)
                             );
  
  b_1 = 1.0/(1.0 + mu2_2B2) * (
                                 (4.0/3.0)*mu*a0
                               - pqr_fus_w(eQ)/Omega
                               - 16.0 * pqr_fus_w(eP)/(B[4]*lock_gamma*Omega)
                             );
  
  // used in  force calc
  a_dw = 1.0/(1.0 - mu2_2B2) * (
                                 (2.0*lambda + (8.0/3.0)*t075)*mu
                               - 24.0 * pqr_fus_w(eQ)/(B[4]*lock_gamma*Omega)
                                 * ( 1.0 - ( 0.29 * t075 / (Ct/solidity) ) )
                             );
  
  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// /SH79/ eqn(38,39)

void FGRotor::rotor::calc_drag_and_side_forces(double rho, double theta_0)
{
  double cy_over_sigma  ;
  double t075 = theta_0 + 0.75 * BladeTwist;

  H_drag = Thrust * a_dw;
  
  cy_over_sigma = (
                      0.75*b_1*lambda - 1.5*a0*mu*lambda + 0.25*a_1*b_1*mu
                    - a0*a_1*sqr(mu) + (1.0/6.0)*a0*a_1
                    - (0.75*mu*a0 - (1.0/3.0)*b_1 - 0.5*sqr(mu)*b_1)*t075
                  );
  cy_over_sigma *= LiftCurveSlope/2.0;
  
  J_side = BladeNum * BladeChord * Radius * rho * sqr(Omega*Radius) * cy_over_sigma;

  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// transform rotor forces from control axes to shaft axes, and express
// in body axes /SH79/ eqn(40,41)

FGColumnVector3 FGRotor::rotor::body_forces(double a_ic, double b_ic)
{
    FGColumnVector3 F_s(
          - H_drag*cos(beta_orient) - J_side*sin(beta_orient) + Thrust*b_ic,
          - H_drag*sin(beta_orient) + J_side*cos(beta_orient) + Thrust*a_ic,
          - Thrust);    

    if (dump_req && (flags & eMain) ) {
      printf("# abß:  % f % f % f\n", a_ic, b_ic, beta_orient );
      printf("# HJT:  % .2f % .2f % .2f\n", H_drag, J_side, Thrust );
      printf("# F_s:  % .2f % .2f % .2f\n", F_s(1), F_s(2), F_s(3) );
      FGColumnVector3 F_h;   
      F_h = ShaftToBody * F_s;
      printf("# F_h:  % .2f % .2f % .2f\n", F_h(1), F_h(2), F_h(3) );
    }

    return ShaftToBody * F_s;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// rotational sense is handled here
// still a todo: how to get propper values for 'BladeMassMoment'
// here might be a good place to tweak hovering stability, check /AM50/.

FGColumnVector3 FGRotor::rotor::body_moments(FGColumnVector3 F_h, double a_ic, double b_ic)
{
  FGColumnVector3 M_s, M_hub, M_h;
  
  FGColumnVector3 h_pos(RelDistance_xhub, 0.0, RelHeight_zhub);

  // vermutlich ein biege moment, bzw.widerstands moment ~ d^3
  double M_w_tilde = 0.0 ;
  double mf = 0.0 ;
 
  M_w_tilde = BladeMassMoment;

  // cyclic flapping relative to shaft axes /SH79/ eqn(43)
  a1s = a_1*cos(beta_orient) + b_1*sin(beta_orient) - b_ic;
  b1s = b_1*cos(beta_orient) - a_1*sin(beta_orient) + a_ic;

  // mind this: no HingeOffset, no additional pitch/roll moments
  mf = 0.5 * (HingeOffset+HingeOffset_hover) * BladeNum * Omega*Omega * M_w_tilde;
  M_s(eL) = mf*b1s;
  M_s(eM) = mf*a1s;
  M_s(eN) = Torque;

  if (flags & eRotCW) {
    M_s(eN) = -M_s(eN);
  }

  if (flags & eCoaxial) {
    M_s(eN) = 0.0;
  }

  M_hub = ShaftToBody * M_s;

  M_h = M_hub + (h_pos * F_h);

  return M_h;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Constructor

FGRotor::FGRotor(FGFDMExec *exec, Element* rotor_element, int num)
                    : FGThruster(exec, rotor_element, num)
{

  FGColumnVector3 location, orientation;
  Element *thruster_element;

  PropertyManager = fdmex->GetPropertyManager();
  dt = fdmex->GetDeltaT();

  /* apply defaults */

  rho = 0.002356; // just a sane value
  
  RPM = 0.0;
  Sense = 1.0;
  tailRotorPresent = false;
  
  effective_tail_col = 0.001; // just a sane value 

  prop_inflow_ratio_lambda =  0.0;
  prop_advance_ratio_mu = 0.0; 
  prop_inflow_ratio_induced_nu = 0.0;
  prop_mr_torque = 0.0;
  prop_thrust_coefficient = 0.0;
  prop_coning_angle = 0.0;

  prop_theta_downwash = prop_phi_downwash = 0.0;

  hover_threshold = 0.0;
  hover_scale = 0.0;

  mr.zero();
  tr.zero();

  // debug stuff
  prop_DumpFlag = 0;

  /* configure */

  Type = ttRotor;
  SetTransformType(FGForce::tCustom);

  // get data from parent and 'mount' the rotor system

  thruster_element = rotor_element->GetParent()->FindElement("sense");
  if (thruster_element) {
    Sense = thruster_element->GetDataAsNumber() >= 0.0 ? 1.0: -1.0;
  }

  thruster_element = rotor_element->GetParent()->FindElement("location");
  if (thruster_element)  location = thruster_element->FindElementTripletConvertTo("IN");
  else          cerr << "No thruster location found." << endl;

  thruster_element = rotor_element->GetParent()->FindElement("orient");
  if (thruster_element)  orientation = thruster_element->FindElementTripletConvertTo("RAD");
  else          cerr << "No thruster orientation found." << endl;

  SetLocation(location);
  SetAnglesToBody(orientation);

  // get main rotor parameters 
  mr.parent = rotor_element;

  int flags = eMain;

  string a_val="";
  a_val = rotor_element->GetAttributeValue("variant");
  if ( a_val == "coaxial" ) {
    flags += eCoaxial;
    cerr << "# found 'coaxial' variant" << endl;
  }  

  if (Sense<0.0) {
    flags += eRotCW;
  }
    
  mr.configure(flags);

  mr.rk.init(0,dt,6);

  // get tail rotor parameters
  tr.parent=rotor_element->FindElement("tailrotor");
  if (tr.parent) {
    tailRotorPresent = true;
  } else {
    tailRotorPresent = false;
    cerr << "# No tailrotor found, assuming a single rotor." << endl;
  }

  if (tailRotorPresent) {
    int flags = eTail;
    if (Sense<0.0) {
      flags += eRotCW;
    }
    tr.configure(flags, &mr);
    tr.rk.init(0,dt,6);
    tr.RpmRatio = tr.NominalRPM/mr.NominalRPM; // 'connect'
  }

  /* remaining parameters */

  // ground effect  
  double c_ground_effect = 0.0;  // uh1 ~ 0.28 , larger values increase the effect
  ground_effect_exp = 0.0;  
  ground_effect_shift = 0.0;

  if (rotor_element->FindElement("cgroundeffect"))
    c_ground_effect = rotor_element->FindElementValueAsNumber("cgroundeffect");

  if (rotor_element->FindElement("groundeffectshift"))
    ground_effect_shift = rotor_element->FindElementValueAsNumberConvertTo("groundeffectshift","FT");

  // prepare calculations, see /TA77/
  if (c_ground_effect > 1e-5) {
    ground_effect_exp = 1.0 / ( 2.0*mr.Radius * c_ground_effect );
  } else {
    ground_effect_exp = 0.0; // disable
  }

  // smooth out jumps in hagl reported, otherwise the ground effect
  // calculation would cause jumps too. 1Hz seems sufficient.
  damp_hagl = Filter(1.0,dt);


  // misc, experimental
  if (rotor_element->FindElement("hoverthreshold"))
    hover_threshold = rotor_element->FindElementValueAsNumberConvertTo("hoverthreshold", "FT/SEC");

  if (rotor_element->FindElement("hoverscale"))
    hover_scale = rotor_element->FindElementValueAsNumber("hoverscale");

  // enable import-export
  bind();

  // unused right now
  prop_rotorbrake->setDoubleValue(0.0);
  prop_freewheel_factor->setDoubleValue(1.0);  

  Debug(0);

}  // Constructor

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRotor::~FGRotor()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// mea-culpa - the connection to the engine might be wrong, but the calling
// convention appears to be 'variable' too.
//   piston call:  
//        return Thruster->Calculate((Eng_HP * hptoftlbssec)-Thruster->GetPowerRequired());
//   turbine call: 
//        Thrust = Thruster->Calculate(Thrust); // allow thruster to modify thrust (i.e. reversing)
//
// Here 'Calculate' takes thrust and estimates the power provided.

double FGRotor::Calculate(double PowerAvailable)
{
  // controls
  double A_IC;       // lateral (roll) control in radians
  double B_IC;       // longitudinal (pitch) control in radians
  double theta_col;  // main rotor collective pitch in radians
  double tail_col;   // tail rotor collective in radians

  // state
  double h_agl_ft = 0.0;
  double Vt ;

  FGColumnVector3 UVW_h;
  FGColumnVector3 PQR_h;

  /* total vehicle velocity including wind effects in feet per second. */
  Vt = fdmex->GetAuxiliary()->GetVt();

  dt = fdmex->GetDeltaT(); // might be variable ?

  dump_req = prop_DumpFlag;
  prop_DumpFlag = 0;

  // fetch often used values
  rho = fdmex->GetAtmosphere()->GetDensity(); // slugs/ft^3.

  UVW_h = fdmex->GetAuxiliary()->GetAeroUVW();
  PQR_h = fdmex->GetAuxiliary()->GetAeroPQR();

  // handle present RPM now, calc omega values.

  if (RPM < mr.MinRPM) { // kludge, otherwise calculations go bananas 
    RPM = mr.MinRPM;
  }

  mr.ActualRPM = RPM;
  mr.Omega = (RPM/60.0)*2.0*M_PI;

  if (tailRotorPresent) {
    tr.ActualRPM = RPM*tr.RpmRatio;
    tr.Omega = (RPM*tr.RpmRatio/60.0)*2.0*M_PI;
  }

  // read control inputs

  A_IC      = prop_lateral_ctrl->getDoubleValue();
  B_IC      = prop_longitudinal_ctrl->getDoubleValue();
  theta_col = prop_collective_ctrl->getDoubleValue();
  tail_col  = 0.0;
  if (tailRotorPresent) {
    tail_col  = prop_antitorque_ctrl->getDoubleValue();
  }


  FGColumnVector3  vHub_ca = mr.hub_vel_body2ca(UVW_h,PQR_h,A_IC,B_IC);
  FGColumnVector3 avFus_ca = mr.fus_angvel_body2ca(PQR_h);


  h_agl_ft = fdmex->GetPropagate()->GetDistanceAGL();

  double filtered_hagl;
  filtered_hagl = damp_hagl.execute( h_agl_ft + ground_effect_shift );

  // gnuplot> plot [-1:50] 1 - exp((-x/44)/.28)
  double ge_factor = 1.0;  
  if (ground_effect_exp > 1e-5) {
    ge_factor -= exp(-filtered_hagl*ground_effect_exp);
  }
  // clamp
  if (ge_factor<0.5) ge_factor=0.5;

  if (dump_req) {
    printf("# GE h: %.3f  (%.3f) f: %f\n", filtered_hagl, h_agl_ft + ground_effect_shift, ge_factor);
  }


  // EXPERIMENTAL: modify rotor for hover, see rotor::body_moments for the consequences
  if (hover_threshold > 1e-5 && Vt < hover_threshold) {
    double scale = 1.0 - Vt/hover_threshold;
    mr.HingeOffset_hover = scale*hover_scale*mr.Radius;
  } else {
    mr.HingeOffset_hover = 0.0;
  }

  // all set, start calculations

  /* MAIN ROTOR */

  mr.calc_flow_and_thrust(dt, rho, theta_col, vHub_ca(eU), vHub_ca(eW), ge_factor);

  prop_inflow_ratio_lambda = mr.lambda;
  prop_advance_ratio_mu = mr.mu;
  prop_inflow_ratio_induced_nu = mr.nu;
  prop_thrust_coefficient = mr.Ct;

  mr.calc_coning_angle(rho, theta_col);
  prop_coning_angle = mr.a0;

  mr.calc_torque(rho, theta_col);
  prop_mr_torque = mr.Torque;

  mr.calc_flapping_angles(rho, theta_col, avFus_ca);
  mr.calc_drag_and_side_forces(rho, theta_col);

  FGColumnVector3 F_h_mr = mr.body_forces(A_IC,B_IC);
  FGColumnVector3 M_h_mr = mr.body_moments(F_h_mr, A_IC, B_IC); 

  // export downwash angles
  // theta: positive for downwash moving from +z_h towards +x_h
  // phi:   positive for downwash moving from +z_h towards -y_h

  prop_theta_downwash = atan2( - UVW_h(eU), mr.v_induced - UVW_h(eW));
  prop_phi_downwash   = atan2(   UVW_h(eV), mr.v_induced - UVW_h(eW));

  mr.force(eX) = F_h_mr(1);
  mr.force(eY) = F_h_mr(2);
  mr.force(eZ) = F_h_mr(3);

  mr.moment(eL) =  M_h_mr(1);
  mr.moment(eM) =  M_h_mr(2); 
  mr.moment(eN) =  M_h_mr(3);

  /* TAIL ROTOR */

  FGColumnVector3 F_h_tr(0.0, 0.0, 0.0);
  FGColumnVector3 M_h_tr(0.0, 0.0, 0.0);

  if (tailRotorPresent) {
    FGColumnVector3  vHub_ca_tr = tr.hub_vel_body2ca(UVW_h,PQR_h);
    FGColumnVector3 avFus_ca_tr = tr.fus_angvel_body2ca(PQR_h);

    tr.calc_flow_and_thrust(dt, rho, tail_col, vHub_ca_tr(eU), vHub_ca_tr(eW));
    tr.calc_coning_angle(rho, tail_col);

    // test code for cantered tail rotor, see if it has a notable effect. /SH79/ eqn(47)
    if (fabs(tr.CantAngleD3)>1e-5) {
      double tan_d3 = tan(tr.CantAngleD3);
      double d_t0t;
      double ca_dt = dt/12.0;
      for (int i = 0; i<12; i++) {
        d_t0t = 1/0.1*(tail_col - tr.a0 * tan_d3 - effective_tail_col);
        effective_tail_col += d_t0t*ca_dt;
      }
    } else {
      effective_tail_col = tail_col;
    }

    tr.calc_torque(rho, effective_tail_col);
    tr.calc_flapping_angles(rho, effective_tail_col, avFus_ca_tr);
    tr.calc_drag_and_side_forces(rho, effective_tail_col);

    F_h_tr = tr.body_forces();
    M_h_tr = tr.body_moments(F_h_tr); 
  }

  tr.force(eX)  =   F_h_tr(1) ;
  tr.force(eY)  =   F_h_tr(2) ;
  tr.force(eZ)  =   F_h_tr(3) ;
  tr.moment(eL) =   M_h_tr(1) ;
  tr.moment(eM) =   M_h_tr(2) ;
  tr.moment(eN) =   M_h_tr(3) ;

/* 
    TODO: 
      check negative mr.Torque conditions
      freewheel factor: assure [0..1] just multiply with available power
      rotorbrake: just steal from available power

*/

  // calculate new RPM, assuming a stiff connection between engine and rotor. 

  double engine_hp     =  PowerAvailable/2.24; // 'undo' force via the estimation factor used in aeromatic
  double engine_torque =  550.0*engine_hp/mr.Omega;
  double Omega_dot     = (engine_torque - mr.Torque) / mr.PolarMoment;

  RPM += ( Omega_dot * dt )/(2.0*M_PI) * 60.0;

  if (0 && dump_req) {
    printf("# SENSE      :  % d % d\n", mr.flags & eRotCW ? -1 : 1, tr.flags & eRotCW ? -1 : 1);
    printf("# vi         :  % f % f\n", mr.v_induced, tr.v_induced);
    printf("# a0 a1 b1   :  % f % f % f\n", mr.a0, mr.a_1, mr.b_1 );
    printf("# m  forces  :  % f % f % f\n", mr.force(eX), mr.force(eY), mr.force(eZ) );
    printf("# m  moments :  % f % f % f\n", mr.moment(eL), mr.moment(eM), mr.moment(eN) );
    printf("# t  forces  :  % f % f % f\n", tr.force(eX), tr.force(eY), tr.force(eZ) );
    printf("# t  moments :  % f % f % f\n", tr.moment(eL), tr.moment(eM), tr.moment(eN) );
  }

  // finally set vFn & vMn
  vFn = mr.force  + tr.force;
  vMn = mr.moment + tr.moment;

  // and just lie here
  Thrust = 0.0; 

  // return unmodified thrust to the turbine. 
  // :TK: As far as I can see the return value is unused.
  return PowerAvailable;

}  // Calculate

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// FGThruster does return 0.0 (the implicit direct thruster)
// piston CALL:  return Thruster->Calculate((Eng_HP * hptoftlbssec)-Thruster->GetPowerRequired());

double FGRotor::GetPowerRequired(void)
{
  PowerRequired = 0.0;
  return PowerRequired;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGRotor::bind(void) {

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);

  PropertyManager->Tie( base_property_name + "/rotor-rpm", this, &FGRotor::GetRPM );
  PropertyManager->Tie( base_property_name + "/thrust-mr-lbs", &mr.Thrust );
  PropertyManager->Tie( base_property_name + "/vi-mr-fps", &mr.v_induced );
  PropertyManager->Tie( base_property_name + "/a0-mr-rad", &mr.a0 );
  PropertyManager->Tie( base_property_name + "/a1-mr-rad", &mr.a1s ); // s means shaft axes
  PropertyManager->Tie( base_property_name + "/b1-mr-rad", &mr.b1s );
  PropertyManager->Tie( base_property_name + "/thrust-tr-lbs", &tr.Thrust );
  PropertyManager->Tie( base_property_name + "/vi-tr-fps", &tr.v_induced );

  // lambda
  PropertyManager->Tie( base_property_name + "/inflow-ratio", &prop_inflow_ratio_lambda );
  // mu
  PropertyManager->Tie( base_property_name + "/advance-ratio", &prop_advance_ratio_mu );
  // nu
  PropertyManager->Tie( base_property_name + "/induced-inflow-ratio", &prop_inflow_ratio_induced_nu );

  PropertyManager->Tie( base_property_name + "/torque-mr-lbsft", &prop_mr_torque );
  PropertyManager->Tie( base_property_name + "/thrust-coefficient", &prop_thrust_coefficient );
  PropertyManager->Tie( base_property_name + "/main-rotor-rpm", &mr.ActualRPM );
  PropertyManager->Tie( base_property_name + "/tail-rotor-rpm", &tr.ActualRPM );

  // position of the downwash
  PropertyManager->Tie( base_property_name + "/theta-downwash-rad", &prop_theta_downwash );
  PropertyManager->Tie( base_property_name + "/phi-downwash-rad", &prop_phi_downwash );  

  // nodes to use via get<xyz>Value
  prop_collective_ctrl = PropertyManager->GetNode(base_property_name + "/collective-ctrl-rad",true);
  prop_lateral_ctrl = PropertyManager->GetNode(base_property_name + "/lateral-ctrl-rad",true);
  prop_longitudinal_ctrl = PropertyManager->GetNode(base_property_name + "/longitudinal-ctrl-rad",true);
  prop_antitorque_ctrl =   PropertyManager->GetNode(base_property_name + "/antitorque-ctrl-rad",true);

  prop_rotorbrake =   PropertyManager->GetNode(base_property_name + "/rotorbrake-hp", true);
  prop_freewheel_factor =   PropertyManager->GetNode(base_property_name + "/freewheel-factor", true);

  PropertyManager->Tie( base_property_name + "/dump-flag", &prop_DumpFlag );

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRotor::GetThrusterLabels(int id, string delimeter)
{

  std::ostringstream buf;

  buf << Name << " RPM (engine " << id << ")";

  return buf.str();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRotor::GetThrusterValues(int id, string delimeter)
{
  std::ostringstream buf;

  buf << RPM;

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGRotor::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n    Rotor Name: " << Name << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGRotor" << endl;
    if (from == 1) cout << "Destroyed:    FGRotor" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim 

