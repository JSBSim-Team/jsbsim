/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSimTurbine.h
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSIMTURBINE_H
#define FGSIMTURBINE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGEngine.h"
#include "FGConfigFile.h"
#include "FGCoefficient.h"

#define ID_SIMTURBINE "$Id: FGSimTurbine.h,v 1.12 2003/11/09 05:32:57 jberndt Exp $"

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
<FG_SIMTURBINE NAME="<name>">
  MILTHRUST   <thrust>
  MAXTHRUST   <thrust>
  BYPASSRATIO <bypass ratio>
  TSFC        <thrust specific fuel consumption>
  ATSFC       <afterburning thrust specific fuel consumption>
  IDLEN1      <idle N1>
  IDLEN2      <idle N2>
  MAXN1       <max N1>
  MAXN2       <max N2>
  AUGMENTED   <0|1>
  AUGMETHOD   <0|1>
  INJECTED    <0|1>
  ...
</FG_SIMTURBINE>
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
  [this item will be ignored when AUGMENTED == 0]
<b>INJECTED</b>
  0 == Water injection not installed
  1 == Water injection installed
</pre>
    @author David P. Culp
    @version $Id: FGSimTurbine.h,v 1.12 2003/11/09 05:32:57 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSimTurbine : public FGEngine
{
public:
  /** Constructor
      @param exec pointer to executive structure
      @param Eng_Cfg pointer to engine config file instance */
  FGSimTurbine(FGFDMExec* exec, FGConfigFile* Eng_cfg);
  /// Destructor
  ~FGSimTurbine();

  enum phaseType { tpOff, tpRun, tpSpinUp, tpStart, tpStall, tpSeize, tpTrim };

  double Calculate(double);
  double CalcFuelNeed(void);
  double GetPowerAvailable(void);
  double Seek(double* var, double target, double accel, double decel);

  virtual phaseType GetPhase(void) { return phase; }
  virtual void SetPhase( phaseType p ) { phase = p; } 

  virtual bool GetOvertemp(void) { return Overtemp; }
  virtual bool GetFire(void) { return Fire; }
  
private:

  typedef vector<FGCoefficient*> CoeffArray;
  CoeffArray ThrustTables;

  phaseType phase;         ///< Operating mode, or "phase"
  double MilThrust;        ///< Maximum Unaugmented Thrust, static @ S.L. (lbf)
  double MaxThrust;        ///< Maximum Augmented Thrust, static @ S.L. (lbf)
  double BypassRatio;      ///< Bypass Ratio
  double TSFC;             ///< Thrust Specific Fuel Consumption (lbm/hr/lbf)
  double ATSFC;            ///< Augmented TSFC (lbm/hr/lbf)
  double IdleN1;           ///< Idle N1
  double IdleN2;           ///< Idle N2
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
  int Augmented;           ///< = 1 if augmentation installed
  int Injected;            ///< = 1 if water injection installed
  int AugMethod;           ///< = 0 if using property /engine[n]/augmentation
                           ///< = 1 if using last 1% of throttle movement

  double Off(void);
  double Run(void);
  double SpinUp(void);
  double Start(void);
  double Stall(void);
  double Seize(void);
  double Trim(void);

  void SetDefaults(void);
  bool Load(FGConfigFile *ENG_cfg);
  void Debug(int from);

};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

