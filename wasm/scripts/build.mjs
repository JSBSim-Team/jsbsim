#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { copyFile, mkdir, mkdtemp, readFile, readdir, realpath, rename, stat, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { buildInputSha256, captureRepository, contentManifest, copyManifest, generatedBuildIdentity, sha256, verifyContent, verifyGeneratedBindings, verifyWorkspaceInputs, writeJson } from "./build-system/source.mjs";

import { checkToolchain, rejectAmbientCompilerOverrides } from "./build-system/toolchain.mjs";

const sdkRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

function argumentsFor(argv) {
  rejectAmbientCompilerOverrides(process.env);
  const options = { allowDirty: false, allowNetwork: false, prepareOnly: false };
  for (const arg of argv) {
    if (arg === "--allow-dirty") options.allowDirty = true;
    else if (arg === "--allow-network") options.allowNetwork = true;
    else if (arg === "--prepare-only") options.prepareOnly = true;
    else throw new Error("Unknown build option: " + arg);
  }
  if (process.env.JSBSIM_SOURCE_DIR || process.env.JSBSIM_SOURCE_ROOT || process.env.JSBSIM_BUILD_DESCRIPTOR) {
    throw new Error("The package builds the enclosing repository; source/descriptor environment overrides cannot select other inputs.");
  }
  return options;
}

function command(executable, args, options = {}) {
  const result = spawnSync(executable, args, { stdio: "inherit", ...options });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(executable + " " + args.join(" ") + " failed with " + (result.signal ?? result.status));
  return result;
}
function version(executable, args = ["--version"]) {
  const result = command(executable, args, { encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] });
  return (result.stdout + result.stderr).trim();
}
async function toolchain() {
  const emscripten = version("em++").split("\n")[0];
  const verbose = version("em++", ["-v"]);
  const configPath = version("em-config", ["EM_CONFIG"]);
  const configBytes = await readFile(configPath);
  return { configuration: { path: configPath, sha256: sha256(configBytes), bytes: configBytes }, identity: { node: process.version, npm: version("npm"), emscripten,
    cmake: version("cmake").split("\n")[0],
    clang: verbose.split("\n").find(line => /clang version/.test(line)) ?? emscripten,
    platform: process.platform, arch: process.arch, emscriptenConfigSha256: sha256(configBytes) } };
}

function publicNative(source) {
  return { origin: source.origin, commit: source.commit, contentSha256: source.contentSha256, dirty: source.dirty };
}
async function createDescriptor(options) {
  const cache = path.join(sdkRoot, "build", "sources");
  const { native, sdk } = await captureRepository(sdkRoot, { allowDirty: options.allowDirty, cacheRoot: cache });
  const packageJson = JSON.parse(await readFile(path.join(sdk.root, "package.json"), "utf8"));
  const lock = JSON.parse(await readFile(path.join(sdk.root, "package-lock.json"), "utf8"));
  if (lock.name !== packageJson.name || lock.version !== packageJson.version ||
      lock.packages?.[""]?.name !== packageJson.name || lock.packages?.[""]?.version !== packageJson.version) {
    throw new Error("SDK package and package-lock identities disagree.");
  }
  const measuredToolchain = await toolchain();
  const tools = measuredToolchain.identity;
  checkToolchain(tools, JSON.parse(await readFile(path.join(sdk.root, "build-toolchain.lock.json"), "utf8")));
  const recipe = { schemaVersion: 2, package: { name: packageJson.name, version: packageJson.version }, native: publicNative(native),
    sdk: { commit: sdk.commit, contentSha256: sdk.contentSha256, dirty: sdk.dirty, path: "wasm" },
    mode: "in-tree", toolchain: tools, options: { buildType: "Release", cxxStandard: 17, sdkTarget: "es2022" } };
  const inputSha256 = buildInputSha256({ ...recipe, build: { mode: recipe.mode, toolchain: recipe.toolchain, options: recipe.options } });
  const identity = { schemaVersion: 2, package: recipe.package, native: recipe.native, sdk: recipe.sdk,
    build: { mode: recipe.mode, inputSha256, toolchain: recipe.toolchain, options: recipe.options } };
  await mkdir(path.join(sdkRoot, "build", "attempts"), { recursive: true });
  const attemptRoot = await mkdtemp(path.join(sdkRoot, "build", "attempts", inputSha256.slice(0, 12) + "-"));
  await writeFile(path.join(attemptRoot, "emscripten-config.py"), measuredToolchain.configuration.bytes);
  const toolchainConfiguration = { path: measuredToolchain.configuration.path, sha256: measuredToolchain.configuration.sha256 };
  const workspaceRoot = path.join(attemptRoot, "sdk");
  await copyManifest(sdk.root, workspaceRoot, sdk.manifest);
  const descriptor = { schemaVersion: 2, identity, native, sdk, attemptRoot, workspaceRoot, toolchainConfiguration,
    wasmBuildRoot: path.join(attemptRoot, "wasm"), distRoot: path.join(workspaceRoot, "dist") };
  await writeJson(path.join(attemptRoot, "descriptor.json"), descriptor);
  return descriptor;
}

