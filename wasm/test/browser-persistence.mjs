#!/usr/bin/env node
// Explicit post-build browser check; no server, app checkout or external fixtures.
import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { chromium } from "playwright";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
if (args.some(arg => !arg.startsWith("--artifact=")) || args.length > 1) {
  throw new Error("Usage: node test/browser-persistence.mjs [--artifact=/absolute/artifact/directory]");
}
const artifactRoot = args.length ? path.resolve(args[0].slice("--artifact=".length)) :
  JSON.parse(await readFile(path.join(root, "build/last-build.json"), "utf8")).artifactRoot;
const metadata = JSON.parse(await readFile(path.join(artifactRoot, "dist/build-metadata.json"), "utf8"));
assert.equal(metadata.validation?.status, "passed", "Only exercise an accepted build artifact");
const files = new Map();
for (const [name, expected] of Object.entries(metadata.files)) {
  assert.ok(!name.startsWith("/") && !name.split("/").includes("..") && !name.includes("\\"));
  const body = await readFile(path.join(artifactRoot, "dist", name));
  assert.equal(createHash("sha256").update(body).digest("hex"), expected, `Artifact changed: ${name}`);
  files.set(`/dist/${name}`, body);
}

const origin = "http://jsbsim-wasm.test";
const requested = new Set();
const pageErrors = [];
const browser = await chromium.launch({ headless: true });
let identity;
try {
  const context = await browser.newContext({ serviceWorkers: "block" });
  await context.route("**/*", async route => {
    const url = new URL(route.request().url());
    if (url.origin !== origin) return route.abort();
    if (url.pathname === "/") return route.fulfill({ contentType: "text/html", body: "<!doctype html><title>JSBSim persistence check</title>" });
    const body = files.get(url.pathname);
    if (!body) return route.fulfill({ status: 404, body: "Missing artifact" });
    requested.add(url.pathname);
    const contentType = url.pathname.endsWith(".wasm") ? "application/wasm" :
      url.pathname.endsWith(".json") ? "application/json" : "text/javascript";
    await route.fulfill({ contentType, body });
  });
  const page = await context.newPage();
  page.on("pageerror", error => pageErrors.push(error.message));
  await page.goto(origin);
  for (const phase of ["write", "restore-and-delete", "verify-deletion"]) {
    if (phase !== "write") await page.reload();
    const result = await page.evaluate(async phase => {
      const { JSBSimSdk, buildIdentity } = await import("/dist/index.js");
      const { wasmModuleUrl, wasmBinaryUrl } = await import("/dist/wasm.js");
      const sdk = await JSBSimSdk.create({ moduleUrl: wasmModuleUrl, wasmUrl: wasmBinaryUrl,
        persistence: { enabled: true }, log: { console: false } });
      const check = (value, message) => { if (!value) throw new Error(message); };
      try {
        check(sdk.vfs.hasIdbfsSupport(), "Built runtime does not provide IDBFS");
        if (phase === "write") {
          sdk.writeDataFile("nested/text.txt", "persistent flight data\n");
          sdk.writeDataFile("nested/bytes.bin", new Uint8Array([0, 1, 127, 128, 255]));
          await sdk.syncToPersistence();
        } else if (phase === "restore-and-delete") {
          check(sdk.readDataFile("nested/text.txt") === "persistent flight data\n", "Text was not restored after navigation");
          check(Array.from(sdk.readDataFile("nested/bytes.bin", "binary")).join(",") === "0,1,127,128,255", "Binary data was not restored");
          sdk.vfs.fs.unlink(sdk.vfs.resolveRuntimePath("nested/text.txt"));
          sdk.writeDataFile("replacement.txt", "second snapshot");
          await sdk.syncToPersistence();
        } else {
          check(!sdk.vfs.fs.analyzePath(sdk.vfs.resolveRuntimePath("nested/text.txt")).exists, "Deleted file was restored");
          check(sdk.readDataFile("replacement.txt") === "second snapshot", "Updated snapshot was not restored");
          check(Array.from(sdk.readDataFile("nested/bytes.bin", "binary")).join(",") === "0,1,127,128,255", "Unchanged binary was lost");
        }
      } finally { sdk.destroy(); }
      check(sdk.exec.isDeleted(), "SDK left its native executive allocated");
      return { phase, identity: buildIdentity };
    }, phase);
    assert.equal(result.phase, phase);
    assert.deepEqual(result.identity, metadata.identity, "Browser loaded a different SDK identity");
    identity = result.identity;
  }
  assert.ok(requested.has("/dist/wasm/jsbsim_wasm.mjs"));
  assert.ok(requested.has("/dist/wasm/jsbsim_wasm.wasm"));
  assert.deepEqual(pageErrors, [], "Unexpected browser runtime errors");
  console.log(JSON.stringify({ status: "passed", browser: browser.version(), identity,
    checks: ["IDBFS linked", "text and binary survive navigation", "deletions and updates persist", "executive disposed after each phase"],
    loadedFiles: [...requested].sort().map(name => ({ path: name.slice(6), sha256: metadata.files[name.slice(6)] })),
  }, null, 2));
} finally { await browser.close(); }
