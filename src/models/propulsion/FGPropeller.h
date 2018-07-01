/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropeller.h
 Author:       Jon S. Berndt
 Date started: 08/24/00

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

HISTORY
--------------------------------------------------------------------------------
08/24/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPELLER_H
#define FGPROPELLER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGThruster.h"
#include "math/FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** FGPropeller models a propeller given the tabular data for Ct (thrust) and
    Cp (power), indexed by the advance ratio "J".

### Configuration File Format

~~~{.xml}
<sense> {1 | -1} </sense> 
<p_factor> {number} </p_factor>
<propeller name="{string}" version="{string}">
  <ixx> {number} </ixx>
  <diameter unit="IN"> {number} </diameter>
  <numblades> {number} </numblades>
  <gearratio> {number} </gearratio>
  <minpitch> {number} </minpitch>
  <maxpitch> {number} </maxpitch>
  <minrpm> {number} </minrpm>
  <maxrpm> {number} </maxrpm>
  <constspeed> {number} </constspeed>
  <reversepitch> {number} </reversepitch>
  <ct_factor> {number} </ct_factor>
  <cp_factor> {number} </cp_factor>

  <table name="C_THRUST" type="internal">
    <tableData>
      {numbers}
    </tableData>
  </table>

  <table name="C_POWER" type="internal">
    <tableData>
      {numbers}
    </tableData>
  </table>

  <table name="CT_MACH" type="internal">
    <tableData>
      {numbers}
    </tableData>
  </table>

  <table name="CP_MACH" type="internal">
    <tableData>
      {numbers}
    </tableData>
  </table>

</propeller>
~~~

### Configuration Parameters

<pre>
    \<ixx>           - Propeller rotational inertia.
    \<diameter>      - Propeller disk diameter.
    \<numblades>     - Number of blades.
    \<gearratio>     - Ratio of (engine rpm) / (prop rpm).
    \<minpitch>      - Minimum blade pitch angle.
    \<maxpitch>      - Maximum blade pitch angle.
    \<minrpm>        - Minimum rpm target for constant speed propeller.
    \<maxrpm>        - Maximum rpm target for constant speed propeller.
    \<constspeed>    - 1 = constant speed mode, 0 = manual pitch mode. 
    \<reversepitch>  - Blade pitch angle for reverse.
    \<sense>         - Direction of rotation (1=clockwise as viewed from cockpit,
                        -1=anti-clockwise as viewed from cockpit). Sense is
                       specified in the parent tag of the propeller.
    \<p_factor>      - P factor. It is specified in the parent tag of
                       the propeller.
    \<ct_factor>     - A multiplier for the coefficients of thrust.
    \<cp_factor>     - A multiplier for the coefficients of power.
</pre>

Two tables are needed. One for coefficient of thrust (Ct) and one for
coefficient of power (Cp).

Two tables are optional. They apply a factor to Ct and Cp based on the
helical tip Mach.

The parameters <sense> and <p_factor> must be specified at the parent level i.e.
in the <thruster> element. This allows to specify different sense and P factor
values for each propeller of the model while using the same definition file for
all the propellers.

In addition to thrust, the propeller applies two moments to the aircraft:
- The torque that tends to roll the aircraft in the direction opposite to the
propeller rotation,
- and the gyroscopic moment.

It should be noted that historically the gyroscopic moment had an incorrect
sign. The correct sign can be obtained by specifying a **version** attribute
higher than 1.0 to the propeller definition
~~~.xml
<propeller name="a_prop" version="1.1">
  <!-- propeller definition -->
</propeller>
~~~
For backward compatibility, the absence of the **version** attribute will result
in the gyroscopic moment to be computed with the legacy incorrect sign.

Several references were helpful, here:
+ Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
Wiley & Sons, 1979 ISBN 0-471-03032-5
+ Edwin Hartman, David Biermann, "The Aerodynamic Characteristics of
Full Scale Propellers Having 2, 3, and 4 Blades of Clark Y and R.A.F. 6
Airfoil Sections", NACA Report TN-640, 1938 (?)
+ Various NACA Technical Notes and Reports

