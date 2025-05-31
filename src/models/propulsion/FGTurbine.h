/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTurbine.h
 Author:       David Culp
 Date started: 03/11/2003

 ------------- Copyright (C) 2003  David Culp (daveculp@cox.net)----------

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
03/11/2003  DPC  Created, based on FGTurbine
09/22/2003  DPC  Added starting, stopping, new framework
04/29/2004  DPC  Renamed from FGSimTurbine to FGTurbine

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTURBINE_H
#define FGTURBINE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFunction;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class models a turbine engine.  Based on Jon Berndt's FGTurbine module.
    Here the term "phase" signifies the engine's mode of operation.  At any given
    time the engine is in only one phase.  At simulator startup the engine will be
    placed in the Trim phase in order to provide a simplified thrust value without
    throttle lag.  When trimming is complete the engine will go to the Off phase,
    unless the value FGEngine::Running has been previously set to true, in which
    case the engine will go to the Run phase.  Once an engine is in the Off phase
    the full starting procedure (or airstart) must be used to get it running.
<P>
    - STARTING (on ground):
      -# Set the control FGEngine::Starter to true.  The engine will spin up to
         a maximum of about %25 N2 (%5.2 N1). This value may be changed using the <startnX> parameter.
		 This simulates the action of a pneumatic starter.
      -# After reaching %15 N2 set the control FGEngine::Cutoff to false. If fuel
         is available the engine will now accelerate to idle.  The starter will
         automatically be set to false after the start cycle.
<P>
    - STARTING (in air):
      -# Increase speed to obtain a minimum of %15 N2.  If this is not possible,
         the starter may be used to assist.
      -# Place the control FGEngine::Cutoff to false.
<P>
    Ignition is assumed to be on anytime the Cutoff control is set to false,
    therefore a separate ignition system is not modeled.

<h3>Configuration File Format:</h3>
@code
 <turbine_engine name="{string}">
  <milthrust unit="{LBS | N}"> {number} </milthrust>
  <maxthrust unit="{LBS | N}"> {number} </maxthrust>
  <bypassratio> {number} </bypassratio>
  <bleed> {number} </bleed>
  <tsfc> {number} </tsfc>
  <atsfc> {number} </atsfc>
  <ignitionn1> {number} </ignitionn1>
  <ignitionn2> {number} </ignitionn2>
  <idlen1> {number} </idlen1>
  <idlen2> {number} </idlen2>
  <n1spinup> {number} </n1spinup>
  <n2spinup> {number} </n2spinup>
  <n1startrate> {number} </n1startrate>
  <n2startrate> {number} </n2startrate>
  <n1spindown> {number} </n1spindown>
  <n2spindown> {number} </n2spindown>
  <maxn1> {number} </maxn1>
  <maxn2> {number} </maxn2>
  <augmented> {0 | 1} </augmented>
  <augmethod> {0 | 1 | 2} </augmethod>
  <injected> {0 | 1} </injected>
  <injection-time> {number} </injection-time>
  <disable-windmill> {0 | 1}</disable-windmill>
 </turbine_engine>
@endcode

<h3>Definition of the turbine engine configuration file parameters:</h3>

<pre>
  milthrust   - Maximum thrust, static, at sea level.
  maxthrust   - Afterburning thrust, static, at sea level.
  bypassratio - Ratio of bypass air flow to core air flow.
  bleed       - Thrust reduction factor due to losses (0.0 to 1.0).
  tsfc        - Thrust-specific fuel consumption at cruise, lbm/hr/lbf
  atsfc       - Afterburning TSFC, lbm/hr/lbf
  ignitionn1  - Fan rotor rpm (% of max) while starting
  ignitionn2  - Core rotor rpm (% of max) while starting
  idlen1      - Fan rotor rpm (% of max) at idle
  idlen2      - Core rotor rpm (% of max) at idle
  n1spinup    - Fan rotor rpm starter acceleration to ignitionn1 value (default 1.0)
  n2spinup    - Core rotor rpm starter acceleration to ignitionn2 value (default 3.0)
  n1startrate - Fan rotor rpm time taken to accelerate from ignitionn1 to idlen1 value (default 1.4)
  n2startrate - Core rotor rpm time taken to accelerate to ignitionn2 idlen2 value (default 2.0)
  n1spindown  - Factor used in calculation for fan rotor time to spool down to zero (default 2.0)
  n2spindown  - Factor used in calculation for core rotor time to spool down to zero (default 2.0)
  maxn1       - Fan rotor rpm (% of max) at full throttle
  maxn2       - Core rotor rpm (% of max) at full throttle
  augmented
              0 = afterburner not installed
              1 = afterburner installed
  augmethod
              0 = afterburner activated by property /engines/engine[n]/augmentation
              1 = afterburner activated by pushing throttle above 99% position
              2 = throttle range is expanded in the FCS, and values above 1.0 are afterburner range
  injected
              0 = Water injection not installed
              1 = Water injection installed
  injection-time - Time, in seconds, of water injection duration
  InjN1increment - % increase in N1 when injection is taking place
  InjN2increment - % increase in N2 when injection is taking place
  disable-windmill - flag that disables engine windmilling when off if true
