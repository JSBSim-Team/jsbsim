/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRotor.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the rotor object

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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
11/15/10  T.Kreitler treated flow solver bug, flow and torque calculations 
                     simplified, tiploss influence removed from flapping angles
01/10/11  T.Kreitler changed to single rotor model
03/06/11  T.Kreitler added brake, clutch, and experimental free-wheeling-unit,
                     reasonable estimate for inflowlag

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>

#include "FGRotor.h"
#include "input_output/FGXMLElement.h"
#include "models/FGMassBalance.h"

using std::cerr;
using std::endl;
using std::ostringstream;
using std::cout;

namespace JSBSim {

static const char *IdSrc = "$Id: FGRotor.cpp,v 1.17 2011/09/24 14:26:46 jentron Exp $";
static const char *IdHdr = ID_ROTOR;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MISC
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static inline double sqr(double x) { return x*x; }

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Constructor

FGRotor::FGRotor(FGFDMExec *exec, Element* rotor_element, int num)
  : FGThruster(exec, rotor_element, num),
    rho(0.002356),                                  // environment
    Radius(0.0), BladeNum(0),                       // configuration parameters
    Sense(1.0), NominalRPM(0.0), MinimalRPM(0.0), MaximalRPM(0.0), 
    ExternalRPM(0), RPMdefinition(0), ExtRPMsource(NULL),
    BladeChord(0.0), LiftCurveSlope(0.0), BladeTwist(0.0), HingeOffset(0.0),
    BladeFlappingMoment(0.0), BladeMassMoment(0.0), PolarMoment(0.0),
    InflowLag(0.0), TipLossB(0.0),
    GroundEffectExp(0.0), GroundEffectShift(0.0),
    LockNumberByRho(0.0), Solidity(0.0),            // derived parameters
    RPM(0.0), Omega(0.0),                           // dynamic values
    beta_orient(0.0),
    a0(0.0), a_1(0.0), b_1(0.0), a_dw(0.0),
    a1s(0.0), b1s(0.0),
    H_drag(0.0), J_side(0.0), Torque(0.0), C_T(0.0),
    lambda(-0.001), mu(0.0), nu(0.001), v_induced(0.0),
    theta_downwash(0.0), phi_downwash(0.0),
    ControlMap(eMainCtrl),                          // control
    CollectiveCtrl(0.0), LateralCtrl(0.0), LongitudinalCtrl(0.0),
    BrakeCtrlNorm(0.0), MaxBrakePower(0.0),
    FreeWheelPresent(0), FreeWheelThresh(0.0),      // free-wheeling-unit (FWU)
    FreeWheelTransmission(0.0)
{
  FGColumnVector3 location(0.0, 0.0, 0.0), orientation(0.0, 0.0, 0.0);
  Element *thruster_element;

  // initialise/set remaining variables
  SetTransformType(FGForce::tCustom);
  PropertyManager = exec->GetPropertyManager();
  Type = ttRotor;
  GearRatio = 1.0;

  dt = exec->GetDeltaT();
  for (int i=0; i<5; i++) R[i] = 0.0;
  for (int i=0; i<5; i++) B[i] = 0.0;

  // get positions 
  thruster_element = rotor_element->GetParent()->FindElement("sense");
  if (thruster_element) {
    double s = thruster_element->GetDataAsNumber();
    if (s < -0.1) {
      Sense = -1.0; // 'CW' as seen from above
    } else if (s < 0.1) {
      Sense = 0.0;  // 'coaxial'
    } else {
      Sense = 1.0; // 'CCW' as seen from above
    }
  }

  thruster_element = rotor_element->GetParent()->FindElement("location");
  if (thruster_element) {
    location = thruster_element->FindElementTripletConvertTo("IN");
  } else {
    cerr << "No thruster location found." << endl;
  }

  thruster_element = rotor_element->GetParent()->FindElement("orient");
  if (thruster_element) {
    orientation = thruster_element->FindElementTripletConvertTo("RAD");
  } else {
    cerr << "No thruster orientation found." << endl;
  }

  SetLocation(location);
  SetAnglesToBody(orientation);
  InvTransform = Transform().Transposed();

  // wire controls
  ControlMap = eMainCtrl;
  if (rotor_element->FindElement("controlmap")) {
    string cm = rotor_element->FindElementValue("controlmap");
    cm = to_upper(cm);
    if (cm == "TAIL") {
      ControlMap = eTailCtrl;
    } else if (cm == "TANDEM") {
      ControlMap = eTandemCtrl;
    } else {
      cerr << "# found unknown controlmap: '" << cm << "' using main rotor config."  << endl;
    }
  }

  // ExternalRPM -- is the RPM dictated ?
  if (rotor_element->FindElement("ExternalRPM")) {
    ExternalRPM = 1;
    RPMdefinition = (int) rotor_element->FindElementValueAsNumber("ExternalRPM");
  }

  // configure the rotor parameters
  Configure(rotor_element);

  // shaft representation - a rather simple transform, 
  // but using a matrix is safer.
  TboToHsr.InitMatrix(   0.0, 0.0, 1.0,
                         0.0, 1.0, 0.0,
                        -1.0, 0.0, 0.0  );
  HsrToTbo  =  TboToHsr.Transposed();

  // smooth out jumps in hagl reported, otherwise the ground effect
  // calculation would cause jumps too. 1Hz seems sufficient.
  damp_hagl = Filter(1.0, dt);

  // avoid too abrupt changes in power transmission
  FreeWheelLag = Filter(200.0,dt);

  // enable import-export
  BindModel();

  Debug(0);

}  // Constructor

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRotor::~FGRotor(){
  Debug(1);
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 5in1: value-fetch-convert-default-return function 

double FGRotor::ConfigValueConv( Element* el, const string& ename, double default_val, 
                                  const string& unit, bool tell)
{

  Element *e = NULL;
  double val = default_val;

  string pname = "*No parent element*";

  if (el) {
    e = el->FindElement(ename);
    pname = el->GetName();
  }

  if (e) {
    if (unit.empty()) {
      val = e->GetDataAsNumber();
    } else {
      val = el->FindElementValueAsNumberConvertTo(ename,unit);
    }
  } else {
    if (tell) {
      cerr << pname << ": missing element '" << ename <<
                       "' using estimated value: " << default_val << endl;
    }
  }

  return val;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRotor::ConfigValue(Element* el, const string& ename, double default_val, bool tell)
{
  return ConfigValueConv(el, ename, default_val, "", tell);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// 1. read configuration and try to fill holes, ymmv
// 2. calculate derived parameters
void FGRotor::Configure(Element* rotor_element)
{

  double estimate;
  const bool yell   = true;
  const bool silent = false;


  Radius = 0.5 * ConfigValueConv(rotor_element, "diameter", 42.0, "FT", yell); 
  Radius = Constrain(1e-3, Radius, 1e9);
  
  BladeNum = (int) ConfigValue(rotor_element, "numblades", 3 , yell);
  
  GearRatio = ConfigValue(rotor_element, "gearratio", 1.0, yell);

  // make sure that v_tip (omega*r) is below 0.7mach ~ 750ft/s
  estimate = (750.0/Radius)/(2.0*M_PI) * 60.0;  // 7160/Radius
  NominalRPM = ConfigValue(rotor_element, "nominalrpm", estimate, yell);
  NominalRPM = Constrain(2.0, NominalRPM, 1e9);

  MinimalRPM = ConfigValue(rotor_element, "minrpm", 1.0);
  MinimalRPM = Constrain(1.0, MinimalRPM, NominalRPM - 1.0);

  MaximalRPM = ConfigValue(rotor_element, "maxrpm", 2.0*NominalRPM);
  MaximalRPM = Constrain(NominalRPM, MaximalRPM, 1e9);

  estimate = Constrain(0.07, 2.0/Radius , 0.14); // guess solidity
  estimate = estimate * M_PI*Radius/BladeNum;
  BladeChord = ConfigValueConv(rotor_element, "chord", estimate, "FT", yell);

  LiftCurveSlope = ConfigValue(rotor_element, "liftcurveslope", 6.0); // "1/RAD"
  BladeTwist = ConfigValueConv(rotor_element, "twist", -0.17, "RAD");

  HingeOffset = ConfigValueConv(rotor_element, "hingeoffset", 0.05 * Radius, "FT" );

  estimate = sqr(BladeChord) * sqr(Radius - HingeOffset) * 0.57;
  BladeFlappingMoment = ConfigValueConv(rotor_element, "flappingmoment", estimate, "SLUG*FT2");   
  BladeFlappingMoment = Constrain(1.0e-6, BladeFlappingMoment, 1e9);

  // guess mass from moment of a thin stick, and multiply by the blades cg distance
  estimate = ( 3.0 * BladeFlappingMoment / sqr(Radius) ) * (0.45 * Radius) ;
  BladeMassMoment = ConfigValue(rotor_element, "massmoment", estimate); // unit is slug-ft
  BladeMassMoment = Constrain(0.001, BladeMassMoment, 1e9);

  estimate = 1.1 * BladeFlappingMoment * BladeNum;
  PolarMoment = ConfigValueConv(rotor_element, "polarmoment", estimate, "SLUG*FT2");
  PolarMoment = Constrain(1e-6, PolarMoment, 1e9);

  // "inflowlag" is treated further down.

  TipLossB = ConfigValue(rotor_element, "tiplossfactor", 1.0, silent);

  estimate = 0.01 * PolarMoment ; // guesses for huey, bo105 20-30hp
  MaxBrakePower  = ConfigValueConv(rotor_element, "maxbrakepower", estimate, "HP");
  MaxBrakePower *= hptoftlbssec;

  // ground effect
  if (rotor_element->FindElement("cgroundeffect")) {
    double cge,gee;
    cge = rotor_element->FindElementValueAsNumber("cgroundeffect");
    cge = Constrain(1e-9, cge, 1.0);
    gee = 1.0 / ( 2.0*Radius * cge );
    cerr << "# *** 'cgroundeffect' is defunct." << endl;
    cerr << "# *** use 'groundeffectexp' with: " << gee << endl;
  }

  GroundEffectExp = ConfigValue(rotor_element, "groundeffectexp", 0.0);
  GroundEffectShift = ConfigValueConv(rotor_element, "groundeffectshift", 0.0, "FT");

  // handle optional free-wheeling-unit (FWU)
  FreeWheelPresent = 0;
  FreeWheelTransmission = 1.0;
  if (rotor_element->FindElement("freewheelthresh")) {
    FreeWheelThresh = rotor_element->FindElementValueAsNumber("freewheelthresh");
    if (FreeWheelThresh > 1.0) {
      FreeWheelPresent = 1;
      FreeWheelTransmission = 0.0;
    }
  }

  // precalc often used powers
  R[0]=1.0; R[1]=Radius;   R[2]=R[1]*R[1]; R[3]=R[2]*R[1]; R[4]=R[3]*R[1];
  B[0]=1.0; B[1]=TipLossB; B[2]=B[1]*B[1]; B[3]=B[2]*B[1]; B[4]=B[3]*B[1];

  // derived parameters
  LockNumberByRho = LiftCurveSlope * BladeChord * R[4] / BladeFlappingMoment;
  Solidity = BladeNum * BladeChord / (M_PI * Radius);

  // estimate inflow lag, see /GE49/ eqn(1)
  double omega_tmp = (NominalRPM/60.0)*2.0*M_PI;
  estimate = 16.0/(LockNumberByRho*rho * omega_tmp ); // 16/(gamma*Omega)
  // printf("# Est. InflowLag: %f\n", estimate);
  InflowLag = ConfigValue(rotor_element, "inflowlag", estimate, yell);
  InflowLag = Constrain(1.0e-6, InflowLag, 2.0);

  return;
} // Configure

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// calculate control-axes components of total airspeed at the hub.
// sets rotor orientation angle (beta) as side effect. /SH79/ eqn(19-22)

FGColumnVector3 FGRotor::hub_vel_body2ca( const FGColumnVector3 &uvw, 
                                                 const FGColumnVector3 &pqr,
                                                 double a_ic, double b_ic)
{
  FGColumnVector3  v_r, v_shaft, v_w;
  FGColumnVector3 pos;

  pos = fdmex->GetMassBalance()->StructuralToBody(GetActingLocation());

  v_r = uvw + pqr*pos;
  v_shaft = TboToHsr * InvTransform * v_r;

  beta_orient = atan2(v_shaft(eV),v_shaft(eU));

  v_w(eU) = v_shaft(eU)*cos(beta_orient) + v_shaft(eV)*sin(beta_orient);
  v_w(eV) = 0.0;
  v_w(eW) = v_shaft(eW) - b_ic*v_shaft(eU) - a_ic*v_shaft(eV);

  return v_w;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// express fuselage angular velocity in control axes /SH79/ eqn(30,31)

FGColumnVector3 FGRotor::fus_angvel_body2ca( const FGColumnVector3 &pqr)
{
  FGColumnVector3 av_s_fus, av_w_fus;    

  // for comparison:
  // av_s_fus = BodyToShaft * pqr; /SH79/ 
  // BodyToShaft = TboToHsr * InvTransform
  av_s_fus = TboToHsr * InvTransform * pqr;

  av_w_fus(eP)=   av_s_fus(eP)*cos(beta_orient) + av_s_fus(eQ)*sin(beta_orient);
  av_w_fus(eQ)= - av_s_fus(eP)*sin(beta_orient) + av_s_fus(eQ)*cos(beta_orient);
  av_w_fus(eR)=   av_s_fus(eR);

  return av_w_fus;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// The calculation is a bit tricky because thrust depends on induced velocity,
// and vice versa.
//
// The flow_scale parameter (ranging from 0.5-1.0) is used to approximate a
// reduction of inflow if the helicopter is close to the ground, yielding to
// higher thrust, see /TA77/ eqn(10a).

void FGRotor::calc_flow_and_thrust( double theta_0, double Uw, double Ww, 
                                    double flow_scale)
{

  double ct_over_sigma = 0.0;
  double c0, ct_l, ct_t0, ct_t1;
  double mu2;

  mu = Uw/(Omega*Radius); // /SH79/ eqn(24)
  if (mu > 0.7) mu = 0.7;
  mu2 = sqr(mu);
  
  ct_t0 = (1.0/3.0*B[3] + 1.0/2.0 * TipLossB*mu2 - 4.0/(9.0*M_PI) * mu*mu2 ) * theta_0;
  ct_t1 = (1.0/4.0*B[4] + 1.0/4.0 * B[2]*mu2) * BladeTwist;

  ct_l  = (1.0/2.0*B[2] + 1.0/4.0 * mu2) * lambda; // first time

  c0 = (LiftCurveSlope/2.0)*(ct_l + ct_t0 + ct_t1) * Solidity;
  c0 = c0 / ( 2.0 * sqrt( sqr(mu) + sqr(lambda) ) + 1e-15);

  // replacement for /SH79/ eqn(26).
  // ref: dnu/dt = 1/tau ( Ct / (2*sqrt(mu^2+lambda^2))  -  nu )
  // taking mu and lambda constant, this integrates to

  nu  = flow_scale * ((nu - c0) * exp(-dt/InflowLag) + c0);

  // now from nu to lambda, C_T, and Thrust

  lambda = Ww/(Omega*Radius) - nu; // /SH79/ eqn(25)

  ct_l  = (1.0/2.0*B[2] + 1.0/4.0 * mu2) * lambda;

  ct_over_sigma = (LiftCurveSlope/2.0)*(ct_l + ct_t0 + ct_t1); // /SH79/ eqn(27)

  Thrust = BladeNum*BladeChord*Radius*rho*sqr(Omega*Radius) * ct_over_sigma;

  C_T = ct_over_sigma * Solidity;
  v_induced = nu * (Omega*Radius);

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// The coning angle doesn't apply for teetering rotors, but calculating
// doesn't hurt. /SH79/ eqn(29)

void FGRotor::calc_coning_angle(double theta_0)
{
  double lock_gamma = LockNumberByRho * rho;

  double a0_l  = (1.0/6.0  + 0.04 * mu*mu*mu) * lambda;
  double a0_t0 = (1.0/8.0  + 1.0/8.0  * mu*mu) * theta_0;
  double a0_t1 = (1.0/10.0 + 1.0/12.0 * mu*mu) * BladeTwist;
  a0 = lock_gamma * ( a0_l + a0_t0 + a0_t1);
  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Flapping angles relative to control axes /SH79/ eqn(32)

void FGRotor::calc_flapping_angles(double theta_0, const FGColumnVector3 &pqr_fus_w)
{
  double lock_gamma = LockNumberByRho * rho;


  double mu2_2 = sqr(mu)/2.0;
  double t075 = theta_0 + 0.75 * BladeTwist;  // common approximation for rectangular blades
  
  a_1 = 1.0/(1.0 - mu2_2) * (
                                 (2.0*lambda + (8.0/3.0)*t075)*mu
                               + pqr_fus_w(eP)/Omega
                               - 16.0 * pqr_fus_w(eQ)/(lock_gamma*Omega)
                             );

  b_1 = 1.0/(1.0 + mu2_2) * (
                                 (4.0/3.0)*mu*a0
                               - pqr_fus_w(eQ)/Omega
                               - 16.0 * pqr_fus_w(eP)/(lock_gamma*Omega)
                             );

  // used in  force calc
  a_dw = 1.0/(1.0 - mu2_2) * (
                                 (2.0*lambda + (8.0/3.0)*t075)*mu
                               - 24.0 * pqr_fus_w(eQ)/(lock_gamma*Omega)
                                 * ( 1.0 - ( 0.29 * t075 / (C_T/Solidity) ) )
                             );

  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// /SH79/ eqn(38,39)

void FGRotor::calc_drag_and_side_forces(double theta_0)
{
  double cy_over_sigma;
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

// Simplified version of /SH79/ eqn(36). Uses an estimate for blade drag
// (a new config parameter to come...).
// From "Bramwell's Helicopter Dynamics", second edition, eqn(3.43) and (3.44)

void FGRotor::calc_torque(double theta_0)
{
  // estimate blade drag
  double delta_dr = 0.009 + 0.3*sqr(6.0*C_T/(LiftCurveSlope*Solidity));

  Torque = rho * BladeNum * BladeChord * delta_dr * sqr(Omega*Radius) * R[2] * 
           (1.0+4.5*sqr(mu))/8.0
                     - (Thrust*lambda + H_drag*mu)*Radius;

  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// transform rotor forces from control axes to shaft axes, and express
// in body axes /SH79/ eqn(40,41)

FGColumnVector3 FGRotor::body_forces(double a_ic, double b_ic)
{
  FGColumnVector3 F_s(
        - H_drag*cos(beta_orient) - J_side*sin(beta_orient) + Thrust*b_ic,
        - H_drag*sin(beta_orient) + J_side*cos(beta_orient) + Thrust*a_ic,
        - Thrust);    

  return HsrToTbo * F_s;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// calculates the additional moments due to hinge offset and handles 
// torque and sense

FGColumnVector3 FGRotor::body_moments(double a_ic, double b_ic)
{
  FGColumnVector3 M_s, M_hub, M_h;
  double mf;

  // cyclic flapping relative to shaft axes /SH79/ eqn(43)
  a1s = a_1*cos(beta_orient) + b_1*sin(beta_orient) - b_ic;
  b1s = b_1*cos(beta_orient) - a_1*sin(beta_orient) + a_ic;

  mf = 0.5 * HingeOffset * BladeNum * Omega*Omega * BladeMassMoment;

  M_s(eL) = mf*b1s;
  M_s(eM) = mf*a1s;
  M_s(eN) = Torque * Sense ;

  return HsrToTbo * M_s;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGRotor::CalcStatePart1(void)
{
  double A_IC;       // lateral (roll) control in radians
  double B_IC;       // longitudinal (pitch) control in radians
  double theta_col;  // rotor collective pitch in radians

  FGColumnVector3 vHub_ca, avFus_ca;

  double filtered_hagl = 0.0;
  double ge_factor = 1.0;

  // fetch needed values from environment
  rho = in.Density; // slugs/ft^3.
  double h_agl_ft = in.H_agl;
  // update InvTransform, the rotor orientation could have been altered
  InvTransform = Transform().Transposed();

  // handle RPM requirements, calc omega.
  if (ExternalRPM && ExtRPMsource) {
    RPM = ExtRPMsource->getDoubleValue() / GearRatio;
  }

  // MinimalRPM is always >= 1. MaximalRPM is always >= NominalRPM
  RPM = Constrain(MinimalRPM, RPM, MaximalRPM);

  Omega = (RPM/60.0)*2.0*M_PI;

  // set control inputs
  A_IC      = LateralCtrl;
  B_IC      = LongitudinalCtrl;
  theta_col = CollectiveCtrl;

  // ground effect
  if (GroundEffectExp > 1e-5) {
    if (h_agl_ft<0.0) h_agl_ft = 0.0; // clamp
    filtered_hagl = damp_hagl.execute(h_agl_ft) + GroundEffectShift;
    // actual/nominal factor avoids absurd scales at startup
    ge_factor -= exp(-filtered_hagl*GroundEffectExp) * (RPM / NominalRPM);
    if (ge_factor<0.5) ge_factor=0.5; // clamp
  }

  // all set, start calculations

  vHub_ca  = hub_vel_body2ca(in.AeroUVW, in.AeroPQR, A_IC, B_IC);

  avFus_ca = fus_angvel_body2ca(in.AeroPQR);

  calc_flow_and_thrust(theta_col, vHub_ca(eU), vHub_ca(eW), ge_factor);

  calc_coning_angle(theta_col);

  calc_flapping_angles(theta_col, avFus_ca);

  calc_drag_and_side_forces(theta_col);

  calc_torque(theta_col);

  // Fixme: only valid for a 'decent' rotor
  theta_downwash = atan2( -in.AeroUVW(eU), v_induced - in.AeroUVW(eW));
  phi_downwash   = atan2(  in.AeroUVW(eV), v_induced - in.AeroUVW(eW));

  vFn = body_forces(A_IC, B_IC);
  vMn = Transform() * body_moments(A_IC, B_IC); 

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGRotor::CalcStatePart2(double PowerAvailable)
{
  if (! ExternalRPM) {
    // calculate new RPM
    double ExcessTorque = PowerAvailable / Omega;
    double deltaOmega   = ExcessTorque / PolarMoment * in.TotalDeltaT;
    RPM += deltaOmega/(2.0*M_PI) * 60.0;
  }
  RPM = Constrain(MinimalRPM, RPM, MaximalRPM); // trim again
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Simulation of a free-wheeling-unit (FWU). Might need improvements.

void FGRotor::calc_freewheel_state(double p_source, double p_load) {

  // engine is off/detached, release.
  if (p_source<1e-3) { 
    FreeWheelTransmission = 0.0;
    return;
  }

  // engine is driving the rotor, engage.
  if (p_source >= p_load) {
    FreeWheelTransmission = 1.0;
    return;
  }

  // releases if engine is detached, but stays calm if
  // the load changes due to rotor dynamics.
  if (p_source > 0.0 && p_load/(p_source+0.1) > FreeWheelThresh ) {
    FreeWheelTransmission = 0.0;
    return;
  }

  return;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRotor::Calculate(double EnginePower)
{
  double FWmult = 1.0;
  double DeltaPower;

  CalcStatePart1();

  PowerRequired = Torque * Omega + BrakeCtrlNorm * MaxBrakePower;

  if (FreeWheelPresent) {
    calc_freewheel_state(EnginePower * ClutchCtrlNorm, PowerRequired);
    FWmult = FreeWheelLag.execute(FreeWheelTransmission);
  }

  DeltaPower = EnginePower * ClutchCtrlNorm * FWmult - PowerRequired;

  CalcStatePart2(DeltaPower);

  return Thrust;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


bool FGRotor::BindModel(void)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);

  property_name = base_property_name + "/rotor-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetRPM );

  property_name = base_property_name + "/engine-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetEngineRPM );

  property_name = base_property_name + "/rotor-thrust-lbs"; // might be redundant - check!
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetThrust );

  property_name = base_property_name + "/a0-rad";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetA0 );

  property_name = base_property_name + "/a1-rad";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetA1 );

  property_name = base_property_name + "/b1-rad";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetB1 );

  property_name = base_property_name + "/inflow-ratio";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetLambda );

  property_name = base_property_name + "/advance-ratio";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetMu );

  property_name = base_property_name + "/induced-inflow-ratio";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetNu );

  property_name = base_property_name + "/vi-fps";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetVi );

  property_name = base_property_name + "/thrust-coefficient";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetCT );

  property_name = base_property_name + "/torque-lbsft";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetTorque );

  property_name = base_property_name + "/theta-downwash-rad";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetThetaDW );

  property_name = base_property_name + "/phi-downwash-rad";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetPhiDW );

  switch (ControlMap) {
    case eTailCtrl:
      property_name = base_property_name + "/antitorque-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetCollectiveCtrl, &FGRotor::SetCollectiveCtrl);
      break;
    case eTandemCtrl:
      property_name = base_property_name + "/tail-collective-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetCollectiveCtrl, &FGRotor::SetCollectiveCtrl);
      property_name = base_property_name + "/lateral-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetLateralCtrl, &FGRotor::SetLateralCtrl);
      property_name = base_property_name + "/longitudinal-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetLongitudinalCtrl, &FGRotor::SetLongitudinalCtrl);
      break;
    default: // eMainCtrl
      property_name = base_property_name + "/collective-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetCollectiveCtrl, &FGRotor::SetCollectiveCtrl);
      property_name = base_property_name + "/lateral-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetLateralCtrl, &FGRotor::SetLateralCtrl);
      property_name = base_property_name + "/longitudinal-ctrl-rad";
      PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetLongitudinalCtrl, &FGRotor::SetLongitudinalCtrl);
  }

  property_name = base_property_name + "/brake-ctrl-norm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetBrakeCtrl, &FGRotor::SetBrakeCtrl);
  property_name = base_property_name + "/free-wheel-transmission";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetFreeWheelTransmission);

  if (ExternalRPM) {
    if (RPMdefinition == -1) {
      property_name = base_property_name + "/x-rpm-dict";
      ExtRPMsource = PropertyManager->GetNode(property_name, true);
    } else if (RPMdefinition >= 0 && RPMdefinition != EngineNum) {
      string ipn = CreateIndexedPropertyName("propulsion/engine", RPMdefinition);
      property_name = ipn + "/engine-rpm";
      ExtRPMsource = PropertyManager->GetNode(property_name, false);
      if (! ExtRPMsource) {
        cerr << "# Warning: Engine number " << EngineNum << "." << endl;
        cerr << "# No 'engine-rpm' property found for engine " << RPMdefinition << "." << endl;
        cerr << "# Please check order of engine definitons."  << endl;
      }
    } else {
      cerr << "# Engine number " << EngineNum;
      cerr << ", given ExternalRPM value '" << RPMdefinition << "' unhandled."  << endl;
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRotor::GetThrusterLabels(int id, string delimeter)
{

  ostringstream buf;

  buf << Name << " RPM (engine " << id << ")";

  return buf.str();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRotor::GetThrusterValues(int id, string delimeter)
{

  ostringstream buf;

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
  string ControlMapName;

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n    Rotor Name: " << Name << endl;
      cout << "      Diameter = " << 2.0 * Radius << " ft." << endl;
      cout << "      Number of Blades = " << BladeNum << endl;
      cout << "      Gear Ratio = " << GearRatio << endl;
      cout << "      Sense = " << Sense << endl;
      cout << "      Nominal RPM = " << NominalRPM << endl;
      cout << "      Minimal RPM = " << MinimalRPM << endl;
      cout << "      Maximal RPM = " << MaximalRPM << endl;

      if (ExternalRPM) {
        if (RPMdefinition == -1) {
          cout << "      RPM is controlled externally" << endl;
        } else {
          cout << "      RPM source set to engine " << RPMdefinition << endl;
        }
      }

      cout << "      Blade Chord = " << BladeChord << endl;
      cout << "      Lift Curve Slope = " << LiftCurveSlope << endl;
      cout << "      Blade Twist = " << BladeTwist << endl;
      cout << "      Hinge Offset = " << HingeOffset << endl;
      cout << "      Blade Flapping Moment = " << BladeFlappingMoment << endl;
      cout << "      Blade Mass Moment = " << BladeMassMoment << endl;
      cout << "      Polar Moment = " << PolarMoment << endl;
      cout << "      Inflow Lag = " << InflowLag << endl;
      cout << "      Tip Loss = " << TipLossB << endl;
      cout << "      Lock Number = " << LockNumberByRho * 0.002356 << " (SL)" << endl;
      cout << "      Solidity = " << Solidity << endl;
      cout << "      Max Brake Power = " << MaxBrakePower/hptoftlbssec << " HP" << endl;

      switch (ControlMap) {
        case eTailCtrl:    ControlMapName = "Tail Rotor";   break;
        case eTandemCtrl:  ControlMapName = "Tandem Rotor"; break;
        default:           ControlMapName = "Main Rotor";
      }
      cout << "      Control Mapping = " << ControlMapName << endl;

      if (FreeWheelPresent) {
        cout << "      Free Wheel Threshold = " << FreeWheelThresh << endl;
      } else {
        cout << "      No FWU present" << endl;
      }

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

