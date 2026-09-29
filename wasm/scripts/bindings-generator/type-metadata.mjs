import fs from "node:fs";
import path from "node:path";

import { findAstNodes, runClangAstDumpFromFile, runClangAstDumpFromSource, walkAst } from "./clang-ast.mjs";
import { CLASS_NAME, IMPLEMENTATION_PATH, JSBSIM_SRC_DIR } from "./paths.mjs";
import {
  enumLookupVariants,
  isBoolType,
  isNumericType,
  isPointerType,
  isReferenceType,
  maybeLooksLikeEnumTypeName,
  normalizeType,
  resolveEnumInfo,
  stripConstVolatile,
  toTsType
} from "./type-utils.mjs";
import { createMethodArityKey, createMethodKey, methodArityKey, methodKey } from "./signature.mjs";

function escapeRegExp(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function normalizeIncludePath(filePath) {
  return filePath.replace(/\\/g, "/");
}

function collectCommentText(node) {
  if (!node || typeof node !== "object") {
    return "";
  }

  if (node.kind === "TextComment" && typeof node.text === "string") {
    return node.text.replace(/\s+/g, " ").trim();
  }

  const parts = [];
  for (const child of node.inner ?? []) {
    const text = collectCommentText(child);
    if (text) {
      parts.push(text);
    }
  }

  return parts.join(" ").trim();
}

function extractFirstIntegerValue(node) {
  if (!node || typeof node !== "object") {
    return null;
  }

  if (typeof node.value === "string" && /^-?\d+$/.test(node.value)) {
    return Number.parseInt(node.value, 10);
  }

  for (const child of node.inner ?? []) {
    const value = extractFirstIntegerValue(child);
    if (value !== null) {
      return value;
    }
  }

  return null;
}

function extractEnumMembers(enumDeclNode) {
  const members = [];
  let nextValue = 0;

  for (const child of enumDeclNode.inner ?? []) {
    if (child.kind !== "EnumConstantDecl" || !child.name) {
      continue;
    }

    let value = extractFirstIntegerValue(child);
    if (value === null) {
      value = nextValue;
    }

    members.push({ name: child.name, value });
    nextValue = value + 1;
  }

  return members;
}

function makeClassEnumTsName(enumName) {
  const trimmed = enumName.replace(/^e(?=[A-Z])/, "");
  return `${CLASS_NAME}${trimmed}`;
}

function makeFlagTsName(methodName) {
  return `${methodName}Mode`;
}

function parseClassMetadataFromNode(classNode) {
  const enumDefs = [];
  const flagsByMethod = new Map();
  let currentAccess = "private";
  let activeFlagMethod = null;

  for (const child of classNode.inner ?? []) {
    if (child.kind === "AccessSpecDecl") {
      currentAccess = child.access ?? currentAccess;
      if (currentAccess !== "public") {
        activeFlagMethod = null;
      }
      continue;
    }

    if (currentAccess !== "public") {
      activeFlagMethod = null;
      continue;
    }

    if (child.kind === "EnumDecl" && child.name) {
      const members = extractEnumMembers(child);
      if (members.length === 0) {
        continue;
      }

      const enumName = child.name;
      const cppQualifiedName = `JSBSim::${CLASS_NAME}::${enumName}`;

      enumDefs.push({
        kind: "enum",
        tsName: makeClassEnumTsName(enumName),
        cppSimpleName: enumName,
        cppQualifiedName,
        cppTypeNames: new Set([
          enumName,
          `${CLASS_NAME}::${enumName}`,
          cppQualifiedName,
          `enum ${enumName}`,
          `enum ${CLASS_NAME}::${enumName}`
        ]),
        members
      });

      activeFlagMethod = null;
      continue;
    }

    if (child.kind !== "VarDecl") {
      activeFlagMethod = null;
      continue;
    }

    const rawType = child.type?.qualType ?? "";
    const scalarType = stripConstVolatile(rawType).trim();
    if (child.storageClass !== "static" || (scalarType !== "int" && scalarType !== "unsigned int")) {
      activeFlagMethod = null;
      continue;
    }

    const fullCommentNode = (child.inner ?? []).find((innerChild) => innerChild.kind === "FullComment");
    const commentText = collectCommentText(fullCommentNode);
    const modeFlagsMatch = commentText.match(/Mode flags for ([A-Za-z_][A-Za-z0-9_]*)/);
    if (modeFlagsMatch) {
      activeFlagMethod = modeFlagsMatch[1];
    }

    if (!activeFlagMethod) {
      continue;
    }

    const value = extractFirstIntegerValue(child);
    if (value === null || !child.name) {
      continue;
    }

    const existing = flagsByMethod.get(activeFlagMethod) ?? [];
    existing.push({ name: child.name, value });
    flagsByMethod.set(activeFlagMethod, existing);
  }

  const flagDefs = [...flagsByMethod.entries()]
    .map(([methodName, members]) => ({
      kind: "flags",
      tsName: makeFlagTsName(methodName),
      methodName,
      members
    }))
    .filter((flagDef) => flagDef.members.length > 0);

  return {
    enumDefs,
    flagDefs
  };
}

function findHeaderCandidatesForToken(token) {
  const escapedToken = escapeRegExp(token);
  const tokenPattern = new RegExp(`\\b${escapedToken}\\b`);
  const candidates = [];
  const queue = [JSBSIM_SRC_DIR];

  while (queue.length > 0) {
    const dir = queue.pop();
    let entries = [];

    try {
      entries = fs.readdirSync(dir, { withFileTypes: true });
    } catch {
      continue;
    }

    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);

      if (entry.isDirectory()) {
        queue.push(fullPath);
        continue;
      }

      if (!entry.isFile() || !/\.(h|hpp|hxx)$/i.test(entry.name)) {
        continue;
      }

      let source = "";
      try {
        source = fs.readFileSync(fullPath, "utf8");
      } catch {
        continue;
      }

      if (!source.includes(token) || !tokenPattern.test(source) || !source.includes("enum")) {
        continue;
      }

      candidates.push(fullPath);
    }
  }

  return candidates.sort((a, b) => a.length - b.length);
}

