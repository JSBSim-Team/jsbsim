import type { FGFDMExecApi } from "../generated/fgfdmexec-api";

export type BinaryLike = Uint8Array | ArrayBuffer | string;

export interface EmscriptenFileSystems {
  MEMFS: unknown;
  IDBFS?: unknown;
}

export interface EmscriptenFsStats {
  mode: number;
}

export interface EmscriptenFs {
  filesystems: EmscriptenFileSystems;
  mkdir(path: string): void;
  mkdirTree(path: string): void;
  mount(type: unknown, opts: Record<string, unknown>, mountpoint: string): void;
  syncfs(populate: boolean, cb: (error?: Error | null) => void): void;
  readdir(path: string): string[];
  stat(path: string): EmscriptenFsStats;
  isDir(mode: number): boolean;
  isFile(mode: number): boolean;
  readFile(path: string, opts?: { encoding?: "utf8" | "binary" }): string | Uint8Array;
  writeFile(path: string, data: BinaryLike): void;
  analyzePath(path: string): { exists: boolean };
  unlink(path: string): void;
  rmdir(path: string): void;
}

export interface JSBSimRuntimeModule {
  FGFDMExec: new () => FGFDMExecApi;
  FS: EmscriptenFs;
  destroy?(value: unknown): void;
}

export type JSBSimModuleFactory = (options?: Record<string, unknown>) => Promise<JSBSimRuntimeModule>;

export type JSBSimLogStream = "stdout" | "stderr";

export interface JSBSimLogEntry {
  stream: JSBSimLogStream;
  message: string;
  raw: string;
  timestamp: number;
}

export type JSBSimLogHandler = (entry: JSBSimLogEntry) => void;

export interface JSBSimLogOptions {
  console?: boolean;
  stripAnsi?: boolean;
  onStdout?: JSBSimLogHandler;
  onStderr?: JSBSimLogHandler;
  onLog?: JSBSimLogHandler;
}

export interface LoadJSBSimModuleOptions {
  moduleFactory?: JSBSimModuleFactory;
  moduleUrl?: string | URL;
  wasmUrl?: string | URL;
  locateFile?: (path: string, prefix: string) => string;
  log?: JSBSimLogOptions;
}

export interface PersistenceOptions {
  enabled?: boolean;
  idbMountPath?: string;
}

export interface JSBSimSdkOptions extends LoadJSBSimModuleOptions {
  runtimeRoot?: string;
  persistence?: PersistenceOptions;
}
