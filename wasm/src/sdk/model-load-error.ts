import type { JSBSimLogEntry } from "./types";

export interface ModelLoadPaths {
  rootDir: string;
  aircraftPath: string;
  enginePath: string;
  systemsPath: string;
  addModelToPath: boolean;
}

/** An individual load attempt, not an unbounded history of runtime output. */
export class JSBSimModelLoadError extends Error {
  readonly model: string;
  readonly paths: Readonly<ModelLoadPaths>;
  readonly logs: readonly Readonly<JSBSimLogEntry>[];
  readonly failure: "returned-false" | "exception";

  constructor(model: string, paths: ModelLoadPaths, logs: readonly JSBSimLogEntry[], failure: "returned-false" | "exception", cause?: unknown) {
    const detail = logs.slice(-8).map(entry => `[${entry.stream}] ${entry.message}`).join("\n");
    const reason = failure === "returned-false" ? "LoadModel returned false" :
      cause instanceof Error ? cause.message : String(cause);
    super(`Failed to load JSBSim model ${JSON.stringify(model)} (${reason}). Root: ${paths.rootDir}; aircraft: ${paths.aircraftPath}; engine: ${paths.enginePath}; systems: ${paths.systemsPath}.${detail ? `\n${detail}` : ""}`,
      failure === "exception" ? { cause } : undefined);
    this.name = "JSBSimModelLoadError";
    this.model = model;
    this.paths = Object.freeze({ ...paths });
    this.logs = Object.freeze(logs.map(entry => Object.freeze({ ...entry })));
    this.failure = failure;
  }
}