function parseExternalEnumDefinitionFromAst(ast, enumSimpleName, enumQualifiedName) {
  const enumDeclById = new Map();
  walkAst(ast, (node) => {
    if (node.kind === "EnumDecl" && typeof node.id === "string") {
      enumDeclById.set(node.id, node);
    }
  });

  let enumDecl = null;

  const typedefNodes = findAstNodes(ast, (node) => node.kind === "TypedefDecl" && node.name === enumSimpleName);
  for (const typedefNode of typedefNodes) {
    const elaboratedType = (typedefNode.inner ?? []).find((node) => node.kind === "ElaboratedType");
    const ownedTagDeclId = elaboratedType?.ownedTagDecl?.id;
    if (ownedTagDeclId && enumDeclById.has(ownedTagDeclId)) {
      enumDecl = enumDeclById.get(ownedTagDeclId);
      break;
    }
  }

  if (!enumDecl) {
    const allEnumDecls = findAstNodes(ast, (node) => node.kind === "EnumDecl");
    enumDecl = allEnumDecls.find((node) => node.name === enumSimpleName);

    if (!enumDecl) {
      enumDecl = allEnumDecls.find((node) => {
        const constants = (node.inner ?? []).filter((innerNode) => innerNode.kind === "EnumConstantDecl");
        if (constants.length === 0) {
          return false;
        }

        const constantType = constants[0].type?.qualType ?? "";
        return constantType.includes(enumSimpleName) || constantType.includes(enumQualifiedName);
      });
    }
  }

  if (!enumDecl) {
    return null;
  }

  const members = extractEnumMembers(enumDecl);
  if (members.length === 0) {
    return null;
  }

  const normalizedQualifiedType = normalizeType(enumQualifiedName.replace(/^enum\s+/, ""));

  return {
    kind: "enum",
    tsName: enumSimpleName,
    cppSimpleName: enumSimpleName,
    cppQualifiedName: normalizedQualifiedType,
    cppTypeNames: new Set([enumSimpleName, normalizedQualifiedType, `enum ${enumSimpleName}`]),
    members
  };
}

