# Native ownership and model-load diagnostics

A `JSBSimSdk` owns its native `FGFDMExec`. Call `sdk.destroy()` when finished;
repeated calls are harmless. Do not separately delete `sdk.exec`. Destruction
stops SDK log forwarding and invokes the executive's Embind `delete()` once.
Custom runtime factories exposing only `module.destroy(exec)` retain a
compatibility path. A missing destructor is an error.

Path-configuration errors after allocation free the executive. If initialization
and cleanup both fail, an `AggregateError` preserves both failures. Releasing
SDK-owned native objects does not promise that a browser returns an entire
Emscripten module's memory allocation to the operating system.

The generated `JSBSimApi` mirrors native methods with camelCase names and default
arguments. The raw `sdk.exec` object bypasses SDK checks. Complex native values
exposed as numeric opaque handles are borrowed native addresses; they are not
owned JavaScript objects and must not outlive or be reused after their native
owner is replaced. Prefer named property access when a native value has a
property-tree representation.

## Model loading

`loadModel()` and `loadModelWithOptions()` preserve native boolean results.
`loadModelOrThrow()` provides contextual failure information:

```js
try {
  sdk.loadModelOrThrow("c172p");
} catch (error) {
  if (error instanceof JSBSimModelLoadError) {
    console.error(error.model, error.paths, error.logs, error.cause);
  }
  throw error;
}
```

The error distinguishes a false native return from an exception. Numeric WASM
exceptions remain available as `cause`; the SDK does not invent decoded messages.
Each attempt retains at most 128 log entries, with message/raw fields limited to
their last 2048 characters. The error message includes the last eight entries.
Earlier runtime output is excluded and successive attempts use separate buffers.

SDK log entries preserve `raw` native output. Their `message` strips terminal
escapes by default; `stripAnsi: false` keeps them. The SDK does not require a
native logger behavior change. `stdout`, `stderr` and combined `log` listeners
are available through `on`, `off` and `once`.

## Persistence

Persistence uses a separate IDBFS mount and copies a complete snapshot between
it and the runtime MEMFS tree. Their normalized paths must not be equal or nested.
Validation precedes runtime loading and filesystem mutation. In environments
without IndexedDB, persistence requests reject explicitly.

Await synchronization before modifying files or destroying the SDK. Restoration
replaces runtime files, saving replaces persisted files, and destruction does
not implicitly save. Storage availability, quota and simultaneous writes from
other pages remain application concerns.

## Checks

Injected runtime tests exercise lifetime, initialization/cleanup errors,
diagnostics and persistence boundaries. Real WASM tests cover repeated create /
load / run / destroy sequences, failed model loads followed by successful loads,
and allocation cleanup. The browser test covers actual IndexedDB writes,
restoration, deletion and native executive disposal across navigation.
