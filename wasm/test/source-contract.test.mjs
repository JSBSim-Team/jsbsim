import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { after, describe, it } from "node:test";
import { chmod, mkdtemp, mkdir, readFile, rm, symlink, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { buildInputSha256, captureRepository, captureSource, contentManifest, generatedBuildIdentity, installImmutableFile, manifestSha256, readDescriptor,
  safeRelative, sha256, verifyCmakeCache, verifyContent, verifyGeneratedBindings, verifyWorkspaceInputs, writeJson } from "../scripts/build-system/source.mjs";

import { checkToolchain, rejectAmbientCompilerOverrides } from "../scripts/build-system/toolchain.mjs";

const temporary = [];
async function directory() {
  const result = await mkdtemp(path.join(os.tmpdir(), "jsbsim-source-contract-"));
  temporary.push(result);
  return result;
}
after(async () => { for (const root of temporary) await rm(root, { recursive: true, force: true }); });

function fixtureGit(root, ...args) {
  return execFileSync("git", ["-c", "core.hooksPath=/dev/null", "-C", root, ...args], {
    encoding: "utf8", stdio: ["ignore", "pipe", "pipe"],
  }).trim();
}
async function repositoryFixture() {
  const root = await directory(), repository = path.join(root, "repository with spaces");
  const sdkRoot = path.join(repository, "wasm"), cacheRoot = path.join(root, "capture cache");
  await mkdir(path.join(repository, "src"), { recursive: true });
  await mkdir(path.join(sdkRoot, "src"), { recursive: true });
  await writeFile(path.join(repository, ".gitignore"), "build/\nnode_modules/\ndist/\n");
  await writeFile(path.join(repository, "CMakeLists.txt"), "project(JSBSim)\n");
  await writeFile(path.join(repository, "src/FGFDMExec.h"), "class FGFDMExec {};\n");
  await writeFile(path.join(repository, "src/engine.cpp"), "int fixture = 1;\n");
  await writeFile(path.join(sdkRoot, "package.json"), '{"name":"@fixture/jsbsim-wasm","version":"1.0.0"}\n');
  await writeFile(path.join(sdkRoot, "CMakeLists.txt"), "add_executable(jsbsim_wasm bindings.cpp)\n");
  await writeFile(path.join(sdkRoot, "src/sdk.ts"), "export const fixture = 1;\n");
  fixtureGit(repository, "init", "--initial-branch=main");
  fixtureGit(repository, "remote", "add", "origin", "https://github.com/JSBSim-Team/jsbsim.git");
  fixtureGit(repository, "add", ".");
  fixtureGit(repository, "-c", "user.name=Source contract fixture", "-c", "user.email=fixture@example.invalid", "commit", "-m", "Fixture source");
  return { root, repository, sdkRoot, cacheRoot, commit: fixtureGit(repository, "rev-parse", "HEAD") };
}

describe("resolved native source contract", () => {
  it("preserves byte and executable-mode identity while ignoring time metadata", async () => {
    const root = await directory();
    await writeFile(path.join(root, "b.cpp"), "b");
    await writeFile(path.join(root, "a.sh"), "a");
    const before = await contentManifest(root);
    assert.deepEqual(before.map(file => file.path), ["a.sh", "b.cpp"]);
    await chmod(path.join(root, "a.sh"), 0o755);
    const executable = await contentManifest(root);
    assert.notEqual(manifestSha256(before), manifestSha256(executable));
    assert.equal(executable[0].mode, 0o755);
    await writeFile(path.join(root, "b.cpp"), "changed");
    await assert.rejects(verifyContent(root, executable), /changed/);
  });

  it("rejects traversal and Git metadata source paths", () => {
    for (const relative of ["../outside", "/outside", "a/../escape", ".git/config"]) {
      assert.throws(() => safeRelative(relative), /Unsafe/);
    }
  });

  it("refuses uncaptured filesystem symlinks", async () => {
    const root = await directory();
    await symlink(os.tmpdir(), path.join(root, "outside"));
    await assert.rejects(contentManifest(root), /symlinks/);
  });

  it("captures one repository and derives the SDK from that exact snapshot", async () => {
    const fixture = await repositoryFixture();
    const { native, sdk } = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot });
    assert.equal(native.origin, "https://github.com/JSBSim-Team/jsbsim");
    assert.equal(native.commit, fixture.commit);
    assert.equal(sdk.commit, native.commit);
    assert.equal(native.dirty, false);
    assert.equal(sdk.dirty, false);
    assert.equal(sdk.root, path.join(native.root, "wasm"));
    assert.equal(sdk.path, "wasm");
    assert.deepEqual(sdk.manifest, native.manifest.filter(file => file.path.startsWith("wasm/"))
      .map(file => ({ ...file, path: file.path.slice("wasm/".length) })));
    assert.equal(sdk.contentSha256, manifestSha256(sdk.manifest));
    assert.equal(await readFile(path.join(native.root, "src/engine.cpp"), "utf8"), "int fixture = 1;\n");
    assert.equal(await readFile(path.join(sdk.root, "src/sdk.ts"), "utf8"), "export const fixture = 1;\n");
    assert.equal((await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot })).native.root, native.root);
  });

  it("requires explicit dirty capture and couples both identities to engine or SDK edits", async () => {
    const fixture = await repositoryFixture();
    const clean = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot });
    await writeFile(path.join(fixture.repository, "src/engine.cpp"), "int fixture = 2;\n");
    await assert.rejects(captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot }), /dirty|uncommitted/i);
    const engineChanged = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot, allowDirty: true });
    assert.notEqual(engineChanged.native.contentSha256, clean.native.contentSha256);
    assert.equal(engineChanged.sdk.contentSha256, clean.sdk.contentSha256);
    assert.equal(engineChanged.native.commit, clean.native.commit);
    assert.equal(engineChanged.sdk.commit, engineChanged.native.commit);
    assert.equal(engineChanged.native.dirty, true);
    assert.equal(engineChanged.sdk.dirty, true);
    await writeFile(path.join(fixture.sdkRoot, "src/sdk.ts"), "export const fixture = 2;\n");
    const sdkChanged = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot, allowDirty: true });
    assert.notEqual(sdkChanged.native.contentSha256, engineChanged.native.contentSha256);
    assert.notEqual(sdkChanged.sdk.contentSha256, engineChanged.sdk.contentSha256);
    assert.equal(sdkChanged.sdk.commit, sdkChanged.native.commit);
    assert.equal(sdkChanged.sdk.dirty, sdkChanged.native.dirty);
    assert.equal(await readFile(path.join(clean.native.root, "src/engine.cpp"), "utf8"), "int fixture = 1;\n");
  });

  it("captures new source files while excluding ignored build products", async () => {
    const fixture = await repositoryFixture();
    await writeFile(path.join(fixture.sdkRoot, "src/new.ts"), "export const added = true;\n");
    await writeFile(path.join(fixture.repository, "src/new.cpp"), "int added = 1;\n");
    await mkdir(path.join(fixture.sdkRoot, "node_modules"));
    await writeFile(path.join(fixture.sdkRoot, "node_modules/ignored.js"), "not an authored input");
    await mkdir(path.join(fixture.repository, "build"));
    await writeFile(path.join(fixture.repository, "build/ignored.o"), "generated bytes");
    const { native, sdk } = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot, allowDirty: true });
    assert.ok(native.manifest.some(file => file.path === "src/new.cpp"));
    assert.ok(sdk.manifest.some(file => file.path === "src/new.ts"));
    assert.ok(native.manifest.every(file => !file.path.startsWith("build/") && !file.path.includes("node_modules/")));
  });

  it("rejects SDKs outside the enclosing repository's wasm directory and nested repositories", async () => {
    const fixture = await repositoryFixture();
    await assert.rejects(captureRepository(fixture.repository, { cacheRoot: fixture.cacheRoot }));
    await assert.rejects(captureRepository(path.join(fixture.sdkRoot, "src"), { cacheRoot: fixture.cacheRoot }));
    fixtureGit(fixture.sdkRoot, "init", "--initial-branch=nested");
    await assert.rejects(captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot, allowDirty: true }), /root|repository|wasm/i);
  });

  it("rejects a nested repository elsewhere in the captured source tree", async () => {
    const fixture = await repositoryFixture();
    const nestedRoot = path.join(fixture.repository, "vendor/nested-engine");
    await mkdir(nestedRoot, { recursive: true });
    await writeFile(path.join(nestedRoot, "engine.cpp"), "int second_engine = 1;\n");
    fixtureGit(nestedRoot, "init", "--initial-branch=nested");
    fixtureGit(nestedRoot, "add", ".");
    fixtureGit(nestedRoot, "-c", "user.name=Source contract fixture", "-c", "user.email=fixture@example.invalid", "commit", "-m", "Nested source fixture");
    await assert.rejects(captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot, allowDirty: true }), /nested|directory|gitlink|repository/i);
  });

  it("refuses contaminated content-addressed source caches", async () => {
    const fixture = await repositoryFixture();
    const { native } = await captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot });
    await writeFile(path.join(native.root, "src/engine.cpp"), "contaminated snapshot");
    await assert.rejects(captureRepository(fixture.sdkRoot, { cacheRoot: fixture.cacheRoot }), /changed|digest|content/i);
  });

  it("captures exact local inputs and refuses concurrent identity changes", async () => {
    const root = await directory(), sourceRoot = path.join(root, "source");
    await mkdir(sourceRoot);
    await writeFile(path.join(sourceRoot, "new-untracked.cpp"), "source");
    const identity = { commit: "c".repeat(40), dirty: true, trackedDiffSha256: "d".repeat(64) };
    const captured = await captureSource(sourceRoot, path.join(root, "cache"), { identityReader: async () => identity });
    assert.equal(captured.dirty, true);
    assert.equal(await readFile(path.join(captured.root, "new-untracked.cpp"), "utf8"), "source");
    let changed = 0;
    await assert.rejects(captureSource(sourceRoot, path.join(root, "unstable"), {
      identityReader: async () => ({ ...identity, commit: String(++changed).padStart(40, "0") }),
    }), /changed while being captured/);
    let contentChange = 0;
    await assert.rejects(captureSource(sourceRoot, path.join(root, "unstable-content"), {
      identityReader: async () => identity,
      rootsReader: async () => {
        await writeFile(path.join(sourceRoot, "new-untracked.cpp"), "concurrent edit " + ++contentChange);
        return ["new-untracked.cpp"];
      },
    }), /changed while being captured/);
  });
});

