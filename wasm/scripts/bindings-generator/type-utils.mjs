import { CLASS_NAME } from "./paths.mjs";

export const NUMERIC_TYPES = new Set([
  "char",
  "signed char",
  "unsigned char",
  "short",
  "unsigned short",
  "int",
  "unsigned int",
  "long",
  "unsigned long",
  "long long",
  "unsigned long long",
  "float",
  "double",
  "long double",
  "size_t"
]);

export const CXX_PASSTHROUGH_TOKENS = new Set([
  "const",
  "volatile",
  "unsigned",
  "signed",
  "short",
  "long",
  "int",
  "float",
  "double",
  "char",
  "bool",
  "void",
  "size_t",
  "std",
  "string",
  "vector",
  "shared_ptr",
  "unique_ptr",
  "uintptr_t",
  CLASS_NAME,
  "SGPath"
]);

export function normalizeType(type) {
  return type
    .replace(/\s+/g, " ")
    .replace(/\s*([*&<>,])\s*/g, "$1")
    .replace(/\bconst\b\s*/g, "const ")
    .trim();
}

export function stripConstVolatile(type) {
  return type.replace(/\b(const|volatile)\b/g, "").replace(/\s+/g, " ").trim();
}

export function isBoolType(type) {
  return stripConstVolatile(type).replace(/[&*]/g, "").trim() === "bool";
}

export function isStringType(type) {
  const normalized = stripConstVolatile(type).replace(/[&*]/g, "").trim();
  return normalized === "std::string";
}

export function isSGPathType(type) {
  const normalized = stripConstVolatile(type).replace(/[&*]/g, "").trim();
  return normalized === "SGPath";
}

export function isVectorOfStringType(type) {
  const normalized = stripConstVolatile(type).replace(/[&*]/g, "").replace(/\s+/g, "").trim();
  return normalized === "std::vector<std::string>";
}

export function isNumericType(type) {
  const normalized = stripConstVolatile(type).replace(/[&*]/g, "").replace(/\s+/g, " ").trim();
  return NUMERIC_TYPES.has(normalized);
}

export function isPointerType(type) {
  return /\*/.test(type);
}

export function isReferenceType(type) {
  return /&/.test(type);
}

export function baseTypeForPointerCast(type) {
  return normalizeType(type.replace(/[&*]/g, "").trim());
}

export function qualifyCppType(type) {
  const withoutStructClass = type.replace(/\b(struct|class)\s+/g, "");
  return withoutStructClass.replace(/\b([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)\b/g, (token) => {
    if (token.includes("::")) {
      return token;
    }

    if (CXX_PASSTHROUGH_TOKENS.has(token) || NUMERIC_TYPES.has(token)) {
      return token;
    }

    if (token === "PropertyCatalogStructure") {
      return `${CLASS_NAME}::PropertyCatalogStructure`;
    }

    return `JSBSim::${token}`;
  });
}

export function normalizeEnumLookupName(typeName) {
  return stripConstVolatile(typeName)
    .replace(/\benum\s+/g, "")
    .replace(/[&*]/g, "")
    .replace(/\s+/g, "")
    .trim();
}

export function enumLookupVariants(typeName) {
  const base = normalizeEnumLookupName(typeName);
  if (!base) {
    return [];
  }

  const variants = new Set([base]);

  if (base.startsWith("JSBSim::")) {
    variants.add(base.slice("JSBSim::".length));
  } else {
    variants.add(`JSBSim::${base}`);
  }

  if (base.startsWith(`${CLASS_NAME}::`)) {
    variants.add(`JSBSim::${base}`);
  }

  if (base.startsWith(`JSBSim::${CLASS_NAME}::`)) {
    variants.add(base.slice("JSBSim::".length));
  }

  if (base.includes("::")) {
    variants.add(base.split("::").at(-1));
  }

  return [...variants];
}

export function resolveEnumInfo(typeName, enumLookup) {
  for (const variant of enumLookupVariants(typeName)) {
    const enumInfo = enumLookup.get(variant);
    if (enumInfo) {
      return enumInfo;
    }
  }
  return null;
}

export function toTsType(type, { enumLookup, isReturn = false } = {}) {
  const normalized = normalizeType(type);
  const enumInfo = resolveEnumInfo(normalized, enumLookup ?? new Map());
  if (enumInfo) return enumInfo.tsName;
  if (normalized === "void") return "void";
  if (isBoolType(normalized)) return "boolean";
  if (isNumericType(normalized)) return "number";
  if (isStringType(normalized) || isSGPathType(normalized)) return "string";
  if (isReturn && isVectorOfStringType(normalized)) return "string[]";
  return "number";
}

export function maybeLooksLikeEnumTypeName(typeName) {
  const normalized = normalizeType(stripConstVolatile(typeName).replace(/[&*]/g, "").replace(/^enum\s+/, "").trim());
  if (!normalized || normalized === "void") {
    return false;
  }
  if (isBoolType(normalized) || isNumericType(normalized) || isStringType(normalized) || isSGPathType(normalized) || isVectorOfStringType(normalized)) {
    return false;
  }
  if (normalized.includes("<")) {
    return false;
  }
  const simpleName = normalized.split("::").at(-1);
  return /^e[A-Z]/.test(simpleName) || /Mode$/.test(simpleName) || /Type$/.test(simpleName);
}
