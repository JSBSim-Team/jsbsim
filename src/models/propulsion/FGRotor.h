/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRotor.h
 Author:       T. Kreitler
 Date started: 08/24/00

 ------------- Copyright (C) 2010  T. Kreitler (t.kreitler@web.de) -------------

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
01/10/11  T.Kreitler changed to single rotor model
03/06/11  T.Kreitler added brake, clutch, and experimental free-wheeling-unit
02/05/12  T.Kreitler brake, clutch, and FWU now in FGTransmission class

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGROTOR_H
#define FGROTOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGThruster.h"
#include "FGTransmission.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a helicopter rotor.


<h3>Configuration File Format</h3>
@code
<rotor name="{string}">
  <diameter unit="{LENGTH}"> {number} </diameter>
  <numblades> {number} </numblades>
  <gearratio> {number} </gearratio>
  <nominalrpm> {number} </nominalrpm>
  <minrpm> {number} </minrpm>
  <maxrpm> {number} </maxrpm>
  <chord unit="{LENGTH}"> {number} </chord>
  <liftcurveslope Xunit="1/RAD"> {number} </liftcurveslope>
  <twist unit="{ANGLE}"> {number} </twist>
  <hingeoffset unit="{LENGTH}"> {number} </hingeoffset>
  <flappingmoment unit="{MOMENT}"> {number} </flappingmoment>
  <massmoment Xunit="SLUG*FT"> {number} </massmoment>
  <polarmoment unit="{MOMENT}"> {number} </polarmoment>
  <inflowlag> {number} </inflowlag>
  <tiplossfactor> {number} </tiplossfactor>
  <maxbrakepower unit="{POWER}"> {number} </maxbrakepower>
  <gearloss unit="{POWER}"> {number} </gearloss>
  <gearmoment unit="{MOMENT}"> {number} </gearmoment>

  <controlmap> {MAIN|TAIL|TANDEM} </controlmap>
  <ExternalRPM> {number} </ExternalRPM>

  <groundeffectexp> {number} </groundeffectexp>
  <groundeffectshift unit="{LENGTH}"> {number} </groundeffectshift>

</rotor>

//  LENGTH means any of the supported units, same for ANGLE and MOMENT.
//  Xunit-attributes are a hint for currently unsupported units, so
//  values must be provided accordingly.

@endcode

<h3>Configuration Parameters:</h3>

  Brief description and the symbol frequently found in the literature.

<pre>
    \<diameter>           - Rotor disk diameter (2x R).
    \<numblades>          - Number of blades (b).
    \<gearratio>          - Ratio of (engine rpm) / (rotor rpm), usually > 1.
    \<nominalrpm>         - RPM at which the rotor usally operates.
    \<minrpm>             - Lowest RPM used in the model, optional and defaults to 1.
    \<maxrpm>             - Largest RPM used in the model, optional and defaults to 2 x nominalrpm.
    \<chord>              - Blade chord, (c).
    \<liftcurveslope>     - Slope of curve of section lift against section angle of attack,
                             per rad (a).
    \<twist>              - Blade twist from root to tip, (theta_1).
    \<hingeoffset>        - Rotor flapping-hinge offset (e).
    \<flappingmoment>     - Flapping moment of inertia (I_b).
    \<massmoment>         - Blade mass moment. Mass of a single blade times the blade's
                             cg-distance from the hub, optional.
    \<polarmoment>        - Moment of inertia for the whole rotor disk, optional.
    \<inflowlag>          - Rotor inflow time constant, sec. Smaller values yield to quicker
                              responses (typical values for main rotor: 0.1 - 0.2 s).
    \<tiplossfactor>      - Tip-loss factor. The Blade fraction that produces lift.
                              Value usually ranges between 0.95 - 1.0, optional (B).

    \<maxbrakepower>      - Rotor brake, 20-30 hp should work for a mid size helicopter.
    \<gearloss>           - Friction in gear, 0.2% to 3% of the engine power, optional (see notes).
    \<gearmoment>         - Approximation for the moment of inertia of the gear (and engine),
                              defaults to 0.1 * polarmoment, optional.

    \<controlmap>         - Defines the control inputs used (see notes).

    \<ExternalRPM>        - Links the rotor to another rotor, or an user controllable property.

    Experimental properties

    \<groundeffectexp>    - Exponent for ground effect approximation. Values usually range from 0.04
                            for large rotors to 0.1 for smaller ones. As a rule of thumb the effect
                            vanishes at a height 2-3 times the rotor diameter.
                              formula used: exp ( - groundeffectexp * (height+groundeffectshift) )
                            Omitting or setting to 0.0 disables the effect calculation.
    \<groundeffectshift>  - Further adjustment of ground effect, approx. hub height or slightly above
                            (This lessens the influence of the ground effect).

