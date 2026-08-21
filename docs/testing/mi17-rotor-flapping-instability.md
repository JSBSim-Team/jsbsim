# Mi-17 main rotor: open-loop roll/pitch divergence

**Status**: open, unresolved. Config-tuning (inflowlag, hingeoffset, SAS
gain) has been exhausted; likely needs either a deliberately
smaller-than-real `hingeoffset` as a stability compromise, or an actual
`FGRotor.cpp` change (a real flapping time-lag term), not further XML
tuning. Branch: `mi17-gearbox-config`.

## Symptom

Once the corrected main rotor (real Mi-17 data: 69.862 ft diameter, 5
blades, 1.706 ft chord, real blade mass) is spun up to governed RPM (192)
and collective moves even slightly off flat pitch, the aircraft doesn't
climb cleanly — it rolls violently (past 90 degrees within a few seconds)
and the simulation either crashes outright or dies silently with no error
message at all.

This reproduces **with the autopilot fully disabled**
(`scripts/test_mi17_openloop_climb.xml`, both `ap/attitude-hold-on` and
`ap/altitude-hold-on` forced to 0.0 for the entire run), which rules out
the flight-control/autopilot loops as the cause. It is the raw
airframe+rotor plant that is unstable, not a control-loop bug.

## The mechanism

`src/models/propulsion/FGRotor.cpp` models blade flapping (the up/down
flexing each blade does once per revolution) in two steps:

1. **Compute flapping angles.** Each simulation frame, `a_1` (longitudinal
   flapping) and `b_1` (lateral flapping) are solved as a direct algebraic
   function of the current airflow, inflow, and — critically — the
   aircraft's own body rotation rates (`p`, `q`, `r`). There is no time lag
   in this calculation. Compare this to how the rotor's induced airflow is
   handled (`<inflowlag>`, a proper first-order lag with a real time
   constant, `FGRotor::calc_flow_and_thrust()`) — flapping has no
   equivalent lag term. It is assumed to reach its steady-state value
   instantly, every frame.

2. **Turn flapping into a hub moment.** A rotor blade hinged away from the
   shaft axis (a "flapping hinge offset") means that when the blade flaps,
   centrifugal force acting through that offset creates a real physical
   moment at the hub, felt by the airframe as roll/pitch torque
   (`FGRotor::body_moments()`):

   ```cpp
   mf = 0.5 * HingeOffset * BladeNum * Omega*Omega * BladeMassMoment;
   M_s(eL) = mf * b1s;   // roll moment
   M_s(eM) = mf * a1s;   // pitch moment
   M_s(eN) = Torque * Sense;   // yaw moment (unrelated to this issue)
   ```

   This is real physics — real articulated rotors do work this way — but
   `mf` is a fixed gain that scales directly with hinge offset, blade
   count, RPM squared, and blade mass moment.

## Why the real-data correction made it worse

Before this investigation, the model used placeholder rotor geometry: 52
ft diameter, 3 blades, a 2500 slug-ft2-scale blade flapping moment. Once
real Mi-17 data went in — 69.862 ft diameter, 5 blades, real 137 kg
blades — three things changed at once:

- `BladeMassMoment` grew (heavier, longer blades) — `mf` scales with it
  directly.
- The rotor's **Lock number**
  (`LiftCurveSlope * BladeChord * Radius^4 / BladeFlappingMoment`, a
  measure of how aerodynamically responsive flapping is relative to blade
  inertia) increased by roughly 61%, because `Radius^4` grew about 3.26x
  and was only partially offset by the larger derived flapping moment. A
  higher Lock number means flapping responds more eagerly to
  disturbances.
- `HingeOffset` was still the old placeholder, 4.0 ft — far larger than
  the real figure (see below).

Put together: a stronger flapping response (higher Lock number) feeding a
bigger moment-per-radian gain (`mf`), with no time lag anywhere in the
loop. That is an undamped, fast positive-feedback path: a small body rate
produces flapping, flapping produces a moment via `mf`, the moment
produces more body rate, which produces more flapping — all recomputed
instantaneously every 0.0075s simulation frame.

## Evidence trail

All of the following used `scripts/test_mi17_openloop_climb.xml`
(autopilot fully disabled) unless noted, starting from a settled ground
sit with the rotor at governed RPM.

**Ruling out the control loops.** Re-running with `ap/attitude-hold-on`
and `ap/altitude-hold-on` both forced to 0.0 for the whole run reproduces
the identical divergence, at the identical timing, as with the autopilot
engaged. This is a plant problem, not a control-loop problem.

**`HingeOffset` sweep.**

