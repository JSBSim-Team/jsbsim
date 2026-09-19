import path from "node:path";
/** Only explicitly reviewed distribution banners are accepted; actual text stays in identity. */
export function checkToolchain(actual, expected) {
  if (expected?.schemaVersion !== 1) throw new Error("Invalid build toolchain lock.");
  for (const key of ["node", "npm", "cmake"]) {
    if (typeof expected[key] !== "string" || !expected[key] || actual[key] !== expected[key]) {
      throw new Error("Toolchain mismatch for " + key + ": expected " + expected[key] + ", found " + actual[key]);
    }
  }
  const profiles = expected.emscripten?.profiles;
  if (!Array.isArray(profiles) || !profiles.some(profile => profile.banner === actual.emscripten && profile.platforms.includes(actual.platform))) {
    throw new Error("Unapproved Emscripten distribution/banner: " + actual.emscripten + " on " + actual.platform);
  }
}

// These variables can add uncaptured headers/libraries or change compiler/linker
// semantics outside the explicit recorded build recipe. Cache locations alone
// are operational settings and are not treated as source-selection overrides.
const compilerOverrides = [
  "CC", "CXX", "CPP", "LD", "AR", "RANLIB", "CFLAGS", "CXXFLAGS", "CPPFLAGS", "LDFLAGS",
  "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH", "LIBRARY_PATH", "COMPILER_PATH",
  "SDKROOT", "MACOSX_DEPLOYMENT_TARGET", "CMAKE_ARGS", "CMAKE_TOOLCHAIN_FILE", "CMAKE_PREFIX_PATH",
  "CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER", "CMAKE_GENERATOR", "CMAKE_BUILD_TYPE",
  "LLVM_ROOT", "BINARYEN_ROOT", "EM_LLVM_ROOT", "EM_BINARYEN_ROOT", "EMCC_CFLAGS", "EMMAKEN_CFLAGS", "EMMAKEN_JUST_CONFIGURE",
  "EMCC_FORCE_STDLIBS", "EMCC_ONLY_FORCED_STDLIBS", "EMCC_AUTODEBUG", "EMCC_SKIP_SANITY_CHECK",
];
export function rejectAmbientCompilerOverrides(env) {
  if (env.EM_CONFIG && (!env.EMSDK || path.resolve(env.EM_CONFIG) !== path.resolve(env.EMSDK, ".emscripten"))) {
    throw new Error("Compiler-affecting environment EM_CONFIG must be the standard EMSDK/.emscripten file.");
  }
  const found = compilerOverrides.filter(key => typeof env[key] === "string" && env[key].trim());
  if (found.length) throw new Error("Compiler-affecting environment overrides are not part of the build recipe: " + found.join(", "));
}