</pre>

<h3>Notes:</h3>

  <h4>- Controls -</h4>

    The behavior of the rotor is controlled/influenced by following inputs.<ul>
      <li> The power provided by the engine. This is handled by the regular engine controls.</li>
      <li> The collective control input. This is read from the <tt>fdm</tt> property
           <tt>propulsion/engine[x]/collective-ctrl-rad</tt>. See below for tail rotor</li>
      <li> The lateral cyclic input. Read from
           <tt>propulsion/engine[x]/lateral-ctrl-rad</tt>.</li>
      <li> The longitudinal cyclic input. Read from
           <tt>propulsion/engine[x]/longitudinal-ctrl-rad</tt>.</li>
      <li> The tail rotor collective (aka antitorque, aka pedal) control input. Read from
           <tt>propulsion/engine[x]/antitorque-ctrl-rad</tt> or
           <tt>propulsion/engine[x]/tail-collective-ctrl-rad</tt>.</li>

    </ul>

  <h4>- Tail/tandem rotor -</h4>

    Providing <tt>\<ExternalRPM\> 0 \</ExternalRPM\></tt> the tail rotor's RPM
    is linked to to the main (=first, =0) rotor, and specifing
    <tt>\<controlmap\> TAIL \</controlmap\></tt> tells this rotor to read the
    collective input from <tt>propulsion/engine[1]/antitorque-ctrl-rad</tt>
    (The TAIL-map ignores lateral and longitudinal input). The rotor needs to be
    attached to a dummy engine, e.g. an 1HP electrical engine.
    A tandem rotor is setup analogous.

  <h4>- Sense -</h4>

    The 'sense' parameter from the thruster is interpreted as follows, sense=1 means
    counter clockwise rotation of the main rotor, as viewed from above. This is as a far
    as I know more popular than clockwise rotation, which is defined by setting sense to
    -1. Concerning coaxial designs - by setting 'sense' to zero, a Kamov-style rotor is
    modeled (i.e. the rotor produces no torque).

  <h4>- Engine issues -</h4>

    In order to keep the rotor/engine speed constant, use of a RPM-Governor system is
    encouraged (see examples).

    In case the model requires the manual use of a clutch the <tt>\<gearloss\></tt>
    property might need attention.<ul>

    <li> Electrical: here the gear-loss should be rather large to keep the engine
         controllable when the clutch is open (although full throttle might still make it
         spin away).</li>
    <li> Piston: this engine model already has some internal friction loss and also
         looses power if it spins too high. Here the gear-loss could be set to 0.25%
         of the engine power (which is also the approximated default).</li>
    <li> Turboprop: Here the default value might be a bit too small. Also it's advisable
         to adjust the power table for rpm values that are far beyond the nominal value.</li>

    </ul>

  <h4>- Scaling the ground effect -</h4>

    The property <tt>propulsion/engine[x]/groundeffect-scale-norm</tt> allows fdm based
    scaling of the ground effect influence. For instance the effect vanishes at speeds
    above approx. 50kts, or one likes to land on a 'perforated' helipad.

  <h4>- Development hints -</h4>

    Setting <tt>\<ExternalRPM> -1 \</ExternalRPM></tt> the rotor's RPM is controlled  by
    the <tt>propulsion/engine[x]/x-rpm-dict</tt> property. This feature can be useful
    when developing a FDM.


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
    <dt>/GE49/</dt><dd>Gessow, Alfred, Amer, Kenneth B. "An Introduction to the Physical
              Aspects of Helicopter Stability", NACA TN-1982, 1949.</dd>
    </dl>

    @author Thomas Kreitler
  */