async function descriptorFixture() {
  const root = await directory();
  const sdkManifest = [{ path: "src/sdk.ts", mode: 0o644, sha256: sha256("SDK fixture") }];
  const nativeManifest = [{ path: "src/engine.cpp", mode: 0o644, sha256: sha256("engine fixture") },
    ...sdkManifest.map(file => ({ ...file, path: "wasm/" + file.path }))];
  const native = { root: path.join(root, "native"), origin: "https://github.com/JSBSim-Team/jsbsim",
    mode: "in-tree", commit: "a".repeat(40), contentSha256: manifestSha256(nativeManifest), dirty: false, manifest: nativeManifest };
  const sdk = { root: path.join(native.root, "wasm"), path: "wasm", commit: native.commit,
    contentSha256: manifestSha256(sdkManifest), dirty: native.dirty, manifest: sdkManifest };
  const identity = { schemaVersion: 2, package: { name: "@fixture/jsbsim-wasm", version: "1.0.0" },
    native: { origin: native.origin, commit: native.commit, contentSha256: native.contentSha256, dirty: native.dirty },
    sdk: { path: "wasm", commit: sdk.commit, contentSha256: sdk.contentSha256, dirty: sdk.dirty },
    build: { mode: "in-tree", toolchain: { node: "fixture", npm: "fixture", emscripten: "fixture", cmake: "fixture",
      clang: "fixture", platform: "fixture", arch: "fixture", emscriptenConfigSha256: "f".repeat(64) },
      options: { buildType: "Release", cxxStandard: 17, sdkTarget: "es2022" } } };
  identity.build.inputSha256 = buildInputSha256(identity);
  const descriptor = { schemaVersion: 2, workspaceRoot: path.join(root, "workspace"), native, sdk, identity };
  const file = path.join(root, "descriptor.json");
  await writeJson(file, descriptor);
  return { root, file, descriptor };
}

