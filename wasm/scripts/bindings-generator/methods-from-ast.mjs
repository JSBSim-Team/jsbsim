import path from "node:path";

import { runClangAstDumpFromSource, findAstNodes, walkAst } from "./clang-ast.mjs";
import { CLASS_NAME, HEADER_PATH, JSBSIM_SRC_DIR } from "./paths.mjs";
import { isSGPathType, isStringType, normalizeType } from "./type-utils.mjs";

function normalizeIncludePath(filePath) {
  return filePath.replace(/\\/g, "/");
}

function collapseWhitespace(value) {
  return value.replace(/\s+/g, " ").trim();
}

function sanitizeCommentLine(value) {
  const collapsed = collapseWhitespace(value)
    .replace(/^[@{]+\s*/, "")
    .replace(/\s*[@}]+$/, "")
    .trim();

  if (!collapsed || collapsed === "@" || collapsed === "{" || collapsed === "}") {
    return "";
  }

  return collapsed;
}

function collectTextCommentLines(node) {
  const lines = [];

  walkAst(node, (innerNode) => {
    if (innerNode.kind !== "TextComment" || typeof innerNode.text !== "string") {
      return;
    }

    const line = sanitizeCommentLine(innerNode.text);
    if (line) {
      lines.push(line);
    }
  });

  return lines;
}

function collectCommentText(node) {
  return collectTextCommentLines(node).join(" ").trim();
}

function parseMethodComment(fullCommentNode) {
  if (!fullCommentNode) {
    return {
      descriptionLines: [],
      paramDocsByName: new Map(),
      paramDocsByIndex: new Map(),
      returns: ""
    };
  }

  const descriptionLines = [];
  const paramDocsByName = new Map();
  const paramDocsByIndex = new Map();
  let returns = "";

  const topLevel = Array.isArray(fullCommentNode.inner) ? fullCommentNode.inner : [];

  for (const child of topLevel) {
    if (child.kind === "ParagraphComment") {
      const text = collectCommentText(child);
      if (text) {
        descriptionLines.push(text);
      }
      continue;
    }

    if (child.kind === "ParamCommandComment") {
      const text = collectCommentText(child);
      const paramName = typeof child.param === "string" ? child.param : "";
      const paramIndex = Number.isInteger(child.paramIdx) ? child.paramIdx : null;

      if (paramName && text) {
        paramDocsByName.set(paramName, text);
      }

      if (paramIndex !== null && text) {
        paramDocsByIndex.set(paramIndex, text);
      }

      continue;
    }

    if (child.kind === "BlockCommandComment") {
      const command = typeof child.name === "string" ? child.name.toLowerCase() : "";
      if (command === "return" || command === "returns") {
        returns = collectCommentText(child);
      }
    }
  }

  return {
    descriptionLines,
    paramDocsByName,
    paramDocsByIndex,
    returns
  };
}

function findClassNode(ast) {
  return findAstNodes(
    ast,
    (node) => node.kind === "CXXRecordDecl" && node.name === CLASS_NAME && node.completeDefinition === true
  )[0] ?? null;
}

function isPublicMethodNode(node, currentAccess) {
  if (currentAccess !== "public") {
    return false;
  }

  if (node.kind !== "CXXMethodDecl") {
    return false;
  }

  if (node.isImplicit === true) {
    return false;
  }

  if (node.name === CLASS_NAME || node.name === `~${CLASS_NAME}`) {
    return false;
  }

  return true;
}

function inferReturnType(methodNode) {
  const functionType = methodNode.type?.qualType ?? "";
  const openParen = functionType.indexOf("(");
  if (openParen < 0) {
    return "";
  }
  return normalizeType(functionType.slice(0, openParen).trim());
}

function normalizeParamNames(params) {
  const usedNames = new Set();
  const normalized = [];

  for (let index = 0; index < params.length; index += 1) {
    const param = params[index];
    let name = param.name && /^[A-Za-z_][A-Za-z0-9_]*$/.test(param.name) ? param.name : `arg${index}`;

    if (usedNames.has(name)) {
      let suffix = 1;
      while (usedNames.has(`${name}_${suffix}`)) {
        suffix += 1;
      }
      name = `${name}_${suffix}`;
    }

    usedNames.add(name);
    normalized.push({ ...param, name });
  }

  return normalized;
}

const EXPR_WRAPPER_KINDS = new Set([
  "ExprWithCleanups",
  "MaterializeTemporaryExpr",
  "ImplicitCastExpr",
  "CXXBindTemporaryExpr",
  "CXXDefaultArgExpr",
  "ParenExpr"
]);

function unwrapExprNode(node) {
  let current = node;
  const visited = new Set();

  while (current && typeof current === "object" && EXPR_WRAPPER_KINDS.has(current.kind)) {
    if (visited.has(current)) {
      break;
    }
    visited.add(current);

    if (!Array.isArray(current.inner) || current.inner.length === 0) {
      break;
    }

    current = current.inner[0];
  }

  return current;
}

function findDescendant(node, predicate) {
  if (!node || typeof node !== "object") {
    return null;
  }

  if (predicate(node)) {
    return node;
  }

  for (const child of node.inner ?? []) {
    const match = findDescendant(child, predicate);
    if (match) {
      return match;
    }
  }

  return null;
}

