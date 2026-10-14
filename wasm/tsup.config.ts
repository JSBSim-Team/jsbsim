import { defineConfig } from "tsup";

export default defineConfig({
  entry: ["src/index.ts", "src/wasm.ts"],
  format: ["esm"],
  dts: true,
  sourcemap: true,
  clean: false,
  target: "es2022",
});