@author Jon S. Berndt
@see FGEngine
@see FGThruster
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropeller : public FGThruster {

public:
  /** Constructor for FGPropeller.
      @param exec a pointer to the main executive object
      @param el a pointer to the thruster config file XML element
      @param num the number of this propeller */
  FGPropeller(FGFDMExec* exec, Element* el, int num = 0);

  /// Destructor for FGPropeller - deletes the FGTable objects
  ~FGPropeller();

  /// Reset the initial conditions.
  void ResetToIC(void);

  /** Sets the Revolutions Per Minute for the propeller. Normally the propeller
      instance will calculate its own rotational velocity, given the Torque
      produced by the engine and integrating over time using the standard
      equation for rotational acceleration \f$a\f$: \f$a = Q/I\f$ , where
      \f$Q\f$ is Torque and \f$I\f$ is moment of inertia for the propeller.
      @param rpm the rotational velocity of the propeller */
  void SetRPM(double rpm) {RPM = rpm;}

  /** Sets the Revolutions Per Minute for the propeller using the engine gear
      ratio */
  void SetEngineRPM(double rpm) {RPM = rpm/GearRatio;}

  /// Returns true of this propeller is variable pitch
  bool IsVPitch(void) const {return MaxPitch != MinPitch;}

  /** This commands the pitch of the blade to change to the value supplied.
      This call is meant to be issued either from the cockpit or by the flight
      control system (perhaps to maintain constant RPM for a constant-speed
      propeller). This value will be limited to be within whatever is specified
      in the config file for Max and Min pitch. It is also one of the lookup
      indices to the power and thrust tables for variable-pitch propellers.
      @param pitch the pitch of the blade in degrees. */
  void SetPitch(double pitch) {Pitch = pitch;}

  /** Set the propeller pitch.
      @param advance the pitch command in percent (0.0 - 1.0)
   */
  void SetAdvance(double advance) {Advance = advance;}

  /// Sets the P-Factor constant
  void SetPFactor(double pf) {P_Factor = pf;}

  /// Sets propeller into constant speed mode, or manual pitch mode
  void SetConstantSpeed(int mode) {ConstantSpeed = mode;} 

  /// Sets coefficient of thrust multiplier
  void SetCtFactor(double ctf) {CtFactor = ctf;}

  /// Sets coefficient of power multiplier
  void SetCpFactor(double cpf) {CpFactor = cpf;}

  /** Sets the rotation sense of the propeller.
      @param s this value should be +/- 1 ONLY. +1 indicates clockwise rotation as
               viewed by someone standing behind the engine looking forward into
               the direction of flight. */
  void SetSense(double s) { Sense = s;}

  /// Retrieves the pitch of the propeller in degrees.
  double GetPitch(void) const     { return Pitch;         }

  /// Retrieves the RPMs of the propeller
  double GetRPM(void) const       { return RPM;           } 

  /// Calculates the RPMs of the engine based on gear ratio
  double GetEngineRPM(void) const { return RPM * GearRatio;  } 

  /// Retrieves the propeller moment of inertia
  double GetIxx(void) const       { return Ixx;           }

  /// Retrieves the coefficient of thrust multiplier
  double GetCtFactor(void) const  { return CtFactor;      }

  /// Retrieves the coefficient of power multiplier
  double GetCpFactor(void) const  { return CpFactor;      }

  /// Retrieves the propeller diameter
  double GetDiameter(void) const  { return Diameter;      }

  /// Retrieves propeller thrust table
  FGTable* GetCThrustTable(void) const { return cThrust;}
  /// Retrieves propeller power table
  FGTable* GetCPowerTable(void)  const { return cPower; }

  /// Retrieves propeller thrust Mach effects factor
  FGTable* GetCtMachTable(void) const { return CtMach; }
  /// Retrieves propeller power Mach effects factor
  FGTable* GetCpMachTable(void) const { return CpMach; }

  /// Retrieves the Torque in foot-pounds (Don't you love the English system?)
  double GetTorque(void) const  { return vTorque(eX); }

  /** Retrieves the power required (or "absorbed") by the propeller -
      i.e. the power required to keep spinning the propeller at the current
      velocity, air density,  and rotational rate. */
  double GetPowerRequired(void);

  /** Calculates and returns the thrust produced by this propeller.
      Given the excess power available from the engine (in foot-pounds), the thrust is
      calculated, as well as the current RPM. The RPM is calculated by integrating
      the torque provided by the engine over what the propeller "absorbs"
      (essentially the "drag" of the propeller).
      @param PowerAvailable this is the excess power provided by the engine to
      accelerate the prop. It could be negative, dictating that the propeller
      would be slowed.
      @return the thrust in pounds */
  double Calculate(double EnginePower);
  /// Retrieves the P-Factor constant
  FGColumnVector3 GetPFactor(void) const;
  /// Generate the labels for the thruster standard CSV output
  std::string GetThrusterLabels(int id, const std::string& delimeter);
  /// Generate the values for the thruster standard CSV output
  std::string GetThrusterValues(int id, const std::string& delimeter);
  /** Set the propeller reverse pitch.
      @param c the reverse pitch command in percent (0.0 - 1.0)
  */
  void   SetReverseCoef (double c) { Reverse_coef = c; }
  /// Retrieves the reverse pitch command.
  double GetReverseCoef (void) const { return Reverse_coef; }
  /// If true, sets the propeller in reversed position.
  void   SetReverse (bool r) { Reversed = r; }
  /// Returns true if the propeller is in reverse position.
  bool   GetReverse (void) const { return Reversed; }
  /// If true, sets the propeller in feathered position.
  void   SetFeather (bool f) { Feathered = f; }
  /// Returns true if the propeller is in feathered position.
  bool   GetFeather (void) const { return Feathered; }
  /// Retrieves the thrust coefficient
  double GetThrustCoefficient(void) const {return ThrustCoeff;}
  /// Retrieves the Mach number at the propeller tips.
  double GetHelicalTipMach(void) const {return HelicalTipMach;}
  /// Returns a non-zero value if the propeller is constant speed.
  int    GetConstantSpeed(void) const {return ConstantSpeed;}
  /// Set the propeller induced velocity
  void   SetInducedVelocity(double Vi) {Vinduced = Vi;}
  /// Get the propeller induced velocity.
  double GetInducedVelocity(void) const {return Vinduced;}

private:
  int   numBlades;
  double J;
  double RPM;
  double Ixx;
  double Diameter;
  double MaxPitch;
  double MinPitch;
  double MinRPM;
  double MaxRPM;
  double Pitch;
  double P_Factor;
  double Sense, Sense_multiplier;
  double Advance;
  double ExcessTorque;
  double D4;
  double D5;
  double HelicalTipMach;
  double Vinduced;
  FGColumnVector3 vTorque;
  FGTable *cThrust;
  FGTable *cPower;
  FGTable *CtMach;
  FGTable *CpMach;
  double CtFactor;
  double CpFactor;
  int    ConstantSpeed;
  void Debug(int from);
  double ReversePitch; // Pitch, when fully reversed
  bool   Reversed;     // true, when propeller is reversed
  double Reverse_coef; // 0 - 1 defines AdvancePitch (0=MIN_PITCH 1=REVERSE_PITCH)
  bool   Feathered;    // true, if feather command
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