function toNumberLiteral(value) {
  if (typeof value === "number" && Number.isFinite(value)) {
    return `${value}`;
  }

  if (typeof value === "string" && /^[-+]?\d+(\.\d+)?([eE][-+]?\d+)?$/.test(value.trim())) {
    return value.trim();
  }

  return null;
}

function extractTsDefaultValueFromExpr(exprNode, paramType) {
  if (!exprNode || typeof exprNode !== "object") {
    return null;
  }

  const unwrapped = unwrapExprNode(exprNode);
  if (!unwrapped || typeof unwrapped !== "object") {
    return null;
  }

  if (unwrapped.kind === "CXXBoolLiteralExpr" && typeof unwrapped.value === "boolean") {
    return unwrapped.value ? "true" : "false";
  }

  if (unwrapped.kind === "IntegerLiteral" || unwrapped.kind === "FloatingLiteral") {
    return toNumberLiteral(unwrapped.value);
  }

  if (unwrapped.kind === "UnaryOperator" && (unwrapped.opcode === "-" || unwrapped.opcode === "+")) {
    const operand = unwrapExprNode((unwrapped.inner ?? [])[0]);
    if (operand && (operand.kind === "IntegerLiteral" || operand.kind === "FloatingLiteral")) {
      const numberLiteral = toNumberLiteral(operand.value);
      if (numberLiteral) {
        return `${unwrapped.opcode}${numberLiteral.replace(/^[+-]/, "")}`;
      }
    }
  }

  if (unwrapped.kind === "StringLiteral" && typeof unwrapped.value === "string") {
    return unwrapped.value;
  }

  const stringLiteralNode = findDescendant(
    unwrapped,
    (node) => node.kind === "StringLiteral" && typeof node.value === "string"
  );
  if (stringLiteralNode?.value) {
    return stringLiteralNode.value;
  }

  if (isStringType(paramType) || isSGPathType(paramType)) {
    // Default-constructed std::string/SGPath map to an empty JS string.
    if (unwrapped.kind === "CXXConstructExpr" || unwrapped.kind === "CXXTemporaryObjectExpr") {
      return "\"\"";
    }
  }

  return null;
}

function extractParamDefaultValue(paramNode, paramType) {
  if (!Object.hasOwn(paramNode, "init")) {
    return null;
  }

  const defaultExpr = (paramNode.inner ?? [])[0];
  return extractTsDefaultValueFromExpr(defaultExpr, paramType);
}

function extractMethodParams(methodNode) {
  const paramNodes = (methodNode.inner ?? []).filter((node) => node.kind === "ParmVarDecl");
  const params = paramNodes.map((paramNode, index) => {
    const rawName = typeof paramNode.name === "string" ? paramNode.name : "";
    const name = rawName;
    const paramType = normalizeType(paramNode.type?.qualType ?? "void");

    return {
      name,
      type: paramType,
      defaultValue: extractParamDefaultValue(paramNode, paramType)
    };
  });

  return normalizeParamNames(params);
}

function extractMethodJsDoc(parsedComment, params) {
  const paramDocs = [];

  for (let index = 0; index < params.length; index += 1) {
    const param = params[index];
    const byIndex = parsedComment.paramDocsByIndex.get(index);
    const byName = parsedComment.paramDocsByName.get(param.name);
    const text = byIndex ?? byName ?? "";
    if (text) {
      paramDocs.push({ name: param.name, text });
    }
  }

  return {
    descriptionLines: parsedComment.descriptionLines,
    paramDocs,
    returns: parsedComment.returns
  };
}

export function loadClassAstContext() {
  const includeRelPath = normalizeIncludePath(path.relative(JSBSIM_SRC_DIR, HEADER_PATH));
  const probeSource = `#include "${includeRelPath}"\nint main() { return 0; }\n`;
  const ast = runClangAstDumpFromSource(probeSource, { filter: CLASS_NAME });
  if (!ast) {
    throw new Error("Failed to parse FGFDMExec AST with clang.");
  }

  const classNode = findClassNode(ast);
  if (!classNode) {
    throw new Error(`Failed to locate ${CLASS_NAME} class definition in clang AST.`);
  }

  return {
    ast,
    classNode
  };
}

export function extractPublicMethodsFromClass(classNode) {
  const methods = [];
  let currentAccess = "private";

  for (const child of classNode.inner ?? []) {
    if (child.kind === "AccessSpecDecl") {
      currentAccess = child.access ?? currentAccess;
      continue;
    }

    if (!isPublicMethodNode(child, currentAccess)) {
      continue;
    }

    const returnType = inferReturnType(child);
    if (!returnType) {
      continue;
    }

    const fullCommentNode = (child.inner ?? []).find((innerNode) => innerNode.kind === "FullComment");
    const parsedComment = parseMethodComment(fullCommentNode);
    const params = extractMethodParams(child);

    methods.push({
      name: child.name,
      returnType,
      params,
      jsDoc: extractMethodJsDoc(parsedComment, params)
    });
  }

  return methods;
}
