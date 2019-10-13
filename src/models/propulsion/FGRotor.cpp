/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRotor.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the rotor object

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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
02/05/12  T.Kreitler brake, clutch, and FWU now in FGTransmission, 
                     downwash angles relate to shaft orientation

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <sstream>

#include "FGRotor.h"
#include "models/FGMassBalance.h"
#include "models/FGPropulsion.h" // to get the GearRatio from a linked rotor
#include "input_output/FGXMLElement.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::ostringstream;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MISC
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static inline double sqr(double x) { return x*x; }

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Constructor

FGRotor::FGRotor(FGFDMExec *exec, Element* rotor_element, int num)
  : FGThruster(exec, rotor_element, num),
    rho(0.002356),                                  // environment
    Radius(0.0), BladeNum(0),                       // configuration parameters
    Sense(1.0), NominalRPM(0.0), MinimalRPM(0.0), MaximalRPM(0.0), 
    ExternalRPM(0), RPMdefinition(0), ExtRPMsource(NULL), SourceGearRatio(1.0),
    BladeChord(0.0), LiftCurveSlope(0.0), BladeTwist(0.0), HingeOffset(0.0),
    BladeFlappingMoment(0.0), BladeMassMoment(0.0), PolarMoment(0.0),
    InflowLag(0.0), TipLossB(0.0),
    GroundEffectExp(0.0), GroundEffectShift(0.0), GroundEffectScaleNorm(1.0),
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
    Transmission(NULL),                             // interaction with engine
    EngineRPM(0.0), MaxBrakePower(0.0), GearLoss(0.0), GearMoment(0.0)
{
  FGColumnVector3 location(0.0, 0.0, 0.0), orientation(0.0, 0.0, 0.0);
  Element *thruster_element;
  double engine_power_est = 0.0;

  // initialise/set remaining variables
  SetTransformType(FGForce::tCustom);
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
  InvTransform = Transform().Transposed(); // body to custom/native

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
    SourceGearRatio = 1.0;
    RPMdefinition = (int) rotor_element->FindElementValueAsNumber("ExternalRPM");
    int rdef = RPMdefinition;
    if (RPMdefinition>=0) {
      // avoid ourself and (still) unknown engines.
      if (!exec->GetPropulsion()->GetEngine(RPMdefinition) || RPMdefinition==num) {
        RPMdefinition = -1;
      } else {
        FGThruster *tr = exec->GetPropulsion()->GetEngine(RPMdefinition)->GetThruster();
        SourceGearRatio = tr->GetGearRatio();
        // cout << "# got sources' GearRatio: " << SourceGearRatio << endl;
      }
    }
    if (RPMdefinition != rdef) {
      cerr << "# discarded given RPM source (" << rdef << ") and switched to external control (-1)." << endl;
    }
  }

  // process rotor parameters
  engine_power_est = Configure(rotor_element);

  // setup transmission if needed
  if (!ExternalRPM) {

    Transmission = new FGTransmission(exec, num, dt);

    Transmission->SetThrusterMoment(PolarMoment);

    // The MOI sensed behind the gear ( MOI_engine*sqr(GearRatio) ).
    GearMoment = ConfigValueConv(rotor_element, "gearmoment", 0.1*PolarMoment, "SLUG*FT2");
    GearMoment = Constrain(1e-6, GearMoment, 1e9);
    Transmission->SetEngineMoment(GearMoment);

    Transmission->SetMaxBrakePower(MaxBrakePower);

    GearLoss = ConfigValueConv(rotor_element, "gearloss", 0.0025 * engine_power_est, "HP");
    GearLoss = Constrain(0.0, GearLoss, 1e9);
    GearLoss *= hptoftlbssec;
    Transmission->SetEngineFriction(GearLoss);

  }

  // shaft representation - a rather simple transform,
  // but using a matrix is safer.
  TboToHsr = { 0.0, 0.0, 1.0,
               0.0, 1.0, 0.0,
               -1.0, 0.0, 0.0 };
  HsrToTbo  =  TboToHsr.Transposed();

  // smooth out jumps in hagl reported, otherwise the ground effect
  // calculation would cause jumps too. 1Hz seems sufficient.
  damp_hagl = Filter(1.0, dt);

  // enable import-export
  bindmodel(exec->GetPropertyManager());

  Debug(0);

}  // Constructor

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRotor::~FGRotor(){
  if (Transmission) delete Transmission;
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
double FGRotor::Configure(Element* rotor_element)
{

  double estimate, engine_power_est=0.0;
  const bool yell   = true;
  const bool silent = false;


  Radius = 0.5 * ConfigValueConv(rotor_element, "diameter", 42.0, "FT", yell);
  Radius = Constrain(1e-3, Radius, 1e9);
  
  BladeNum = (int) ConfigValue(rotor_element, "numblades", 3 , yell);
  
  GearRatio = ConfigValue(rotor_element, "gearratio", 1.0, yell);
  GearRatio = Constrain(1e-9, GearRatio, 1e9);

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
  BladeFlappingMoment = Constrain(1e-9, BladeFlappingMoment, 1e9);

  // guess mass from moment of a thin stick, and multiply by the blades cg distance
  estimate = ( 3.0 * BladeFlappingMoment / sqr(Radius) ) * (0.45 * Radius) ;
  BladeMassMoment = ConfigValue(rotor_element, "massmoment", estimate); // unit is slug-ft
  BladeMassMoment = Constrain(1e-9, BladeMassMoment, 1e9);

  estimate = 1.1 * BladeFlappingMoment * BladeNum;
  PolarMoment = ConfigValueConv(rotor_element, "polarmoment", estimate, "SLUG*FT2");
  PolarMoment = Constrain(1e-9, PolarMoment, 1e9);

  // "inflowlag" is treated further down.

  TipLossB = ConfigValue(rotor_element, "tiplossfactor", 1.0, silent);

  // estimate engine power (bit of a pity, cause our caller already knows)
  engine_power_est = 0.5 * BladeNum*BladeChord*Radius*Radius;

  estimate = engine_power_est / 30.0;
  MaxBrakePower  = ConfigValueConv(rotor_element, "maxbrakepower", estimate, "HP");
  MaxBrakePower *= hptoftlbssec;

  GroundEffectExp = ConfigValue(rotor_element, "groundeffectexp", 0.0);
  GroundEffectShift = ConfigValueConv(rotor_element, "groundeffectshift", 0.0, "FT");

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
  InflowLag = Constrain(1e-6, InflowLag, 2.0);

  return engine_power_est;
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

// Two blade teetering rotors are often 'preconed' to a fixed angle, but the 
// calculated value is pretty close to the real one. /SH79/ eqn(29)

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

// Get the downwash angles with respect to the shaft axis.
// Given a 'regular'  main rotor, the angles are zero when the downwash points
// down, positive theta values mean that the downwash turns towards the nose,
// and positive phi values mean it turns to the left side. (Note: only airspeed
// is transformed, the rotational speed contribution is ignored.)

void FGRotor::calc_downwash_angles()
{
  FGColumnVector3 v_shaft;
  v_shaft = TboToHsr * InvTransform * in.AeroUVW;

  theta_downwash = atan2( -v_shaft(eU), v_induced - v_shaft(eW)) + a1s;
  phi_downwash   = atan2(  v_shaft(eV), v_induced - v_shaft(eW)) + b1s;

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

void FGRotor::CalcRotorState(void)
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
    RPM = ExtRPMsource->getDoubleValue() * ( SourceGearRatio / GearRatio );
  }

  // MinimalRPM is always >= 1. MaximalRPM is always >= NominalRPM
  RPM = Constrain(MinimalRPM, RPM, MaximalRPM);

  Omega = (RPM/60.0)*2.0*M_PI;

  // set control inputs
  A_IC      = LateralCtrl;
  B_IC      = LongitudinalCtrl;
  theta_col = CollectiveCtrl;

  // optional ground effect, a ge_factor of 1.0 gives no effect
  // and 0.5 yields to maximal influence.
  if (GroundEffectExp > 1e-5) {
    if (h_agl_ft<0.0) h_agl_ft = 0.0; // clamp
    filtered_hagl = damp_hagl.execute(h_agl_ft) + GroundEffectShift;
    // actual/nominal factor avoids absurd scales at startup
    ge_factor -= GroundEffectScaleNorm *
                 ( exp(-filtered_hagl*GroundEffectExp) * (RPM / NominalRPM) );
    ge_factor = Constrain(0.5, ge_factor, 1.0);
  }

  // all set, start calculations ...

  vHub_ca  = hub_vel_body2ca(in.AeroUVW, in.AeroPQR, A_IC, B_IC);

  avFus_ca = fus_angvel_body2ca(in.AeroPQR);

  calc_flow_and_thrust(theta_col, vHub_ca(eU), vHub_ca(eW), ge_factor);

  calc_coning_angle(theta_col);

  calc_flapping_angles(theta_col, avFus_ca);

  calc_drag_and_side_forces(theta_col);

  calc_torque(theta_col);

  calc_downwash_angles();

  // ... and assign to inherited vFn and vMn members
  //     (for processing see FGForce::GetBodyForces).
  vFn = body_forces(A_IC, B_IC);
  vMn = Transform() * body_moments(A_IC, B_IC);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGRotor::Calculate(double EnginePower)
{

  CalcRotorState();

  if (! ExternalRPM) {
    // the RPM values are handled inside Transmission
    Transmission->Calculate(EnginePower, Torque, in.TotalDeltaT);

    EngineRPM = Transmission->GetEngineRPM() * GearRatio;
    RPM = Transmission->GetThrusterRPM();
  } else {
    EngineRPM = RPM * GearRatio;
  }

  RPM = Constrain(MinimalRPM, RPM, MaximalRPM); // trim again

  return Thrust;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


bool FGRotor::bindmodel(FGPropertyManager* PropertyManager)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);

  property_name = base_property_name + "/rotor-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetRPM );

  property_name = base_property_name + "/engine-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetEngineRPM );

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

  property_name = base_property_name + "/groundeffect-scale-norm";
  PropertyManager->Tie( property_name.c_str(), this, &FGRotor::GetGroundEffectScaleNorm,
                                                     &FGRotor::SetGroundEffectScaleNorm );

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

  if (ExternalRPM) {
    if (RPMdefinition == -1) {
      property_name = base_property_name + "/x-rpm-dict";
      ExtRPMsource = PropertyManager->GetNode(property_name, true);
    } else if (RPMdefinition >= 0 && RPMdefinition != EngineNum) {
      string ipn = CreateIndexedPropertyName("propulsion/engine", RPMdefinition);
      property_name = ipn + "/rotor-rpm";
      ExtRPMsource = PropertyManager->GetNode(property_name, false);
      if (! ExtRPMsource) {
        cerr << "# Warning: Engine number " << EngineNum << "." << endl;
        cerr << "# No 'rotor-rpm' property found for engine " << RPMdefinition << "." << endl;
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

string FGRotor::GetThrusterLabels(int id, const string& delimeter)
{

  ostringstream buf;

  buf << Name << " RPM (engine " << id << ")";

  return buf.str();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGRotor::GetThrusterValues(int id, const string& delimeter)
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
          cout << "      RPM source set to thruster " << RPMdefinition << endl;
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
      cout << "      Gear Loss = " << GearLoss/hptoftlbssec << " HP" << endl;
      cout << "      Gear Moment = " << GearMoment << endl;

      switch (ControlMap) {
        case eTailCtrl:    ControlMapName = "Tail Rotor";   break;
        case eTandemCtrl:  ControlMapName = "Tandem Rotor"; break;
        default:           ControlMapName = "Main Rotor";
      }
      cout << "      Control Mapping = " << ControlMapName << endl;

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
    }
  }

}

} // namespace JSBSim
