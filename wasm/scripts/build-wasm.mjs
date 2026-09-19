#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { copyFile, mkdir, readFile, stat } from "node:fs/promises";
import path from "node:path";
import { readDescriptor, verifyCmakeCache, verifyGeneratedBindings, verifyContent } from "./build-system/source.mjs";

const descriptor = await readDescriptor(process.env.JSBSIM_BUILD_DESCRIPTOR);
const workspace = descriptor.workspaceRoot;
await verifyContent(descriptor.native.root, descriptor.native.manifest);
await verifyGeneratedBindings(descriptor);
const cachePath = path.join(descriptor.wasmBuildRoot, "CMakeCache.txt");
try {
  const cache = await readFile(cachePath, "utf8");
  verifyCmakeCache(descriptor, cache);
} catch (error) { if (error.code !== "ENOENT") throw error; }
const run = (command, args) => {
  const result = spawnSync(command, args, { cwd: workspace, env: process.env, stdio: "inherit" });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(command + " failed with " + (result.signal ?? result.status));
};
run("emcmake", ["cmake", "-S", descriptor.native.root, "-B", descriptor.wasmBuildRoot,
  "-DBUILD_WASM_MODULE=ON", "-DBUILD_DOCS=OFF", "-DBUILD_PYTHON_MODULE=OFF",
  "-DBUILD_JULIA_PACKAGE=OFF", "-DBUILD_MATLAB_SFUNCTION=OFF", "-DBUILD_SHARED_LIBS=OFF", "-DSYSTEM_EXPAT=OFF",
  "-DJSBSIM_WASM_BINDINGS=" + path.join(workspace, "generated/FGFDMExecBindings.cpp"),
  "-DJSBSIM_BINDINGS_MANIFEST=" + path.join(workspace, "generated/bindings-manifest.json"),
  "-DJSBSIM_BUILD_INPUT_SHA256=" + descriptor.identity.build.inputSha256,
  "-DJSBSIM_NATIVE_CONTENT_SHA256=" + descriptor.native.contentSha256,
  "-DCMAKE_BUILD_TYPE=Release"]);
run("cmake", ["--build", descriptor.wasmBuildRoot, "--target", "jsbsim_wasm", "--parallel", "8"]);
const out = path.join(descriptor.distRoot, "wasm");
await mkdir(out, { recursive: true });
for (const name of ["jsbsim_wasm.mjs", "jsbsim_wasm.wasm", "jsbsim_wasm.data"]) {
  const source = path.join(descriptor.wasmBuildRoot, name);
  try { await stat(source); }
  catch (error) { if (name.endsWith(".data") && error.code === "ENOENT") continue; throw error; }
  await copyFile(source, path.join(out, name));
}
