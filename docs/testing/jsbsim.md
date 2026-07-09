# JSBSim regression testing procedures

This document records reproducible procedures for the regression checks run
against this fork, beyond what `make -f Makefile.sim test-regression[-full]`
covers on its own (see `docs/setup/ci-jsbsim.md` for that target).

## Full-fleet A/B regression (branch vs. baseline, all `scripts/*.xml`)

**When to use**: any change that touches shared FDM core code (propulsion,
physics, output) that is supposed to be a behavioral no-op for existing
aircraft — e.g. REQ-003/REQ-008 (`FGGearbox`), where the acceptance
criterion is "bit-identical output on the full existing aircraft regression
suite when no `<gearbox>` is configured."

`make -f Makefile.sim test-regression-full` alone is **not** sufficient for
this: it only exercises `check_cases/` (3 cases) against reference CSVs
baked into the repo, and its `scripts/test_*.xml` fork-aircraft-regression
step is glob-matched case-sensitively (`test_*.xml`), so it silently misses
any of our own scripts named with a capital letter (e.g.
`scripts/Test_F450_Launch.xml` does **not** match `test_*.xml` on this
filesystem — confirmed empirically, `shopt -s nullglob; arr=(scripts/test_*.xml)` -> 0 matches even
though the file exists). It also never compares two *builds* against each
other — only a build against static reference data, which can itself be
stale (see the piston_takeoff known-issue below).

For a true "is this change a no-op for the existing fleet" gate, build both
the feature branch and the baseline (`sim-main` or `master`), then run:

```
py admin/compare_fleet_regression.py <repo_a> <repo_b> [outdir] [tol]
```

- `<repo_a>` / `<repo_b>`: two JSBSim checkouts (e.g. a `git worktree` of
  `sim-main` alongside the feature branch's normal working tree), each
  already built (`make -f Makefile.sim build` in each -- Windows:
  `Release/JSBSim.exe`; Linux: `build/src/JSBSim`).
- Runs every `scripts/*.xml` (the full aircraft-script fleet, ~60 scripts
  covering essentially every aircraft in `aircraft/` that ships a runnable
  script) against both binaries, captures every CSV each script produces
  plus its exit code, and diffs cell-by-cell.
- Default tolerance `1e-9` (effectively bit-identical). Pass `1e-5` to
  mirror the `check_cases/RunCheckCases.py` / `Makefile.sim REG_TOL`
  convention instead.
- Exit code 0 iff every script's exit code and every produced CSV matches
  between the two builds within tolerance across the whole fleet.

### Example run (REQ-003 / REQ-008, 2026-07-10)

```
git worktree add ../_regress/jsbsim-simmain sim-main
cd jsbsim               && make -f Makefile.sim build   # feature/REQ-003-fggearbox
cd ../_regress/jsbsim-simmain && make -f Makefile.sim build   # sim-main baseline
py admin/compare_fleet_regression.py \
    C:/Users/Parami/winSim/jsbsim \
    C:/Users/Parami/winSim/_regress/jsbsim-simmain \
    /tmp/fleet_out 1e-9
```

Result: `TOTAL scripts compared: 61 / PASS: 61 / FAIL: 0` -- every script's
exit code and every CSV cell matched within `1e-9` between
`feature/REQ-003-fggearbox` (`72b9021a`) and `sim-main` (`de136562`, the
branch's exact merge-base). Five scripts (`737_cruise_steady_turn_simplex`,
`cannonball`, `kml_output`, `plotfile`, `unitconversions`) exit non-zero on
*both* builds identically (pre-existing, not a regression -- these are
either non-simulation utility/data files or scripts with known unrelated
issues, not aircraft dynamics regressions).

Also run for this REQ: `make -f Makefile.sim test-regression-full` on both
the feature branch and a clean `sim-main` worktree -- fails identically on
both with the two pre-existing issues below, confirming they are not newly
introduced or newly masked by the branch.

### Known pre-existing `test-regression-full` failures (not this branch's fault)

1. **`ground_tests/scripts/groundtest.xml`**: `Could not open file:
   data_output/ground_reactions120hz` on the native-Windows/MSVC build.
   Fully documented in `docs/testing/known-issues.md` ("Windows (MSVC)
   build: `ground_tests/groundtest.xml` regression fails to open its own
   `data_output` file"). Reproduces identically on both
   `feature/REQ-003-fggearbox` and a clean `sim-main` checkout.
2. **`piston_takeoff/scripts/c1723.xml`**: the CSV comparison against
   `check_cases/piston_takeoff/logged_data/JSBout172B.csv` fails with a
   column mismatch -- the checked-in reference CSV has 246 columns, the
   binary now produces 290 (7 columns renamed/dropped since the reference
   was captured, 51 new columns added by upstream JSBSim since then). This
   is a stale reference-data problem, not a code regression: the actual
   simulation run (`c1723.xml` itself) exits 0 and completes normally on
   both builds, and the full-fleet A/B diff above shows `c1723.xml`'s
   `JSBout172B.csv` is bit-identical between the feature branch and
   `sim-main` -- the only failure is against the stale committed reference,
   equally on both branches.
   `check_cases/orbit/scripts/ball_orbit.xml` (`BallOut.csv`, 11 rows)
   passes its reference comparison cleanly on both builds, so this is
   narrowly scoped to the stale `piston_takeoff` reference file, not a
   general reference-data problem.

## Property-tree structure check (no leakage onto non-gearbox aircraft)

For changes that add new tied properties conditionally (e.g. `FGGearbox`'s
`propulsion/gearbox[x]/...` properties, only bound when an aircraft's XML
actually contains a `<gearbox>` element), confirm no existing aircraft's
property catalog changes:

```
Release/JSBSim.exe --root=./ --aircraft=<name> --catalog | grep -i "propulsion\|engine\|thrust"
```

Run against both builds for a representative cross-section and diff:
`c172x` (single piston), `c310` (twin piston, two independent thrusters),
`ah1s` (electric twin-engine helicopter, no gearbox configured), `DHC6`
(twin turboprop). All four were bit-identical between
`feature/REQ-003-fggearbox` and `sim-main` for this REQ.
