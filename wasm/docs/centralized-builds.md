# Building the in-tree WebAssembly package

The package uses the enclosing JSBSim Git checkout. There is no separate engine
checkout, vendored engine archive, source updater, or patch application stage.
A clean checkout is required for release packing; `build:dev` explicitly allows
and labels uncommitted development inputs.

The orchestrator captures the repository once into `wasm/build/sources/`. It
records the Git revision, content digests and dirty state, then copies the SDK
portion into a private build attempt. Bindings, native compilation, TypeScript
output and runtime fixtures all use that captured source. Binding generation
uses the selected Emscripten compiler rather than a separate system Clang.

Generated previews and any older `dist/` output are not build inputs. The build
verifies captured source, authored workspace files, generated bindings and CMake
source identities before accepting output. Compiler-affecting ambient overrides
are rejected rather than silently changing the recorded recipe.

Successful builds create an artifact under `build/artifacts/<digest>/` and update
`build/last-build.json`. Failed builds leave accepted artifacts intact. A separate
`pack:build` command checks artifact bytes and assembles the package; `--release`
also requires clean native and SDK identities from the same repository revision.
Neither packaging mode publishes or performs Git operations.

`buildIdentity` and the `/build-metadata` package export identify the enclosing
revision, SDK path, content digests, compiler/tool versions, build options and
validation commands. The metadata inventories distributed file hashes. Tarball
integrity is recorded outside the tarball to avoid a self-referential digest.
Retain a prior tarball when changing a consuming project's dependency.

Source-contract tests exercise mismatch, changed-source and artifact rejection.
Runtime tests use C172 XML from the captured repository. The separate headless
browser persistence check verifies bytes against the selected artifact metadata
and checks its exported identity in Chromium. Browser checks are recorded
separately from build-time Node checks.

The supported source-build hosts are Linux and macOS with the versions in
`build-toolchain.lock.json`. Git metadata is required for this build contract.
Native builds with `BUILD_WASM_MODULE=OFF` keep their existing tool requirements
and do not run this packaging pipeline. The provisional private package identity
does not imply npm namespace ownership or an upstream release policy.
