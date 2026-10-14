# Optional JavaScript/WebAssembly component of the enclosing JSBSim build.
# Native builds retain the root project's CMake minimum and need no Node tools.
cmake_minimum_required(VERSION 3.20)

if(NOT EMSCRIPTEN OR NOT TARGET libJSBSim)
  message(FATAL_ERROR "Build this component from the JSBSim repository root with BUILD_WASM_MODULE=ON and Emscripten.")
endif()
if(BUILD_SHARED_LIBS)
  message(FATAL_ERROR "The WebAssembly module requires BUILD_SHARED_LIBS=OFF.")
endif()
if(DEFINED JSBSIM_SOURCE_DIR)
  message(FATAL_ERROR "External JSBSIM_SOURCE_DIR selection is not supported. The WebAssembly module uses the enclosing JSBSim source tree.")
endif()

foreach(required JSBSIM_WASM_BINDINGS JSBSIM_BINDINGS_MANIFEST JSBSIM_BUILD_INPUT_SHA256 JSBSIM_NATIVE_CONTENT_SHA256)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "${required} must come from the resolved build descriptor. See wasm/README.md.")
  endif()
endforeach()
if(NOT EXISTS "${JSBSIM_WASM_BINDINGS}")
  message(FATAL_ERROR "Generated bindings not found at ${JSBSIM_WASM_BINDINGS}")
endif()
file(READ "${JSBSIM_BINDINGS_MANIFEST}" BINDINGS_JSON)
string(JSON BINDINGS_SOURCE GET "${BINDINGS_JSON}" nativeContentSha256)
string(JSON BINDINGS_INPUT GET "${BINDINGS_JSON}" inputSha256)
string(JSON BINDINGS_SHA GET "${BINDINGS_JSON}" files "generated/FGFDMExecBindings.cpp")
file(SHA256 "${JSBSIM_WASM_BINDINGS}" ACTUAL_BINDINGS_SHA)
if(NOT BINDINGS_SOURCE STREQUAL JSBSIM_NATIVE_CONTENT_SHA256 OR NOT BINDINGS_INPUT STREQUAL JSBSIM_BUILD_INPUT_SHA256 OR NOT BINDINGS_SHA STREQUAL ACTUAL_BINDINGS_SHA)
  message(FATAL_ERROR "Native source and generated bindings identity mismatch")
endif()

add_executable(jsbsim_wasm "${JSBSIM_WASM_BINDINGS}")
set_target_properties(jsbsim_wasm PROPERTIES
  OUTPUT_NAME "jsbsim_wasm"
  SUFFIX ".mjs"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_features(jsbsim_wasm PRIVATE cxx_std_17)
target_compile_options(jsbsim_wasm PRIVATE -fexceptions)
target_link_libraries(jsbsim_wasm PRIVATE libJSBSim)
target_link_options(jsbsim_wasm PRIVATE
  --bind
  -O3
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=JSBSimWasmModule
  -sENVIRONMENT=web,worker,node
  -sFILESYSTEM=1
  -lidbfs.js
  -sALLOW_MEMORY_GROWTH=1
  -sNO_DISABLE_EXCEPTION_CATCHING
  -sWASM_BIGINT=1
  "-sEXPORTED_RUNTIME_METHODS=['FS']"
)
