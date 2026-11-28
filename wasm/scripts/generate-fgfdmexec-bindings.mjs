#!/usr/bin/env node
import { readFile } from "node:fs/promises";
import { generateBindings } from "./bindings-generator/index.mjs";
import { BUILD_DESCRIPTOR, ROOT_DIR, CPP_OUT_PATH, TS_OUT_PATH, SDK_API_OUT_PATH } from "./bindings-generator/paths.mjs";
import { sha256, verifyContent, writeJson } from "./build-system/source.mjs";
import path from "node:path";

await verifyContent(BUILD_DESCRIPTOR.native.root, BUILD_DESCRIPTOR.native.manifest);
generateBindings();
const files = {};
for (const file of [CPP_OUT_PATH, TS_OUT_PATH, SDK_API_OUT_PATH]) files[path.relative(ROOT_DIR, file).split(path.sep).join("/")] = sha256(await readFile(file));
await writeJson(path.join(ROOT_DIR, "generated/bindings-manifest.json"), {
  schemaVersion: 1, inputSha256: BUILD_DESCRIPTOR.identity.build.inputSha256,
  nativeContentSha256: BUILD_DESCRIPTOR.native.contentSha256, nativeCommit: BUILD_DESCRIPTOR.native.commit, files,
});
