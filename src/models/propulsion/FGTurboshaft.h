/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTurboshaft.h
 Purpose:      This module models a turboshaft engine (drives a gearbox/rotor,
               not a variable-pitch propeller).

 ------------- Copyright (C) 2026 JSBSim contributors -------------

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
Derived from FGTurboProp.h (Jiri "Javky" Javurek, 2004), with the propeller/
beta-range/reverse-pitch handling removed (a turboshaft has none of that; it
drives a gearbox/rotor shaft) and a real, script-settable starter control
added: FGTurboProp's own phase state machine already has tpSpinUp/tpStart
phases for a gradual, governor-moderated start, but nothing in FGTurboProp
ever binds a property to the base FGEngine::Starter flag that those phases
require (see the "starter-norm" pattern FGPiston already uses for its own
engine type). Without that, the only way to start an FGTurboProp engine from
a script is propulsion/set-running, which forces full throttle directly
(FGPropulsion::SetEngineRunning()) and converges to a steady state through
FGPropulsion::GetSteadyState()'s internal iteration loop (up to 6000 steps of
a synthetic 0.5s each) inside a single real simulation frame -- for an
aircraft with an RPM governor moderating throttle in response to shaft RPM,
that produces a large, unphysical one-frame RPM overshoot (confirmed via
aircraft/mi17's own ground-run scripts: shaft RPM spiking to ~1.7x the
governed target before settling back down), because the governor never gets
a chance to react during that synthetic convergence. Driving a start through
this class's own starter-cmd property instead goes through the real,
per-frame phase machine with the real simulation dt, so the governor's
normal throttle output is what the engine actually sees throughout.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTURBOSHAFT_H
#define FGTURBOSHAFT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>
#include "FGEngine.h"
#include "math/FGTable.h"

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Turboshaft engine model: a free-turbine engine driving a fixed shaft (a
    gearbox and, through it, a rotor -- not a variable-pitch propeller with
    its own beta/reverse control). Thermodynamics (N1 spool, ITT, power vs.
    RPM/N1 table lookup, fuel flow) are unchanged from FGTurboProp; only the
    output side (no propeller, no reverse/beta-range throttle remapping) and
    the addition of a real starter-cmd property differ.
<h3>Configuration parameters:</h3>
<pre>
idlen1      [%]
maxn1       [%]
maxpower    [HP]
psfc power specific fuel consumption [pph/HP] for N1=100%
n1idle_max_delay [-] time constant for N1 change
maxstartingtime [sec]
    after this time the automatic starting cycle is interrupted when the engine
    doesn't start (0=automatic starting not present)
startern1   [%]
    when starting, the starter spins the engine up to this N1
ielumaxtorque [lb.ft]
    if torque>ielumaxtorque limiters decrease the throttle
    (ielu = Integrated Electronic Limiter Unit)
itt_delay [-] time constant for ITT change
    (ITT = Inter Turbine Temperature)
</pre>
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTurboshaft : public FGEngine
{
public:
  /** Constructor
      @param Executive pointer to executive structure
      @param el pointer to the XML element representing the turboshaft engine
      @param engine_number engine number*/
  FGTurboshaft(FGFDMExec* Executive, Element *el, int engine_number, struct Inputs& input);

  enum phaseType { tsOff, tsRun, tsSpinUp, tsStart, tsTrim };

  void Calculate(void);
  double CalcFuelNeed(void);

  double GetPowerAvailable(void) const { return (HP * hptoftlbssec); }
  double GetRPM(void) const { return RPM; }
  double GetIeluThrottle(void) const { return (ThrottlePos); }
  bool GetIeluIntervent(void) const { return Ielu_intervent; }

  double Seek(double* var, double target, double accel, double decel);
  double ExpSeek(double* var, double target, double accel, double decel);

  phaseType GetPhase(void) const { return phase; }

  bool GetCutoff(void) const { return Cutoff; }

  double GetN1(void) const {return N1;}
  double GetITT(void) const {return Eng_ITT_degC;}
  double GetEngStarting(void) const { return EngStarting; }

  double getOilPressure_psi () const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}

  inline bool GetGeneratorPower(void) const { return GeneratorPower; }
  inline int GetCondition(void) const { return Condition; }

  void SetPhase( phaseType p ) { phase = p; }
  void SetCutoff(bool cutoff) { Cutoff = cutoff; }

  inline void SetGeneratorPower(bool gp) { GeneratorPower=gp; }
  inline void SetCondition(bool c) { Condition=c; }
  int InitRunning(void);
  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

private:

  phaseType phase;         ///< Operating mode, or "phase"
  double IdleN1;           ///< Idle N1
  double N1;               ///< N1
  double MaxN1;            ///< N1 at 100% throttle
  double N1_factor;        ///< factor to tie N1 and throttle
  double ThrottlePos;      ///< FCS-supplied throttle position, modified locally
  bool Cutoff;

  double OilPressure_psi;
  double OilTemp_degK;

  double Ielu_max_torque;      // max shaft torque (before ielu intervent)
  bool Ielu_intervent;
  double OldThrottle;

  double Idle_Max_Delay;       // time delay for exponential
  double MaxPower;             // max engine power [HP]
  double StarterN1;            // N1 the starter alone can reach [%]
  double MaxStartingTime;      // maximal time for start [s] (-1 means not used)
  double RPM;                  // shaft RPM
  double PSFC;                 // Power specific fuel comsumption [lb/(HP*hr)] at best efficiency
  double CombustionEfficiency;

  double HP;                   // engine power output

  double StartTime;            // engine starting time [s] (0 when starter engaged)

  double  ITT_Delay;           // time delay for exponential growth of ITT
  double  Eng_ITT_degC;
  double  Eng_Temperature;     // temperature inside engine

  bool EngStarting;            // logical output - TRUE if engine is starting
  bool GeneratorPower;
  int Condition;

  double Off(void);
  double Run(void);
  double SpinUp(void);
  double Start(void);

  void SetDefaults(void);
  bool Load(FGFDMExec *exec, Element *el);
  void bindmodel(FGPropertyManager* pm);
  void Debug(int from);

  std::unique_ptr<FGTable> ITT_N1;             // ITT temperature depending on N1
  std::unique_ptr<FGTable> EnginePowerRPM_N1;
  std::shared_ptr<FGParameter> EnginePowerVC;
  std::unique_ptr<FGTable> CombustionEfficiency_N1;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