async function verifyWorkspace(descriptor) {
  if (sha256(await readFile(descriptor.toolchainConfiguration.path)) !== descriptor.toolchainConfiguration.sha256) {
    throw new Error("Emscripten configuration changed during the build.");
  }
  await verifyWorkspaceInputs(descriptor);
  await verifyGeneratedBindings(descriptor);
  await verifyContent(descriptor.native.root, descriptor.native.manifest);
  await verifyContent(descriptor.sdk.root, descriptor.sdk.manifest);
}

async function bundleNativeNotices(descriptor) {
  const notices = [];
  for (const file of descriptor.native.manifest) {
    if (/^(?:COPYING|LICENSE|AUTHORS|NOTICE)(?:[._-].*)?$/i.test(file.path)) notices.push(file);
  }
  if (!notices.length) throw new Error("No native license notices found in selected source.");
  for (const file of notices) {
    const destination = path.join(descriptor.distRoot, "licenses", "jsbsim", file.path);
    await mkdir(path.dirname(destination), { recursive: true });
    await copyFile(path.join(descriptor.native.root, file.path), destination);
  }
  for (const file of ["src/GeographicLib/LICENSE.txt", "src/simgear/xml/COPYING"]) {
    const destination = path.join(descriptor.distRoot, "licenses", "jsbsim", file);
    await mkdir(path.dirname(destination), { recursive: true });
    await copyFile(path.join(descriptor.native.root, file), destination);
  }
  await mkdir(path.join(descriptor.distRoot, "licenses"), { recursive: true });
  await copyFile(path.join(descriptor.workspaceRoot, "LICENSE"), path.join(descriptor.distRoot, "licenses", "sdk-LICENSE"));
  await copyFile(path.join(descriptor.workspaceRoot, "NOTICE"), path.join(descriptor.distRoot, "licenses", "sdk-NOTICE"));
  await writeFile(path.join(descriptor.distRoot, "licenses", "README.txt"),
    "The SDK wrapper retains its MIT notice. The bundled JSBSim runtime retains its native LGPL notices.\n" +
    "Native origin: " + descriptor.identity.native.origin + "\nNative commit: " + descriptor.identity.native.commit + "\n" +
    "Native content SHA-256: " + descriptor.identity.native.contentSha256 + "\n");
}

