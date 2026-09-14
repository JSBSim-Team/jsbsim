# JSBSim WebAssembly and TypeScript package

This optional component builds the enclosing JSBSim engine to WebAssembly and
provides JavaScript/TypeScript bindings for Node.js and browsers. It builds on
[0x62's jsbsim-wasm](https://github.com/0x62/jsbsim-wasm). The original wrapper's
MIT copyright and license are retained in [LICENSE](LICENSE).

Native JSBSim builds keep `BUILD_WASM_MODULE=OFF` and do not need Node.js or
Emscripten. Enabling this component requires Emscripten. The package build
command generates bindings from the same captured native source that it compiles.

## Build and package

The source-build workflow supports Linux and macOS. Install the Node.js, npm,
CMake and Emscripten versions listed in `build-toolchain.lock.json`; `em++`,
`emcmake` and `em-config` must be on `PATH`. These are build requirements,
separate from the package's declared JavaScript runtime requirements.

From the JSBSim repository root:

```sh
npm --prefix wasm ci
npm --prefix wasm run test:source
npm --prefix wasm run build
npm --prefix wasm run pack:build -- --release
```

Use `npm --prefix wasm run build:dev` to explicitly capture uncommitted changes.
Clean release packing rejects such development inputs. The default build uses
npm's populated local cache; `npm run build -- --allow-network` permits fetching
missing npm dependencies. Native source always comes from this checkout.

The build generates C++ bindings, compiles the engine/WASM and TypeScript,
runs the Node tests, and records the resulting artifact in
`wasm/build/last-build.json`. Packing writes a tarball and integrity record under
`wasm/build/packages/`; `wasm/build/last-package.json` identifies the result.
No command publishes a package or modifies Git history.

`@jsbsim/wasm` is a private, provisional package identity for this integration.
Install the recorded tarball by path in a consuming Node.js or browser project.
The contribution does not establish an npm release namespace or publication job.
See [build details](docs/centralized-builds.md) for source capture and artifact checks.

## Running a model in Node.js

Models and their supporting XML files are supplied by the caller; the WASM binary
does not preload aircraft data. After installing the tarball, this example accepts
a JSBSim source directory as its first argument and loads the included C172 model:

```js
import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";
import { JSBSimSdk } from "@jsbsim/wasm";
import { wasmModuleUrl, wasmBinaryUrl } from "@jsbsim/wasm/wasm";

const source = process.argv[2];
const sdk = await JSBSimSdk.create({
  moduleUrl: wasmModuleUrl, wasmUrl: wasmBinaryUrl, log: { console: false },
});
function copyXml(from, to) {
  for (const entry of readdirSync(from, { withFileTypes: true })) {
    const input = path.join(from, entry.name);
    const output = `${to}/${entry.name}`;
    if (entry.isDirectory()) copyXml(input, output);
    else if (entry.name.endsWith(".xml")) sdk.writeDataFile(output, readFileSync(input));
  }
}
try {
  copyXml(path.join(source, "aircraft/c172p"), "aircraft/c172p");
  copyXml(path.join(source, "engine"), "engine");
  copyXml(path.join(source, "systems"), "systems");
  sdk.loadModelOrThrow("c172p");
  sdk.setPropertyValue("ic/h-sl-ft", 3000);
  sdk.setPropertyValue("ic/vc-kts", 90);
  if (!sdk.runIc()) throw new Error("Initial conditions failed");
  for (let frame = 0; frame < 120; frame++) {
    if (!sdk.run()) break;
  }
  console.log(sdk.getPropertyValue("position/h-sl-ft"));
} finally {
  sdk.destroy();
}
```

`JSBSimSdk` extends the generated `JSBSimApi` camelCase wrapper. The raw native
executive is available as `sdk.exec`; its lifetime belongs to the SDK. Call
`sdk.destroy()` when finished, including after failed model loads. Repeated
destruction is harmless. See [ownership and diagnostics](docs/sdk-lifetime-and-diagnostics.md).

## Browsers and persistence

Serve the generated `.mjs` loader and `.wasm` binary with JavaScript and
`application/wasm` MIME types. The package's `/wasm` export provides URLs relative
to the installed module. Bundlers may require excluding `@jsbsim/wasm` from
dependency optimization, or copying the two runtime files into public assets and
passing their URLs explicitly. The `/wasm/module` and `/wasm/binary` exports also
identify the raw artifacts.

MEMFS holds runtime files. IDBFS is included for optional IndexedDB persistence:

```js
const sdk = await JSBSimSdk.create({
  moduleUrl: wasmModuleUrl,
  wasmUrl: wasmBinaryUrl,
  persistence: { enabled: true },
});
try {
  sdk.writeDataFile("settings.txt", "saved value");
  await sdk.syncToPersistence();
} finally {
  sdk.destroy();
}
```

Creation with persistence enabled restores the previous snapshot before the
executive is allocated. `syncToPersistence()` replaces the stored snapshot with
runtime files; `syncFromPersistence()` replaces runtime files with the stored
snapshot. Await synchronization before further filesystem changes or destruction.
Persistence is explicit; destruction does not save automatically.

Defaults are `/runtime` and `/persist`. Custom runtime and persistence roots must
be disjoint after normalization; equal or nested roots are rejected before
runtime creation. IndexedDB is scoped to the browser origin and mount path.
Node.js and browsers without IndexedDB reject persistence requests explicitly.
Applications should handle storage failures and avoid concurrent writers to the
same persistent mount. Raw `sdk.module.FS` access bypasses SDK safeguards.

## Validation

The package build runs source-contract, injected-failure and real-WASM lifecycle
tests against its captured source and model fixtures. Recheck the accepted build
with `npm --prefix wasm test` or `npm --prefix wasm run typecheck`.

The separate browser check uses headless Chromium, serves exact artifact bytes
through Playwright request routing, and opens no dev server:

```sh
cd wasm
npx playwright install --only-shell --no-remove chromium
node test/browser-persistence.mjs
```

It checks IndexedDB text/binary restoration across navigation, persisted deletion
and updates, executive cleanup, and the identity/hashes of the loaded artifact.
Pass `--artifact=/absolute/artifact/directory` to inspect a specific accepted build.
These checks establish software behavior, not real-aircraft calibration.

## License and attribution

The JavaScript/TypeScript wrapper originates with **0x62 (Benedict Lewis)** and
retains its MIT license. The compiled JSBSim engine retains its native LGPL terms.
Packages include component notices under `dist/licenses/`; consult those notices
for the runtime and its dependencies. Source attribution is retained separately
from package release ownership.
