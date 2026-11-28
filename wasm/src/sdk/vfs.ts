import type { BinaryLike, EmscriptenFs, JSBSimRuntimeModule } from "./types";

const ROOT_SEPARATOR = "/";

function normalizePath(path: string): string {
  if (path.includes("\0")) throw new TypeError("Virtual filesystem paths cannot contain NUL.");
  const parts: string[] = [];
  for (const part of path.split(ROOT_SEPARATOR)) {
    if (!part || part === ".") continue;
    if (part === "..") parts.pop();
    else parts.push(part);
  }
  return `${ROOT_SEPARATOR}${parts.join(ROOT_SEPARATOR)}`;
}

function joinPath(...parts: string[]): string {
  return normalizePath(parts.join(ROOT_SEPARATOR));
}

function ensureDir(fs: EmscriptenFs, path: string): void {
  const normalized = normalizePath(path);
  if (normalized === ROOT_SEPARATOR) {
    return;
  }

  const segments = normalized.split(ROOT_SEPARATOR).filter(Boolean);
  let current = "";
  for (const segment of segments) {
    current = `${current}${ROOT_SEPARATOR}${segment}`;
    if (!fs.analyzePath(current).exists) {
      fs.mkdir(current);
    }
  }
}

function listChildren(fs: EmscriptenFs, path: string): string[] {
  return fs.readdir(path).filter((entry) => entry !== "." && entry !== "..");
}

function removeTree(fs: EmscriptenFs, targetPath: string): void {
  if (!fs.analyzePath(targetPath).exists) {
    return;
  }

  const stat = fs.stat(targetPath);
  if (fs.isFile(stat.mode)) {
    fs.unlink(targetPath);
    return;
  }

  for (const child of listChildren(fs, targetPath)) {
    removeTree(fs, joinPath(targetPath, child));
  }

  fs.rmdir(targetPath);
}

function copyTree(fs: EmscriptenFs, sourcePath: string, destinationPath: string): void {
  const sourceStat = fs.stat(sourcePath);
  if (fs.isFile(sourceStat.mode)) {
    ensureDir(fs, destinationPath.slice(0, destinationPath.lastIndexOf(ROOT_SEPARATOR)) || ROOT_SEPARATOR);
    const data = fs.readFile(sourcePath, { encoding: "binary" }) as Uint8Array;
    fs.writeFile(destinationPath, data);
    return;
  }

  ensureDir(fs, destinationPath);
  for (const child of listChildren(fs, sourcePath)) {
    copyTree(fs, joinPath(sourcePath, child), joinPath(destinationPath, child));
  }
}

function syncFs(fs: EmscriptenFs, populate: boolean): Promise<void> {
  return new Promise((resolve, reject) => {
    fs.syncfs(populate, (error) => {
      if (error) {
        reject(error);
        return;
      }
      resolve();
    });
  });
}

export class WasmVfsManager {
  readonly fs: EmscriptenFs;
  readonly runtimeRoot: string;
  readonly idbMountPath: string;

  private idbMounted = false;

  /** Persistence copies complete trees, so their normalized roots must be disjoint. */
  static resolveRoots(runtimeRoot: string, idbMountPath: string): { runtimeRoot: string; idbMountPath: string } {
    runtimeRoot = normalizePath(runtimeRoot);
    idbMountPath = normalizePath(idbMountPath);
    const contains = (parent: string, child: string): boolean =>
      parent === ROOT_SEPARATOR || child === parent || child.startsWith(parent + ROOT_SEPARATOR);
    if (contains(runtimeRoot, idbMountPath) || contains(idbMountPath, runtimeRoot)) {
      throw new RangeError("Runtime and persistence roots must be disjoint directories.");
    }
    return { runtimeRoot, idbMountPath };
  }

  constructor(module: JSBSimRuntimeModule, runtimeRoot: string, idbMountPath: string) {
    const roots = WasmVfsManager.resolveRoots(runtimeRoot, idbMountPath);
    this.fs = module.FS;
    this.runtimeRoot = roots.runtimeRoot;
    this.idbMountPath = roots.idbMountPath;

    ensureDir(this.fs, this.runtimeRoot);
  }

  hasIdbfsSupport(): boolean {
    return typeof indexedDB !== "undefined" && Boolean(this.fs.filesystems.IDBFS);
  }

  async enablePersistence(): Promise<void> {
    if (!this.hasIdbfsSupport()) {
      throw new Error("IDBFS is unavailable in this environment.");
    }

    if (!this.idbMounted) {
      ensureDir(this.fs, this.idbMountPath);
      this.fs.mount(this.fs.filesystems.IDBFS as object, {}, this.idbMountPath);
      this.idbMounted = true;
    }

    await this.syncFromPersistence();
  }

  resolveRuntimePath(path: string): string {
    return path.startsWith(ROOT_SEPARATOR) ? normalizePath(path) : joinPath(this.runtimeRoot, path);
  }

  resolvePersistencePath(path: string): string {
    return path.startsWith(ROOT_SEPARATOR) ? normalizePath(path) : joinPath(this.idbMountPath, path);
  }

  writeRuntimeFile(path: string, data: BinaryLike): string {
    const fullPath = this.resolveRuntimePath(path);
    const dir = fullPath.slice(0, fullPath.lastIndexOf(ROOT_SEPARATOR)) || ROOT_SEPARATOR;
    ensureDir(this.fs, dir);
    this.fs.writeFile(fullPath, data);
    return fullPath;
  }

  readRuntimeFile(path: string, encoding: "utf8" | "binary" = "utf8"): string | Uint8Array {
    return this.fs.readFile(this.resolveRuntimePath(path), { encoding });
  }

  mkdirRuntime(path: string): string {
    const fullPath = this.resolveRuntimePath(path);
    ensureDir(this.fs, fullPath);
    return fullPath;
  }

  async syncFromPersistence(): Promise<void> {
    if (!this.idbMounted) {
      throw new Error("IDBFS is not mounted.");
    }

    await syncFs(this.fs, true);

    for (const entry of listChildren(this.fs, this.runtimeRoot)) {
      removeTree(this.fs, joinPath(this.runtimeRoot, entry));
    }

    for (const entry of listChildren(this.fs, this.idbMountPath)) {
      copyTree(this.fs, joinPath(this.idbMountPath, entry), joinPath(this.runtimeRoot, entry));
    }
  }

  async syncToPersistence(): Promise<void> {
    if (!this.idbMounted) {
      throw new Error("IDBFS is not mounted.");
    }

    for (const entry of listChildren(this.fs, this.idbMountPath)) {
      removeTree(this.fs, joinPath(this.idbMountPath, entry));
    }

    for (const entry of listChildren(this.fs, this.runtimeRoot)) {
      copyTree(this.fs, joinPath(this.runtimeRoot, entry), joinPath(this.idbMountPath, entry));
    }

    await syncFs(this.fs, false);
  }
}