async function finalize(descriptor, commands) {
  await verifyWorkspace(descriptor);
  await bundleNativeNotices(descriptor);
  const files = Object.fromEntries((await contentManifest(descriptor.distRoot)).map(file => [file.path, file.sha256]));
  const bindings = JSON.parse(await readFile(path.join(descriptor.workspaceRoot, "generated/bindings-manifest.json"), "utf8"));
  const inputHash = name => descriptor.sdk.manifest.find(file => file.path === name)?.sha256;
  const packageFiles = Object.fromEntries(await Promise.all(["package.json", "LICENSE", "LICENSES.md", "NOTICE", "README.md"].map(async name =>
    [name, sha256(await readFile(path.join(descriptor.workspaceRoot, name)))])));
  const metadata = { schemaVersion: 1, identity: descriptor.identity, files, packageFiles,
    provenance: {
      repository: { commit: descriptor.native.commit, contentSha256: descriptor.native.contentSha256, sdkPath: "wasm" },
      nativeArchive: null,
      nativeSourceLockSha256: null,
      sdkDependencyLockSha256: inputHash("package-lock.json"),
      toolchainLockSha256: inputHash("build-toolchain.lock.json"),
      generatedBindings: bindings,
    },
    validation: { status: "passed", commands } };
  await writeJson(path.join(descriptor.distRoot, "build-metadata.json"), metadata);
  const artifactSha256 = sha256(JSON.stringify({ identity: descriptor.identity, files }));
  const artifactRoot = path.join(sdkRoot, "build", "artifacts", artifactSha256);
  const staging = await mkdtemp(path.join(sdkRoot, "build", ".artifact-"));
  await copyManifest(descriptor.distRoot, path.join(staging, "dist"), await contentManifest(descriptor.distRoot));
  for (const name of ["package.json", "LICENSE", "LICENSES.md", "NOTICE", "README.md"]) {
    await copyFile(path.join(descriptor.workspaceRoot, name), path.join(staging, name));
  }
  await mkdir(path.dirname(artifactRoot), { recursive: true });
  try { await rename(staging, artifactRoot); }
  catch (error) {
    if (!["EEXIST", "ENOTEMPTY"].includes(error.code)) throw error;
    const expected = await contentManifest(staging);
    await verifyContent(artifactRoot, expected);
    await (await import("node:fs/promises")).rm(staging, { recursive: true, force: true });
  }
  const result = { schemaVersion: 1, artifactSha256, artifactRoot,
    descriptor: path.join(descriptor.attemptRoot, "descriptor.json"), identity: descriptor.identity };
  // A pointer identifies a completed candidate; it never rewrites live dist.
  const pointer = path.join(sdkRoot, "build", "last-build.json");
  const pointerStaging = pointer + "." + process.pid;
  await writeJson(pointerStaging, result);
  await rename(pointerStaging, pointer);
  console.log(JSON.stringify(result, null, 2));
  return result;
}

async function build(options) {
  const descriptor = await createDescriptor(options);
  const descriptorFile = path.join(descriptor.attemptRoot, "descriptor.json");
  if (options.prepareOnly) { console.log(descriptorFile); return; }
  const env = { ...process.env, JSBSIM_BUILD_DESCRIPTOR: descriptorFile,
    JSBSIM_SOURCE_ROOT: descriptor.native.root, GITHUB_SHA: descriptor.identity.native.commit,
    TRAVIS_COMMIT: descriptor.identity.native.commit, APPVEYOR_REPO_COMMIT: descriptor.identity.native.commit };
  const cwd = descriptor.workspaceRoot;
  const commands = [];
  const run = (exe, args) => command(exe, args, { cwd, env });
  run("npm", ["ci", "--ignore-scripts", "--no-audit", "--fund=false", ...(options.allowNetwork ? [] : ["--offline"])]);
  await writeFile(path.join(cwd, "src/build-identity.ts"), generatedBuildIdentity(descriptor.identity));
  run(process.execPath, ["scripts/generate-fgfdmexec-bindings.mjs"]);
  run(process.execPath, ["scripts/build-wasm.mjs"]);
  run(path.join(cwd, "node_modules", ".bin", "tsup"), ["--config", "tsup.config.ts"]);
  run(path.join(cwd, "node_modules", ".bin", "tsc"), ["--noEmit"]);
  commands.push("tsc --noEmit (frozen SDK workspace)");
  const testFiles = (await readdir(path.join(cwd, "test"))).filter(name => name.endsWith(".test.mjs")).sort();
  run(process.execPath, ["--test", ...testFiles.map(name => "test/" + name)]);
  commands.push("node --test test/*.test.mjs (frozen SDK workspace and resolved native fixtures)");
  await finalize(descriptor, commands);
}

try { await build(argumentsFor(process.argv.slice(2))); }
catch (error) { console.error(error.stack ?? error); process.exitCode = 1; }
