/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropeller.h
 Author:       Jon S. Berndt
 Date started: 08/24/00

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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
#include <math/FGTable.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPELLER "$Id: FGPropeller.h,v 1.5 2005/11/12 16:28:19 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Propeller modeling class.
    FGPropeller models a propeller given the tabular data for Ct and Cp
    indexed by advance ratio "J". The data for the propeller is
    stored in a config file named "prop_name.xml". The propeller config file
    is referenced from the main aircraft config file in the "Propulsion" section.
    See the constructor for FGPropeller to see what is read in and what should
    be stored in the config file.<br>
    Several references were helpful, here:<ul>
    <li>Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
     Wiley & Sons, 1979 ISBN 0-471-03032-5</li>
    <li>Edwin Hartman, David Biermann, "The Aerodynamic Characteristics of
    Full Scale Propellers Having 2, 3, and 4 Blades of Clark Y and R.A.F. 6
    Airfoil Sections", NACA Report TN-640, 1938 (?)</li>
    <li>Various NACA Technical Notes and Reports</li>
    </ul>
    @author Jon S. Berndt
    @version $Id: FGPropeller.h,v 1.5 2005/11/12 16:28:19 jberndt Exp $
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
      @param el a pointer to the thruster config file XML element*/
  FGPropeller(FGFDMExec* exec, Element* el, int num = 0);

  /// Destructor for FGPropeller - deletes the FGTable objects
  ~FGPropeller();

  /** Sets the Revolutions Per Minute for the propeller. Normally the propeller
      instance will calculate its own rotational velocity, given the Torque
      produced by the engine and integrating over time using the standard
      equation for rotational acceleration "a": a = Q/I , where Q is Torque and
      I is moment of inertia for the propeller.
      @param rpm the rotational velocity of the propeller */
  void SetRPM(double rpm) {RPM = rpm;}

  /// Returns true of this propeller is variable pitch
  bool IsVPitch(void) {return MaxPitch != MinPitch;}

  /** This commands the pitch of the blade to change to the value supplied.
      This call is meant to be issued either from the cockpit or by the flight
      control system (perhaps to maintain constant RPM for a constant-speed
      propeller). This value will be limited to be within whatever is specified
      in the config file for Max and Min pitch. It is also one of the lookup
      indices to the power and thrust tables for variable-pitch propellers.
      @param pitch the pitch of the blade in degrees. */
  void SetPitch(double pitch) {Pitch = pitch;}

  void SetAdvance(double advance) {Advance = advance;}

  /// Sets the P-Factor constant
  void SetPFactor(double pf) {P_Factor = pf;}

  /** Sets the rotation sense of the propeller.
      @param s this value should be +/- 1 ONLY. +1 indicates clockwise rotation as
               viewed by someone standing behind the engine looking forward into
               the direction of flight. */
  void SetSense(double s) { Sense = s;}

  /// Retrieves the pitch of the propeller in degrees.
  double GetPitch(void)         { return Pitch;         }

  /// Retrieves the RPMs of the propeller
  double GetRPM(void)           { return RPM;           }

  /// Retrieves the propeller moment of inertia
  double GetIxx(void)           { return Ixx;           }

  /// Retrieves the Torque in foot-pounds (Don't you love the English system?)
  double GetTorque(void)        { return vTorque(eX);    }

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
  double Calculate(double PowerAvailable);
  FGColumnVector3 GetPFactor(void);
  string GetThrusterLabels(int id, string delimeter);
  string GetThrusterValues(int id, string delimeter);

  void   SetReverseCoef (double c) { Reverse_coef = c; }
  double GetReverseCoef (void) { return Reverse_coef; }
  void   SetReverse (bool r) { Reversed = r; }
  bool   GetReverse (void) { return Reversed; }
  void   SetFeather (bool f) { Feathered = f; }
  bool   GetFeather (void) { return Feathered; }

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
  double Sense;
  double Advance;
  double ExcessTorque;
  double D4;
  double D5;
  FGColumnVector3 vTorque;
  FGTable *cThrust;
  FGTable *cPower;
  void Debug(int from);
  double ReversePitch; // Pitch, when fully reversed
  bool   Reversed;		 // true, when propeller is reversed
  double Reverse_coef; // 0 - 1 defines AdvancePitch (0=MIN_PITCH 1=REVERSE_PITCH)
  bool   Feathered;    // true, if feather command
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

