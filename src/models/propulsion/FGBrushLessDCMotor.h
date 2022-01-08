/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGBrussLessDCMotor.h
 Author:       Paolo Becchi
 Date started: 1-1-2022

 ----- Copyright (C) 2022  Paolo Becchi (pbecchi@aerobusiness.it) --------------
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
  found on the world wide web at http://www.gnu.org


HISTORY
--------------------------------------------------------------------------------
1/01/2022 MDV  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGBrushLessDCMotor_H
#define FGBrushLessDCMotor_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models an electric brushless DC motor or more appropriately permanent magnet
    synchronous motor.
    FGElectric models an electric motor based on the configuration file
    \<power> parameter.  The throttle controls motor output linearly from
    zero to \<power>.  This power value (converted internally to horsepower)
    is then used by FGPropeller to apply torque to the propeller.  At present
    there is no battery model available, so this motor does not consume any
    energy.  There is no internal friction.
    @author David Culp
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGBrushLessDCMotor : public FGEngine
{
public:
  /// Constructor
  FGBrushLessDCMotor(FGFDMExec* exec, Element *el, int engine_number, FGEngine::Inputs& input);
  /// Destructor
  ~FGBrushLessDCMotor();

  void Calculate(void);
  double GetPowerAvailable(void) {return (HP * hptoftlbssec);}
  double GetCurrentRequired(void) {return CurrentRequired;}
  double getRPM(void) {return RPM;}
  double CalcFuelNeed(void);
  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

private:

  // constants
  double hptowatts;
  double noLoadCurrent;      // 0 torque current [A]
  double coilResistance;     // internal resistance [Ohm]
  double PowerWatts;         // maximum engine power
  double MaxCurrent;         // maximum current [A]
  double MaxVolts;           // max voltage available from battery [V]
  double VelocityConstant;   //.speed constant of brusless DC motors [RPM/V]
  double TorqueConstant=6.1217;
  double RPM;                // revolutions per minute
  double HP;                 // engine output, in horsepower
  double V;                  // speed control commanded voltage 
  double DeltaRPM;           // desired RPM set by voltage
  double MaxTorque;          // maximum available torque from motor
  double TorqueAvailable;    //.torque available at set voltage
  double TargetTorque;       // torque applied to propeller
  double TorqueRequired;     // propeller torque at current RPM
  double CurrentRequired;    // current required at current RPM
  double EnginePower;        // power in pounds *feet
  double InertiaTorque;      // acceleration torque due to rotating inertia
  double DecelerationFactor = 1; //factor to increase deceleration time
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
