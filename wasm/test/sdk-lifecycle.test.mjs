import assert from "node:assert/strict";
import { describe, it } from "node:test";
import { JSBSimModelLoadError, JSBSimSdk } from "../dist/index.js";

async function fixture({ load, configureError, legacy = false, deleteError } = {}) {
  const events = [];
  let output;
  let instance;
  class Exec {
    deleted = false;
    constructor() { instance = this; if (legacy) this.delete = undefined; }
    SetRootDir(value) { this.root = value; if (configureError) throw configureError; }
    SetAircraftPath(value) { this.aircraft = value; return true; }
    SetEnginePath(value) { this.engine = value; return true; }
    SetSystemsPath(value) { this.systems = value; return true; }
    SetOutputPath() { return true; }
    GetRootDir() { return this.root; }
    GetAircraftPath() { return this.aircraft; }
    GetEnginePath() { return this.engine; }
    GetSystemsPath() { return this.systems; }
    LoadModel(...args) { events.push("load"); return load ? load(output, args) : true; }
    Run() { events.push("run"); return true; }
    RunIC() { return true; }
    isDeleted() { return this.deleted; }
    delete() { events.push("exec.delete"); this.deleted = true; if (deleteError) throw deleteError; }
  }
  const moduleFactory = async options => {
    output = options;
    return {
      FGFDMExec: Exec,
      FS: { analyzePath: () => ({ exists: true }) },
      destroy(value) { assert.equal(value, instance); events.push("module.destroy"); },
    };
  };
  const sdk = await JSBSimSdk.create({ moduleFactory, log: { console: false } });
  return { sdk, events, output, instance };
}

describe("SDK-owned native lifetime", () => {
  it("deletes the executive exactly once and rejects use after destruction", async () => {
    const { sdk, events } = await fixture();
    sdk.destroy();
    sdk.destroy();
    assert.deepEqual(events, ["exec.delete"]);
    assert.equal(sdk.isDestroyed, true);
    assert.throws(() => sdk.run(), /destroyed/);
    assert.throws(() => sdk.runIc(), /destroyed/);
    assert.throws(() => sdk.loadModel("a"), /destroyed/);
    assert.throws(() => sdk.writeDataFile("a", "data"), /destroyed/);
    await assert.rejects(sdk.enablePersistence(), /destroyed/);
    await assert.rejects(sdk.syncToPersistence(), /destroyed/);
  });

  it("preserves a failed model load's boolean return", async () => {
    const { sdk, events } = await fixture({ load: () => false });
    assert.equal(sdk.loadModel("missing"), false);
    sdk.destroy();
    assert.deepEqual(events, ["load", "exec.delete"]);
  });

  it("supports the legacy module destructor only when delete is absent", async () => {
    const { sdk, events } = await fixture({ legacy: true });
    sdk.destroy();
    sdk.destroy();
    assert.deepEqual(events, ["module.destroy"]);
  });

  it("preserves destructor failures without a second deletion attempt", async () => {
    const { sdk, events } = await fixture({ deleteError: new Error("native teardown") });
    assert.throws(() => sdk.destroy(), AggregateError);
    sdk.destroy();
    assert.deepEqual(events, ["exec.delete"]);
  });

  it("stops delivering SDK logs after disposal", async () => {
    const { sdk, output } = await fixture();
    let count = 0;
    sdk.on("log", () => { count++; });
    output.print("before");
    sdk.destroy();
    output.print("after");
    assert.equal(count, 1);
    assert.throws(() => sdk.on("log", () => {}), /destroyed/);
  });

  it("frees the executive when path configuration fails during create", async () => {
    let deleted = 0;
    const failure = new Error("bad root");
    await assert.rejects(JSBSimSdk.create({
      log: { console: false },
      moduleFactory: async () => ({
        FGFDMExec: class {
          SetRootDir() { throw failure; }
          delete() { deleted++; }
        },
        FS: { analyzePath: () => ({ exists: true }) },
      }),
    }), error => error === failure);
    assert.equal(deleted, 1);
  });
});