| `HingeOffset` | Source | `mf` (lb-ft / rad flapping) | Result |
|---|---|---|---|
| 4.0 ft | unsourced placeholder | ~575,000 (old rotor's smaller Omega/mass partially offset this) | stable (old rotor only; not retested against the corrected rotor) |
| 1.74655 ft | `FGRotor.cpp` default, `0.05 * Radius` | ~275,000 | roll past 130 deg within ~4s from a standing start |
| 0.72178 ft | **real Mi-17 manual figure, 220mm** | ~117,000 (2.35x smaller than the 1.74655 ft case) | onset delayed: flat (roll < 1 deg) until t~140s, then diverges over ~3-4s instead of under 1s. Still ultimately fails via a proper `*CRASH DETECTED*` ground-impact event around t=144.8s. |

The real hinge offset is a genuine, sourced, roughly 2.35x reduction in
the destabilizing gain and measurably slows the divergence — but does not
eliminate it.

**Flapping-angle trace at `HingeOffset` = 0.72178 ft** (from
`test_mi17_openloop_climb.xml`'s `flap-trace *` events), confirming the
mechanism directly — note the flapping angles themselves (`a1-rad`,
`b1-rad`) stay modest throughout; it is the coupling gain, not extreme
flapping, that produces the large moment:

| t (s) | roll (deg) | `p` (rad/s) | `a1-rad` | `b1-rad` |
|---|---|---|---|---|
| 138 | -3.5 | -- | -0.011 | -0.059 |
| 140 | 0.4 | -0.041 | -0.027 | 0.009 |
| 141 | 6.5 | 0.022 | -0.025 | 0.022 |
| 142 | 26.5 | 0.609 | 0.002 | -0.011 |
| 143 | 83.4 | 1.511 | 0.006 | -0.177 |
| 144.8 | -- | -- | -- | -- (`*CRASH DETECTED*`) |

`p` (roll rate) goes from essentially zero to 1.51 rad/s in about two
seconds — a doubling time under half a second.

**SAS rate-damping gain bracket.** `afcs.xml`'s stability augmentation
(`ap/stability-aug-on`, defaults to 1.0, always active regardless of
`ap/attitude-hold-on`) provides `ap/sas-roll-cmd = -gain * p-rad_sec` and
the pitch equivalent, summed into the cyclic control chain. Traced end to
end and confirmed correctly wired (`ap/sas-roll-cmd` matches `-gain * p`
exactly; `propulsion/engine[0]/lateral-ctrl-rad` tracks the summed
command through `rotor_control.xml`'s mixing with roughly unity gain —
real cyclic authority is reaching the rotor). Swept the shared pitch/roll
gain (originally -0.1) from -0.1 to -3.0:

| gain | last gear-contact time | outcome |
|---|---|---|
| -0.1 | 144.8s | crashes (baseline) |
| -0.3 | 143.0s | `p-rad_sec` reaches 1,494,267 -- numerical blowup, not physical |
| -0.5 | 140.5s | crashes, earlier than baseline |
| -0.8 | 158.0s | best survivor, but still crashes, `p-rad_sec` ~1.5 at last read |
| -1.2 | 139.9s | crashes, earlier than baseline |
| -2.0 | 140.4s | crashes |
| -3.0 | 141.1s | crashes |

No gain in this range stabilizes the aircraft, and the response is
non-monotonic — some higher gains fail *earlier* than the baseline, and
-0.3 produces an outright nonphysical blowup. This is consistent with a
feedback loop whose natural growth rate is too fast for a simple
proportional-on-rate correction, computed once per 0.0075s frame with no
lag anywhere in the loop, to reliably counter.

## Why this resists further config tuning

Every lever that is purely a number in the XML config (`inflowlag`,
`hingeoffset`, SAS gain) has now been swept across a wide range. Hinge
offset — the one lever grounded in real manual data — helped
meaningfully but is not sufficient alone. The others do not help at all,
and pushing them further sometimes causes outright numerical blowups
rather than convergence. That pattern (a real, physically-grounded fix
providing partial but incomplete improvement, and every other scalar
knob failing or making things chaotic) is the signature of the *model
structure* being the limiting factor, not any single mistuned constant:
`FGRotor.cpp` has no damping term in the flapping calculation itself, and
this rotor's real-world scale is large enough to expose that gap.

## Cross-check against a sibling repo

`mi17-jsbsim-model` (a separate, earlier-stage repo prototyping the same
aircraft on an AH-1S-derived base) hit the identical failure signature
while updating its own rotor to real Mi-17-scale dimensions (commit
history: "Updated mi17 dimensions. But having some aero model core
dumps" -> "Fix aerodynamic core dump issue, but now the helicopter is
rotating uncontrollably" -> "After tweaking"). While debugging blind (no
evidence it had access to real hinge-offset data), it drove
`hingeoffset` from an inherited 3.30 ft down to 0.05 ft on the main rotor
and 0.015 ft on the tail rotor -- both toward zero, the same direction
the real 220mm figure points, though nowhere near as small, and its final
state was never confirmed fixed (just "after tweaking", not "working").

That repo's final tweaked `<twist>` value is a bare `-5` with no unit
attribute; `FGRotor.cpp` defaults an un-suffixed `twist` value to
radians, so that is actually -5 radians (-286 degrees) -- almost
certainly a missing `unit="DEG"` bug there, not a value to adopt.

## Paths forward

1. **Deliberately use a smaller-than-real `hingeoffset`.** A compromise
   that trades physical accuracy for stability, similar to what
   `mi17-jsbsim-model` converged to. Cheap to try, but moves further away
   from the real, sourced 220mm manual figure already in
   `aircraft/mi17/Engines/mi17_main_rotor.xml`.
2. **Add a real flapping time-lag to `FGRotor.cpp`.** Analogous to how
   `<inflowlag>` already works for induced velocity. This would fix the
   root cause rather than compensate for it, but is a genuine C++ change
   to JSBSim's core rotor model, not a config edit, and needs its own
   validation against the existing `FGRotor` regression suite
   (`check_cases/`) to avoid regressing other aircraft that use this
   thruster type (e.g. `aircraft/ah1s`).

## Where the supporting artifacts live

- `aircraft/mi17/Engines/mi17_main_rotor.xml` — current rotor config, with
  the real twist and hinge-offset derivations documented inline.
- `scripts/test_mi17_openloop_climb.xml` — the open-loop diagnostic used
  to isolate the plant from the control loops and to trace flapping
  angles, SAS commands, and cyclic authority through the divergence.
- `scripts/test_mi17_hover.xml` — the full hover script; rewritten during
  this investigation (ramped collective-bias and collective-cmd-norm
  restoration instead of instantaneous steps, extended ground-settle
  time) but still does not complete a clean run for the reasons above.
- `scripts/test_mi17_ground_run_idle.xml` — used to measure the real RPM
  governor settle time (120s) that `test_mi17_hover.xml`'s ground-settle
  window is now based on.
