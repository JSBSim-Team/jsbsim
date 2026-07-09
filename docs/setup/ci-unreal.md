# Unreal CI setup (`UnrealEngine/Makefile.sim`)

This document backs the `unreal-devops`-owned pipeline at
`UnrealEngine/Makefile.sim` in this fork. All pipeline logic lives in that
Makefile; `.github/workflows/sim-ci-unreal.yml` (when added) is meant to be a
thin wrapper that only checks out the repo and runs
`make -f UnrealEngine/Makefile.sim ci`. If that command passes locally, CI
must pass -- there is no CI-only logic.

Invoke every target from the fork root as:

```
make -f UnrealEngine/Makefile.sim <target>
```

or from `UnrealEngine/` itself as `make -f Makefile.sim <target>`.

## What runs where

| stage              | needs UE install? | runs on hosted GitHub runners? |
|---------------------|:---:|:---:|
| `setup`             | no (reports UE presence) | yes |
| `validate`          | no | yes |
| `lint`              | no | yes |
| `build`             | yes | no (self-hosted only) |
| `test`              | yes | no (self-hosted only) |

Hosted GitHub Actions runners do not have Unreal Engine installed. The CI
workflow therefore only runs `validate` + `lint` on hosted runners; `build`
and `test` require a self-hosted runner with a full UE install (see
"Self-hosted runner" below). `make ... ci` degrades gracefully: when no UE
install is detected it still runs `setup` + `validate` + `lint`, then prints
a prominent `SKIPPED` banner (and a `:warning:` row in the job summary when
`GITHUB_STEP_SUMMARY` is set) instead of silently reporting success. A
skipped UE build must never look like a pass.

## Supported platforms: dual-OS (Windows native + Linux)

`Makefile.sim` auto-detects the host OS via Make's built-in `$(OS)` variable
(`Windows_NT` on Windows, unset/blank elsewhere) and picks the matching UE
toolchain paths. Both are maintained side by side -- neither replaces the
other.

### Native Windows (Git Bash / MSYS2 shell -- not WSL)

- UE install autodetection probes, in order: `UE_ROOT` env var, then
  `C:\Program Files\Epic Games\UE_5.*`, then `C:\Program Files\Epic
  Games\UE_4.*` (i.e. `/c/Program Files/Epic Games/UE_5*` etc. as seen from
  Git Bash). A directory counts as a UE install iff it contains
  `Engine\Build\BatchFiles\RunUAT.bat`.
