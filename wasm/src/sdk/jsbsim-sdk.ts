import type { FGFDMExecApi } from "../generated/fgfdmexec-api";
import { JSBSimApi } from "../generated/jsbsim-api";
import type { BinaryLike, JSBSimLogEntry, JSBSimRuntimeModule, JSBSimSdkOptions } from "./types";
import { loadJSBSimModule } from "./load-module";
import { WasmVfsManager } from "./vfs";
import { JSBSimModelLoadError } from "./model-load-error";

export interface ConfigurePathsOptions {
  rootDir?: string;
  aircraftPath?: string;
  enginePath?: string;
  systemsPath?: string;
  outputPath?: string;
}

export interface LoadModelOptions {
  aircraftPath?: string;
  enginePath?: string;
  systemsPath?: string;
  addModelToPath?: boolean;
}

const DEFAULT_RUNTIME_ROOT = "/runtime";
const DEFAULT_IDB_ROOT = "/persist";

export type JSBSimSdkLogEvent = "stdout" | "stderr" | "log";
export type JSBSimSdkLogListener = (entry: JSBSimLogEntry) => void;

export class JSBSimSdk extends JSBSimApi {
  readonly module: JSBSimRuntimeModule;
  readonly vfs: WasmVfsManager;
  private readonly logListeners: Record<JSBSimSdkLogEvent, Set<JSBSimSdkLogListener>>;
  private destroyed = false;
  private modelLoadLog: JSBSimLogEntry[] | null = null;
  private stopForwardingLogs: () => void = () => {};

  private constructor(module: JSBSimRuntimeModule, exec: FGFDMExecApi, vfs: WasmVfsManager) {
    super(exec);
    this.module = module;
    this.vfs = vfs;
    this.logListeners = {
      stdout: new Set(),
      stderr: new Set(),
      log: new Set(),
    };
  }

  /**
   * Loads the JSBSim runtime module, creates `FGFDMExec`, and initializes VFS.
   */
  static async create(options: JSBSimSdkOptions = {}): Promise<JSBSimSdk> {
    const { runtimeRoot, idbMountPath } = WasmVfsManager.resolveRoots(
      options.runtimeRoot ?? DEFAULT_RUNTIME_ROOT,
      options.persistence?.idbMountPath ?? DEFAULT_IDB_ROOT,
    );
    const originalLog = options.log;
    let bufferedLogEntries: JSBSimLogEntry[] = [];
    let emitSdkLog: ((entry: JSBSimLogEntry) => void) | null = null;

    const forwardLogEntry = (entry: JSBSimLogEntry): void => {
      if (emitSdkLog) {
        emitSdkLog(entry);
        return;
      }

      bufferedLogEntries.push(entry);
    };

    const module = await loadJSBSimModule({
      ...options,
      log: {
        ...(originalLog ?? {}),
        onLog: (entry) => {
          originalLog?.onLog?.(entry);
          forwardLogEntry(entry);
        },
      },
    });

    const vfs = new WasmVfsManager(module, runtimeRoot, idbMountPath);
    if (options.persistence?.enabled) {
      await vfs.enablePersistence();
    }

    const exec = new module.FGFDMExec();
    const sdk = new JSBSimSdk(module, exec, vfs);
    emitSdkLog = (entry) => sdk.emitLogEntry(entry);
    sdk.stopForwardingLogs = () => {
      emitSdkLog = () => {};
      bufferedLogEntries = [];
    };
    try {
      for (const entry of bufferedLogEntries) emitSdkLog(entry);
      bufferedLogEntries = [];
      sdk.configurePaths();
      return sdk;
    } catch (cause) {
      try {
        sdk.destroy();
      } catch (cleanupError) {
        throw new AggregateError([cause, cleanupError], "JSBSim SDK initialization and cleanup failed.");
      }
      throw cause;
    }
  }

  /**
   * Registers a handler for JSBSim log output events.
   */
  on(event: JSBSimSdkLogEvent, listener: JSBSimSdkLogListener): this {
    this.requireAlive();
    this.logListeners[event].add(listener);
    return this;
  }

  /**
   * Removes a previously registered log handler.
   */
  off(event: JSBSimSdkLogEvent, listener: JSBSimSdkLogListener): this {
    this.logListeners[event].delete(listener);
    return this;
  }

  /**
   * Registers a one-time handler for a log output event.
   */
  once(event: JSBSimSdkLogEvent, listener: JSBSimSdkLogListener): this {
    const wrapper: JSBSimSdkLogListener = (entry) => {
      this.off(event, wrapper);
      listener(entry);
    };

    return this.on(event, wrapper);
  }

  private emitLogEntry(entry: JSBSimLogEntry): void {
    if (this.destroyed) return;
    if (this.modelLoadLog) {
      this.modelLoadLog.push({ ...entry, message: entry.message.slice(-2048), raw: entry.raw.slice(-2048) });
      if (this.modelLoadLog.length > 128) this.modelLoadLog.shift();
    }
    this.emitLogEvent(entry.stream, entry);
    this.emitLogEvent("log", entry);
  }

  private emitLogEvent(event: JSBSimSdkLogEvent, entry: JSBSimLogEntry): void {
    for (const listener of this.logListeners[event]) {
      listener(entry);
    }
  }

  /**
   * Sets standard JSBSim runtime directories on `FGFDMExec`.
   */
  configurePaths(options: ConfigurePathsOptions = {}): void {
    this.requireAlive();
    const rootDir = options.rootDir ?? this.vfs.runtimeRoot;
    const aircraftPath = options.aircraftPath ?? "aircraft";
    const enginePath = options.enginePath ?? "engine";
    const systemsPath = options.systemsPath ?? "systems";
    const outputPath = options.outputPath ?? "output";

    this.setRootDir(rootDir);
    this.setAircraftPath(aircraftPath);
    this.setEnginePath(enginePath);
    this.setSystemsPath(systemsPath);
    this.setOutputPath(outputPath);
  }