describe("build consumers reject inconsistent resolved identities", () => {
  it("accepts a coherent descriptor and rejects native/SDK identity or manifest substitution", async () => {
    const { file, descriptor } = await descriptorFixture();
    assert.deepEqual(await readDescriptor(file), descriptor);
    const changes = [
      value => { value.native.commit = "c".repeat(40); },
      value => { value.native.contentSha256 = "d".repeat(64); },
      value => { value.native.dirty = true; },
      value => { value.sdk.commit = "c".repeat(40); },
      value => { value.sdk.manifest[0].sha256 = "e".repeat(64); },
      value => { value.native.manifest[0].sha256 = "e".repeat(64); },
      value => { value.native.origin = "https://github.com/another/jsbsim"; },
    ];
    for (const change of changes) {
      const invalid = structuredClone(descriptor);
      change(invalid);
      await writeJson(file, invalid);
      await assert.rejects(readDescriptor(file), /identity mismatch|manifest digest mismatch|origin mismatch|subtree/i);
    }
  });

  it("recomputes the recipe and refuses stale options, malformed manifests and old source modes", async () => {
    const { file, descriptor } = await descriptorFixture();
    const staleRecipe = structuredClone(descriptor);
    staleRecipe.identity.build.toolchain.clang = "different compiler";
    await writeJson(file, staleRecipe);
    await assert.rejects(readDescriptor(file), /recipe identity mismatch/);
    const duplicate = structuredClone(descriptor);
    duplicate.native.manifest.push(duplicate.native.manifest[0]);
    await writeJson(file, duplicate);
    await assert.rejects(readDescriptor(file), /unsorted source manifest/);
    for (const mode of ["pinned", "local"]) {
      const obsolete = structuredClone(descriptor);
      obsolete.native.mode = obsolete.identity.build.mode = mode;
      obsolete.identity.build.inputSha256 = buildInputSha256(obsolete.identity);
      await writeJson(file, obsolete);
      await assert.rejects(readDescriptor(file), /Invalid build descriptor|mode/i);
    }
    const oldSchema = structuredClone(descriptor);
    oldSchema.schemaVersion = oldSchema.identity.schemaVersion = 1;
    oldSchema.identity.build.inputSha256 = buildInputSha256(oldSchema.identity);
    await writeJson(file, oldSchema);
    await assert.rejects(readDescriptor(file), /Invalid build descriptor/i);
  });

  it("refuses a separately valid SDK identity, unrelated root or substituted subtree", async () => {
    const { file, descriptor } = await descriptorFixture();
    const changes = [
      value => { value.sdk.commit = value.identity.sdk.commit = "b".repeat(40); },
      value => { value.sdk.dirty = value.identity.sdk.dirty = true; },
      value => { value.sdk.root = path.join(value.native.root, "other-sdk"); },
      value => { value.identity.sdk.path = "other-sdk"; },
      value => { value.sdk.path = "other-sdk"; },
      value => {
        value.sdk.manifest[0].sha256 = sha256("a different SDK with its own valid digest");
        value.sdk.contentSha256 = value.identity.sdk.contentSha256 = manifestSha256(value.sdk.manifest);
      },
      value => {
        value.native.manifest.push({ path: "wasm/undeclared.ts", mode: 0o644, sha256: sha256("omitted from SDK subset") });
        value.native.contentSha256 = value.identity.native.contentSha256 = manifestSha256(value.native.manifest);
      },
    ];
    for (const change of changes) {
      const invalid = structuredClone(descriptor);
      change(invalid);
      invalid.identity.build.inputSha256 = buildInputSha256(invalid.identity);
      await writeJson(file, invalid);
      await assert.rejects(readDescriptor(file), /same repository|same snapshot|subtree|root|in-tree|identity|path/i);
    }
  });

  it("includes the identity schema and SDK location in the build recipe digest", async () => {
    const { descriptor } = await descriptorFixture();
    for (const change of [value => { value.schemaVersion = 1; }, value => { value.sdk.path = "other-sdk"; }]) {
      const changed = structuredClone(descriptor.identity);
      change(changed);
      assert.notEqual(buildInputSha256(changed), descriptor.identity.build.inputSha256);
    }
  });

  it("requires a CMake cache rooted at this snapshot with WASM enabled and the same recipe", async () => {
    const { descriptor } = await descriptorFixture();
    const cache = (root, input, enabled = "ON") =>
      `CMAKE_HOME_DIRECTORY:INTERNAL=${root}\nBUILD_WASM_MODULE:BOOL=${enabled}\nJSBSIM_BUILD_INPUT_SHA256:UNINITIALIZED=${input}\n`;
    assert.doesNotThrow(() => verifyCmakeCache(descriptor, cache(descriptor.native.root, descriptor.identity.build.inputSha256)));
    assert.throws(() => verifyCmakeCache(descriptor, cache(descriptor.sdk.root, descriptor.identity.build.inputSha256)), /different|WASM/i);
    assert.throws(() => verifyCmakeCache(descriptor, cache(path.join(descriptor.native.root, "other"), descriptor.identity.build.inputSha256)), /different|WASM/i);
    assert.throws(() => verifyCmakeCache(descriptor, cache(descriptor.native.root, "f".repeat(64))), /different|WASM/i);
    assert.throws(() => verifyCmakeCache(descriptor, cache(descriptor.native.root, descriptor.identity.build.inputSha256, "OFF")), /different|WASM/i);
    assert.throws(() => verifyCmakeCache(descriptor, ""), /different|WASM/i);
    const oldCache = `JSBSIM_SOURCE_DIR:UNINITIALIZED=${descriptor.native.root}\nJSBSIM_BUILD_INPUT_SHA256:UNINITIALIZED=${descriptor.identity.build.inputSha256}\n`;
    assert.throws(() => verifyCmakeCache(descriptor, oldCache), /different|WASM/i);
  });

  it("requires matching binding source, recipe and all generated C++/TypeScript bytes", async () => {
    const { descriptor } = await descriptorFixture();
    const files = Object.fromEntries(["generated/FGFDMExecBindings.cpp", "src/generated/fgfdmexec-api.ts", "src/generated/jsbsim-api.ts"]
      .map(file => [file, sha256(file)]));
    for (const file of Object.keys(files)) {
      await mkdir(path.dirname(path.join(descriptor.workspaceRoot, file)), { recursive: true });
      await writeFile(path.join(descriptor.workspaceRoot, file), file);
    }
    const stampFile = path.join(descriptor.workspaceRoot, "generated/bindings-manifest.json");
    const stamp = { schemaVersion: 1, inputSha256: descriptor.identity.build.inputSha256,
      nativeContentSha256: descriptor.native.contentSha256, nativeCommit: descriptor.native.commit, files };
    await writeJson(stampFile, stamp);
    assert.deepEqual(await verifyGeneratedBindings(descriptor), stamp);
    for (const key of ["inputSha256", "nativeContentSha256", "nativeCommit"]) {
      await writeJson(stampFile, { ...stamp, [key]: "different" });
      await assert.rejects(verifyGeneratedBindings(descriptor), /different source/);
    }
    const incomplete = structuredClone(stamp);
    delete incomplete.files["src/generated/jsbsim-api.ts"];
    await writeJson(stampFile, incomplete);
    await assert.rejects(verifyGeneratedBindings(descriptor), /Incomplete/);
    await writeJson(stampFile, stamp);
    await writeFile(path.join(descriptor.workspaceRoot, "src/generated/jsbsim-api.ts"), "stale wrapper");
    await assert.rejects(verifyGeneratedBindings(descriptor), /Generated bindings changed/);
  });
});

