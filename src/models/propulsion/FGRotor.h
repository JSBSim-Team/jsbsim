/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRotor.h
 Author:       T. Kreitler
 Date started: 08/24/00

 ------------- Copyright (C) 2010  T. Kreitler -------------

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
01/01/10  T.Kreitler test implementation

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGROTOR_H
#define FGROTOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGThruster.h"
#include "math/FGTable.h"
#include "math/FGRungeKutta.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ROTOR "$Id: FGRotor.h,v 1.6 2010/06/02 04:05:13 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a rotor system. The default configuration consists of main and
    tail rotor. A practical way to define the positions is to start with an
    imaginary gear-box near the cg of the vehicle. 

    In this case the location in the thruster definition should be
    approximately equal to the cg defined in the <tt>fdm_config/mass_balance</tt>
    section. If the default orientation (roll=pitch=yaw=0) is used the
    positions of the rotor hubs are now defined relative to the location
    of the thruster (i.e. the cg-centered body coordinate system).


<h3>Configuration File Format:</h3>
@code
<rotor name="{string}" variant="{string}">
  <diameter unit="{LENGTH}"> {number} </diameter>
  <numblades> {number} </numblades>
  <xhub unit="{LENGTH}">  {number} </xhub>
  <zhub unit="{LENGTH}"> {number} </zhub>
  <nominalrpm> {number} </nominalrpm>
  <minrpm>   {number} </minrpm>
  <chord unit="{LENGTH}"> {number} </chord>
  <liftcurveslope Xunit="1/RAD"> {number} </liftcurveslope>
  <flappingmoment unit="{MOMENT}"> {number} </flappingmoment>
  <twist unit="{ANGLE}"> {number} </twist>
  <massmoment Xunit="SLUG*FT"> {number} </massmoment>
  <tiplossfactor> {number} </tiplossfactor>
  <polarmoment unit="{MOMENT}"> {number}</polarmoment>
  <inflowlag> {number} </inflowlag>
  <shafttilt unit="{ANGLE}"> {number} </shafttilt>
  <hingeoffset unit="{LENGTH}"> {number} </hingeoffset>
  <tailrotor>
    <diameter unit="{LENGTH}"> {number} </diameter>
    <numblades> {number} </numblades>
    <xhub unit="{LENGTH}">{number} </xhub>
    <zhub unit="{LENGTH}">{number} </zhub>
    <nominalrpm> {number} </nominalrpm>
    <chord unit="{LENGTH}"> {number} </chord>
    <liftcurveslope Xunit="1/RAD"> {number} </liftcurveslope>
    <flappingmoment unit="{MOMENT}"> {number} </flappingmoment>
    <twist unit="RAD"> {number} </twist>
    <massmoment Xunit="SLUG*FT"> {number} </massmoment>
    <tiplossfactor> {number} </tiplossfactor>
    <inflowlag> {number} </inflowlag>
    <hingeoffset unit="{LENGTH}"> {number} </hingeoffset>
    <cantangle unit="{ANGLE}"> {number} </cantangle>
  </tailrotor>
  <cgroundeffect> {number} </cgroundeffect>
  <groundeffectshift unit="{LENGTH}"> {number} </groundeffectshift>
</rotor>

//  LENGTH means any of the supported units, same for ANGLE and MOMENT.
//  Xunit-attributes are a hint for currently unsupported units, so 
//  values must be provided accordingly.

@endcode

<h3>Configuration Parameters:</h3>

  Brief description and the symbol frequently found in the literature.

<pre>
    \<diameter>           - Rotor disk diameter (R).
    \<numblades>          - Number of blades (b).
    \<xhub>               - Relative height in body coordinate system, thus usually negative.
    \<zhub>               - Relative distance in body coordinate system, close to zero 
                             for main rotor, and usually negative for the tail rotor. 
    \<nominalrpm>         - RPM at which the rotor usally operates. 
    \<minrpm>             - Lowest RPM generated by the code, optional.
    \<chord>              - Blade chord, (c).
    \<liftcurveslope>     - Slope of curve of section lift against section angle of attack,
                             per rad (a).
    \<flappingmoment>     - Flapping moment of inertia (I_b).
    \<twist>              - Blade twist from root to tip, (theta_1).
    \<massmoment>         - Blade mass moment. (Biege/Widerstands-moment)
    \<tiplossfactor>      - Tip-loss factor. The Blade fraction that produces lift.
                              Value usually ranges between 0.95 - 1.0, optional (B).
    \<polarmoment>        - Moment of inertia for the whole rotor disk, optional.
    \<inflowlag>          - Rotor inflow time constant, sec.
    \<shafttilt>          - Orientation of the rotor shaft, negative angles define
                              a 'forward' tilt. Used by main rotor, optional.
    \<hingeoffset>        - Rotor flapping-hinge offset (e).
    
    Experimental properties
    
    \<cantangle>          - Flapping hinge cantangle used by tail rotor, optional.
    \<cgroundeffect>      - Parameter for exponent in ground effect approximation. Value should
                              be in the range 0.2 - 0.35, 0.0 disables, optional.
    \<groundeffectshift>  - Further adjustment of ground effect. 

