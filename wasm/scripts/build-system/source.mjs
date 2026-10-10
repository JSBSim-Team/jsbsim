import { createHash } from "node:crypto";
import { execFileSync } from "node:child_process";
import { chmod, copyFile, link, lstat, mkdir, mkdtemp, readFile, readdir, realpath, rename, rm, writeFile } from "node:fs/promises";
import path from "node:path";

export const sha256 = value => createHash("sha256").update(value).digest("hex");
export const manifestSha256 = manifest => sha256(JSON.stringify(manifest));
const HEX = /^[a-f0-9]{64}$/;
const excludedDirectories = new Set([".git", "node_modules", "build", "dist", ".context", ".venv", "__pycache__"]);
const posix = value => value.split(path.sep).join("/");

export function safeRelative(value) {
  if (typeof value !== "string" || !value || value.includes("\0") || value.includes("\\") ||
      value.startsWith("/") || /^[A-Za-z]:/.test(value) || value.split("/").some(part => part === ".." || part === ".git")) {
    throw new Error("Unsafe source path: " + value);
  }
  const normalized = path.posix.normalize(value).replace(/^\.\//, "").replace(/\/$/, "");
  if (!normalized || normalized === ".") throw new Error("Empty source path.");
  return normalized;
}

/** Digest contract: sorted POSIX paths, normalized executable mode, file SHA. */
export async function contentManifest(root, { roots = ["."], exclude = () => false, ignoreBuildDirectories = false, rejectGitDirectories = false } = {}) {
  const files = [];
  async function visit(relative) {
    const name = path.basename(relative);
    if (name === ".git" && rejectGitDirectories) throw new Error("Nested Git metadata is not allowed in a frozen workspace.");
    if (name === ".git" || name === ".DS_Store" || exclude(posix(relative)) ||
        (ignoreBuildDirectories && excludedDirectories.has(name))) return;
    const absolute = path.join(root, relative), info = await lstat(absolute);
    if (info.isSymbolicLink()) throw new Error("Source symlinks are not captured: " + absolute);
    if (info.isDirectory()) {
      for (const child of (await readdir(absolute)).sort()) await visit(path.join(relative, child));
    } else if (info.isFile()) {
      files.push({ path: safeRelative(posix(relative)), mode: info.mode & 0o111 ? 0o755 : 0o644,
        sha256: sha256(await readFile(absolute)) });
    } else throw new Error("Unsupported source file: " + absolute);
  }
  for (const relative of roots) await visit(relative);
  files.sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
  if (new Set(files.map(file => file.path)).size !== files.length) throw new Error("Duplicate source input.");
  return files;
}

export async function verifyContent(root, expected) {
  const actual = await contentManifest(root);
  if (manifestSha256(actual) !== manifestSha256(expected)) throw new Error("Captured source content changed: " + root);
}

export async function copyManifest(from, to, manifest) {
  for (const file of manifest) {
    const target = path.join(to, safeRelative(file.path));
    await mkdir(path.dirname(target), { recursive: true });
    await copyFile(path.join(from, file.path), target);
    await chmod(target, file.mode);
  }
}

function git(root, args) {
  return execFileSync("git", ["--no-optional-locks", "-C", root, ...args], { encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
}
export async function nativeInputRoots(root) {
  const paths = git(root, ["ls-files", "--cached", "--others", "--exclude-standard", "-z"]).split("\0").filter(Boolean);
  const retained = [];
  for (const file of new Set(paths)) {
    try {
      const info = await lstat(path.join(root, file));
      if (info.isDirectory()) throw new Error("Nested repositories or Git submodules cannot be captured as source: " + file);
      retained.push(safeRelative(file));
    }
    catch (error) { if (error.code !== "ENOENT") throw error; }
  }
  return retained.sort();
}
export async function gitIdentity(root) {
  const canonical = await realpath(root);
  if (await realpath(git(canonical, ["rev-parse", "--show-toplevel"])) !== canonical) {
    throw new Error("Source is not the root of its own repository: " + canonical);
  }
  return { commit: git(canonical, ["rev-parse", "HEAD"]),
    dirty: git(canonical, ["status", "--porcelain", "--untracked-files=all"]).length > 0,
    trackedDiffSha256: sha256(git(canonical, ["diff", "--binary", "HEAD"])) };
}

/** Snapshot content and identity must both stay stable throughout capture. */
export async function captureSource(root, cache, options = {}) {
  root = await realpath(root);
  await mkdir(cache, { recursive: true });
  const scan = async () => contentManifest(root, { ...options,
    roots: options.rootsReader ? await options.rootsReader(root) : options.roots });
  for (let attempt = 0; attempt < 3; attempt++) {
    const identityReader = options.identityReader ?? gitIdentity;
    const identity = await identityReader(root);
    const before = await scan();
    const staging = await mkdtemp(path.join(cache, ".capture-"));
    try {
      await copyManifest(root, staging, before);
      const after = await scan(), finalIdentity = await identityReader(root);
      if (manifestSha256(before) !== manifestSha256(after) || JSON.stringify(identity) !== JSON.stringify(finalIdentity)) continue;
      await verifyContent(staging, before);
      const digest = manifestSha256(before), destination = path.join(cache, digest);
      try { await rename(staging, destination); }
      catch (error) {
        if (!["EEXIST", "ENOTEMPTY"].includes(error.code)) throw error;
        await verifyContent(destination, before);
      }
      return { root: destination, originRoot: root, ...identity, contentSha256: digest, manifest: before };
    } finally { await rm(staging, { recursive: true, force: true }); }
  }
  throw new Error("Source changed while being captured; stop concurrent edits and retry: " + root);
}

/** Every WASM build captures the enclosing engine repository once. */
export async function captureRepository(sdkRoot, { allowDirty = false, cacheRoot } = {}) {
  sdkRoot = await realpath(sdkRoot);
  const repositoryRoot = await realpath(path.resolve(sdkRoot, ".."));
  if (path.basename(sdkRoot) !== "wasm" ||
      await realpath(git(sdkRoot, ["rev-parse", "--show-toplevel"])) !== repositoryRoot) {
    throw new Error("WASM source must be the wasm/ directory of its enclosing JSBSim repository.");
  }
  const identityReader = async root => ({ ...await gitIdentity(root),
    origin: git(root, ["remote", "get-url", "origin"]).replace(/^git@github\.com:/, "https://github.com/").replace(/\.git$/, "") });
  const capture = await captureSource(repositoryRoot, path.join(cacheRoot, "repository"), {
    rootsReader: nativeInputRoots, identityReader,
  });
  if (capture.dirty && !allowDirty) throw new Error("In-tree builds require a clean repository. Use --allow-dirty for a labeled development candidate.");
  if (!capture.manifest.some(file => file.path === "src/FGFDMExec.h") ||
      !capture.manifest.some(file => file.path === "CMakeLists.txt") ||
      !capture.manifest.some(file => file.path === "wasm/CMakeLists.txt")) {
    throw new Error("Captured repository is missing the native engine or in-tree WASM target.");
  }
  const native = { ...capture, mode: "in-tree" };
  const manifest = sdkManifest(capture.manifest);
  const sdk = { root: path.join(capture.root, "wasm"), originRoot: sdkRoot, path: "wasm",
    commit: capture.commit, dirty: capture.dirty, contentSha256: manifestSha256(manifest), manifest };
  await verifyContent(sdk.root, sdk.manifest);
  return { native, sdk };
}

export function sdkManifest(repositoryManifest) {
  const manifest = repositoryManifest.filter(file => file.path.startsWith("wasm/"))
    .map(file => ({ ...file, path: file.path.slice(5) }));
  if (!manifest.length) throw new Error("Repository has no WASM SDK source.");
  return manifest;
}

export async function writeJson(file, value) {
  await mkdir(path.dirname(file), { recursive: true });
  await writeFile(file, JSON.stringify(value, null, 2) + "\n");
}

export function buildInputSha256(identity) {
  const i = identity;
  return sha256(JSON.stringify({
    ...(i.schemaVersion === 2 ? { schemaVersion: 2 } : {}),
    package: { name: i.package.name, version: i.package.version },
    native: { origin: i.native.origin, commit: i.native.commit, contentSha256: i.native.contentSha256, dirty: i.native.dirty },
    sdk: { commit: i.sdk.commit, contentSha256: i.sdk.contentSha256, dirty: i.sdk.dirty, ...(i.schemaVersion === 2 ? { path: i.sdk.path } : {}) },
    mode: i.build.mode,
    toolchain: Object.fromEntries(["node", "npm", "emscripten", "cmake", "clang", "platform", "arch", "emscriptenConfigSha256"].map(key => [key, i.build.toolchain[key]])),
    options: { buildType: i.build.options.buildType, cxxStandard: i.build.options.cxxStandard, sdkTarget: i.build.options.sdkTarget },
  }));
}

function validateManifest(manifest) {
  if (!Array.isArray(manifest) || !manifest.length) throw new Error("Missing source manifest.");
  let previous = "";
  for (const file of manifest) {
    const name = safeRelative(file.path);
    if (name !== file.path || name <= previous || ![0o644, 0o755].includes(file.mode) || !HEX.test(file.sha256 ?? "")) {
      throw new Error("Invalid or unsorted source manifest.");
    }
    previous = name;
  }
}

export async function readDescriptor(file) {
  if (!file) throw new Error("A resolved JSBSIM_BUILD_DESCRIPTOR is required; use npm run build or build:dev.");
  const descriptor = JSON.parse(await readFile(file, "utf8")), identity = descriptor.identity;
  if (descriptor.schemaVersion !== 2 || identity?.schemaVersion !== 2 || !HEX.test(identity.build?.inputSha256 ?? "") ||
      typeof descriptor.workspaceRoot !== "string" || !path.isAbsolute(descriptor.workspaceRoot) ||
      typeof identity.package?.name !== "string" || !identity.package.name || typeof identity.package?.version !== "string" || !identity.package.version ||
      identity.build.mode !== "in-tree") throw new Error("Invalid build descriptor.");
  for (const kind of ["native", "sdk"]) {
    const source = descriptor[kind], declared = identity[kind];
    if (!source || typeof source.root !== "string" || !path.isAbsolute(source.root) ||
        !/^[a-f0-9]{40}$/.test(declared?.commit ?? "") || !HEX.test(declared?.contentSha256 ?? "") ||
        typeof declared?.dirty !== "boolean" || source.commit !== declared.commit ||
        source.contentSha256 !== declared.contentSha256 || source.dirty !== declared.dirty) {
      throw new Error(kind + " descriptor identity mismatch.");
    }
    validateManifest(source.manifest);
    if (manifestSha256(source.manifest) !== source.contentSha256) throw new Error(kind + " source manifest digest mismatch.");
  }
  if (typeof identity.native.origin !== "string" || !identity.native.origin || descriptor.native.origin !== identity.native.origin) throw new Error("Native origin mismatch.");
  if (descriptor.native.mode !== "in-tree" || identity.sdk.path !== "wasm" || descriptor.sdk.path !== "wasm" ||
      identity.native.commit !== identity.sdk.commit || identity.native.dirty !== identity.sdk.dirty ||
      path.resolve(descriptor.sdk.root) !== path.join(path.resolve(descriptor.native.root), "wasm") ||
      manifestSha256(sdkManifest(descriptor.native.manifest)) !== descriptor.sdk.contentSha256) {
    throw new Error("Native and SDK inputs must come from the same captured repository and wasm/ subtree.");
  }
  for (const key of ["node", "npm", "emscripten", "cmake", "clang", "platform", "arch"]) {
    if (typeof identity.build.toolchain?.[key] !== "string" || !identity.build.toolchain[key]) throw new Error("Missing toolchain identity.");
  }
  if (!HEX.test(identity.build.toolchain.emscriptenConfigSha256 ?? "")) throw new Error("Missing Emscripten configuration digest.");
  if (identity.build.options?.buildType !== "Release" || identity.build.options.cxxStandard !== 17 || identity.build.options.sdkTarget !== "es2022" ||
      buildInputSha256(identity) !== identity.build.inputSha256) throw new Error("Build recipe identity mismatch.");
  return descriptor;
}

export function verifyCmakeCache(descriptor, cache) {
  if (cache.match(/^CMAKE_HOME_DIRECTORY:[^=]*=(.+)$/m)?.[1] !== descriptor.native.root ||
      cache.match(/^BUILD_WASM_MODULE:[^=]*=(.+)$/m)?.[1] !== "ON" ||
      cache.match(/^JSBSIM_BUILD_INPUT_SHA256:[^=]*=(.+)$/m)?.[1] !== descriptor.identity.build.inputSha256) {
    throw new Error("CMake cache belongs to a different resolved source.");
  }
}

export async function verifyGeneratedBindings(descriptor) {
  const workspace = descriptor.workspaceRoot;
  const stamp = JSON.parse(await readFile(path.join(workspace, "generated/bindings-manifest.json"), "utf8"));
  if (stamp.schemaVersion !== 1 || stamp.inputSha256 !== descriptor.identity.build.inputSha256 ||
      stamp.nativeContentSha256 !== descriptor.native.contentSha256 || stamp.nativeCommit !== descriptor.native.commit) {
    throw new Error("Generated bindings belong to a different source/build identity.");
  }
  const required = ["generated/FGFDMExecBindings.cpp", "src/generated/fgfdmexec-api.ts", "src/generated/jsbsim-api.ts"];
  if (!stamp.files || Object.keys(stamp.files).sort().join("|") !== [...required].sort().join("|")) throw new Error("Incomplete generated binding inventory.");
  for (const file of required) {
    if (sha256(await readFile(path.join(workspace, file))) !== stamp.files[file]) throw new Error("Generated bindings changed: " + file);
  }
  return stamp;
}

/** Atomically install completed bytes without replacing an earlier immutable file. */
export async function installImmutableFile(from, to) {
  await mkdir(path.dirname(to), { recursive: true });
  try { await link(from, to); }
  catch (error) {
    if (error.code !== "EEXIST") throw error;
    if ((await lstat(to)).isSymbolicLink() || sha256(await readFile(from)) !== sha256(await readFile(to))) {
      throw new Error("Immutable output already exists with different bytes: " + to);
    }
  }
}

export function generatedBuildIdentity(identity) {
  return "// Generated from frozen inputs. Do not edit.\nexport const buildIdentity = " + JSON.stringify(identity, null, 2) +
    " as const;\nexport type JSBSimBuildIdentity = typeof buildIdentity;\n";
}

export async function verifyWorkspaceInputs(descriptor) {
  const outputDirectories = ["node_modules", "dist", "build", "generated", "src/generated"];
  const generated = relative => relative === "src/build-identity.ts" ||
    outputDirectories.some(directory => relative === directory || relative.startsWith(directory + "/")) || relative.endsWith(".tsbuildinfo");
  const actual = await contentManifest(descriptor.workspaceRoot, { exclude: generated, rejectGitDirectories: true });
  const expected = descriptor.sdk.manifest.filter(file => !generated(file.path));
  if (manifestSha256(actual) !== manifestSha256(expected)) throw new Error("SDK authored workspace inputs changed after capture.");
  if (await readFile(path.join(descriptor.workspaceRoot, "src/build-identity.ts"), "utf8") !== generatedBuildIdentity(descriptor.identity)) {
    throw new Error("Generated workspace build identity changed.");
  }
}