</pre>

<h3>NOTES:</h3>
<pre>
    Bypass ratio is used only to estimate engine acceleration time.  The
    effect of bypass ratio on engine efficiency is already included in
    the TSFC value.  Feel free to set this parameter (even for turbojets) to
    whatever value gives a desired spool-up rate. Default value is 0.

    The bleed factor is multiplied by thrust to give a resulting thrust
    after losses.  This can represent losses due to bleed, or any other cause.
    Default value is 0.  A common value would be 0.04.

    Nozzle position, for variable area exhaust nozzles, is provided for users
    needing to drive a nozzle gauge or animate a virtual nozzle.

    This model can only be used with the "direct" thruster.  See the file:
    /engine/direct.xml
</pre>
    @author David P. Culp
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTurbine : public FGEngine
{
public:
  /** Constructor
      @param Executive pointer to executive structure
      @param el pointer to the XML element representing the turbine engine
      @param engine_number engine number  */
  FGTurbine(FGFDMExec* Executive, Element *el, int engine_number, struct Inputs& input);

  enum phaseType { tpOff, tpRun, tpSpinUp, tpStart, tpStall, tpSeize, tpTrim };

  void Calculate(void);
  double CalcFuelNeed(void);
  double GetPowerAvailable(void) const;
  /** A lag filter.
      Used to control the rate at which values are allowed to change.
      @param var a pointer to a variable of type double
      @param target the desired (target) value
      @param accel the rate, per second, the value may increase
      @param decel the rate, per second, the value may decrease    */
  double Seek(double* var, double target, double accel, double decel);

  phaseType GetPhase(void) { return phase; }

  bool GetOvertemp(void)  const {return Overtemp; }
  bool GetInjection(void) const {return Injection;}
  bool GetFire(void) const { return Fire; }
  bool GetAugmentation(void) const {return Augmentation;}
  bool GetReversed(void) const { return Reversed; }
  bool GetCutoff(void) const { return Cutoff; }
  int GetIgnition(void) const {return Ignition;}

  double GetInlet(void) const { return InletPosition; }
  double GetNozzle(void) const { return NozzlePosition; }
  double GetBleedDemand(void) const {return BleedDemand;}
  double GetN1(void) const {return N1;}
  double GetN2(void) const {return N2;}
  double GetEPR(void) const {return EPR;}
  double GetEGT(void) const {return EGT_degC;}

  double GetMaxN1(void) const {return MaxN1;}
  double GetMaxN2(void) const {return MaxN2;}
  double getOilPressure_psi () const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}
  double GetInjectionTimer(void) const {return InjectionTimer;}
  double GetInjWaterNorm(void) const {return InjWaterNorm;}
  double GetInjN1increment(void) const {return InjN1increment;}
  double GetInjN2increment(void) const {return InjN2increment;}

  void SetInjection(bool injection) {Injection = injection;}
  void SetIgnition(int ignition) {Ignition = ignition;}
  void SetAugmentation(bool augmentation) {Augmentation = augmentation;}
  void SetPhase( phaseType p ) { phase = p; }
  void SetEPR(double epr) {EPR = epr;}
  void SetBleedDemand(double bleedDemand) {BleedDemand = bleedDemand;}
  void SetReverse(bool reversed) { Reversed = reversed; }
  void SetCutoff(bool cutoff) { Cutoff = cutoff; }
  void SetMaxN1(double maxn1) {MaxN1 = maxn1;}
  void SetMaxN2(double maxn2) {MaxN2 = maxn2;}
  void SetInjectionTimer(double injtimer) {InjectionTimer = injtimer;}
  void SetInjWaterNorm(double injwater) {InjWaterNorm = injwater;}
  void SetInjN1increment(double injN1inc) {InjN1increment = injN1inc;}
  void SetInjN2increment(double injN2inc) {InjN2increment = injN2inc;}

  int InitRunning(void);
  void ResetToIC(void);

  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

