/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGBldc.h
 Author:       Matt Vacanti
 Date started: 11/08/2020

 ----- Copyright (C) 2020  Matt Vacanti (mdvacanti@gmail.com) --------------

UPDATE LICENSE

HISTORY
--------------------------------------------------------------------------------
11/08/2020 MDV  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGBLDC_H
#define FGBLDC_H

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

class FGBldc : public FGEngine
{
public:
  /// Constructor
  FGBldc(FGFDMExec* exec, Element *el, int engine_number, FGEngine::Inputs& input);
  /// Destructor
  ~FGBldc();

  void Calculate(void);
  double GetPowerAvailable(void) {return (HP * hptoftlbssec);}
  double GetCurrentRequired(void) {return CurrentRequired;}
  double getRPM(void) {return RPM;}
  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

private:

  double CalcFuelNeed(void);

  //double BrakeHorsePower;

  // constants
  double hptowatts;
  double noLoadCurrent;
  double coilResistance;
  double PowerWatts;         // maximum engine power
  double MaxCurrent;         // maximum current
  double MaxVolts;
  double VelocityConstant;
  double TorqueConstant;
  double RPM;                // revolutions per minute
  double HP;                 // engine output, in horsepower
  double V;                  // speed control commanded voltage
  double CommandedRPM;       // desired RPM set by voltage
  double MaxTorque;          // maximum available torque from motor
  double DeltaRPM;
  double TorqueAvailable;
  double TargetTorque;
  double TorqueRequired;
  double CurrentRequired;
  double EnginePower;
  double DeltaTorque;
  double  deceleration_time = 0.5;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
