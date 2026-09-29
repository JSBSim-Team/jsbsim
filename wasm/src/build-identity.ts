/** Replaced only inside a resolved build workspace. */
export interface JSBSimBuildIdentity {
  readonly schemaVersion: 2;
  readonly package: { readonly name: string; readonly version: string };
  readonly native: { readonly origin: string; readonly commit: string; readonly contentSha256: string; readonly dirty: boolean };
  readonly sdk: { readonly path: "wasm"; readonly commit: string; readonly contentSha256: string; readonly dirty: boolean };
  readonly build: {
    readonly mode: "in-tree";
    readonly inputSha256: string;
    readonly toolchain: Readonly<Record<"node" | "npm" | "emscripten" | "cmake" | "clang" | "platform" | "arch", string>>;
    readonly options: { readonly buildType: "Release"; readonly cxxStandard: 17; readonly sdkTarget: "es2022" };
  };
}
export const buildIdentity: JSBSimBuildIdentity = (() => {
  throw new Error("SDK build identity requires the resolved build pipeline; run npm run build or build:dev and consume its package.");
})();