private:

  phaseType phase;         ///< Operating mode, or "phase"
  double MilThrust;        ///< Maximum Unaugmented Thrust, static @ S.L. (lbf)
  double MaxThrust;        ///< Maximum Augmented Thrust, static @ S.L. (lbf)
  double BypassRatio;      ///< Bypass Ratio
  std::unique_ptr<FGParameter> TSFC;   ///< Thrust Specific Fuel Consumption (lbm/hr/lbf)
  std::unique_ptr<FGParameter> ATSFC;  ///< Augmented TSFC (lbm/hr/lbf)
  double IdleN1;           ///< Idle N1
  double IdleN2;           ///< Idle N2
  double IgnitionN1;       ///< Ignition N1
  double IgnitionN2;       ///< Ignition N2
  double N1;               ///< N1
  double N2;               ///< N2
  double N2norm;           ///< N2 normalized (0=idle, 1=max)
  double MaxN1;            ///< N1 at 100% throttle
  double MaxN2;            ///< N2 at 100% throttle
  double IdleFF;           ///< Idle Fuel Flow (lbm/hr)
  double N1_factor;        ///< factor to tie N1 and throttle
  double N2_factor;        ///< factor to tie N2 and throttle
  double ThrottlePos;      ///< FCS-supplied throttle position - modified for local use!
  double AugmentCmd;       ///< modulated afterburner command (0.0 to 1.0)
  double N1_spinup;        ///< N1 spin up rate from pneumatic starter (per second)
  double N2_spinup;        ///< N2 spin up rate from pneumatic starter (per second)
  double N1_start_rate;    ///< N1 spin up rate from ignition (per second)
  double N2_start_rate;    ///< N2 spin up rate from ignition (per second)
  double N1_spindown;      ///< N1 spin down factor
  double N2_spindown;      ///< N2 spin down factor
  bool Stalled;            ///< true if engine is compressor-stalled
  bool Seized;             ///< true if inner spool is seized
  bool Overtemp;           ///< true if EGT exceeds limits
  bool Fire;               ///< true if engine fire detected
  bool Injection;
  bool Augmentation;
  bool Reversed;
  bool Cutoff;
  bool disableWindmill;    ///< flag to disable windmilling of engine in Off phase
  int Injected;            ///< = 1 if water injection installed
  int Ignition;
  int Augmented;           ///< = 1 if augmentation installed
  int AugMethod;           ///< = 0 if using property /engine[n]/augmentation
                           ///< = 1 if using last 1% of throttle movement
                           ///< = 2 if using FCS-defined throttle
  double EGT_degC;
  double EPR;
  double OilPressure_psi;
  double OilTemp_degK;
  double BleedDemand;
  double InletPosition;
  double NozzlePosition;
  double correctedTSFC;
  double InjectionTimer;
  double InjectionTime;
  double InjWaterNorm;
  double InjN1increment;
  double InjN2increment;

  double Off(void);
  double Run();
  double SpinUp(void);
  double Start(void);
  double Stall(void);
  double Seize(void);
  double Trim();

  std::shared_ptr<FGFunction> IdleThrustLookup;
  std::shared_ptr<FGFunction> MilThrustLookup;
  std::shared_ptr<FGFunction> MaxThrustLookup;
  std::shared_ptr<FGFunction> InjectionLookup;
  FGFDMExec *FDMExec;
  std::shared_ptr<FGParameter> N1SpoolUp;
  std::shared_ptr<FGParameter> N1SpoolDown;
  std::shared_ptr<FGParameter> N2SpoolUp;
  std::shared_ptr<FGParameter> N2SpoolDown;

  bool Load(FGFDMExec *exec, Element *el);
  void bindmodel(FGPropertyManager* pm);
  void Debug(int from);

  friend class FGSpoolUp;
  friend class FGSimplifiedTSFC;
};

class FGSpoolUp : public FGParameter
{
public:
  FGSpoolUp(FGTurbine* _turb, double BPR, double factor)
    : turb(_turb), delay(factor * 90.0 / (BPR + 3.0)) {}
  std::string GetName(void) const { return std::string(); };
  double GetValue(void) const {
    // adjust acceleration for N2 and atmospheric density
    double n = std::min(1.0, turb->N2norm + 0.1);
    return delay / (1 + 3 * (1-n)*(1-n)*(1-n) + (1 - turb->in.DensityRatio));
  }
private:
  FGTurbine* turb;
  double delay; ///< Inverse spool-up time from idle to 100% (seconds)
};

class FGSimplifiedTSFC : public FGParameter
{
public:
  FGSimplifiedTSFC(FGTurbine* _turb, double tsfcVal)
    : turb(_turb), tsfc(tsfcVal) {}

  std::string GetName(void) const { return std::string(); }

  double GetValue(void) const {
    // Correction/denormalisation for temp and thrust
      double T = turb->in.Temperature;
      double N2norm = turb->N2norm;
      return tsfc * sqrt(T / 389.7) * (0.84 + (1 - N2norm) * (1 - N2norm));
  }

private:
  FGTurbine* turb;
  double tsfc;
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
