# Known issues

## Windows (MSVC) build: `ground_tests/groundtest.xml` regression fails to open its own `data_output` file

**Status**: open, tracked here. Not a CI-pipeline bug — `jsbsim-programmer` territory.

**Symptom**: on the native-Windows build (`Release/JSBSim.exe`, built via
`JSBSim.sln`/MSBuild — see `docs/setup/ci-jsbsim.md`), running
`make -f Makefile.sim test-regression` fails specifically on
`check_cases/ground_tests/scripts/groundtest.xml`:

```
No filename given.

In file ./scripts/groundtest.xml: line 8
Could not open file: data_output/ground_reactions120hz
Script file Path "scripts/groundtest.xml" was not successfully loaded
```

The referenced file, `data_output/ground_reactions120hz.xml`, genuinely
exists at that exact relative path in the working directory at the time of
the failure (confirmed by listing the directory right before the failing
run). The other two check_cases exercised by `test-regression`
(`ground_tests/scripts/systems-rate-test-0.xml` and
`piston_takeoff/scripts/c1723.xml`) both pass cleanly on the same Windows
build, so this is narrowly scoped to this one script/output-file
combination, not a systemic build or pipeline problem.

**Reproduced two ways**, ruling out a Git-Bash/MSYS argument-mangling
artifact:
1. Via `make -f Makefile.sim test-regression` (Git Bash).
2. Directly invoking `Release\JSBSim.exe --root=./ --script=scripts/groundtest.xml`
   from PowerShell, cwd set to the copied case directory. Same failure,
   same message.

**Where to start looking**: `src/input_output/FGXMLFileRead.cpp`,
`FGXMLFileRead::LoadXMLDocument()`. The "No filename given." message comes
from `SGPath::isNull()` being true for a *different*, apparently-optional
document load earlier in the same run (this line is likely benign — some
other external config genuinely isn't present, which is normal). The actual
failure is the *following* `infile.open(filename)` call for
`data_output/ground_reactions120hz` returning "not open" despite the file
being present on disk. Plausible root causes to investigate: `SGPath`
relative-path resolution against `--root=./` under MSVC vs. GCC, encoding/
locale handling in `sg_ifstream::open` on Windows, or a `.xml` extension-
append edge case (`SGPath::concat(".xml")`) that behaves differently when
the path was constructed from a Windows-style working directory.

**Why this isn't blocking the Windows CI pipeline merge**: the pipeline
itself is doing its job correctly here — it built the Windows binary, ran
the regression harness, and accurately reported a real failure rather than
masking it. This is a genuine JSBSim runtime bug on the Windows/MSVC build,
separate from the `Makefile.sim` build/test-regression/lint wiring, which is
otherwise fully verified working (see `docs/setup/ci-jsbsim.md`). Fixing it
is C++ debugging work for `jsbsim-programmer`, not a devops/CI task.

**Impact while open**: `make -f Makefile.sim test-regression` (and
therefore `ci`) will report FAILED on Windows until this is fixed. This is
intentional — a failing regression must never be hidden by softening the
pipeline's pass/fail criteria.

## UE 5.4 build fails: `FEngineCommand` collides with `UserToolBoxBasicCommand`'s `UEngineCommand`

**Status**: open, tracked here. Not a CI-pipeline bug — plugin/engine-config
territory (`unreal-plugin-developer` or `unreal-programmer`), not devops.

**Symptom**: `make -f UnrealEngine/Makefile.sim build` fails during
UnrealHeaderTool's header parse pass:

```
C:\Program Files\Epic Games\UE_5.4\Engine\Plugins\Experimental\UserToolBoxBasicCommand\Source\UserToolBoxBasicCommand\Public\EngineCommand.h(12):
Error: Class 'UEngineCommand' shares engine name 'EngineCommand' with struct
'FEngineCommand' in .../Plugins/JSBSimFlightDynamicsModel/Source/JSBSimFlightDynamicsModel/Public/FDMTypes.h(140)
```

Unreal's reflection system (UHT) requires globally unique names across the
whole engine + project for reflected types, comparing names with their
`U`/`F`/`A` prefix stripped. `FEngineCommand` (a `USTRUCT` in
`UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/JSBSimFlightDynamicsModel/Public/FDMTypes.h`)
collides with `UEngineCommand` (a `UCLASS` shipped inside the
**Experimental/UserToolBoxBasicCommand** engine plugin, which is enabled on
this particular UE 5.4 install).

**Why this isn't a fork-hygiene fix we should make lightly**:
`FDMTypes.h`/`FEngineCommand` is part of JSBSim-Team's own shipped
`JSBSimFlightDynamicsModel` plugin — not something this fork added. Renaming
it would be a real deviation from upstream (fork policy: minimal edits,
additions in new files/classes where possible), so it needs a deliberate
decision, not a devops-pipeline patch.

**Two independent ways to resolve, either is legitimate**:
1. **Disable `UserToolBoxBasicCommand`** for this project (it's an
   Experimental/marketplace-style engine plugin, not something JSBSim's
   plugin needs) — via the project's `.uproject`/`DefaultEngine.ini`
   plugin-enable list, or a `-Plugin=` exclusion. Untested by this pass;
   whoever picks this up should verify it doesn't have other side effects
   and that it's reliably reproducible across machines (i.e. not
   accidentally relying on a plugin that's enabled by default in some UE
   installs and not others).
2. **Rename `FEngineCommand`** in our fork to something less collision-prone
   (e.g. `FJSBSimEngineCommand`) — a deliberate, documented deviation from
   upstream, updating all call sites.

**Why this isn't blocking the Windows CI pipeline merge**: the pipeline
itself is doing its job correctly here — `setup` correctly found the UE
5.4 install, `_require-jsbsim-staged` correctly verified the JSBSim
ThirdParty lib was staged (confirmed present, from the root Makefile.sim's
build), and `build` correctly invoked `RunUAT BuildPlugin` → UnrealBuildTool
and surfaced UHT's real compile error instead of masking it. `validate` and
`lint` (which need no UE install) both pass cleanly; `setup` passes cleanly
including live UE 5.4 detection. This is a genuine plugin/engine-config
issue, separate from the `UnrealEngine/Makefile.sim` build/validate/lint
wiring, which is otherwise fully verified working (see
`docs/setup/ci-unreal.md`).

**Impact while open**: `make -f UnrealEngine/Makefile.sim build` (and
therefore `ci`, when a UE install is detected) will fail on any machine
where `UserToolBoxBasicCommand` (or another engine plugin with a colliding
reflected name) happens to be enabled, until one of the two fixes above
lands. This is intentional — a failing build must never be hidden by
softening the pipeline's pass/fail criteria.
