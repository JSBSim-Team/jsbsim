import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";

import { AST_TMP_PREFIX, JSBSIM_SRC_DIR, BINDGEN_COMPILER } from "./paths.mjs";

function parseClangJsonOutput(output) {
  const trimmed = output.trim();
  if (!trimmed) {
    return null;
  }

  try {
    return JSON.parse(trimmed);
  } catch {
    // Continue with multi-document parsing.
  }

  const docs = [];
  let start = -1;
  let depth = 0;
  let inString = false;
  let escaped = false;

  for (let index = 0; index < trimmed.length; index += 1) {
    const ch = trimmed[index];

    if (start === -1) {
      if (ch === "{") {
        start = index;
        depth = 1;
        inString = false;
        escaped = false;
      }
      continue;
    }

    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (ch === "\\") {
        escaped = true;
      } else if (ch === "\"") {
        inString = false;
      }
      continue;
    }

    if (ch === "\"") {
      inString = true;
      continue;
    }

    if (ch === "{") {
      depth += 1;
      continue;
    }

    if (ch === "}") {
      depth -= 1;
      if (depth === 0) {
        const jsonChunk = trimmed.slice(start, index + 1);
        try {
          docs.push(JSON.parse(jsonChunk));
        } catch {
          return null;
        }
        start = -1;
      }
    }
  }

  if (docs.length === 0) {
    return null;
  }

  if (docs.length === 1) {
    return docs[0];
  }

  return {
    kind: "ClangAstDocumentSet",
    inner: docs
  };
}

export function runClangAstDumpFromFile(sourcePath, { filter } = {}) {
  const compilers = [BINDGEN_COMPILER];

  for (const compiler of compilers) {
    const args = ["-std=c++17", "-I", JSBSIM_SRC_DIR, "-fsyntax-only"];

    if (filter) {
      args.push("-Xclang", `-ast-dump-filter=${filter}`);
    }

    args.push("-Xclang", "-ast-dump=json", sourcePath);

    const result = spawnSync(compiler, args, {
      encoding: "utf8",
      maxBuffer: 128 * 1024 * 1024
    });

    if (result.error || result.status !== 0) {
      continue;
    }

    const parsed = parseClangJsonOutput(result.stdout);
    if (!parsed) {
      continue;
    }

    return parsed;
  }

  return null;
}

export function runClangAstDumpFromSource(source, { filter } = {}) {
  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), AST_TMP_PREFIX));
  const tempSourcePath = path.join(tempDir, "probe.cpp");
  fs.writeFileSync(tempSourcePath, source, "utf8");

  try {
    return runClangAstDumpFromFile(tempSourcePath, { filter });
  } finally {
    fs.rmSync(tempDir, { recursive: true, force: true });
  }
}

export function walkAst(node, visit) {
  if (!node || typeof node !== "object") {
    return;
  }

  visit(node);

  const children = Array.isArray(node.inner) ? node.inner : [];
  for (const child of children) {
    walkAst(child, visit);
  }
}

export function findAstNodes(root, predicate) {
  const nodes = [];
  walkAst(root, (node) => {
    if (predicate(node)) {
      nodes.push(node);
    }
  });
  return nodes;
}