- **Path-layout gotcha**: on Windows, `RunUAT.bat`, `Build.bat`,
  `Rebuild.bat`, and `Clean.bat` live *directly* under
  `Engine\Build\BatchFiles\` -- unlike Linux/Mac, where the equivalent
  `.sh` scripts are nested one level deeper under a per-platform
  subfolder (`Engine/Build/BatchFiles/Linux/Build.sh`,
  `Engine/Build/BatchFiles/Mac/Build.sh`). `Makefile.sim` accounts for this
  difference in how it builds the `UBT_SH` path per OS; it is the one detail
  a naive `.sh` -> `.bat` port would get wrong.
- The automation-test binary is `Engine\Binaries\Win64\UnrealEditor-Cmd.exe`.
- `UE_PLATFORM` defaults to `Win64` on Windows (override with
  `UE_PLATFORM=...` if you ever need a cross-target build).
- Requires Visual Studio 2022 with the "Desktop development with C++" and
  "Game development with C++" workloads -- inherited from Unreal Engine
  itself (UBT shells out to MSVC/`vswhere`); this is not something
  `Makefile.sim` installs or checks for directly, only UE's own build
  succeeding or failing on it.
- Confirmed working install locations on the reference dev machine:
  `C:\Program Files\Epic Games\UE_5.4` and `C:\Program Files\Epic
  Games\UE_5.8`.

### Linux

- UE install autodetection probes `UE_ROOT`, then `$HOME/UnrealEngine*`,
  `$HOME/UE_5*`, `$HOME/UE_4*`, `/opt/UnrealEngine*`, `/opt/unreal-engine*`,
  `/opt/UE_5*`, `/usr/local/UnrealEngine*`. A directory counts as a UE
  install iff it contains `Engine/Build/BatchFiles/RunUAT.sh`.
- `Build.sh`/`RunUAT.sh` wrappers and the automation-test binary
  (`Engine/Binaries/Linux/UnrealEditor-Cmd`) follow the per-platform
  subfolder layout under `Engine/Build/BatchFiles/<Platform>/`, unchanged
  from before Windows support was added.
- `UE_PLATFORM` defaults to `Linux`.

## Precondition: the JSBSim ThirdParty lib must be staged first

`Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim.Build.cs`
declares the plugin's JSBSim dependency as an **External** module: it links
a prebuilt `JSBSim.lib`/`JSBSim.dll` (Windows) or `libJSBSim.so`/`.dylib`
(Linux/Mac) that must already exist under
`Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/{Include,Lib}`
*before* UnrealBuildTool runs. Nothing in `UnrealEngine/Makefile.sim` stages
those files -- that is produced by the **root** `Makefile.sim`'s `build`
target (owned by `jsbsim-devops`), which compiles `JSBSimForUnreal.sln` via
MSBuild specifically to produce this staged output.

`make -f UnrealEngine/Makefile.sim build` therefore runs a
`_require-jsbsim-staged` precondition check before it invokes RunUAT/UBT. If
the headers or library are missing, it fails fast with:

```
ERROR: JSBSim ThirdParty lib/headers not staged. Run 'make -f Makefile.sim build'
at the repo root first (jsbsim-devops's build stages the lib this plugin links
against) before building the Unreal plugin/project.
```

Fix by running the root pipeline first:

```
make -f Makefile.sim build        # from the fork root; stages JSBSim.lib/.dll (or libJSBSim.*)
make -f UnrealEngine/Makefile.sim build
```

Without this check, a missing lib instead surfaces as an opaque UBT
`BuildException` thrown from `JSBSim.Build.cs`'s `CheckForFile` deep inside a
multi-minute compile -- the precondition turns that into an immediate,
actionable error.

## `python3` must actually run, not just exist on PATH

`validate` and `lint` are implemented as embedded `python3` scripts (no
`jq`, ever -- JSON parsing is always Python). `setup` therefore does not
just check `command -v python3`; it execs `python3 -c "print(1)"` and checks
that it actually succeeds.

This matters on Windows: a fresh Windows 11 install ships a `python3.exe`
**execution-alias stub** under `...\WindowsApps\` that satisfies `command -v
python3` (it's on `PATH`) but does not run a real interpreter -- invoking it
either launches the Microsoft Store listing for Python or no-ops, depending
on Windows version. `setup` detects this specific trap (by checking whether
the resolved `python3` path contains `WindowsApps`) and reports it by name,
with the fix:

- Install a real Python from python.org, or `winget install
  Python.Python.3.12`.
- Make sure the real install's directory precedes `WindowsApps` on `PATH`.
- Or disable the alias entirely: Settings > Apps > Advanced app settings >
  App execution aliases > turn off the `python3.exe`/`python.exe` entries.

## Self-hosted runner (build + test)

Hosted GitHub Actions runners cannot install Unreal Engine (multi-hundred-GB
download gated behind an Epic Games account/EULA, plus a paid-tier minutes
budget that would make it impractical anyway). Full `build` + `test` needs a
**self-hosted runner** with UE already installed. That job is scaffolded
disabled/commented in `.github/workflows/sim-ci-unreal.yml` pending such a
runner being provisioned; until then, hosted CI only ever asserts
`validate` + `lint`, with the `SKIPPED` banner covering `build`/`test`.

To stand up a self-hosted runner:

1. Provision a Windows (or Linux) machine with Unreal Engine installed
   (Epic Games Launcher, or a source build) at one of the autodetected
   locations above, or set `UE_ROOT` in the runner's environment.
2. On Windows, install Visual Studio 2022 with the C++ game-development
   workloads (see above).
3. Register the machine as a GitHub Actions self-hosted runner for this
   repo (`Settings > Actions > Runners > New self-hosted runner`), with a
   label such as `self-hosted-ue`.
4. Ensure the root `Makefile.sim`'s `build` target can also run on that
   runner (it stages the JSBSim ThirdParty lib this plugin needs -- see
   the precondition section above) before enabling the commented-out
   `build`/`test` job in `sim-ci-unreal.yml`.
5. Uncomment the self-hosted job in `.github/workflows/sim-ci-unreal.yml`
   and point it at the `self-hosted-ue` label.

## Useful commands

```
# from the fork root
make -f UnrealEngine/Makefile.sim help
make -f UnrealEngine/Makefile.sim setup
make -f UnrealEngine/Makefile.sim validate
make -f UnrealEngine/Makefile.sim lint
make -f UnrealEngine/Makefile.sim ci            # setup+validate+lint always; build+test if UE found

# force a specific UE install
UE_ROOT="/c/Program Files/Epic Games/UE_5.4" make -f UnrealEngine/Makefile.sim ci
```