it("accepts only reviewed Emscripten distribution banners and preserves actual compiler identity", async () => {
  const lock = JSON.parse(await readFile(new URL("../build-toolchain.lock.json", import.meta.url), "utf8"));
  const actual = { node: lock.node, npm: lock.npm, cmake: lock.cmake, platform: "linux", emscripten: "" };
  for (const profile of lock.emscripten.profiles) {
    actual.emscripten = profile.banner;
    assert.doesNotThrow(() => checkToolchain(actual, lock));
    assert.equal(actual.emscripten, profile.banner);
  }
  assert.throws(() => checkToolchain({ ...actual, emscripten: actual.emscripten + "-unknown" }, lock), /Unapproved/);
  assert.throws(() => checkToolchain({ ...actual, emscripten: actual.emscripten.replace("6.0.9", "6.0.8") }, lock), /Unapproved/);
  assert.throws(() => checkToolchain({ ...actual, npm: "different" }, lock), /Toolchain mismatch/);
});

it("reuses identical immutable output bytes and refuses replacement", async () => {
  const root = await directory(), input = path.join(root, "input"), target = path.join(root, "output");
  await writeFile(input, "accepted bytes");
  await installImmutableFile(input, target);
  await installImmutableFile(input, target);
  // Use a new inode: editing a hardlinked staging file would also edit its target.
  const changed = path.join(root, "changed");
  await writeFile(changed, "different bytes");
  await assert.rejects(installImmutableFile(changed, target), /different bytes/);
  assert.equal(await readFile(target, "utf8"), "accepted bytes");
});

