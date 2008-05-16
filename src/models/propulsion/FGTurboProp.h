/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTurboProp.h
 Author:       Jiri "Javky" Javurek
               based on SimTurbine and Turbine engine from David Culp
 Date started: 05/14/2004

 ------------- Copyright (C) 2004  (javky@email.cz)----------

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
05/14/2004  Created

//JVK (mark)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTURBOPROP_H
#define FGTURBOPROP_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGEngine.h"
#include <input_output/FGXMLElement.h>
#include <math/FGTable.h>

#define ID_TURBOPROP "$Id: FGTurboProp.h,v 1.8 2008/05/16 04:04:32 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Turboprop engine model.  For an example of this model in use see the file:
    engine/engtm601.xml
 <h3>Configuration parameters:</h3>
<pre>
milthrust   [LBS]
idlen1      [%]
maxn1       [%]
betarangeend[%]
    if ThrottleCmd < betarangeend/100.0 then engine power=idle, propeller pitch
    is controled by ThrottleCmd (between MINPITCH and  REVERSEPITCH).
    if ThrottleCmd > betarangeend/100.0 then engine power increases up to max reverse power
reversemaxpower [%]
    max engine power in reverse mode
maxpower    [HP]
psfc power specific fuel consumption [pph/HP] for N1=100%
n1idle_max_delay [-] time constant for N1 change
maxstartenginetime [sec]
    after this time the automatic starting cycle is interrupted when the engine
    doesn't start (0=automatic starting not present)
startern1   [%]
    when starting starter spin up engine to this spin
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

class FGTurboProp : public FGEngine
{
public:
  /** Constructor
      @param Executive pointer to executive structure
      @param el pointer to the XML element representing the turbine engine
      @param engine_number engine number*/
  FGTurboProp(FGFDMExec* Executive, Element *el, int engine_number);
  /// Destructor
  ~FGTurboProp();

  enum phaseType { tpOff, tpRun, tpSpinUp, tpStart, tpStall, tpSeize, tpTrim };

  double Calculate(void);
  double CalcFuelNeed(void);

  inline double GetPowerAvailable(void) const {return (Eng_HP * hptoftlbssec);}
  inline double GetPowerAvailable_HP(void) const {return (Eng_HP);}
  inline double GetPropRPM(void) const {return (Prop_RPM);}
  inline double GetThrottleCmd(void) const {return (ThrottleCmd);}
  inline bool GetIeluIntervent(void) const { return Ielu_intervent; }

  double Seek(double* var, double target, double accel, double decel);
  double ExpSeek(double* var, double target, double accel, double decel);

  phaseType GetPhase(void) const { return phase; }

  bool GetOvertemp(void) const {return Overtemp; }
  bool GetFire(void) const { return Fire; }
  bool GetReversed(void) const { return Reversed; }
  bool GetCutoff(void) const { return Cutoff; }
  int GetIgnition(void) const {return Ignition;}

  double GetInlet(void) const { return InletPosition; }
  double GetNozzle(void) const { return NozzlePosition; }
  double GetN1(void) const {return N1;}
  double GetN2(void) const {return N2;}
  double GetEPR(void) const {return EPR;}
  double GetITT(void) const {return Eng_ITT_degC;}
  double GetEngStarting(void) const { return EngStarting; }

  double getOilPressure_psi () const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}

  inline bool GetGeneratorPower(void) const { return GeneratorPower; }
  inline int GetCondition(void) const { return Condition; }

  void SetIgnition(int ignition) {Ignition = ignition;}
  void SetPhase( phaseType p ) { phase = p; }
  void SetEPR(double epr) {EPR = epr;}
  void SetReverse(bool reversed) { Reversed = reversed; }
  void SetCutoff(bool cutoff) { Cutoff = cutoff; }

  inline void SetGeneratorPower(bool gp) { GeneratorPower=gp; }
  inline void SetCondition(bool c) { Condition=c; }
  int InitRunning(void);
  string GetEngineLabels(string delimeter);  // added from Turbine 0.9.6
  string GetEngineValues(string delimeter);  // added from Turbine 0.9.6

private:

  phaseType phase;         ///< Operating mode, or "phase"
  double MilThrust;        ///< Maximum Unaugmented Thrust, static @ S.L. (lbf)
  double IdleN1;           ///< Idle N1
  double IdleN2;           ///< Idle N2
  double N1;               ///< N1
  double N2;               ///< N2
  double MaxN1;            ///< N1 at 100% throttle
  double MaxN2;            ///< N2 at 100% throttle
  double IdleFF;           ///< Idle Fuel Flow (lbm/hr)
  double delay;            ///< Inverse spool-up time from idle to 100% (seconds)
  double dt;               ///< Simulator time slice
  double N1_factor;        ///< factor to tie N1 and throttle
  double N2_factor;        ///< factor to tie N2 and throttle
  double ThrottleCmd;      ///< FCS-supplied throttle position
  double TAT;              ///< total air temperature (deg C)
  bool Stalled;            ///< true if engine is compressor-stalled
  bool Seized;             ///< true if inner spool is seized
  bool Overtemp;           ///< true if EGT exceeds limits
  bool Fire;               ///< true if engine fire detected
  bool Reversed;
  bool Cutoff;
  int Ignition;

  double EPR;
  double OilPressure_psi;
  double OilTemp_degK;
  double InletPosition;
  double NozzlePosition;

  double Ielu_max_torque;      // max propeller torque (before ielu intervent)
  bool Ielu_intervent;
  double OldThrottle;

  double BetaRangeThrottleEnd; // coef (0-1) where is end of beta-range
  double ReverseMaxPower;      // coef (0-1) multiplies max throttle on reverse

  double Idle_Max_Delay;       // time delay for exponencial
  double MaxPower;             // max engine power [HP]
  double StarterN1;	       // rotates of generator maked by starter [%]
  double MaxStartingTime;      // maximal time for start [s] (-1 means not used)
  double Prop_RPM;             // propeller RPM
  double Velocity;
  double rho;
  double PSFC;                 // Power specific fuel comsumption [lb/(HP*hr)] at best efficiency

  double Eng_HP;               // current engine power

  double StartTime;	       // engine strating time [s] (0 when start button pushed)

  double  ITT_Delay;	       // time delay for exponencial grow of ITT
  double  Eng_ITT_degC;
  double  Eng_Temperature;     // temperature inside engine

  bool EngStarting;            // logicaly output - TRUE if engine is starting
  bool GeneratorPower;
  int Condition;

  double Off(void);
  double Run(void);
  double SpinUp(void);
  double Start(void);

  void SetDefaults(void);
  bool Load(FGFDMExec *exec, Element *el);
  void bindmodel(void);
  void Debug(int from);

  FGTable* ITT_N1;             // ITT temperature depending on throttle command
  FGTable* EnginePowerRPM_N1;
  FGTable* EnginePowerVC;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