function resolveExternalEnumDefinition(enumQualifiedName) {
  const enumSimpleName = enumQualifiedName.replace(/^enum\s+/, "").split("::").at(-1);
  if (!enumSimpleName) {
    return null;
  }

  const candidateHeaders = findHeaderCandidatesForToken(enumSimpleName);
  if (candidateHeaders.length === 0) {
    return null;
  }

  for (const headerPath of candidateHeaders) {
    const includeRelPath = normalizeIncludePath(path.relative(JSBSIM_SRC_DIR, headerPath));
    const probeTypeCandidates = [enumQualifiedName, enumSimpleName];
    if (!enumQualifiedName.includes("::")) {
      probeTypeCandidates.push(`JSBSim::${enumSimpleName}`);
    }

    for (const probeType of new Set(probeTypeCandidates)) {
      const probeSource = `#include "${includeRelPath}"\n${probeType} __codex_enum_probe = static_cast<${probeType}>(0);\nint main() { return static_cast<int>(__codex_enum_probe); }\n`;
      const ast = runClangAstDumpFromSource(probeSource, { filter: enumSimpleName });
      if (!ast) {
        continue;
      }

      const enumDef = parseExternalEnumDefinitionFromAst(ast, enumSimpleName, enumQualifiedName);
      if (enumDef) {
        return enumDef;
      }
    }
  }

  return null;
}

function addEnumDefinition(enumDefs, enumDef) {
  const hasDuplicate = enumDefs.some(
    (existing) => existing.tsName === enumDef.tsName || existing.cppQualifiedName === enumDef.cppQualifiedName
  );
  if (!hasDuplicate) {
    enumDefs.push(enumDef);
    return true;
  }
  return false;
}

function buildEnumLookup(enumDefs) {
  const lookup = new Map();
  for (const enumDef of enumDefs) {
    for (const cppTypeName of enumDef.cppTypeNames) {
      for (const variant of enumLookupVariants(cppTypeName)) {
        lookup.set(variant, enumDef);
      }
    }
  }
  return lookup;
}

function parseImplementationEnumParamOverrides() {
  const ast = runClangAstDumpFromFile(IMPLEMENTATION_PATH, { filter: `${CLASS_NAME}::` });
  const overridesByMethodKey = new Map();
  const overridesByArityKey = new Map();
  const discoveredEnumTypes = new Set();

  if (!ast) {
    return {
      overridesByMethodKey,
      overridesByArityKey,
      discoveredEnumTypes
    };
  }

  const methodNodes = findAstNodes(ast, (node) => node.kind === "CXXMethodDecl" && typeof node.name === "string");

  for (const methodNode of methodNodes) {
    const paramNodes = (methodNode.inner ?? []).filter((node) => node.kind === "ParmVarDecl");
    if (paramNodes.length === 0) {
      continue;
    }

    const methodParamTypes = paramNodes.map((paramNode) => normalizeType(paramNode.type?.qualType ?? "void"));
    const signatureKey = createMethodKey(methodNode.name, methodParamTypes);
    const arityKey = createMethodArityKey(methodNode.name, paramNodes.length);
    const paramIndexById = new Map(paramNodes.map((paramNode, index) => [paramNode.id, index]));

    const castNodes = findAstNodes(
      methodNode,
      (node) => (node.kind === "CStyleCastExpr" || node.kind === "CXXStaticCastExpr") && typeof node.type?.qualType === "string"
    );

    for (const castNode of castNodes) {
      const castType = normalizeType(castNode.type.qualType.replace(/^enum\s+/, ""));
      if (!castType || isBoolType(castType) || isNumericType(castType) || isPointerType(castType) || isReferenceType(castType)) {
        continue;
      }

      const paramRefs = findAstNodes(
        castNode,
        (node) => node.kind === "DeclRefExpr" && node.referencedDecl?.kind === "ParmVarDecl" && typeof node.referencedDecl.id === "string"
      );

      for (const paramRef of paramRefs) {
        const paramIndex = paramIndexById.get(paramRef.referencedDecl.id);
        if (paramIndex === undefined) {
          continue;
        }

        let perSignature = overridesByMethodKey.get(signatureKey);
        if (!perSignature) {
          perSignature = new Map();
          overridesByMethodKey.set(signatureKey, perSignature);
        }

        if (!perSignature.has(paramIndex)) {
          perSignature.set(paramIndex, castType);
        }

        let perArity = overridesByArityKey.get(arityKey);
        if (!perArity) {
          perArity = new Map();
          overridesByArityKey.set(arityKey, perArity);
        }

        if (!perArity.has(paramIndex)) {
          perArity.set(paramIndex, castType);
        }

        discoveredEnumTypes.add(castType);
      }
    }
  }

  return {
    overridesByMethodKey,
    overridesByArityKey,
    discoveredEnumTypes
  };
}

