/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTurbine.h
 Author:       David Culp
 Date started: 03/11/2003

 ------------- Copyright (C) 2003  David Culp (davidculp2@comcast.net)----------

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

#include <vector>
#include "FGEngine.h"
#include <input_output/FGXMLElement.h>
#include <math/FGFunction.h>

#define ID_TURBINE "$Id: FGTurbine.h,v 1.4 2005/09/17 17:46:04 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
         a maximum of about %25 N2 (%5.2 N1).  This simulates the action of a
         pneumatic starter.
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
    therefore a seperate ignition system is not modeled.

Configuration File Format
<pre>
\<FG_TURBINE NAME="<name>">
  MILTHRUST   \<thrust>
  MAXTHRUST   \<thrust>
  BYPASSRATIO \<bypass ratio>
  TSFC        \<thrust specific fuel consumption>
  ATSFC       \<afterburning thrust specific fuel consumption>
  IDLEN1      \<idle N1>
  IDLEN2      \<idle N2>
  MAXN1       \<max N1>
  MAXN2       \<max N2>
  AUGMENTED   \<0|1>
  AUGMETHOD   \<0|1>
  INJECTED    \<0|1>
  ...
\</FG_TURBINE>
</pre>
Definition of the turbine engine configuration file parameters:
<pre>
<b>MILTHRUST</b> - Maximum thrust, static, at sea level, lbf.
<b>MAXTHRUST</b> - Afterburning thrust, static, at sea level, lbf
[this value will be ignored when AUGMENTED is zero (false)].
<b>BYPASSRATIO</b> - Ratio of bypass air flow to core air flow.
<b>TSFC</b> - Thrust-specific fuel consumption, lbm/hr/lbf
[i.e. fuel flow divided by thrust].
<b>ATSFC</b> - Afterburning TSFC, lbm/hr/lbf
[this value will be ignored when AUGMENTED is zero (false)]
<b>IDLEN1</b> - Fan rotor rpm (% of max) at idle
<b>IDLEN2</b> - Core rotor rpm (% of max) at idle
<b>MAXN1</b> - Fan rotor rpm (% of max) at full throttle [not always 100!]
<b>MAXN2</b> - Core rotor rpm (% of max) at full throttle [not always 100!]
<b>AUGMENTED</b>
  0 == afterburner not installed
  1 == afterburner installed
<b>AUGMETHOD</b>
  0 == afterburner activated by property /engines/engine[n]/augmentation
  1 == afterburner activated by pushing throttle above 99% position
  2 == throttle range is expanded in the FCS, and values above 1.0 are afterburner range
  [this item will be ignored when AUGMENTED == 0]
<b>INJECTED</b>
  0 == Water injection not installed
  1 == Water injection installed
</pre>
    @author David P. Culp
    @version "$Id: FGTurbine.h,v 1.4 2005/09/17 17:46:04 jberndt Exp $"
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
      @param engine_number engine number*/
  FGTurbine(FGFDMExec* Executive, Element *el, int engine_number);
  /// Destructor
  ~FGTurbine();

  enum phaseType { tpOff, tpRun, tpSpinUp, tpStart, tpStall, tpSeize, tpTrim };

  double Calculate(void);
  double CalcFuelNeed(void);
  double GetPowerAvailable(void);
  double Seek(double* var, double target, double accel, double decel);

  phaseType GetPhase(void) { return phase; }

  bool GetOvertemp(void)  {return Overtemp; }
  bool GetInjection(void) {return Injection;}
  bool GetFire(void) { return Fire; }
  bool GetAugmentation(void) {return Augmentation;}
  bool GetReversed(void) { return Reversed; }
  bool GetCutoff(void) { return Cutoff; }
  int GetIgnition(void) {return Ignition;}

  double GetInlet(void) { return InletPosition; }
  double GetNozzle(void) { return NozzlePosition; }
  double GetBleedDemand(void) {return BleedDemand;}
  double GetN1(void) {return N1;}
  double GetN2(void) {return N2;}
  double GetEPR(void) {return EPR;}
  double GetEGT(void) {return EGT_degC;}

  double getOilPressure_psi () const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}

  void SetInjection(bool injection) {Injection = injection;}
  void SetIgnition(int ignition) {Ignition = ignition;}
  void SetAugmentation(bool augmentation) {Augmentation = augmentation;}
  void SetPhase( phaseType p ) { phase = p; }
  void SetEPR(double epr) {EPR = epr;}
  void SetBleedDemand(double bleedDemand) {BleedDemand = bleedDemand;}
  void SetReverse(bool reversed) { Reversed = reversed; }
  void SetCutoff(bool cutoff) { Cutoff = cutoff; }

  string GetEngineLabels(string delimeter);
  string GetEngineValues(string delimeter);

private:

  phaseType phase;         ///< Operating mode, or "phase"
  double MilThrust;        ///< Maximum Unaugmented Thrust, static @ S.L. (lbf)
  double MaxThrust;        ///< Maximum Augmented Thrust, static @ S.L. (lbf)
  double BypassRatio;      ///< Bypass Ratio
  double TSFC;             ///< Thrust Specific Fuel Consumption (lbm/hr/lbf)
  double ATSFC;            ///< Augmented TSFC (lbm/hr/lbf)
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
  double ThrottlePos;      ///< FCS-supplied throttle position
  double AugmentCmd;       ///< modulated afterburner command (0.0 to 1.0)
  double TAT;              ///< total air temperature (deg C)
  bool Stalled;            ///< true if engine is compressor-stalled
  bool Seized;             ///< true if inner spool is seized
  bool Overtemp;           ///< true if EGT exceeds limits
  bool Fire;               ///< true if engine fire detected
  bool Injection;
  bool Augmentation;
  bool Reversed;
  bool Cutoff;
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

  double Off(void);
  double Run();
  double SpinUp(void);
  double Start(void);
  double Stall(void);
  double Seize(void);
  double Trim();

  FGFunction *IdleThrustLookup;
  FGFunction *MilThrustLookup;
  FGFunction *MaxThrustLookup;
  FGFunction *InjectionLookup;

  void SetDefaults(void);
  bool Load(FGFDMExec *exec, Element *el);
  void bindmodel(void);
  void unbind(void);
  void Debug(int from);

};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

