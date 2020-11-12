/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 JSBSim
 Author:       Jon S. Berndt
 Date started: 08/24/00
 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

 Header:       FGTransmission.h
 Author:       T.Kreitler
 Date started: 02/05/12

 ------------- Copyright (C) 2012  T. Kreitler (t.kreitler@web.de) -------------

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
02/05/12  T.Kreitler  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRANSMISSION_H
#define FGTRANSMISSION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Utility class that handles power transmission in conjunction with FGRotor.

  This class provides brake, clutch and free-wheel-unit (FWU) functionality
  for the rotor model. Also it is responsible for the RPM calculations.

  When the engine is off the brake could be used to slow/hold down a spinning
  rotor. The maximum brake power is defined in the rotors' config file.
  (Right now there is no checking if the input is in the [0..1] range.)

  The clutch operation is based on a heuristic approach. In the intermediate
  state the transfer is proportional to the clutch position. But equal RPM
  values are enforced on the thruster and rotor sides when approaching the
  closed state.

  The FWU inhibits that the rotor is driving the engine. To do so, the code
  just predicts the upcoming FWU state based on current torque conditions.

  Some engines won't work properly when the clutch is open. To keep them
  controllable some load must be provided on the engine side (EngineFriction,
  aka gear-loss). See the notes under 'Engine issues' in FGRotor.

<h3>Property tree</h3>

  The following properties are created (with x = your thruster number):
  <pre>
    propulsion/engine[x]/brake-ctrl-norm
    propulsion/engine[x]/free-wheel-transmission
    propulsion/engine[x]/clutch-ctrl-norm
  </pre>

<h3>Notes</h3>

  <ul>
    <li> EngineFriction is assumed constant, so better orientate at low RPM
         values, because piston and turboprop engines don't 'like' high
         load at startup.</li>
    <li> The model doesn't support backward operation.</li>
    <li> And even worse, the torque calculations silently assume a minimal
         RPM value of approx. 1.</li>
  </ul>

*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTransmission : public FGJSBBase {

public:
  /** Constructor for FGTransmission.
      @param exec a pointer to the main executive object
      @param num the number of the thruster that uses this object
      @param dt simulation delta T */
  FGTransmission(FGFDMExec *exec, int num, double dt);

  /// Destructor for FGTransmission
  ~FGTransmission();

  void Calculate(double EnginePower, double ThrusterTorque, double dt);

  void   SetMaxBrakePower(double x) {MaxBrakePower=x;}
  double GetMaxBrakePower() const {return MaxBrakePower;}
  void   SetEngineFriction(double x) {EngineFriction=x;}
  double GetEngineFriction() const {return EngineFriction;}
  void   SetEngineMoment(double x) {EngineMoment=x;}
  double GetEngineMoment() const {return EngineMoment;}
  void   SetThrusterMoment(double x) {ThrusterMoment=x;}
  double GetThrusterMoment() const {return ThrusterMoment;}

  double GetFreeWheelTransmission() const {return FreeWheelTransmission;}
  void   SetEngineRPM(double x) {EngineRPM=x;}
  double GetEngineRPM() {return EngineRPM;}
  void   SetThrusterRPM(double x) {ThrusterRPM=x;}
  double GetThrusterRPM() {return ThrusterRPM;}

  double GetBrakeCtrlNorm() const {return BrakeCtrlNorm;}
  void   SetBrakeCtrlNorm(double x) {BrakeCtrlNorm=x;}
  double GetClutchCtrlNorm() const {return ClutchCtrlNorm;}
  void   SetClutchCtrlNorm(double x) {ClutchCtrlNorm=x;}

private:
  bool BindModel(int num, FGPropertyManager* pm);
  void Debug(int from);

  inline double omega_to_rpm(double w) {
    return w * 9.54929658551372014613302580235; // omega/(2.0*PI) * 60.0
  }
  inline double rpm_to_omega(double r) {
    return r * 0.104719755119659774615421446109; // (rpm/60.0)*2.0*PI
  }

  Filter FreeWheelLag;
  double FreeWheelTransmission; // state, 0: free, 1:locked

  double ThrusterMoment;
  double EngineMoment;   // estimated MOI of gear and engine, influences acceleration
  double EngineFriction; // estimated friction in gear and possibly engine

  double ClutchCtrlNorm;
  double BrakeCtrlNorm;
  double MaxBrakePower;

  double EngineRPM;
  double ThrusterRPM;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
