// Run by the build orchestrator against the frozen native fixtures and this attempt's dist.
import assert from "node:assert/strict";
import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";
import { describe, it } from "node:test";
import { JSBSimModelLoadError, JSBSimSdk } from "../dist/index.js";
import { wasmBinaryUrl, wasmModuleUrl } from "../dist/wasm.js";

const nativeRoot = process.env.JSBSIM_SOURCE_ROOT;
assert.ok(nativeRoot, "Set JSBSIM_SOURCE_ROOT from the verified build descriptor; no vendor fallback is permitted.");

function copyXml(sdk, from, to) {
  for (const entry of readdirSync(from, { withFileTypes: true })) {
    const input = path.join(from, entry.name);
    const output = `${to}/${entry.name}`;
    if (entry.isDirectory()) copyXml(sdk, input, output);
    else if (entry.name.endsWith(".xml")) sdk.writeDataFile(output, readFileSync(input));
  }
}

async function createC172(log = { console: false }) {
  const sdk = await JSBSimSdk.create({ moduleUrl: wasmModuleUrl, wasmUrl: wasmBinaryUrl, log });
  try {
    copyXml(sdk, path.join(nativeRoot, "aircraft/c172p"), "aircraft/c172p");
    copyXml(sdk, path.join(nativeRoot, "engine"), "engine");
    copyXml(sdk, path.join(nativeRoot, "systems"), "systems");
    assert.equal(sdk.loadModel("c172p"), true);
    sdk.setPropertyValue("ic/h-sl-ft", 3000);
    sdk.setPropertyValue("ic/vc-kts", 90);
    assert.equal(sdk.runIc(), true);
    return sdk;
  } catch (cause) {
    sdk.destroy();
    throw cause;
  }
}

describe("real WASM lifetime and diagnostics", () => {
  it("deletes the owned executive once across repeated lifecycles", async () => {
    for (let cycle = 0; cycle < 3; cycle++) {
      const sdk = await createC172();
      assert.ok(Number.isFinite(sdk.getPropertyValue("velocities/u-fps")));
      const originalDelete = sdk.exec.delete.bind(sdk.exec);
      let deletes = 0;
      sdk.exec.delete = () => { deletes++; originalDelete(); };
      sdk.destroy();
      sdk.destroy();
      assert.equal(deletes, 1);
      assert.equal(sdk.exec.isDeleted(), true);
      assert.throws(() => sdk.run(), /destroyed/);
    }
  });

  it("recovers after successful and failed model replacements", async () => {
    const sdk = await createC172();
    try {
      for (const model of ["c172p", "missing-aircraft", "c172p", "missing-aircraft"]) {
        assert.equal(sdk.loadModel(model), model === "c172p");
        if (model === "c172p") {
          assert.equal(sdk.runIc(), true);
          assert.equal(sdk.run(), true);
          assert.ok(Number.isFinite(sdk.getPropertyValue("velocities/u-fps")));
        }
      }
      assert.equal(sdk.loadModel("c172p"), true);
      assert.equal(sdk.runIc(), true);
      assert.equal(sdk.run(), true);
    } finally { sdk.destroy(); }
    assert.equal(sdk.exec.isDeleted(), true);
  });

  it("preserves native exception handling when invalid propulsion rejects a load", async () => {
    const sdk = await createC172();
    try {
      const original = readFileSync(path.join(nativeRoot, "aircraft/c172p/c172p.xml"), "utf8");
      // FGPropulsion::Load throws XMLLogException for this missing element,
      // then catches it in the native implementation and returns false.
      // A link-only exception flag cannot restore that catch if the native
      // object files were compiled with Emscripten's default catch disabling.
      const invalid = original.replace(/<thruster\b[^>]*>[\s\S]*?<\/thruster>/, "");
      assert.notEqual(invalid, original, "The fixture must contain the removed thruster");
      sdk.writeDataFile("aircraft/missing-thruster/missing-thruster.xml", invalid);
      assert.equal(sdk.loadModel("missing-thruster"), false);
      assert.equal(sdk.loadModel("c172p"), true);
      assert.equal(sdk.runIc(), true);
      assert.equal(sdk.run(), true);
      assert.ok(Number.isFinite(sdk.getPropertyValue("velocities/u-fps")));
    } finally { sdk.destroy(); }
    assert.equal(sdk.exec.isDeleted(), true);
  });

  it("reports script-loading failure through the public entry point", async () => {
    const sdk = await createC172();
    try { assert.equal(sdk.loadScript("/runtime/missing-script.xml"), false); }
    finally { sdk.destroy(); }
    assert.equal(sdk.exec.isDeleted(), true);
  });

  it("frees a real executive when initialization fails after allocation", async () => {
    const factory = (await import(wasmModuleUrl)).default;
    const failure = new Error("injected path configuration failure");
    let allocated;
    await assert.rejects(JSBSimSdk.create({
      wasmUrl: wasmBinaryUrl,
      log: { console: false },
      moduleFactory: async options => {
        const module = await factory(options);
        const Executive = module.FGFDMExec;
        return {
          ...module,
          FGFDMExec: function () {
            allocated = new Executive();
            allocated.SetRootDir = () => { throw failure; };
            return allocated;
          },
        };
      },
    }), error => error === failure);
    assert.ok(allocated);
    assert.equal(allocated.isDeleted(), true);
  });

  it("reports attempt-specific native errors with normalized SDK messages", async () => {
    const logs = [];
    const sdk = await createC172({ console: false, onLog: entry => logs.push(entry) });
    try {
      for (const model of ["first-missing-aircraft", "second-missing-aircraft"]) {
        assert.throws(() => sdk.loadModelOrThrow(model), error => {
          assert.ok(error instanceof JSBSimModelLoadError);
          assert.equal(error.model, model);
          assert.ok(error.logs.length > 0);
          assert.ok(error.logs.every(entry => entry.raw.length <= 2048));
          if (model.startsWith("second")) assert.ok(!error.logs.some(entry => entry.raw.includes("first-missing-aircraft")));
          return true;
        });
      }
      assert.equal(sdk.loadModelOrThrow("c172p"), true);
      assert.ok(logs.length > 0, "exercise native logging rather than an empty trace");
      assert.ok(logs.every(entry => !entry.message.includes("\u001b[")), "SDK messages strip terminal escapes by default");
    } finally { sdk.destroy(); }
  });
});
