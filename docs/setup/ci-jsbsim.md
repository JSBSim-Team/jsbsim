# JSBSim CI setup (root `Makefile.sim`)

This document backs the `jsbsim-devops`-owned pipeline at `Makefile.sim` in
the fork root. All pipeline logic lives in that Makefile; `.github/workflows/
sim-ci.yml` is a thin wrapper that only provisions the runner and runs
`make -f Makefile.sim ci`. If that command passes locally, CI must pass.

Invoke every target as:

```
make -f Makefile.sim <target>
```

## Supported platforms: dual-OS, but NOT identical (Windows native + Linux)

`Makefile.sim` auto-detects the host OS via Make's built-in `$(OS)` variable
(`Windows_NT` on Windows, unset/blank elsewhere). The two paths use genuinely
different toolchains, not just different messages:

| stage | Linux/CI | Windows (native dev) |
|---|---|---|
| `setup` | cmake, g++/clang++, make, git, python | vswhere.exe -> MSBuild.exe, git, python |
| `build` | `cmake -B build && cmake --build build` | MSBuild: `JSBSim.sln` (Release\|x64) then `JSBSimForUnreal.sln` (1_Release\|x64 and 2_Debug\|x64) |
| `test` (ctest) | runs (cxxtest + python-module suite) | **SKIPPED** (prominent banner, not a silent pass) |
| `test-regression`(-full) | runs against `build/src/JSBSim` | runs against `Release/JSBSim.exe` |
| `lint` | runs | runs |
| `clean` | removes `build/` | removes `Release/`, `Debug/`, `x64/` |

**Why `test` is skipped on Windows, not shimmed**: neither `JSBSim.sln` nor
`JSBSimForUnreal.sln` has a test project. The C++ unit tests (cxxtest) and
the Python-module test suite are both wired through the *cmake configure*
step, which the Windows path deliberately avoids (this project builds
JSBSim via the same Unreal/Visual-Studio toolchain the UE plugin uses, not
cmake+gcc, on Windows). Reintroducing cmake+gcc just for this one target
would mean a second, separate compiler and build tree — two sources of
truth for "does it compile" — for marginal benefit. `test-regression` (which
*does* run for real on Windows, against the MSBuild-produced `JSBSim.exe`)
is the local substitute. `ci`/`ci-full` drop `test` from the aggregate on
Windows rather than silently swallowing it into a pass — **this is a
standing, intentional exception to "if it passes locally it must pass in
CI"**: `build`/`test-regression`/`lint` must match across platforms; `test`
does not.

### Native Windows (Git Bash / MSYS2 shell — not WSL)

- Requires Visual Studio 2022 with **both** of these Individual Components
  (not installed by the "Desktop development with C++" workload defaults):
  - `MSVC v142 - VS 2019 C++ x64/x86 build tools` (needed by
    `JSBSimForUnreal.vcxproj`)
  - `MSVC v143 x64/x86 build tools (v14.38-17.X)` — this specific v14.38
    sub-version, not just "latest v143" (needed by `JSBSim.vcxproj`; UE5.4/
    5.5 pin this exact toolset)
- `vswhere.exe` (ships with any VS2022 install, including Build-Tools-only)
  locates `MSBuild.exe`; the expected path is
  `C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe`.
- Repo paths are converted with `cygpath -w` before being passed to
  `MSBuild.exe` — it's a native .NET tool and doesn't reliably parse
  MSYS/Git-Bash POSIX-style paths (`/c/Users/...`); relying on Git Bash's
  argv path-mangling heuristic instead of doing this explicitly is a known
  source of "works interactively, fails from `make`" bugs.
- `JSBSimForUnreal.sln`'s solution-level configuration names are literally
  `1_Release|x64` / `2_Debug|x64`, not `Release`/`Debug` — `build` matches
  these exactly (MSBuild silently no-ops a config name that doesn't match).
  **Both** are built, not just Release: `JSBSim.Build.cs`'s
  `SetupWindowsPlatform()` only looks in `LibDebug/` for a UE `DebugGame`
  config, and throws if it's missing, so skipping the Debug build silently
  breaks Debug UE builds later.
- `test-regression`'s regression-fixture staging uses `cp -r`, not `ln -s`
  — symlinks are unreliable/unprivileged on this kind of Windows setup
  (no Developer Mode), so the fix is applied unconditionally (all OSes),
  not just on Windows; `build/regression/` is wiped and rebuilt every run
  so there's no downside to copying instead of linking.

### Linux / CI

Unchanged from before Windows support was added: `cmake -B build -S . &&
cmake --build build -j`, then `ctest` from `build/`.

## `python3`/`python` must actually run, not just exist on PATH

Every target uses a `$(PYTHON)` variable, resolved once at the top of the
file by actually invoking the interpreter (`python3 -c "print(1)"`, falling
back to `python`) — never a hardcoded `python3`, and never a bare
`command -v` presence check.

This matters on Windows: a fresh Windows 11 install ships `python`/`python3`
execution-alias stubs under `...\WindowsApps\` that satisfy `command -v` (or
`where`) but do not run a real interpreter — invoking one either opens the
Microsoft Store listing for Python or exits without doing anything useful.
`setup` fails loudly and names this trap explicitly if neither `python3` nor
`python` actually runs, with the fix:

- Install real Python from python.org (check "Add python.exe to PATH"), or
- Disable the stub: Settings > Apps > Advanced app settings > App execution
  aliases > turn off `python.exe`/`python3.exe`.

## Downstream dependency: the UE plugin needs this build to run first

`UnrealEngine/Makefile.sim build` (owned by `unreal-devops`) requires this
repo's Windows `build` to have already staged
`UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/
{Include,Lib}` via `JSBSimForUnreal.sln` — sequence `make -f Makefile.sim
build` (this file) before `make -f UnrealEngine/Makefile.sim build`. See
`docs/setup/ci-unreal.md` for the plugin side of this precondition.

## Useful commands

```
make -f Makefile.sim help
make -f Makefile.sim setup
make -f Makefile.sim build
make -f Makefile.sim test-regression
make -f Makefile.sim ci             # Windows: setup+build+test-regression+lint (test SKIPPED)
                                     # Linux:   setup+build+test+test-regression+lint
```
