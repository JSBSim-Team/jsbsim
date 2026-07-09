/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGearbox.h
 Author:       jsbsim-programmer (twin-engine-helicopter feature, REQ-003)
 Date started: 07/10/26

 ------------- Copyright (C) 2026 -------------

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
07/10/26  Created. See docs/adr/ADR-001-twin-engine-rotorcraft-transmission.md
          and docs/interfaces/twin-engine-gearbox-interface.md (normative).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGGEARBOX_H
#define FGGEARBOX_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <vector>
#include <memory>

#include "FGJSBBase.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGEngine;
class FGThruster;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Combines N engines onto one shared output shaft/thruster (e.g. a twin-
    engine helicopter's main-rotor gearbox).

    This generalizes FGTransmission's single-engine brake/clutch/free-wheel-
    unit (FWU) math (see FGTransmission.h) to N independent engine "channels"
    that each keep their own FWU/clutch state while sharing one combined
    output-shaft RPM and inertia (\<gearmoment\>). A channel that is cut to
    zero power (via the engine's ordinary Cutoff/Running controls) free-
    wheels off the shared shaft exactly like a single-engine FGTransmission
    does today -- no special-cased "engine failed" branch is needed.

    FGGearbox owns the shared FGThruster (constructed from its \<thruster\>
    child, same rotor/propeller/nozzle/direct config format as a normal
    \<engine\>\<thruster\>). Each feeding FGEngine gets a non-owning pointer
    to that same FGThruster (see FGEngine::AssignGearboxThruster) so its
    existing GetThrust()/GetBodyForces()/GetMoments() accessors keep working
    unchanged; only FGGearbox calls the shared Thruster's Calculate(), once
    per frame, after resolving the combined shaft RPM from summed channel
    torque (see FGPropulsion::Run()).

    <h3>Rotor thrusters and RPM control</h3>

    Because the combined shaft RPM is resolved by FGGearbox rather than by a
    per-rotor FGTransmission, a \<rotor\> thruster driven by a gearbox MUST
    be configured with \<ExternalRPM\>-1\</ExternalRPM\> in its own rotor
    config file. This keeps FGRotor from instantiating its own (single-
    engine) FGTransmission internally, and makes it read its RPM each frame
    from the "x-rpm-dict" property that FGGearbox drives -- an existing,
    unmodified FGRotor mechanism (see FGRotor.h), reused rather than
    special-cased. FGGearbox validates this requirement at load time.

    <h3>Configuration File Format</h3>
@code
    <gearbox name="{string}">
        <gearmoment unit="{SLUG*FT2}">    {number} </gearmoment>
        <maxbrakepower unit="{POWER}">    {number} </maxbrakepower>

        <input engine="{integer}">
            <gearratio>                   {number} </gearratio>
            <enginemoment unit="{SLUG*FT2}"> {number} </enginemoment>
            <enginefriction unit="{POWER}">  {number} </enginefriction>
        </input>
        ... one or more further <input> elements, one per feeding engine ...

        <thruster file="{string}">
            <location unit="{IN | M}"> <x/><y/><z/> </location>
            <orient unit="{RAD | DEG}"> <roll/><pitch/><yaw/> </orient>
        </thruster>
    </gearbox>
@endcode

    <h3>Property tree</h3>

    The following properties are created (with x = the gearbox's own index,
    and n = the feeding engine's index):
    <pre>
      propulsion/gearbox[x]/shaft-rpm             (read-only)
      propulsion/gearbox[x]/brake-ctrl-norm        (read-write)
      propulsion/gearbox[x]/one-engine-inoperative (read-only)

      propulsion/engine[n]/output-torque-lbft      (read-only, per channel)
      propulsion/engine[n]/free-wheel-transmission (read-only, per channel)
      propulsion/engine[n]/clutch-ctrl-norm        (read-write, per channel)
    </pre>

    See docs/interfaces/twin-engine-gearbox-interface.md (normative) for the
    full frozen schema/property contract.

    @author jsbsim-programmer
    @see FGTransmission
    @see FGRotor
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGearbox : public FGJSBBase {

public:
  /** Constructor for FGGearbox. Parses gearmoment/maxbrakepower, one
      channel per <input engine="x">, and the shared <thruster>. Wires each
      referenced engine to this gearbox via FGEngine::AssignGearboxThruster.
      Throws XMLLogException/LogException (JSBSim's standard load-time error
      convention, see FGLog.h) identifying the offending element on any of
      the validation failures documented in
      docs/requirements/REQ-004-gearbox-xml-parsing.md:
        - an <input> references an engine index that already has a thruster
          (its own inline <thruster>, or an earlier gearbox's claim)
        - an <input> references an out-of-range engine index
        - the gearbox itself has no <thruster> child
        - a <rotor> thruster is missing the required
          <ExternalRPM>-1</ExternalRPM>
      @param exec a pointer to the main executive object
      @param gearbox_element a pointer to the <gearbox> config file element.
             Its <thruster> child must already have been resolved (any
             file="..." reference opened/merged) by the caller, exactly as
             FGPropulsion::Load() does for a plain <engine><thruster>.
      @param num the index of this gearbox within FGPropulsion's list
      @param engines the already-loaded engine list, indexed by engine
             number, used to resolve <input engine="x"> references */
  FGGearbox(FGFDMExec* exec, Element* gearbox_element, int num,
            const std::vector<std::shared_ptr<FGEngine>>& engines);

  /// Destructor for FGGearbox. Owns (and deletes) the shared FGThruster.
  ~FGGearbox();

  /** Feeds one engine channel's computed power into this gearbox for the
      current frame. Called by a gearbox-fed FGEngine's own Calculate()
      (FGElectric/FGTurboProp), in place of Thruster->Calculate(power).
      @param engine_index the global engine index (matches an <input
             engine="x">) whose channel should receive this power
      @param power_ftlbs_per_sec the engine's delivered power this frame */
  void SetChannelPower(int engine_index, double power_ftlbs_per_sec);

  /** Resolves the combined shaft RPM from summed, per-channel free-wheel/
      clutch-limited torque (reusing FGTransmission's per-channel FWU/clutch
      heuristic, generalized to N channels sharing one shaft/inertia), then
      calls the shared Thruster's Calculate() exactly once. Called from
      FGPropulsion::Run()'s second pass, after every engine (gearbox-fed or
      not) has run its own Calculate().
      No dynamic allocation or blocking calls -- real-time safe.
      @param dt the simulation delta-T for this frame */
  void Calculate(double dt);

  FGThruster* GetThruster(void) const {return Thruster;}

  double GetShaftRPM(void) const {return ThrusterRPM;}
  double GetBrakeCtrlNorm(void) const {return BrakeCtrlNorm;}
  void   SetBrakeCtrlNorm(double x) {BrakeCtrlNorm = x;}
  /// True when a channel has free-wheeled off the shaft while the shaft is
  /// still turning under the remaining channel(s) -- i.e. one-engine-
  /// inoperative, derived purely from existing per-channel FWU/torque state
  /// (see docs/requirements/REQ-005-engine-out-instrumentation.md).
  bool GetOneEngineInoperative(void) const;

  double GetGearMoment(void) const {return GearMoment;}
  double GetMaxBrakePower(void) const {return MaxBrakePower;}

  size_t GetNumChannels(void) const {return Channels.size();}

  std::string GetName(void) const {return Name;}

  // Per-channel accessors, indexed by this channel's position within
  // Channels (not by engine number -- see BindChannel for the property-
  // tree naming, which does use the engine number). Public so JSBSim's
  // indexed property-Tie template can bind them, and so a future load-
  // sharing governor (REQ-006) can read delivered torque directly.
  double GetChannelFreeWheel(int channel_idx) const {return Channels[channel_idx].FreeWheelTransmission;}
  double GetChannelOutputTorque(int channel_idx) const {return Channels[channel_idx].OutputTorque;}
  double GetChannelClutchCtrlNorm(int channel_idx) const {return Channels[channel_idx].ClutchCtrlNorm;}
  void   SetChannelClutchCtrlNorm(int channel_idx, double x) {Channels[channel_idx].ClutchCtrlNorm = x;}

private:
  /// One engine's independent free-wheel/clutch channel into the shared
  /// shaft -- generalizes FGTransmission's single (engine-side) state.
  struct Channel {
    FGEngine* Engine = nullptr;
    int EngineIndex = -1;
    double GearRatio = 1.0;        // reporting only, mirrors FGRotor's own <gearratio>
    double EngineMoment = 1.0;     // <enginemoment>, SLUG*FT2
    double EngineFriction = 0.0;   // <enginefriction>, converted to FT*LBS/SEC
    double ClutchCtrlNorm = 1.0;
    double EngineRPM = 0.0;
    double FreeWheelTransmission = 1.0; // state, 0: free, 1: locked
    Filter FreeWheelLag;
    double InputPower = 0.0;       // this frame's power from SetChannelPower()
    double OutputTorque = 0.0;     // delivered torque through clutch/FWU
  };

  bool BindGearbox(int num, FGPropertyManager* pm);
  bool BindChannel(size_t channel_idx, FGPropertyManager* pm);
  void Debug(int from);

  inline double omega_to_rpm(double w) const {
    return w * 9.54929658551372014613302580235; // omega/(2.0*PI) * 60.0
  }
  inline double rpm_to_omega(double r) const {
    return r * 0.104719755119659774615421446109; // (rpm/60.0)*2.0*PI
  }

  std::string Name;

  FGThruster* Thruster;
  SGPropertyNode_ptr XRpmDictNode; // this gearbox's thruster's x-rpm-dict

  std::vector<Channel> Channels;

  double GearMoment;    // combined output-shaft inertia, SLUG*FT2
  double MaxBrakePower; // FT*LBS/SEC
  double BrakeCtrlNorm;
  double ThrusterRPM;   // combined output shaft RPM
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