</pre>

<h3>Notes:</h3>  

   The behavior of the rotor is controlled/influenced by 5 inputs.<ul>
     <li> The power provided by the engine. This is handled by the regular engine controls.</li>
     <li> The collective control input. This is read from the <tt>fdm</tt> property 
          <tt>propulsion/engine[x]/collective-ctrl-rad</tt>.</li>
     <li> The lateral cyclic input. Read from
          <tt>propulsion/engine[x]/lateral-ctrl-rad</tt>.</li>
     <li> The longitudinal cyclic input. Read from 
          <tt>propulsion/engine[x]/longitudinal-ctrl-rad</tt>.</li>
     <li> The tail collective (aka antitorque, aka pedal) control input. Read from
          <tt>propulsion/engine[x]/antitorque-ctrl-rad</tt>.</li>

   </ul>

   In order to keep the rotor speed constant, use of a RPM-Governor system is encouraged.

   It is possible to use different orientation/locations for the rotor system, but then xhub
   and zhub are no longer aligned to the body frame and need proper recalculation.

   To model a standalone main rotor just omit the <tailrotor/> element. If you provide
   a plain <tailrotor/> element all tail rotor parameters are estimated.
   
   The 'sense' parameter from the thruster is interpreted as follows, sense=1 means
   counter clockwise rotation of the main rotor, as viewed from above. This is as a far
   as I know more popular than clockwise rotation, which is defined by setting sense to
   -1 (to be honest, I'm just 99.9% sure that the orientation is handled properly).
   
   Concerning coaxial designs: By providing the 'variant' attribute with value 'coaxial'
   a Kamov-style rotor is modeled - i.e. the rotor produces no torque.


<h3>References:</h3>  

    <dl>    
    <dt>/SH79/</dt><dd>Shaugnessy, J. D., Deaux, Thomas N., and Yenni, Kenneth R.,
              "Development and Validation of a Piloted Simulation of a 
              Helicopter and External Sling Load",  NASA TP-1285, 1979.</dd>
    <dt>/BA41/</dt><dd>Bailey,F.J.,Jr., "A Simplified Theoretical Method of Determining
              the Characteristics of a Lifting Rotor in Forward Flight", NACA Rep.716, 1941.</dd>
    <dt>/AM50/</dt><dd>Amer, Kenneth B.,"Theory of Helicopter Damping in Pitch or Roll and a
              Comparison With Flight Measurements", NACA TN-2136, 1950.</dd>
    <dt>/TA77/</dt><dd>Talbot, Peter D., Corliss, Lloyd D., "A Mathematical Force and Moment
              Model of a UH-1H Helicopter for Flight Dynamics Simulations", NASA TM-73,254, 1977.</dd>   
    </dl>

    @author Thomas Kreitler
    @version $Id: FGRotor.h,v 1.6 2010/06/02 04:05:13 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGRotor : public FGThruster {

   enum eRotorFlags {eNone=0, eMain=1, eTail=2, eCoaxial=4, eRotCW=8} ;
   struct rotor {

     virtual ~rotor();
     void zero();
     void configure(int f, const rotor *xmain = NULL);

     // assist in parameter retrieval
     double cnf_elem(const string& ename, double default_val=0.0, const string& unit = "", bool tell=false);
     double cnf_elem(const string& ename, double default_val=0.0, bool tell=false);

     // rotor dynamics
     void calc_flow_and_thrust(double dt, double rho, double theta_0, double Uw, double Ww, double flow_scale = 1.0);

     void calc_torque(double rho, double theta_0);
     void calc_coning_angle(double rho, double theta_0);
     void calc_flapping_angles(double rho, double theta_0, const FGColumnVector3 &pqr_fus_w);
     void calc_drag_and_side_forces(double rho, double theta_0);

     // transformations
     FGColumnVector3 hub_vel_body2ca( const FGColumnVector3 &uvw, const FGColumnVector3 &pqr, 
                                      double a_ic = 0.0 , double b_ic = 0.0 );
     FGColumnVector3 fus_angvel_body2ca( const FGColumnVector3 &pqr);

     FGColumnVector3 body_forces(double a_ic = 0.0 , double b_ic = 0.0 );
     FGColumnVector3 body_moments(FGColumnVector3 F_h, double a_ic = 0.0 , double b_ic = 0.0 );
         
     // bookkeeping
     int flags                  ;
     Element *parent            ;

     // used in flow calculation
     // FGRK4 rk                  ;  // use this after checking
     FGRKFehlberg rk            ;
     int          reports       ;

     // configuration parameters
     double Radius              ;
     int    BladeNum            ;
     double RelDistance_xhub    ;
     double RelShift_yhub       ;
     double RelHeight_zhub      ;
     double NominalRPM          ;
     double MinRPM              ;
     double BladeChord          ;
     double LiftCurveSlope      ;
     double BladeFlappingMoment ;
     double BladeTwist          ;
     double BladeMassMoment     ;
     double TipLossB            ;
     double PolarMoment         ;
     double InflowLag           ;
     double ShaftTilt           ;
     double HingeOffset         ;
     double HingeOffset_hover   ;
     double CantAngleD3         ;

     double theta_shaft         ;
     double phi_shaft           ;

     // derived parameters
     double LockNumberByRho     ;
     double solidity            ; // aka sigma
     double RpmRatio            ; // main_to_tail, hmm
     double R[5]                ; // Radius powers
     double B[6]                ; // TipLossB powers

     FGMatrix33 BodyToShaft     ; // [S]T, see /SH79/ eqn(17,18)
     FGMatrix33 ShaftToBody     ; // [S]

     // dynamic values
     double ActualRPM           ;
     double Omega               ; // must be > 0 
     double beta_orient         ;
     double a0                  ; // coning angle (rad)
     double a_1, b_1, a_dw      ;
     double a1s, b1s            ; // cyclic flapping relative to shaft axes, /SH79/ eqn(43)
     double H_drag, J_side      ;

     double Torque              ;
     double Thrust              ;
     double Ct                  ;
     double lambda              ; // inflow ratio
     double mu                  ; // tip-speed ratio 
     double nu                  ; // induced inflow ratio
     double v_induced           ; // always positive [ft/s]

     // results
     FGColumnVector3 force      ;
     FGColumnVector3 moment     ;


     // declare the problem function
     class dnuFunction : public FGRungeKuttaProblem {
       public:
         void update_params(rotor *r, double ct_t01, double fs, double w);
       private:
         double pFunc(double x, double y);
         // some shortcuts
         double k_sat, ct_lambda, k_wor, k_theta, mu2, k_flowscale;
     } flowEquation;


   };


public:
  /** Constructor
      @param exec pointer to executive structure
      @param rotor_element pointer to XML element in the config file
      @param num the number of this rotor  */
  FGRotor(FGFDMExec *exec, Element* rotor_element, int num);

  /// Destructor
  ~FGRotor();

  void SetRPM(double rpm) {RPM = rpm;}

  /** Calculates forces and moments created by the rotor(s) and updates 
      vFn,vMn state variables. RPM changes are handled inside, too. 
      The RPM change is based on estimating the torque provided by the engine.

      @param PowerAvailable here this is the thrust (not power) provided by a turbine
      @return PowerAvailable */
  double Calculate(double);

  double GetRPM(void)     const { return RPM;           }
  double GetDiameter(void)      { return mr.Radius*2.0; }

  // Stubs. Right now this rotor-to-engine interface is just a hack.
  double GetTorque(void)        { return 0.0; /* return mr.Torque;*/  }
  double GetPowerRequired(void); 

  // Stubs. Only main rotor RPM is returned
  string GetThrusterLabels(int id, string delimeter);
  string GetThrusterValues(int id, string delimeter);

private:

  bool bind(void);

  double RPM;
  double Sense; // default is counter clockwise rotation of the main rotor (viewed from above)
  bool   tailRotorPresent;

  void Debug(int from);

  FGPropertyManager* PropertyManager;

  rotor mr;
  rotor tr; 

  Filter damp_hagl;

  double rho;
  
  double effective_tail_col; // /SH79/ eqn(47)

  double ground_effect_exp;
  double ground_effect_shift;

  double hover_threshold;
  double hover_scale;

  // fdm imported controls
  FGPropertyManager* prop_collective_ctrl;
  FGPropertyManager* prop_lateral_ctrl;
  FGPropertyManager* prop_longitudinal_ctrl;
  FGPropertyManager* prop_antitorque_ctrl;

  FGPropertyManager* prop_freewheel_factor;
  FGPropertyManager* prop_rotorbrake;

  // fdm export ...
  double prop_inflow_ratio_lambda;
  double prop_advance_ratio_mu;
  double prop_inflow_ratio_induced_nu;
  double prop_mr_torque;
  double prop_coning_angle;

  double prop_theta_downwash;
  double prop_phi_downwash;

  double prop_thrust_coefficient;
  double prop_lift_coefficient;

  double dt; // deltaT doesn't do the thing 

  // devel/debug stuff  
  int prop_DumpFlag;   // causes 1-time dump on stdout

};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
