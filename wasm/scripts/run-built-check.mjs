#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { readFile, readdir } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { contentManifest, gitIdentity, manifestSha256, nativeInputRoots, readDescriptor, verifyContent, verifyGeneratedBindings, verifyWorkspaceInputs } from "./build-system/source.mjs";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const operation = process.argv[2];
if (!["test", "typecheck"].includes(operation)) throw new Error("Expected test or typecheck.");
const candidate = JSON.parse(await readFile(path.join(root, "build/last-build.json"), "utf8"));
const descriptor = await readDescriptor(candidate.descriptor);
await verifyContent(descriptor.native.root, descriptor.native.manifest);
await verifyContent(descriptor.sdk.root, descriptor.sdk.manifest);
await verifyWorkspaceInputs(descriptor);
await verifyGeneratedBindings(descriptor);
const origin = descriptor.native.originRoot;
const liveIdentity = await gitIdentity(origin);
const liveManifest = await contentManifest(origin, { roots: await nativeInputRoots(origin) });
if (liveIdentity.commit !== descriptor.native.commit || liveIdentity.dirty !== descriptor.native.dirty ||
    manifestSha256(liveManifest) !== descriptor.native.contentSha256) {
  throw new Error("Repository inputs changed after the accepted build; rebuild before checking this source revision.");
}
const metadata = JSON.parse(await readFile(path.join(candidate.artifactRoot, "dist/build-metadata.json"), "utf8"));
const actual = Object.fromEntries((await contentManifest(path.join(candidate.artifactRoot, "dist")))
  .filter(file => file.path !== "build-metadata.json").map(file => [file.path, file.sha256]));
if (JSON.stringify(actual) !== JSON.stringify(metadata.files)) throw new Error("Accepted candidate files changed.");
const workspaceFiles = Object.fromEntries((await contentManifest(descriptor.distRoot))
  .filter(file => file.path !== "build-metadata.json").map(file => [file.path, file.sha256]));
if (JSON.stringify(workspaceFiles) !== JSON.stringify(metadata.files)) throw new Error("Frozen test workspace artifacts changed.");
const env = { ...process.env, JSBSIM_BUILD_DESCRIPTOR: candidate.descriptor, JSBSIM_SOURCE_ROOT: descriptor.native.root };
const extra = process.argv.slice(3);
const command = operation === "typecheck" ? path.join(descriptor.workspaceRoot, "node_modules/.bin/tsc") : process.execPath;
const defaultTests = (await readdir(path.join(descriptor.workspaceRoot, "test"))).filter(name => name.endsWith(".test.mjs")).sort().map(name => "test/" + name);
const args = operation === "typecheck" ? ["--noEmit", ...extra] :
  ["--test", ...(extra.length ? extra : defaultTests)];
const result = spawnSync(command, args, { cwd: descriptor.workspaceRoot, env, stdio: "inherit" });
if (result.error) throw result.error;
process.exitCode = result.status ?? 1;
