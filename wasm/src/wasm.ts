/**
 * URL to the packaged Emscripten module entrypoint.
 */
export const wasmModuleUrl = new URL("./wasm/jsbsim_wasm.mjs", import.meta.url);

/**
 * URL to the packaged wasm binary.
 */
export const wasmBinaryUrl = new URL("./wasm/jsbsim_wasm.wasm", import.meta.url);

/**
 * Relative path to the packaged Emscripten module entrypoint.
 */
export const wasmModulePath = "./wasm/jsbsim_wasm.mjs";

/**
 * Relative path to the packaged wasm binary.
 */
export const wasmBinaryPath = "./wasm/jsbsim_wasm.wasm";
