import path from "node:path";
import { readDescriptor } from "../build-system/source.mjs";

export const BUILD_DESCRIPTOR = await readDescriptor(process.env.JSBSIM_BUILD_DESCRIPTOR);
export const ROOT_DIR = BUILD_DESCRIPTOR.workspaceRoot;
export const JSBSIM_SRC_DIR = path.join(BUILD_DESCRIPTOR.native.root, "src");
export const BINDGEN_COMPILER = "em++";
export const CLASS_NAME = "FGFDMExec";
export const HEADER_PATH = path.join(JSBSIM_SRC_DIR, `${CLASS_NAME}.h`);
export const IMPLEMENTATION_PATH = path.join(JSBSIM_SRC_DIR, `${CLASS_NAME}.cpp`);
export const CPP_OUT_PATH = path.join(ROOT_DIR, "generated/FGFDMExecBindings.cpp");
export const TS_OUT_PATH = path.join(ROOT_DIR, "src/generated/fgfdmexec-api.ts");
export const SDK_API_OUT_PATH = path.join(ROOT_DIR, "src/generated/jsbsim-api.ts");
export const AST_TMP_PREFIX = "jsbsim-bindgen-";
