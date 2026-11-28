#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { mkdir, mkdtemp, readFile, rename, rm } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { contentManifest, installImmutableFile, sha256, writeJson } from "./build-system/source.mjs";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const candidate = process.argv.find(value => value.startsWith("--artifact="))?.slice(11);
const release = process.argv.includes("--release");
for (const argument of process.argv.slice(2)) {
  if (!argument.startsWith("--artifact=") && argument !== "--release") throw new Error("Unknown pack option: " + argument);
}
const artifactRoot = candidate ? path.resolve(candidate) : JSON.parse(await readFile(path.join(root, "build/last-build.json"), "utf8")).artifactRoot;
const metadata = JSON.parse(await readFile(path.join(artifactRoot, "dist/build-metadata.json"), "utf8"));
if (metadata.validation?.status !== "passed") throw new Error("Artifact has no passed build validation.");
const releaseEligible = metadata.identity.schemaVersion === 2 && metadata.identity.build.mode === "in-tree" && metadata.identity.sdk.path === "wasm" && metadata.identity.native.commit === metadata.identity.sdk.commit && !metadata.identity.native.dirty && !metadata.identity.sdk.dirty;
if (release && !releaseEligible) {
  throw new Error("Release packing requires clean native and SDK inputs from the same repository revision.");
}
const actual = Object.fromEntries((await contentManifest(path.join(artifactRoot, "dist")))
  .filter(file => file.path !== "build-metadata.json").map(file => [file.path, file.sha256]));
if (JSON.stringify(actual) !== JSON.stringify(metadata.files)) throw new Error("Completed artifact files changed.");
if (Object.keys(metadata.packageFiles ?? {}).sort().join("|") !== "LICENSE|LICENSES.md|NOTICE|README.md|package.json") throw new Error("Incomplete package-file inventory.");
for (const [file, expected] of Object.entries(metadata.packageFiles)) {
  if (sha256(await readFile(path.join(artifactRoot, file))) !== expected) throw new Error("Package source file changed: " + file);
}
const outputRoot = path.join(root, "build", "packages", path.basename(artifactRoot));
await mkdir(outputRoot, { recursive: true });
const staging = await mkdtemp(path.join(root, "build", ".pack-"));
const result = spawnSync("npm", ["pack", "--json", "--ignore-scripts", "--pack-destination", staging], { cwd: artifactRoot, encoding: "utf8" });
if (result.error) throw result.error;
if (result.status !== 0) throw new Error(result.stderr);
const pack = JSON.parse(result.stdout)[0], tarball = path.join(outputRoot, pack.filename);
if (pack.name !== metadata.identity.package.name || pack.version !== metadata.identity.package.version) throw new Error("Packed identity differs from build identity.");
await installImmutableFile(path.join(staging, pack.filename), tarball);
const report = { schemaVersion: 1, tarball, sha256: sha256(await readFile(tarball)),
  integrity: pack.integrity, package: metadata.identity.package, identity: metadata.identity, releaseEligible };
await writeJson(path.join(staging, "package-integrity.json"), report);
await installImmutableFile(path.join(staging, "package-integrity.json"), path.join(outputRoot, "package-integrity.json"));
const pointer = path.join(root, "build", "last-package.json");
await writeJson(path.join(staging, "last-package.json"), report);
await rename(path.join(staging, "last-package.json"), pointer);
await rm(staging, { recursive: true, force: true });
console.log(JSON.stringify(report, null, 2));
