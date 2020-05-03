# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
include_directories(${CMAKE_SOURCE_DIR}/src)
set(JSBSIM_PYX ${CMAKE_CURRENT_BINARY_DIR}/jsbsim.pyx)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pyx.in ${JSBSIM_PYX})
set_source_files_properties(${JSBSIM_PYX} PROPERTIES CYTHON_IS_CXX TRUE)

# Autogenerate the Python module doc strings from Doxygen docs
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxy2PyDocStrings.py
    doxy2pydocs.py)
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/documentation/xml)
    execute_process(COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  endif()
  execute_process(COMMAND ${PYTHON_EXECUTABLE} python/doxy2pydocs.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # Prepare the sphinx build files
  configure_file(${CMAKE_SOURCE_DIR}/doc/python/sphinx/conf.py.in
    ${CMAKE_BINARY_DIR}/documentation/conf.py)
  file(COPY ${CMAKE_SOURCE_DIR}/doc/python/sphinx/index.rst
    DESTINATION ${CMAKE_BINARY_DIR}/documentation)
endif(DOXYGEN_FOUND)

# Build the Python module using Cython and the JSBSim library
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
compile_pyx(jsbsim _JSBSIM_CXX ${JSBSIM_PYX})
file(RELATIVE_PATH JSBSIM_CXX ${CMAKE_CURRENT_BINARY_DIR} ${_JSBSIM_CXX})

# Check if we are using Visual Studio msbuild
if(MSVC)
  string(TOUPPER CMAKE_GENERATOR _GENERATOR)
  if(NOT (_GENERATOR STREQUAL NINJA))
    set(USING_MSBUILD 1)
  endif()
endif(MSVC)

# Duplicate the files for the source distribution of JSBSim
cmake_policy(SET CMP0051 NEW)
get_target_property(libJSBSim_SOURCE_FILES libJSBSim SOURCES)
get_target_property(libJSBSim_DIRECTORY libJSBSim TARGET_DIRECTORY)
get_target_property(libJSBSim_LINK_LIBRARIES libJSBSim LINK_LIBRARIES)
get_directory_property(FLAGS_LIST DIRECTORY ${libJSBSim_DIRECTORY} COMPILE_DEFINITIONS)
file(RELATIVE_PATH libJSBSim_PATH ${CMAKE_SOURCE_DIR} ${libJSBSim_DIRECTORY})

file(COPY ${CMAKE_SOURCE_DIR}/README.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/COPYING DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/COPYING ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt)
file(COPY ${CMAKE_SOURCE_DIR}/src/simgear/xml/xmltok_impl.c
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml)
file(COPY ${CMAKE_SOURCE_DIR}/src/simgear/xml/xmltok_ns.c
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml)

foreach(_FILE "MANIFEST.in" "jsbsim.pxd" "ExceptionManagement.h"
              "fpectl/fpectlmodule.h" "JSBSim.py")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/JSBSim.py ${CMAKE_CURRENT_BINARY_DIR}/JSBSim)

# Copy each source file and collect the compile flags specified by add_definitions()
foreach(OBJECT ${libJSBSim_SOURCE_FILES})
  string(SUBSTRING ${OBJECT} 0 17 HEADER)
  if (${HEADER} STREQUAL "$<TARGET_OBJECTS:")
    string(LENGTH ${OBJECT} OBJECT_LENGTH)
    math(EXPR TARGET_LENGTH "${OBJECT_LENGTH}-18")
    string(SUBSTRING ${OBJECT} 17 ${TARGET_LENGTH} TARGET)
    get_target_property(TARGET_SOURCE_FILES ${TARGET} SOURCES)
    get_target_property(TARGET_SOURCE_DIRECTORY ${TARGET} TARGET_DIRECTORY)
    get_directory_property(FLAGS DIRECTORY ${TARGET_SOURCE_DIRECTORY} COMPILE_DEFINITIONS)
    list(APPEND FLAGS_LIST ${FLAGS})
    file(RELATIVE_PATH TARGET_PATH ${CMAKE_SOURCE_DIR} ${TARGET_SOURCE_DIRECTORY})
    file(MAKE_DIRECTORY ${TARGET_PATH})
    foreach(_FILE ${TARGET_SOURCE_FILES})
      file(COPY ${TARGET_SOURCE_DIRECTORY}/${_FILE} DESTINATION ${TARGET_PATH})
      list(APPEND SOURCE_FILES ${TARGET_PATH}/${_FILE})
    endforeach()
  else()
    file(COPY ${libJSBSim_DIRECTORY}/${OBJECT} DESTINATION ${libJSBSim_PATH})
    list(APPEND SOURCE_FILES ${libJSBSim_PATH}/${OBJECT})
  endif()
endforeach(OBJECT)

foreach(_FILE ${SOURCE_FILES})
  get_tail(${_FILE} FILE_EXTENSION ".")
  if((NOT ${FILE_EXTENSION} STREQUAL "h") AND (NOT ${FILE_EXTENSION} STREQUAL "hxx"))
    string(APPEND JSBSIM_SOURCE_FILES "'${_FILE}',")
  endif()
endforeach(_FILE)

foreach(_LIB ${libJSBSim_LINK_LIBRARIES})
  string(REGEX MATCH "(^[A-Za-z0-9_]+).lib" LIBNAME ${_LIB})
  if (LIBNAME STREQUAL "")
    string(APPEND JSBSIM_LINK_LIBRARIES "'${_LIB}',")
  else()
    string(APPEND JSBSIM_LINK_LIBRARIES "'${CMAKE_MATCH_1}',")
  endif()
endforeach(_LIB)

# Clean up the compile flags list
list(REMOVE_DUPLICATES FLAGS_LIST)
foreach(FLAG ${FLAGS_LIST})
  string(APPEND JSBSIM_FLAGS "'${FLAG}',")
endforeach()

file(RELATIVE_PATH BUILD_ROOT_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(SETUP_PY ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
configure_file(setup.py.in ${SETUP_PY})

execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findModuleFileName.py jsbsim OUTPUT_VARIABLE PYTHON_MODULE_NAME)
set(JSBSIM_PYTHON_MODULE ${CMAKE_BINARY_DIR}/tests/${PYTHON_MODULE_NAME})

# setup.py build_ext is called with --force because dependencies and time stamps
# are managed by CMake so we don't want setup.py to check them as well.
add_custom_command(OUTPUT ${JSBSIM_PYTHON_MODULE}
  DEPENDS ${SETUP_PY} ${JSBSIM_CXX} $<TARGET_FILE:libJSBSim>
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -b ${CMAKE_BINARY_DIR}/tests --force
                                           $<IF:$<BOOL:${USING_MSBUILD}>,--config,> $<IF:$<BOOL:${USING_MSBUILD}>,$<CONFIG>,>
                                           $<IF:$<BOOL:${MINGW}>,--compiler,> $<IF:$<BOOL:${MINGW}>,mingw32,>
  COMMENT "Building Python module...")

add_subdirectory(fpectl)

add_custom_target(PythonJSBSim ALL DEPENDS ${JSBSIM_PYTHON_MODULE})
add_dependencies(PythonJSBSim libJSBSim)

# Install the JSBSim Python module
if (INSTALL_PYTHON_MODULE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  install(FILES ${JSBSIM_PYTHON_MODULE} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