/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGRotor :  public FGThruster {

  enum eCtrlMapping {eMainCtrl=0, eTailCtrl, eTandemCtrl};

public:

  /** Constructor for FGRotor.
      @param exec a pointer to the main executive object
      @param rotor_element a pointer to the thruster config file XML element
      @param num the number of this rotor */
  FGRotor(FGFDMExec *exec, Element* rotor_element, int num);

  /// Destructor for FGRotor
  ~FGRotor();

  /// Returns the power required by the rotor.
  double GetPowerRequired(void) { return PowerRequired; }

  /// Returns the scalar thrust of the rotor, and adjusts the RPM value.
  double Calculate(double EnginePower);


  /// Retrieves the RPMs of the rotor.
  double GetRPM(void) const { return RPM; }
  void   SetRPM(double rpm) { RPM = rpm; }

  /// Retrieves the RPMs of the Engine, as seen from this rotor.
  double GetEngineRPM(void) const {return EngineRPM;} //{ return GearRatio*RPM; }
  void SetEngineRPM(double rpm) {EngineRPM = rpm;} //{ RPM = rpm/GearRatio; }
  /// Tells the rotor's gear ratio, usually the engine asks for this.
  double GetGearRatio(void) { return GearRatio; }
  /// Retrieves the thrust of the rotor.
  double GetThrust(void) const { return Thrust; }

  /// Retrieves the rotor's coning angle
  double GetA0(void) const { return a0; }
  /// Retrieves the longitudinal flapping angle with respect to the rotor shaft
  double GetA1(void) const { return a1s; }
  /// Retrieves the lateral flapping angle with respect to the rotor shaft
  double GetB1(void) const { return b1s; }

  /// Retrieves the inflow ratio
  double GetLambda(void) const { return lambda; }
  /// Retrieves the tip-speed (aka advance) ratio
  double GetMu(void) const { return mu; }
  /// Retrieves the induced inflow ratio
  double GetNu(void) const { return nu; }
  /// Retrieves the induced velocity
  double GetVi(void) const { return v_induced; }
  /// Retrieves the thrust coefficient
  double GetCT(void) const { return C_T; }
  /// Retrieves the torque
  double GetTorque(void) const { return Torque; }

  /// Downwash angle - positive values point forward (given a horizontal spinning rotor)
  double GetThetaDW(void) const { return theta_downwash; }
  /// Downwash angle - positive values point leftward (given a horizontal spinning rotor)
  double GetPhiDW(void) const { return phi_downwash; }

  /// Retrieves the ground effect scaling factor.
  double GetGroundEffectScaleNorm(void) const { return GroundEffectScaleNorm; }
  /// Sets the ground effect scaling factor.
  void   SetGroundEffectScaleNorm(double g) { GroundEffectScaleNorm = g; }

  /// Retrieves the collective control input in radians.
  double GetCollectiveCtrl(void) const { return CollectiveCtrl; }
  /// Retrieves the lateral control input in radians.
  double GetLateralCtrl(void) const { return LateralCtrl; }
  /// Retrieves the longitudinal control input in radians.
  double GetLongitudinalCtrl(void) const { return LongitudinalCtrl; }

  /// Sets the collective control input in radians.
  void SetCollectiveCtrl(double c) { CollectiveCtrl = c; }
  /// Sets the lateral control input in radians.
  void SetLateralCtrl(double c) { LateralCtrl = c; }
  /// Sets the longitudinal control input in radians.
  void SetLongitudinalCtrl(double c) { LongitudinalCtrl = c; }

  // Stubs. Only main rotor RPM is returned
  std::string GetThrusterLabels(int id, const std::string& delimeter);
  std::string GetThrusterValues(int id, const std::string& delimeter);

private:

  // assist in parameter retrieval
  double ConfigValueConv( Element* e, const std::string& ename, double default_val=0.0,
                                      const std::string& unit = "", bool tell=false);

  double ConfigValue( Element* e, const std::string& ename, double default_val=0.0,
                                  bool tell=false);

  double Configure(Element* rotor_element);

  void CalcRotorState(void);

  // rotor dynamics
  void calc_flow_and_thrust(double theta_0, double Uw, double Ww, double flow_scale = 1.0);
  void calc_coning_angle(double theta_0);
  void calc_flapping_angles(double theta_0, const FGColumnVector3 &pqr_fus_w);
  void calc_drag_and_side_forces(double theta_0);
  void calc_torque(double theta_0);
  void calc_downwash_angles();

  // transformations
  FGColumnVector3 hub_vel_body2ca( const FGColumnVector3 &uvw, const FGColumnVector3 &pqr,
                                   double a_ic = 0.0 , double b_ic = 0.0 );
  FGColumnVector3 fus_angvel_body2ca( const FGColumnVector3 &pqr);
  FGColumnVector3 body_forces(double a_ic = 0.0 , double b_ic = 0.0 );
  FGColumnVector3 body_moments(double a_ic = 0.0 , double b_ic = 0.0 );

  // interface
  bool bindmodel(FGPropertyManager* pm);
  void Debug(int from);

  // environment
  double dt;
  double rho;
  Filter damp_hagl;

  // configuration parameters
  double Radius;
  int    BladeNum;

  // rpm control
  double Sense;
  double NominalRPM;
  double MinimalRPM;
  double MaximalRPM;
  int    ExternalRPM;
  int    RPMdefinition;
  SGPropertyNode_ptr ExtRPMsource;
  double SourceGearRatio;

  // 'real' rotor parameters
  double BladeChord;
  double LiftCurveSlope;
  double BladeTwist;
  double HingeOffset;
  double BladeFlappingMoment;
  double BladeMassMoment;
  double PolarMoment;
  double InflowLag;
  double TipLossB;

  // groundeffect
  double GroundEffectExp;
  double GroundEffectShift;
  double GroundEffectScaleNorm;

  // derived parameters
  double LockNumberByRho;
  double Solidity; // aka sigma
  double R[5]; // Radius powers
  double B[5]; // TipLossB powers

  // Some of the calculations require shaft axes. So the
  // thruster orientation (Tbo, with b for body) needs to be
  // expressed/represented in helicopter shaft coordinates (Hsr).
  FGMatrix33 InvTransform;
  FGMatrix33 TboToHsr;
  FGMatrix33 HsrToTbo;

  // dynamic values
  double RPM;
  double Omega;          // must be > 0
  double beta_orient;    // rotor orientation angle (rad)
  double a0;             // coning angle (rad)
  double a_1, b_1, a_dw; // flapping angles
  double a1s, b1s;       // cyclic flapping relative to shaft axes, /SH79/ eqn(43)
  double H_drag, J_side; // Forces

  double Torque;
  double C_T;        // rotor thrust coefficient
  double lambda;     // inflow ratio
  double mu;         // tip-speed ratio
  double nu;         // induced inflow ratio
  double v_induced;  // induced velocity, usually positive [ft/s]

  double theta_downwash;
  double phi_downwash;

  // control
  eCtrlMapping ControlMap;
  double CollectiveCtrl;
  double LateralCtrl;
  double LongitudinalCtrl;

  // interaction with engine
  FGTransmission *Transmission;
  double EngineRPM;
  double MaxBrakePower;
  double GearLoss;
  double GearMoment;

};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