  /** Loads a model while checking executive lifetime and overload arguments. */
  override loadModel(model: string, addModelToPath?: boolean): boolean;
  override loadModel(aircraftPath: string, enginePath: string, systemsPath: string, model: string, addModelToPath?: boolean): boolean;
  override loadModel(first: string, second?: string | boolean, systemsPath?: string, model?: string, addModelToPath = true): boolean {
    this.requireAlive();
    if (typeof second === "string") {
      if (systemsPath === undefined || model === undefined) throw new TypeError("Model path overload requires all three paths and a model name.");
      return this.exec.LoadModel(first, second, systemsPath, model, addModelToPath);
    }
    return this.exec.LoadModel(first, second ?? true);
  }

  override run(): boolean {
    this.requireAlive();
    return super.run();
  }

  override runIc(): boolean {
    this.requireAlive();
    return super.runIc();
  }

  /**
   * Loads an aircraft model using optional path overrides.
   */
  loadModelWithOptions(model: string, options: LoadModelOptions = {}): boolean {
    const addModelToPath = options.addModelToPath ?? true;

    if (options.aircraftPath || options.enginePath || options.systemsPath) {
      return this.loadModel(
        options.aircraftPath ?? "aircraft",
        options.enginePath ?? "engine",
        options.systemsPath ?? "systems",
        model,
        addModelToPath,
      );
    }

    return this.loadModel(model, addModelToPath);
  }

  /**
   * Loads a model or throws an error containing paths, bounded native logs,
   * and the original thrown value (including numeric Wasm exceptions).
   * The existing boolean-returning load APIs retain their behavior.
   */
  loadModelOrThrow(model: string, options: LoadModelOptions = {}): true {
    this.requireAlive();
    if (this.modelLoadLog) throw new Error("A diagnostic model load is already in progress.");
    const paths = {
      rootDir: this.getRootDir(),
      aircraftPath: options.aircraftPath ?? this.getAircraftPath(),
      enginePath: options.enginePath ?? this.getEnginePath(),
      systemsPath: options.systemsPath ?? this.getSystemsPath(),
      addModelToPath: options.addModelToPath ?? true,
    };
    const logs: JSBSimLogEntry[] = [];
    this.modelLoadLog = logs;
    try {
      let loaded: boolean;
      try {
        loaded = this.loadModelWithOptions(model, options);
      } catch (cause) {
        throw new JSBSimModelLoadError(model, paths, logs, "exception", cause);
      }
      if (!loaded) throw new JSBSimModelLoadError(model, paths, logs, "returned-false");
      return true;
    } finally {
      this.modelLoadLog = null;
    }
  }

  /**
   * Loads a script with JSBSim defaults for optional arguments.
   */
  override loadScript(path: string, deltaT = 0, initFile = ""): boolean {
    this.requireAlive();
    return super.loadScript(path, deltaT, initFile);
  }

  loadScriptWithDefaults(path: string, deltaT = 0, initFile = ""): boolean {
    return this.loadScript(path, deltaT, initFile);
  }

  /**
   * Writes data to MEMFS (relative to runtime root) and returns resolved path.
   */
  writeDataFile(path: string, data: BinaryLike): string {
    this.requireAlive();
    return this.vfs.writeRuntimeFile(path, data);
  }

  /**
   * Reads data from MEMFS (relative to runtime root).
   */
  readDataFile(path: string, encoding: "utf8" | "binary" = "utf8"): string | Uint8Array {
    this.requireAlive();
    return this.vfs.readRuntimeFile(path, encoding);
  }

  /**
   * Creates a runtime directory and returns resolved path.
   */
  mkdir(path: string): string {
    this.requireAlive();
    return this.vfs.mkdirRuntime(path);
  }

  /**
   * Synchronizes IDBFS -> MEMFS when persistence is enabled.
   */
  async syncFromPersistence(): Promise<void> {
    this.requireAlive();
    await this.vfs.syncFromPersistence();
  }

  /**
   * Synchronizes MEMFS -> IDBFS when persistence is enabled.
   */
  async syncToPersistence(): Promise<void> {
    this.requireAlive();
    await this.vfs.syncToPersistence();
  }

  /**
   * Mounts IDBFS and performs an initial pull.
   */
  async enablePersistence(): Promise<void> {
    this.requireAlive();
    await this.vfs.enablePersistence();
  }

  get isDestroyed(): boolean {
    return this.destroyed;
  }

  private requireAlive(): void {
    if (this.destroyed) throw new Error("JSBSimSdk has been destroyed.");
  }

  /** Frees SDK-owned native objects once, even after partial initialization. */
  destroy(): void {
    if (this.destroyed) return;
    this.destroyed = true;
    this.stopForwardingLogs();
    this.logListeners.stdout.clear();
    this.logListeners.stderr.clear();
    this.logListeners.log.clear();
    this.modelLoadLog = null;
    const errors: unknown[] = [];
    try {
      // Embind lifetime methods are not part of the generated C++ API.
      const native = this.exec as FGFDMExecApi & { delete?: () => void; isDeleted?: () => boolean };
      if (!native.isDeleted?.()) {
        if (typeof native.delete === "function") native.delete();
        else if (this.module.destroy) this.module.destroy(this.exec);
        else throw new Error("The JSBSim runtime exposes no native executive destructor.");
      }
    } catch (error) { errors.push(error); }
    if (errors.length) throw new AggregateError(errors, "Failed to destroy JSBSim SDK.");
  }
}
