# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
set(JSBSIM_PYX ${CMAKE_CURRENT_BINARY_DIR}/jsbsim.pyx)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pyx.in ${JSBSIM_PYX})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pxd ${CMAKE_CURRENT_BINARY_DIR}/jsbsim.pxd COPYONLY)
set_source_files_properties(${JSBSIM_PYX} PROPERTIES CYTHON_IS_CXX TRUE
                                                     INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src)

# Autogenerate the Python module doc strings from Doxygen docs
if(DOXYGEN_FOUND AND BUILD_DOCS)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxy2PyDocStrings.py
    doxy2pydocs.py)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} python/doxy2pydocs.py
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # Prepare the sphinx build files
  configure_file(${PROJECT_SOURCE_DIR}/doc/python/sphinx/conf.py.in
    ${CMAKE_BINARY_DIR}/documentation/conf.py)
  file(COPY ${PROJECT_SOURCE_DIR}/doc/python/sphinx/index.rst
    DESTINATION ${CMAKE_BINARY_DIR}/documentation)
endif(DOXYGEN_FOUND AND BUILD_DOCS)

# Build the Python module using Cython and the JSBSim library
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
file(RELATIVE_PATH libJSBSim_PATH ${PROJECT_SOURCE_DIR} ${libJSBSim_DIRECTORY})

file(COPY ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/COPYING DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/GeographicLib/LICENSE.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt ${CMAKE_CURRENT_BINARY_DIR}/GeographicLib-LICENSE.txt)
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/COPYING ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt)

set(SIMGEAR_XML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/simgear/xml)
file(COPY ${SIMGEAR_XML_SOURCE_DIR}/COPYING DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/COPYING ${CMAKE_CURRENT_BINARY_DIR}/libexpat-LICENSE.txt)
configure_file(${SIMGEAR_XML_SOURCE_DIR}/xmltok_impl.c
               ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml/xmltok_impl.c COPYONLY)
configure_file(${SIMGEAR_XML_SOURCE_DIR}/xmltok_ns.c
               ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml/xmltok_ns.c COPYONLY)

foreach(_FILE "MANIFEST.in" "ExceptionManagement.h" "fpectl/fpectlmodule.h" "pyproject.toml")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${_FILE} COPYONLY)
endforeach()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/JSBSim.py ${CMAKE_CURRENT_BINARY_DIR}/JSBSim COPYONLY)

# Duplicate data files (i.e. aircraft, engines, etc.)
file(COPY ${PROJECT_SOURCE_DIR}/aircraft DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
file(COPY ${PROJECT_SOURCE_DIR}/engine DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
file(COPY ${PROJECT_SOURCE_DIR}/systems DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)
file(COPY ${PROJECT_SOURCE_DIR}/scripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data)

# Copy each source file
foreach(OBJECT ${libJSBSim_SOURCE_FILES})
  if(${OBJECT} MATCHES "TARGET_OBJECTS:([^ >]+)")
    get_target_property(TARGET_SOURCE_FILES ${CMAKE_MATCH_1} SOURCES)
    get_target_property(TARGET_SOURCE_DIRECTORY ${CMAKE_MATCH_1} TARGET_DIRECTORY)
    file(RELATIVE_PATH TARGET_PATH ${PROJECT_SOURCE_DIR} ${TARGET_SOURCE_DIRECTORY})
    file(MAKE_DIRECTORY ${TARGET_PATH})
    foreach(_FILE ${TARGET_SOURCE_FILES})
      configure_file(${TARGET_SOURCE_DIRECTORY}/${_FILE} ${TARGET_PATH}/${_FILE} COPYONLY)
      list(APPEND SOURCE_FILES ${TARGET_PATH}/${_FILE})
    endforeach()
  else()
    configure_file(${libJSBSim_DIRECTORY}/${OBJECT} ${libJSBSim_PATH}/${OBJECT} COPYONLY)
    list(APPEND SOURCE_FILES ${libJSBSim_PATH}/${OBJECT})
  endif()
endforeach(OBJECT)

foreach(_FILE ${SOURCE_FILES})
  get_filename_component(FILE_EXTENSION ${_FILE} EXT)
  if((NOT ${FILE_EXTENSION} STREQUAL ".h") AND (NOT ${FILE_EXTENSION} STREQUAL ".hxx") AND (NOT ${FILE_EXTENSION} STREQUAL ".hpp"))
    list(APPEND JSBSIM_SOURCE_FILES ${_FILE})
  endif()
endforeach(_FILE)

# BUILD_ROOT_PATH is used by setup.py to determine if the library has already
# been built.
file(RELATIVE_PATH BUILD_ROOT_PATH ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(SETUP_PY ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
configure_file(setup.py.in ${SETUP_PY})

execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findModuleFileName.py jsbsim OUTPUT_VARIABLE PYTHON_MODULE_NAME)
set(JSBSIM_PYTHON_MODULE ${CMAKE_BINARY_DIR}/tests/${PYTHON_MODULE_NAME})

# setup.py build_ext is called with --force because dependencies and time stamps
# are managed by CMake so we don't want setup.py to check them as well.
add_custom_command(OUTPUT ${JSBSIM_PYTHON_MODULE}
  DEPENDS ${SETUP_PY} ${JSBSIM_CXX} ${CMAKE_CURRENT_SOURCE_DIR}/ExceptionManagement.h $<TARGET_FILE:libJSBSim>
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -b ${CMAKE_BINARY_DIR}/tests --force
                                           $<$<BOOL:${USING_MSBUILD}>:--config> $<$<BOOL:${USING_MSBUILD}>:$<CONFIG>>
                                           $<$<BOOL:${BUILD_SHARED_LIBS}>:--sharedlibs>
                                           $<$<BOOL:${MINGW}>:--compiler> $<$<BOOL:${MINGW}>:mingw32>
  COMMENT "Building Python module...")

add_subdirectory(fpectl)

add_custom_target(PythonJSBSim ALL DEPENDS ${JSBSIM_PYTHON_MODULE})
add_dependencies(PythonJSBSim libJSBSim)

# Windows needs the DLL to be copied locally for unit tests to run.
if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/tests/JSBSim.dll
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                      $<TARGET_FILE:libJSBSim>
                      ${CMAKE_BINARY_DIR}/tests)
  add_custom_target(CopyJSBSimDLL ALL DEPENDS ${CMAKE_BINARY_DIR}/tests/JSBSim.dll)
  add_dependencies(CopyJSBSimDLL libJSBSim)
endif(WIN32 AND BUILD_SHARED_LIBS)

# Install the JSBSim Python module
if (INSTALL_JSBSIM_PYTHON_MODULE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  install(FILES ${JSBSIM_PYTHON_MODULE} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
