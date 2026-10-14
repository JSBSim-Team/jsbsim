import fs from "node:fs";
import path from "node:path";

import { CPP_OUT_PATH, HEADER_PATH, ROOT_DIR, SDK_API_OUT_PATH, TS_OUT_PATH } from "./paths.mjs";
import { buildTypeMetadata } from "./type-metadata.mjs";
import { extractPublicMethodsFromClass, loadClassAstContext } from "./methods-from-ast.mjs";
import { renderCppOutput } from "./render-cpp.mjs";
import { renderJsbsimApiClass } from "./render-jsbsim-api.mjs";
import { renderTsInterface } from "./render-ts.mjs";

function ensureParentDir(filePath) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
}

export function generateBindings() {
  if (!fs.existsSync(HEADER_PATH)) {
    throw new Error(`Missing JSBSim header: ${HEADER_PATH}. Run the package build from a complete JSBSim checkout.`);
  }

  const { classNode } = loadClassAstContext();
  const methods = extractPublicMethodsFromClass(classNode);
  const typeMetadata = buildTypeMetadata(methods, classNode);

  const cppOut = renderCppOutput(methods, typeMetadata.enumLookup, "FGFDMExec.h");
  const tsOut = renderTsInterface(methods, typeMetadata);
  const sdkApiOut = renderJsbsimApiClass(methods, typeMetadata);

  ensureParentDir(CPP_OUT_PATH);
  ensureParentDir(TS_OUT_PATH);
  ensureParentDir(SDK_API_OUT_PATH);

  fs.writeFileSync(CPP_OUT_PATH, cppOut, "utf8");
  fs.writeFileSync(TS_OUT_PATH, tsOut, "utf8");
  fs.writeFileSync(SDK_API_OUT_PATH, sdkApiOut, "utf8");

  process.stdout.write(`Generated ${methods.length} method bindings.\n`);
  process.stdout.write(`Detected ${typeMetadata.enumDefs.length} enum type map(s) and ${typeMetadata.flagDefs.length} flag map(s).\n`);
  if (typeMetadata.unresolvedEnumTypes.size > 0) {
    process.stdout.write(`Unresolved enum candidates: ${[...typeMetadata.unresolvedEnumTypes].join(", ")}\n`);
  }
  process.stdout.write(`- ${path.relative(ROOT_DIR, CPP_OUT_PATH)}\n`);
  process.stdout.write(`- ${path.relative(ROOT_DIR, TS_OUT_PATH)}\n`);
  process.stdout.write(`- ${path.relative(ROOT_DIR, SDK_API_OUT_PATH)}\n`);
}
