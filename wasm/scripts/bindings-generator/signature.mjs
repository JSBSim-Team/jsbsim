import { normalizeType } from "./type-utils.mjs";

export function createMethodArityKey(methodName, paramCount) {
  return `${methodName}#${paramCount}`;
}

export function createMethodKey(methodName, paramTypes) {
  const signature = paramTypes.map((type) => normalizeType(type)).join(",");
  return `${methodName}(${signature})`;
}

export function methodKey(method) {
  return createMethodKey(method.name, method.params.map((param) => param.type));
}

export function methodArityKey(method) {
  return createMethodArityKey(method.name, method.params.length);
}
