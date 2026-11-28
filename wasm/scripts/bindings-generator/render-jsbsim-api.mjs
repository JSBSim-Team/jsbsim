import { methodKey } from "./signature.mjs";
import { toTsType } from "./type-utils.mjs";

const BUILTIN_TS_TYPE_NAMES = new Set([
  "string",
  "number",
  "boolean",
  "void",
  "unknown",
  "any",
  "never",
  "undefined",
  "null",
  "object",
  "bigint",
  "symbol",
  "Array",
  "ReadonlyArray"
]);

export const DEFAULT_JSBSIM_API_IGNORE_METHODS = [
  "SetOutputFileName",
  "GetOutputFileName"
];

function escapeJsDoc(value) {
  return value.replace(/\*\//g, "*\\/");
}

function renderJsDocLines(method) {
  const descriptionLines = method.jsDoc?.descriptionLines ?? [];
  const paramDocs = method.jsDoc?.paramDocs ?? [];
  const returns = method.jsDoc?.returns ?? "";

  if (descriptionLines.length === 0 && paramDocs.length === 0 && !returns) {
    return [];
  }

  const lines = ["  /**"];

  for (const line of descriptionLines) {
    lines.push(`   * ${escapeJsDoc(line)}`);
  }

  for (const paramDoc of paramDocs) {
    lines.push(`   * @param ${paramDoc.name} ${escapeJsDoc(paramDoc.text)}`);
  }

  if (returns) {
    lines.push(`   * @returns ${escapeJsDoc(returns)}`);
  }

  lines.push("   */");
  return lines;
}

function splitFirstTokenForSetGet(tokens) {
  if (tokens.length === 0) {
    return tokens;
  }

  const [first, ...rest] = tokens;
  const firstLower = first.toLowerCase();

  if (firstLower.startsWith("set") && firstLower.length > 3) {
    return ["set", firstLower.slice(3), ...rest.map((token) => token.toLowerCase())];
  }

  if (firstLower.startsWith("get") && firstLower.length > 3) {
    return ["get", firstLower.slice(3), ...rest.map((token) => token.toLowerCase())];
  }

  return [firstLower, ...rest.map((token) => token.toLowerCase())];
}

function splitNameIntoWords(name) {
  const tokens = [];
  const parts = name.split(/_+/).filter(Boolean);

  for (const part of parts) {
    const partTokens =
      part.match(/[A-Z]{2,}s(?=$|[A-Z_])|[A-Z]+(?![a-z])|[A-Z][a-z]*|[a-z]+|[0-9]+/g) ?? [part];
    tokens.push(...partTokens);
  }

  return splitFirstTokenForSetGet(tokens).filter(Boolean);
}

function toCamelCaseName(name) {
  const words = splitNameIntoWords(name);
  if (words.length === 0) {
    return name.length > 0 ? `${name[0].toLowerCase()}${name.slice(1)}` : name;
  }

  const [head, ...tail] = words;
  const normalizedTail = tail.map((word) => `${word[0]?.toUpperCase() ?? ""}${word.slice(1)}`);
  return `${head}${normalizedTail.join("")}`;
}

function renderParam(param, options = {}) {
  const { optionalForDefault = false, includeDefaultInitializer = false } = options;

  if (includeDefaultInitializer && param.defaultValue !== null) {
    return `${param.name}: ${param.typeName} = ${param.defaultValue}`;
  }

  if (optionalForDefault && param.defaultValue !== null) {
    return `${param.name}?: ${param.typeName}`;
  }

  return `${param.name}: ${param.typeName}`;
}

function renderParams(params, options = {}) {
  return params.map((param) => renderParam(param, options)).join(", ");
}

function renderCallArgs(params) {
  return params.map((param) => param.name).join(", ");
}

function collectTypeIdentifiers(typeName) {
  const identifiers = typeName.match(/[A-Za-z_][A-Za-z0-9_]*/g) ?? [];
  return identifiers.filter((identifier) => !BUILTIN_TS_TYPE_NAMES.has(identifier));
}

function buildApiMethods(methods, typeMetadata, ignoreMethodsSet) {
  const groupedMethods = [];
  const groupedByCamelName = new Map();

  for (const method of methods) {
    if (ignoreMethodsSet.has(method.name)) {
      continue;
    }

    const signatureKey = methodKey(method);
    const methodParamOverrides = typeMetadata.paramTypeOverrides.get(signatureKey);
    const params = method.params.map((param, index) => ({
      name: param.name,
      typeName: methodParamOverrides?.get(index) ?? toTsType(param.type, { enumLookup: typeMetadata.enumLookup }),
      defaultValue: param.defaultValue ?? null
    }));

    const returnType = typeMetadata.returnTypeOverrides.get(signatureKey) ?? toTsType(method.returnType, {
      enumLookup: typeMetadata.enumLookup,
      isReturn: true
    });

    const camelName = toCamelCaseName(method.name);
    const existingGroup = groupedByCamelName.get(camelName);

    if (existingGroup && existingGroup.sourceMethodName !== method.name) {
      throw new Error(
        `Cannot generate JSBSimApi: "${existingGroup.sourceMethodName}" and "${method.name}" both map to "${camelName}".`
      );
    }

    const overload = {
      method,
      params,
      returnType
    };

    if (!existingGroup) {
      const newGroup = {
        camelName,
        sourceMethodName: method.name,
        overloads: [overload]
      };

      groupedByCamelName.set(camelName, newGroup);
      groupedMethods.push(newGroup);
      continue;
    }

    existingGroup.overloads.push(overload);
  }

  return groupedMethods;
}

function getRequiredParamCount(params) {
  let trailingDefaultCount = 0;
  for (let index = params.length - 1; index >= 0; index -= 1) {
    if (params[index].defaultValue === null) {
      break;
    }
    trailingDefaultCount += 1;
  }

  return params.length - trailingDefaultCount;
}

function buildDefaultFillCasesForOverloads(overloads) {
  const fillCases = [];
  const overloadRanges = overloads.map((overload) => ({
    minArgs: getRequiredParamCount(overload.params),
    maxArgs: overload.params.length
  }));

  for (let overloadIndex = 0; overloadIndex < overloads.length; overloadIndex += 1) {
    const overload = overloads[overloadIndex];
    const { minArgs, maxArgs } = overloadRanges[overloadIndex];

    for (let providedArgCount = minArgs; providedArgCount < maxArgs; providedArgCount += 1) {
      const acceptorCount = overloadRanges.filter(
        (range) => providedArgCount >= range.minArgs && providedArgCount <= range.maxArgs
      ).length;

      if (acceptorCount !== 1) {
        continue;
      }

      const missingParams = overload.params.slice(providedArgCount);
      if (missingParams.some((param) => param.defaultValue === null)) {
        continue;
      }

      fillCases.push({
        providedArgCount,
        defaultValues: missingParams.map((param) => param.defaultValue)
      });
    }
  }

  fillCases.sort((a, b) => a.providedArgCount - b.providedArgCount);
  return fillCases;
}

function collectTypeImports(groupedMethods) {
  const imports = new Set(["FGFDMExecApi"]);

  for (const group of groupedMethods) {
    for (const overload of group.overloads) {
      for (const param of overload.params) {
        for (const identifier of collectTypeIdentifiers(param.typeName)) {
          imports.add(identifier);
        }
      }

      for (const identifier of collectTypeIdentifiers(overload.returnType)) {
        imports.add(identifier);
      }
    }
  }

  return [...imports].sort((a, b) => a.localeCompare(b));
}

export function renderJsbsimApiClass(methods, typeMetadata, options = {}) {
  const ignoreMethods = options.ignoreMethods ?? DEFAULT_JSBSIM_API_IGNORE_METHODS;
  const ignoreMethodsSet = new Set(ignoreMethods);
  const groupedMethods = buildApiMethods(methods, typeMetadata, ignoreMethodsSet);
  const imports = collectTypeImports(groupedMethods);

  const lines = [
    "// Generated by scripts/generate-fgfdmexec-bindings.mjs.",
    "// Do not edit manually.",
    "",
    `import type { ${imports.join(", ")} } from "./fgfdmexec-api";`,
    "",
    "export class JSBSimApi {",
    "  readonly exec: FGFDMExecApi;",
    "",
    "  constructor(exec: FGFDMExecApi) {",
    "    this.exec = exec;",
    "  }",
    ""
  ];

  for (const group of groupedMethods) {
    if (group.overloads.length === 1) {
      const overload = group.overloads[0];
      const params = renderParams(overload.params, { includeDefaultInitializer: true });
      const callArgs = renderCallArgs(overload.params);

      lines.push(...renderJsDocLines(overload.method));
      lines.push(`  ${group.camelName}(${params}): ${overload.returnType} {`);

      if (overload.returnType === "void") {
        lines.push(`    this.exec.${group.sourceMethodName}(${callArgs});`);
      } else {
        lines.push(`    return this.exec.${group.sourceMethodName}(${callArgs});`);
      }

      lines.push("  }", "");
      continue;
    }

    const returnTypes = [...new Set(group.overloads.map((overload) => overload.returnType))];
    const returnType = returnTypes.join(" | ");
    const defaultFillCases = buildDefaultFillCasesForOverloads(group.overloads);

    for (const overload of group.overloads) {
      lines.push(...renderJsDocLines(overload.method));
      lines.push(
        `  ${group.camelName}(${renderParams(overload.params, { optionalForDefault: true })}): ${overload.returnType};`
      );
    }

    lines.push(`  ${group.camelName}(...args: unknown[]): ${returnType} {`);
    if (defaultFillCases.length > 0) {
      lines.push("    const normalizedArgs = [...args] as unknown[];");
      lines.push("    switch (normalizedArgs.length) {");
      for (const fillCase of defaultFillCases) {
        lines.push(`      case ${fillCase.providedArgCount}:`);
        lines.push(`        normalizedArgs.push(${fillCase.defaultValues.join(", ")});`);
        lines.push("        break;");
      }
      lines.push("      default:");
      lines.push("        break;");
      lines.push("    }");
    }

    if (returnType === "void") {
      if (defaultFillCases.length > 0) {
        lines.push(`    (this.exec.${group.sourceMethodName} as (...innerArgs: unknown[]) => void)(...normalizedArgs);`);
      } else {
        lines.push(`    (this.exec.${group.sourceMethodName} as (...innerArgs: unknown[]) => void)(...args);`);
      }
    } else {
      if (defaultFillCases.length > 0) {
        lines.push(
          `    return (this.exec.${group.sourceMethodName} as (...innerArgs: unknown[]) => ${returnType})(...normalizedArgs);`
        );
      } else {
        lines.push(
          `    return (this.exec.${group.sourceMethodName} as (...innerArgs: unknown[]) => ${returnType})(...args);`
        );
      }
    }
    lines.push("  }", "");
  }

  lines.push("}");

  return lines.join("\n");
}
