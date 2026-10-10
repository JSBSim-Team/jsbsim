import type {
  JSBSimLogEntry,
  JSBSimLogOptions,
  JSBSimLogStream,
  JSBSimModuleFactory,
  JSBSimRuntimeModule,
  LoadJSBSimModuleOptions,
} from "./types";

const ANSI_ESCAPE_PATTERN = /\u001b\[[0-?]*[ -/]*[@-~]/g;

function toHref(value: string | URL): string {
  if (value instanceof URL) {
    return value.href;
  }
  return value;
}

function stringifyLogArg(value: unknown): string {
  if (typeof value === "string") {
    return value;
  }

  if (value === null || value === undefined) {
    return String(value);
  }

  if (typeof value === "number" || typeof value === "boolean" || typeof value === "bigint") {
    return String(value);
  }

  try {
    return JSON.stringify(value);
  } catch {
    return String(value);
  }
}

function normalizeLogText(raw: string, options: JSBSimLogOptions): string {
  if (options.stripAnsi ?? true) {
    return raw.replace(ANSI_ESCAPE_PATTERN, "");
  }

  return raw;
}

function emitLog(stream: JSBSimLogStream, args: unknown[], options: JSBSimLogOptions = {}): void {
  const raw = args.map(stringifyLogArg).join(" ");
  const message = normalizeLogText(raw, options);

  if (message === "") {
    return;
  }

  const entry: JSBSimLogEntry = {
    stream,
    message,
    raw,
    timestamp: Date.now(),
  };

  if (options.console ?? true) {
    if (stream === "stderr") {
      console.error(message);
    } else {
      console.log(message);
    }
  }

  if (stream === "stdout") {
    options.onStdout?.(entry);
  } else {
    options.onStderr?.(entry);
  }

  options.onLog?.(entry);
}

async function resolveFactory(options: LoadJSBSimModuleOptions): Promise<JSBSimModuleFactory> {
  if (options.moduleFactory) {
    return options.moduleFactory;
  }

  const moduleUrl = options.moduleUrl ?? "/wasm/jsbsim_wasm.mjs";
  const loaded = await import(/* @vite-ignore */ toHref(moduleUrl));

  if (typeof loaded.default !== "function") {
    throw new Error(
      `Expected a default Emscripten module factory export from ${moduleUrl.toString()}`,
    );
  }

  return loaded.default as JSBSimModuleFactory;
}

export async function loadJSBSimModule(
  options: LoadJSBSimModuleOptions = {},
): Promise<JSBSimRuntimeModule> {
  const moduleFactory = await resolveFactory(options);

  const locateFile = (path: string, prefix: string): string => {
    if (options.wasmUrl && path.endsWith(".wasm")) {
      return toHref(options.wasmUrl);
    }

    if (options.locateFile) {
      return options.locateFile(path, prefix);
    }

    return `${prefix}${path}`;
  };

  return moduleFactory({
    locateFile,
    print: (...args: unknown[]) => {
      emitLog("stdout", args, options.log);
    },
    printErr: (...args: unknown[]) => {
      emitLog("stderr", args, options.log);
    },
  });
}