it("rejects mutated frozen authored inputs, unknown files and generated identity before rechecking", async () => {
  const { descriptor } = await descriptorFixture();
  await mkdir(path.join(descriptor.workspaceRoot, "test"), { recursive: true });
  await mkdir(path.join(descriptor.workspaceRoot, "src"), { recursive: true });
  const testFile = path.join(descriptor.workspaceRoot, "test/regression.mjs");
  await writeFile(testFile, "original assertions");
  const identityFile = path.join(descriptor.workspaceRoot, "src/build-identity.ts");
  await writeFile(identityFile, "authored placeholder");
  const trackedPreviews = ["generated/FGFDMExecBindings.cpp", "src/generated/fgfdmexec-api.ts",
    "src/generated/jsbsim-api.ts"];
  for (const file of trackedPreviews) {
    await mkdir(path.dirname(path.join(descriptor.workspaceRoot, file)), { recursive: true });
    await writeFile(path.join(descriptor.workspaceRoot, file), "tracked preview before generation");
  }
  descriptor.sdk.manifest = await contentManifest(descriptor.workspaceRoot);
  await writeFile(identityFile, generatedBuildIdentity(descriptor.identity));
  for (const output of ["build/compiled.wasm", "dist/index.js", "node_modules/package/index.js", "src/generated/jsbsim-api.ts", "generated/bindings-manifest.json"]) {
    await mkdir(path.dirname(path.join(descriptor.workspaceRoot, output)), { recursive: true });
    await writeFile(path.join(descriptor.workspaceRoot, output), "generated output");
  }
  for (const file of trackedPreviews) {
    await writeFile(path.join(descriptor.workspaceRoot, file), "regenerated for this snapshot");
  }
  await assert.doesNotReject(verifyWorkspaceInputs(descriptor));
  await writeFile(testFile, "weakened assertions");
  await assert.rejects(verifyWorkspaceInputs(descriptor), /authored workspace inputs changed/);
  await writeFile(testFile, "original assertions");
  const unknownFile = path.join(descriptor.workspaceRoot, "src/uncaptured.ts");
  await writeFile(unknownFile, "source added after freezing inputs");
  await assert.rejects(verifyWorkspaceInputs(descriptor), /authored workspace inputs changed/);
  await rm(unknownFile);
  const nestedGit = path.join(descriptor.workspaceRoot, ".git");
  await mkdir(nestedGit);
  await writeFile(path.join(nestedGit, "config"), "unexpected repository metadata");
  await assert.rejects(verifyWorkspaceInputs(descriptor), /Git|repository|authored workspace inputs/i);
  await rm(nestedGit, { recursive: true });
  await writeFile(identityFile, "a different build identity");
  await assert.rejects(verifyWorkspaceInputs(descriptor), /build identity changed/);
});

it("refuses ambient flags or include paths that would escape the recorded build recipe", () => {
  assert.doesNotThrow(() => rejectAmbientCompilerOverrides({ EMSDK: "/official/emsdk", EM_CONFIG: "/official/emsdk/.emscripten" }));
  assert.throws(() => rejectAmbientCompilerOverrides({ EMSDK: "/official/emsdk", EM_CONFIG: "/different/config" }), /EM_CONFIG/);
  assert.doesNotThrow(() => rejectAmbientCompilerOverrides({ CFLAGS: "", CPATH: "  ", EM_CACHE: "/cache", PATH: "/tools" }));
  for (const key of ["CFLAGS", "CXXFLAGS", "CPPFLAGS", "LDFLAGS", "EMCC_CFLAGS", "EMMAKEN_CFLAGS", "CPATH", "CPLUS_INCLUDE_PATH", "LIBRARY_PATH", "EM_CONFIG", "CMAKE_TOOLCHAIN_FILE"]) {
    assert.throws(() => rejectAmbientCompilerOverrides({ [key]: "override" }), /Compiler-affecting environment/);
  }
});
