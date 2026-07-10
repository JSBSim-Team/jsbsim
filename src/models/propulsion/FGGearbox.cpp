/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGearbox.cpp
 Author:       jsbsim-programmer (twin-engine-helicopter feature, REQ-003)
 Date started: 07/10/26

 ------------- Copyright (C) 2026 -------------

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
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
See FGGearbox.h. Generalizes FGTransmission's single-engine brake/clutch/
free-wheel-unit math (per-channel) to N engines sharing one output shaft.

HISTORY
--------------------------------------------------------------------------------
07/10/26  Created.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGGearbox.h"
#include "FGEngine.h"
#include "FGPropeller.h"
#include "FGNozzle.h"
#include "FGRotor.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using std::string;
using std::cout;
using std::endl;

namespace JSBSim {

namespace {
  // 5-in-1 value-fetch-convert-default-return helper, mirroring FGRotor's
  // own ConfigValueConv (kept local to this file rather than shared, to
  // avoid a cross-class edit for what is otherwise a private convenience).
  double ConfigValueConv(Element* el, const string& ename, double default_val,
                          const string& unit = "")
  {
    Element* e = el ? el->FindElement(ename) : nullptr;
    if (!e) return default_val;
    if (unit.empty()) return e->GetDataAsNumber();
    return el->FindElementValueAsNumberConvertTo(ename, unit);
  }
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGGearbox::FGGearbox(FGFDMExec* exec, Element* gearbox_element, int num,
                      const std::vector<std::shared_ptr<FGEngine>>& engines)
  : Thruster(nullptr), GearMoment(1.0), MaxBrakePower(0.0),
    BrakeCtrlNorm(0.0), ThrusterRPM(0.0)
{
  Name = gearbox_element->GetAttributeValue("name");

  // gearmoment is physically load-bearing (it is the combined shaft's
  // inertia -- see ADR-001), so unlike maxbrakepower/enginefriction it is
  // required rather than silently defaulted.
  if (!gearbox_element->FindElement("gearmoment")) {
    XMLLogException err(exec->GetLogger(), gearbox_element);
    err << "<gearbox name=\"" << Name << "\"> is missing the required "
        << "<gearmoment> element (combined output-shaft inertia).";
    throw err;
  }
  GearMoment = gearbox_element->FindElementValueAsNumberConvertTo("gearmoment", "SLUG*FT2");
  GearMoment = Constrain(1e-6, GearMoment, 1e9);

  MaxBrakePower = ConfigValueConv(gearbox_element, "maxbrakepower", 0.0, "HP");
  MaxBrakePower = Constrain(0.0, MaxBrakePower, 1e9);
  MaxBrakePower *= hptoftlbssec;

  double dt = exec->GetDeltaT();
  auto PropertyManager = exec->GetPropertyManager();

  // --- Parse <input engine="x"> channels ---------------------------------

  Element* input_element = gearbox_element->FindElement("input");
  while (input_element) {
    if (!input_element->HasAttribute("engine")) {
      XMLLogException err(exec->GetLogger(), input_element);
      err << "<gearbox name=\"" << Name << "\"> <input> is missing the "
          << "required \"engine\" attribute.";
      throw err;
    }

    int engine_index = (int) input_element->GetAttributeValueAsNumber("engine");

    if (engine_index < 0 || (size_t)engine_index >= engines.size()) {
      XMLLogException err(exec->GetLogger(), input_element);
      err << "<gearbox name=\"" << Name << "\"> <input engine=\"" << engine_index
          << "\"> references a non-existent engine (" << engines.size()
          << " engine(s) defined).";
      throw err;
    }

    // Reject a second <input engine="x"> for the same engine within THIS
    // <gearbox> element, before Channels grows to include it. This must be
    // checked here, against the Channels built so far by this loop -- the
    // engine->GetThruster() check below only becomes non-null once
    // AssignGearboxThruster() runs for every channel, which happens after
    // this whole <input>-parsing loop completes, so it cannot see a
    // same-gearbox duplicate (it only catches an engine already claimed by
    // its own inline <thruster> or an earlier <gearbox>).
    for (const auto& existing : Channels) {
      if (existing.EngineIndex == engine_index) {
        XMLLogException err(exec->GetLogger(), input_element);
        err << "<gearbox name=\"" << Name << "\"> has more than one <input "
            << "engine=\"" << engine_index << "\"> element referencing the "
            << "same engine. Each engine may appear in at most one <input> "
            << "per <gearbox>.";
        throw err;
      }
    }

    FGEngine* engine = engines[engine_index].get();
    // Per ADR-001 ("Explicitly out of scope"): FGGearbox only works with
    // power/torque-based engines. FGElectric and FGTurboProp are the only
    // engine subclasses whose Calculate() checks FeedsGearbox() and hands
    // power to a channel instead of calling Thruster->Calculate() itself;
    // any other engine type here would silently drive the shared thruster
    // multiple times per frame with the wrong physics.
    if (engine->GetType() != FGEngine::etElectric &&
        engine->GetType() != FGEngine::etTurboprop) {
      XMLLogException err(exec->GetLogger(), input_element);
      err << "<gearbox name=\"" << Name << "\"> <input engine=\"" << engine_index
          << "\"> references an engine type that cannot feed a gearbox. "
          << "Only electric_engine and turboprop_engine are supported "
          << "(power/torque-based, per ADR-001).";
      throw err;
    }
    if (engine->GetThruster()) {
      // Subsumes both REQ-004 validation cases: an engine that already has
      // its own inline <thruster>, and an engine already claimed by an
      // earlier <gearbox> (whose claim also sets GetThruster() non-null).
      XMLLogException err(exec->GetLogger(), input_element);
      err << "<gearbox name=\"" << Name << "\"> <input engine=\"" << engine_index
          << "\"> references an engine that already has a thruster (either "
          << "its own inline <thruster>, or it was already claimed by "
          << "another <gearbox>). Each engine may feed at most one "
          << "thruster or gearbox.";
      throw err;
    }

    Channel c;
    c.Engine = engine;
    c.EngineIndex = engine_index;
    c.GearRatio = ConfigValueConv(input_element, "gearratio", 1.0);
    c.GearRatio = Constrain(1e-9, c.GearRatio, 1e9);
    c.EngineMoment = ConfigValueConv(input_element, "enginemoment", 1.0, "SLUG*FT2");
    c.EngineMoment = Constrain(1e-6, c.EngineMoment, 1e9);
    c.EngineFriction = ConfigValueConv(input_element, "enginefriction", 0.0, "HP");
    c.EngineFriction = Constrain(0.0, c.EngineFriction, 1e9) * hptoftlbssec;
    c.ClutchCtrlNorm = 1.0;
    c.EngineRPM = 0.0;
    c.FreeWheelTransmission = 1.0;
    c.FreeWheelLag = Filter(200.0, dt); // avoid too abrupt changes, same as FGTransmission
    c.InputPower = 0.0;
    c.OutputTorque = 0.0;
    Channels.push_back(c);

    input_element = gearbox_element->FindNextElement("input");
  }

  if (Channels.empty()) {
    XMLLogException err(exec->GetLogger(), gearbox_element);
    err << "<gearbox name=\"" << Name << "\"> has no <input> channels.";
    throw err;
  }

  // Per-channel <input><gearratio> is reporting-only (see Channel::GearRatio
  // and docs/interfaces/twin-engine-gearbox-interface.md section 1): all
  // channels physically share ONE combined output shaft/RPM (ThrusterRPM)
  // by construction, so Calculate() derives every channel's engine-RPM
  // feedback from that single shared value, never from a channel's own
  // GearRatio. A <gearbox> whose <input> elements specify differing
  // <gearratio> values would therefore silently mislead anything reading
  // that value back (telemetry, docs, a future governor), even though the
  // simulated physics is unaffected -- flag it at load time rather than
  // leaving it a silent trap.
  for (size_t i = 1; i < Channels.size(); i++) {
    if (std::fabs(Channels[i].GearRatio - Channels[0].GearRatio) > 1e-9) {
      FGXMLLogging log(exec->GetLogger(), gearbox_element, LogLevel::WARN);
      log << "<gearbox name=\"" << Name << "\"> <input> elements specify "
          << "differing <gearratio> values (engine " << Channels[0].EngineIndex
          << ": " << Channels[0].GearRatio << ", engine "
          << Channels[i].EngineIndex << ": " << Channels[i].GearRatio
          << "). All channels in one <gearbox> share a single combined "
          << "output shaft, so <gearratio> is reporting-only and every "
          << "channel's engine-RPM feedback is derived from that one "
          << "shared shaft RPM regardless of this value -- differing "
          << "values here do not change the simulated physics, but will "
          << "misreport intended mechanical ratio.";
    }
  }

  // The lowest referenced engine index is used as the shared thruster's
  // "engine number" for property-tie purposes (propulsion/engine[x]/
  // collective-ctrl-rad, rotor-rpm, etc.) -- the same slot a single dummy
  // engine would occupy for a non-gearbox rotor.
  int thruster_engine_num = Channels.front().EngineIndex;
  for (const auto& c : Channels)
    if (c.EngineIndex < thruster_engine_num) thruster_engine_num = c.EngineIndex;

  // --- Construct the shared <thruster> ------------------------------------
  //
  // ACCEPTED RISK (noted in REQ-003 code review, not fixed here): Thruster
  // is allocated via raw `new` below, before the rest of this constructor
  // (AssignGearboxThruster/BindGearbox/BindChannel calls) runs. None of
  // that remaining code can currently throw, so this is not a live leak
  // today -- but it is fragile against a future edit that adds a throwing
  // call between the `new` and the destructor becoming reachable (the
  // destructor's `delete Thruster` never runs if the constructor exits via
  // exception). If more logic is added here, prefer wrapping this in
  // std::unique_ptr<FGThruster> (releasing it into the raw Thruster member
  // only after everything that could throw has run) over patching around
  // this note.

  Element* thruster_element = gearbox_element->FindElement("thruster");
  if (!thruster_element) {
    XMLLogException err(exec->GetLogger(), gearbox_element);
    err << "<gearbox name=\"" << Name << "\"> has no <thruster> definition.";
    throw err;
  }

  Element* document;
  if ((document = thruster_element->FindElement("propeller"))) {
    Thruster = new FGPropeller(exec, document, thruster_engine_num);
  } else if ((document = thruster_element->FindElement("nozzle"))) {
    Thruster = new FGNozzle(exec, document, thruster_engine_num);
  } else if ((document = thruster_element->FindElement("rotor"))) {
    // A gearbox-driven rotor must not run its own internal (single-engine)
    // FGTransmission -- that would double-count the FWU/clutch math this
    // class already does per channel. Requiring ExternalRPM=-1 keeps
    // FGRotor's Transmission member null and makes it read RPM each frame
    // from the (existing, unmodified) "x-rpm-dict" property that this
    // gearbox drives in Calculate(). See class doc comment in FGGearbox.h.
    Element* extrpm_el = document->FindElement("ExternalRPM");
    int rpmdef = extrpm_el ? (int) extrpm_el->GetDataAsNumber() : 0;
    if (!extrpm_el || rpmdef != -1) {
      XMLLogException err(exec->GetLogger(), thruster_element);
      err << "<gearbox name=\"" << Name << "\"> <thruster><rotor> must "
          << "specify <ExternalRPM>-1</ExternalRPM> so its RPM is driven by "
          << "the combining gearbox instead of an internal single-engine "
          << "transmission.";
      throw err;
    }
    Thruster = new FGRotor(exec, document, thruster_engine_num);
  } else if ((document = thruster_element->FindElement("direct"))) {
    Thruster = new FGThruster(exec, document, thruster_engine_num);
  } else {
    XMLLogException err(exec->GetLogger(), thruster_element);
    err << "<gearbox name=\"" << Name << "\"> Unknown thruster type.";
    throw err;
  }

  if (Thruster->GetType() == FGThruster::ttRotor) {
    string base = CreateIndexedPropertyName("propulsion/engine", thruster_engine_num);
    // FGRotor's own bindmodel() creates this node (RPMdefinition==-1) as
    // part of constructing Thruster above, so this lookup must not create.
    XRpmDictNode = PropertyManager->GetNode(base + "/x-rpm-dict", false);
  }

  // --- Wire each channel's engine to the now-existing shared thruster ----

  for (size_t i = 0; i < Channels.size(); i++)
    Channels[i].Engine->AssignGearboxThruster(this, (int)i, Thruster,
                                               PropertyManager.get());

  BindGearbox(num, PropertyManager.get());
  for (size_t i = 0; i < Channels.size(); i++)
    BindChannel(i, PropertyManager.get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGearbox::~FGGearbox()
{
  delete Thruster;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGearbox::SetChannelPower(int engine_index, double power_ftlbs_per_sec)
{
  for (auto& c : Channels) {
    if (c.EngineIndex == engine_index) {
      c.InputPower = power_ftlbs_per_sec;
      return;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Generalizes FGTransmission::Calculate() (basically P = Q*w and
// Q_Engine + (-Q_Rotor) = J * dw/dt) from one engine channel to N channels
// sharing one output shaft/inertia (GearMoment). Each channel keeps its own
// FWU/clutch decision (identical heuristic to FGTransmission, evaluated
// against the shared shaft's predicted omega instead of an exclusively-
// owned thruster's), so a channel cut to zero power free-wheels off the
// shaft independently of the others, with no torque discontinuity (the raw
// 0/1 free-wheel decision is smoothed through FreeWheelLag exactly as in
// FGTransmission, before it ever multiplies a torque value).
//
// No dynamic allocation or blocking calls -- Channels is fixed-size by the
// time Calculate() can run (only grown during Load()).

void FGGearbox::Calculate(double dt)
{
  double thruster_reaction_torque = Thruster ? Thruster->GetTorque() : 0.0;

  double thruster_omega = rpm_to_omega(ThrusterRPM);
  double safe_thruster_omega = thruster_omega < 1e-1 ? 1e-1 : thruster_omega;
  double thruster_torque = thruster_reaction_torque +
      Constrain(0.0, BrakeCtrlNorm, 1.0) * MaxBrakePower / safe_thruster_omega;

  double sum_coupled_torque = 0.0;
  double sum_coupled_inertia = 0.0;
  double total_channel_power = 0.0;

  for (auto& c : Channels) {
    total_channel_power += c.InputPower;

    double engine_omega = rpm_to_omega(c.EngineRPM);
    double safe_engine_omega = engine_omega < 1e-1 ? 1e-1 : engine_omega;
    double engine_torque = c.InputPower / safe_engine_omega
                          - c.EngineFriction / safe_engine_omega;

    // Would this channel's FWU release this frame? Same predictive test as
    // FGTransmission::Calculate(), evaluated against the shared shaft.
    double engine_d_omega = engine_torque / c.EngineMoment * dt;
    double thruster_d_omega = -thruster_torque / GearMoment * dt;

    if (thruster_omega + thruster_d_omega > engine_omega + engine_d_omega)
      c.FreeWheelTransmission = 0.0;
    else
      c.FreeWheelTransmission = 1.0;

    double fw_mult = c.FreeWheelLag.execute(c.FreeWheelTransmission);
    double coupling = fw_mult * Constrain(0.0, c.ClutchCtrlNorm, 1.0);

    // Delivered torque through this channel's clutch/FWU -- goes to ~0 as
    // the channel free-wheels, even if the (windmilling) engine still has
    // some torque of its own. See docs/requirements/REQ-005.
    c.OutputTorque = coupling * engine_torque;

    if (coupling < 0.999999) {
      double ch_engine_d_omega =
          (engine_torque - thruster_torque*coupling) /
          (GearMoment*coupling + c.EngineMoment) * dt;
      c.EngineRPM += omega_to_rpm(ch_engine_d_omega);

      // Simulate transit to static friction, pulling this channel's RPM
      // toward the shared shaft's RPM as coupling -> 1. Mirrors
      // FGTransmission's own quadratic blend, generalized so the shaft
      // side is a shared reference rather than a single exclusively-owned
      // thruster's state.
      double coupling_sq = coupling*coupling;
      c.EngineRPM = (1.0-coupling_sq)*c.EngineRPM + coupling_sq*ThrusterRPM;
    } else {
      c.EngineRPM = ThrusterRPM; // rigidly locked to the shared shaft
    }

    if (c.EngineRPM < 0.0) c.EngineRPM = 0.0;

    sum_coupled_torque  += coupling * engine_torque;
    sum_coupled_inertia += coupling * c.EngineMoment;
  }

  double combined_inertia = GearMoment + sum_coupled_inertia;
  double d_omega_shaft = (sum_coupled_torque - thruster_torque) / combined_inertia * dt;
  ThrusterRPM += omega_to_rpm(d_omega_shaft);
  if (ThrusterRPM < 0.0) ThrusterRPM = 0.0;

  if (Thruster) {
    if (XRpmDictNode) XRpmDictNode->setDoubleValue(ThrusterRPM * Thruster->GetGearRatio());
    Thruster->Calculate(total_channel_power);
  }

  for (auto& c : Channels) c.InputPower = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGearbox::GetOneEngineInoperative(void) const
{
  if (Channels.size() < 2) return false;

  bool any_freewheeling = false;
  for (const auto& c : Channels)
    if (c.FreeWheelTransmission < 0.5) any_freewheeling = true;

  // "while the shaft is still turning under the other channel(s)" -- guards
  // against reporting OEI while the whole gearbox is simply spun down/off.
  return any_freewheeling && ThrusterRPM > 1.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGearbox::BindGearbox(int num, FGPropertyManager* PropertyManager)
{
  string base = CreateIndexedPropertyName("propulsion/gearbox", num);

  PropertyManager->Tie( (base+"/shaft-rpm").c_str(), this, &FGGearbox::GetShaftRPM);
  PropertyManager->Tie( (base+"/brake-ctrl-norm").c_str(), this,
      &FGGearbox::GetBrakeCtrlNorm, &FGGearbox::SetBrakeCtrlNorm);
  PropertyManager->Tie( (base+"/one-engine-inoperative").c_str(), this,
      &FGGearbox::GetOneEngineInoperative);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGearbox::BindChannel(size_t channel_idx, FGPropertyManager* PropertyManager)
{
  string base = CreateIndexedPropertyName("propulsion/engine", Channels[channel_idx].EngineIndex);
  int idx = (int) channel_idx;

  PropertyManager->Tie( (base+"/free-wheel-transmission").c_str(), this, idx,
      &FGGearbox::GetChannelFreeWheel);
  PropertyManager->Tie( (base+"/output-torque-lbft").c_str(), this, idx,
      &FGGearbox::GetChannelOutputTorque);
  PropertyManager->Tie( (base+"/clutch-ctrl-norm").c_str(), this, idx,
      &FGGearbox::GetChannelClutchCtrlNorm, &FGGearbox::SetChannelClutchCtrlNorm);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGGearbox::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n    Gearbox Name: " << Name << "\n";
      cout << "      Gear Moment = " << GearMoment << "\n";
      cout << "      Max Brake Power = " << MaxBrakePower/hptoftlbssec << " HP\n";
      cout << "      Channels = " << Channels.size() << "\n";
      for (const auto& c : Channels) {
        cout << "        engine " << c.EngineIndex
             << ": gearratio=" << c.GearRatio
             << " enginemoment=" << c.EngineMoment
             << " enginefriction=" << c.EngineFriction/hptoftlbssec << " HP\n";
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGGearbox" << endl;
    if (from == 1) cout << "Destroyed:    FGGearbox" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

}