describe("diagnostic model loads", () => {
  it("retains both native log streams, model paths, and numeric exception cause", async () => {
    const { sdk } = await fixture({ load(output) {
      output.print("Reading aircraft/c172p/c172p.xml");
      output.printErr("Could not open engine/direct.xml");
      throw 478744;
    } });
    assert.throws(() => sdk.loadModelOrThrow("c172p"), error => {
      assert.ok(error instanceof JSBSimModelLoadError);
      assert.equal(error.cause, 478744);
      assert.equal(error.model, "c172p");
      assert.equal(error.failure, "exception");
      assert.equal(error.paths.rootDir, "/runtime");
      assert.deepEqual(error.logs.map(entry => entry.stream), ["stdout", "stderr"]);
      assert.match(error.message, /direct\.xml/);
      assert.match(error.message, /478744/);
      return true;
    });
    sdk.destroy();
  });

  it("preserves the boolean API and distinguishes a false result from an exception", async () => {
    const { sdk } = await fixture({ load: () => false });
    assert.equal(sdk.loadModel("missing"), false);
    assert.throws(() => sdk.loadModelOrThrow("missing"), error => {
      assert.equal(error.failure, "returned-false");
      assert.equal(error.cause, undefined);
      return true;
    });
    sdk.destroy();
  });

  it("bounds attempt logs and excludes earlier output", async () => {
    const { sdk, output } = await fixture({ load(log) {
      for (let index = 0; index < 200; index++) log.printErr(index + ":" + "x".repeat(5000));
      return false;
    } });
    output.print("unrelated earlier output");
    assert.throws(() => sdk.loadModelOrThrow("bad"), error => {
      assert.equal(error.logs.length, 128);
      assert.ok(error.logs.every(entry => entry.message.length <= 2048 && entry.raw.length <= 2048));
      assert.ok(!error.message.includes("unrelated"));
      assert.ok(Object.isFrozen(error.logs));
      return true;
    });
    sdk.destroy();
  });

  it("supports explicit paths and restores capture state after success", async () => {
    let attempts = 0;
    const { sdk, output } = await fixture({ load(log, args) {
      attempts++;
      assert.deepEqual(args, ["air", "eng", "sys", "a", false]);
      if (attempts === 1) return true;
      log.printErr("second attempt");
      return false;
    } });
    const options = { aircraftPath: "air", enginePath: "eng", systemsPath: "sys", addModelToPath: false };
    assert.equal(sdk.loadModelOrThrow("a", options), true);
    output.print("between attempts");
    assert.throws(() => sdk.loadModelOrThrow("a", options), error => {
      assert.deepEqual(error.logs.map(entry => entry.message), ["second attempt"]);
      assert.equal(error.paths.addModelToPath, false);
      return true;
    });
    sdk.destroy();
  });
});

describe("persistence boundaries", () => {
  it("rejects equal and nested normalized roots before loading a runtime", async () => {
    for (const [runtimeRoot, idbMountPath] of [
      ["/runtime", "/runtime"], ["/runtime", "/runtime/persist"],
      ["/persist/runtime", "/persist"], ["/", "/persist"],
      ["/runtime", "/"], ["//runtime/./", "/runtime"],
      ["/runtime", "/other/../runtime"], ["relative", "relative/persist"],
    ]) {
      let calls = 0;
      await assert.rejects(JSBSimSdk.create({ runtimeRoot,
        persistence: { idbMountPath },
        moduleFactory: async () => { calls++; throw new Error("must not load"); },
      }), /disjoint/);
      assert.equal(calls, 0);
    }
  });

  it("does not confuse sibling prefixes with nested directories", async () => {
    let root;
    const sdk = await JSBSimSdk.create({ runtimeRoot: "/data/./runtime",
      persistence: { idbMountPath: "/data/runtime-cache/" },
      log: { console: false },
      moduleFactory: async () => ({
        FGFDMExec: class {
          SetRootDir(value) { root = value; }
          SetAircraftPath() {} SetEnginePath() {} SetSystemsPath() {} SetOutputPath() {}
          delete() {}
        },
        FS: { analyzePath: () => ({ exists: true }) },
      }),
    });
    assert.equal(root, "/data/runtime");
    assert.equal(sdk.vfs.idbMountPath, "/data/runtime-cache");
    sdk.destroy();
  });

  it("rejects persistence when IndexedDB or its backend is unavailable", async () => {
    const { sdk } = await fixture();
    try {
      sdk.module.FS.filesystems = { MEMFS: {} };
      await assert.rejects(sdk.enablePersistence(), /IDBFS is unavailable/);
      await assert.rejects(sdk.syncFromPersistence(), /not mounted/);
      await assert.rejects(sdk.syncToPersistence(), /not mounted/);
    } finally { sdk.destroy(); }
  });

  it("retains both executive initialization and cleanup failures", async () => {
    const failure = new Error("configuration failed");
    const cleanupFailure = new Error("delete failed");
    await assert.rejects(JSBSimSdk.create({
      log: { console: false },
      moduleFactory: async () => ({
        FGFDMExec: class {
          SetRootDir() { throw failure; }
          delete() { throw cleanupFailure; }
        },
        FS: { analyzePath: () => ({ exists: true }) },
      }),
    }), error => {
      assert.ok(error instanceof AggregateError);
      assert.equal(error.errors[0], failure);
      assert.deepEqual(error.errors[1].errors, [cleanupFailure]);
      return true;
    });
  });
});