function setMethodParamOverride(paramTypeOverrides, methodKeyValue, paramIndex, tsTypeName) {
  let methodOverrides = paramTypeOverrides.get(methodKeyValue);
  if (!methodOverrides) {
    methodOverrides = new Map();
    paramTypeOverrides.set(methodKeyValue, methodOverrides);
  }

  if (!methodOverrides.has(paramIndex)) {
    methodOverrides.set(paramIndex, tsTypeName);
  }
}

export function buildTypeMetadata(methods, classNode) {
  const classMetadata = parseClassMetadataFromNode(classNode);
  const enumDefs = [...classMetadata.enumDefs];
  const flagDefs = [...classMetadata.flagDefs];

  const implementationMetadata = parseImplementationEnumParamOverrides();
  const enumTypeCandidates = new Set(implementationMetadata.discoveredEnumTypes);

  for (const method of methods) {
    for (const param of method.params) {
      if (maybeLooksLikeEnumTypeName(param.type)) {
        enumTypeCandidates.add(param.type);
      }
    }

    if (maybeLooksLikeEnumTypeName(method.returnType)) {
      enumTypeCandidates.add(method.returnType);
    }
  }

  let enumLookup = buildEnumLookup(enumDefs);
  const unresolvedEnumTypes = new Set();

  for (const enumTypeCandidate of enumTypeCandidates) {
    if (resolveEnumInfo(enumTypeCandidate, enumLookup)) {
      continue;
    }

    const resolvedEnumDef = resolveExternalEnumDefinition(enumTypeCandidate);
    if (!resolvedEnumDef) {
      unresolvedEnumTypes.add(enumTypeCandidate);
      continue;
    }

    addEnumDefinition(enumDefs, resolvedEnumDef);
    enumLookup = buildEnumLookup(enumDefs);
  }

  const paramTypeOverrides = new Map();
  const returnTypeOverrides = new Map();

  for (const method of methods) {
    const signatureKey = methodKey(method);
    const arityKey = methodArityKey(method);

    const fromSignature = implementationMetadata.overridesByMethodKey.get(signatureKey);
    const fromArity = implementationMetadata.overridesByArityKey.get(arityKey);
    const candidateOverrides = fromSignature ?? fromArity;

    for (const [paramIndex, enumType] of candidateOverrides ?? []) {
      const enumInfo = resolveEnumInfo(enumType, enumLookup);
      if (!enumInfo) {
        unresolvedEnumTypes.add(enumType);
        continue;
      }
      setMethodParamOverride(paramTypeOverrides, signatureKey, paramIndex, enumInfo.tsName);
    }

    for (const enumDef of enumDefs) {
      if (
        method.name === `Set${enumDef.cppSimpleName}` &&
        method.params.length > 0 &&
        toTsType(method.params[0].type, { enumLookup }) === "number"
      ) {
        setMethodParamOverride(paramTypeOverrides, signatureKey, 0, enumDef.tsName);
      }

      if (
        method.name === `Get${enumDef.cppSimpleName}` &&
        method.params.length === 0 &&
        toTsType(method.returnType, { enumLookup, isReturn: true }) === "number" &&
        !returnTypeOverrides.has(signatureKey)
      ) {
        returnTypeOverrides.set(signatureKey, enumDef.tsName);
      }
    }

    for (const flagDef of flagDefs) {
      if (
        method.name === flagDef.methodName &&
        method.params.length > 0 &&
        toTsType(method.params[0].type, { enumLookup }) === "number"
      ) {
        setMethodParamOverride(paramTypeOverrides, signatureKey, 0, flagDef.tsName);
      }
    }
  }

  return {
    enumDefs,
    flagDefs,
    enumLookup,
    paramTypeOverrides,
    returnTypeOverrides,
    unresolvedEnumTypes
  };
}
