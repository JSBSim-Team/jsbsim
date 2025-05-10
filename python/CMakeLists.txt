# Import the Cython utilities for CMake
include(UseCython)

# Get the current year to update the copyrights.
string(TIMESTAMP THIS_YEAR "%Y")

# Declare JSBSim as a C++ project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JSBSIM_PYX ${CMAKE_CURRENT_BINARY_DIR}/_jsbsim.pyx)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pyx.in ${JSBSIM_PYX})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pxd ${CMAKE_CURRENT_BINARY_DIR}/_jsbsim.pxd COPYONLY)
set_source_files_properties(${JSBSIM_PYX} PROPERTIES CYTHON_IS_CXX TRUE
                                          INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/src)

# Autogenerate the Python module doc strings from Doxygen docs
if(DOXYGEN_FOUND AND BUILD_DOCS)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxy2PyDocStrings.py
    doxy_to_pyx.py COPYONLY)
  execute_process(COMMAND ${Python3_EXECUTABLE} python/doxy_to_pyx.py --pyxfile=${JSBSIM_PYX} --doxdir=${CMAKE_BINARY_DIR}/documentation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # Prepare the sphinx build files
  configure_file(${PROJECT_SOURCE_DIR}/doc/python/sphinx/conf.py.in
    ${CMAKE_BINARY_DIR}/documentation/conf.py)
  file(COPY ${PROJECT_SOURCE_DIR}/doc/python/sphinx/index.rst
    DESTINATION ${CMAKE_BINARY_DIR}/documentation)
endif(DOXYGEN_FOUND AND BUILD_DOCS)

# Build the Python module using Cython and the JSBSim library
compile_pyx(_jsbsim _JSBSIM_CXX ${JSBSIM_PYX})
file(RELATIVE_PATH JSBSIM_CXX ${CMAKE_CURRENT_BINARY_DIR} ${_JSBSIM_CXX})

# Build the package directory
set(JSBSIM_PACKAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/jsbsim)
file(MAKE_DIRECTORY ${JSBSIM_PACKAGE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py ${JSBSIM_PACKAGE_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jsbsim.pxd ${JSBSIM_PACKAGE_DIR}/_jsbsim.pxd COPYONLY)

# Build the package directory in the test folder
set(JSBSIM_TEST_DIR ${CMAKE_BINARY_DIR}/tests)
set(JSBSIM_TEST_PACKAGE_DIR ${JSBSIM_TEST_DIR}/jsbsim)
file(MAKE_DIRECTORY ${JSBSIM_TEST_PACKAGE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py ${JSBSIM_TEST_PACKAGE_DIR} COPYONLY)

# Duplicate the files for the source distribution of JSBSim
cmake_policy(SET CMP0051 NEW)
get_target_property(libJSBSim_SOURCE_FILES libJSBSim SOURCES)
get_target_property(libJSBSim_DIRECTORY libJSBSim TARGET_DIRECTORY)
file(RELATIVE_PATH libJSBSim_PATH ${PROJECT_SOURCE_DIR} ${libJSBSim_DIRECTORY})

file(COPY ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/COPYING DESTINATION ${JSBSIM_PACKAGE_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/GeographicLib/LICENSE.txt DESTINATION ${JSBSIM_PACKAGE_DIR})
file(RENAME ${JSBSIM_PACKAGE_DIR}/LICENSE.txt ${JSBSIM_PACKAGE_DIR}/GeographicLib-LICENSE.txt)
file(RENAME ${JSBSIM_PACKAGE_DIR}/COPYING ${JSBSIM_PACKAGE_DIR}/LICENSE.txt)

set(SIMGEAR_XML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/simgear/xml)
file(COPY ${SIMGEAR_XML_SOURCE_DIR}/COPYING DESTINATION ${JSBSIM_PACKAGE_DIR})
file(RENAME ${JSBSIM_PACKAGE_DIR}/COPYING ${JSBSIM_PACKAGE_DIR}/libexpat-LICENSE.txt)
configure_file(${SIMGEAR_XML_SOURCE_DIR}/xmltok_impl.c
               ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml/xmltok_impl.c COPYONLY)
configure_file(${SIMGEAR_XML_SOURCE_DIR}/xmltok_ns.c
               ${CMAKE_CURRENT_BINARY_DIR}/src/simgear/xml/xmltok_ns.c COPYONLY)

foreach(_FILE "MANIFEST.in" "ExceptionManagement.h" "fpectl/fpectlmodule.h" "pyproject.toml")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${_FILE} COPYONLY)
endforeach()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/JSBSim.py ${CMAKE_CURRENT_BINARY_DIR}/JSBSim.py COPYONLY)

# Duplicate data files (i.e. aircraft, engines, etc.)
file(COPY ${PROJECT_SOURCE_DIR}/aircraft DESTINATION ${JSBSIM_PACKAGE_DIR} FILES_MATCHING PATTERN "*.xml")
file(COPY ${PROJECT_SOURCE_DIR}/engine DESTINATION ${JSBSIM_PACKAGE_DIR} FILES_MATCHING PATTERN "*.xml")
file(COPY ${PROJECT_SOURCE_DIR}/systems DESTINATION ${JSBSIM_PACKAGE_DIR} FILES_MATCHING PATTERN "*.xml")
file(COPY ${PROJECT_SOURCE_DIR}/scripts DESTINATION ${JSBSIM_PACKAGE_DIR} FILES_MATCHING PATTERN "*.xml")

# Copy each source file
foreach(OBJECT ${libJSBSim_SOURCE_FILES})
  if(${OBJECT} MATCHES "TARGET_OBJECTS:([^ >]+)")
    get_target_property(TARGET_SOURCE_FILES ${CMAKE_MATCH_1} SOURCES)
    get_target_property(TARGET_SOURCE_DIRECTORY ${CMAKE_MATCH_1} TARGET_DIRECTORY)
    file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR} ${TARGET_SOURCE_DIRECTORY})
    set(TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_RELATIVE_PATH})
    file(MAKE_DIRECTORY ${TARGET_PATH})
    foreach(_FILE ${TARGET_SOURCE_FILES})
      configure_file(${TARGET_SOURCE_DIRECTORY}/${_FILE} ${TARGET_PATH}/${_FILE} COPYONLY)
      list(APPEND SOURCE_FILES ${TARGET_RELATIVE_PATH}/${_FILE})
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

configure_file(setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py)

python3_add_library(_jsbsim MODULE ${JSBSIM_CXX} ${CMAKE_CURRENT_SOURCE_DIR}/ExceptionManagement.h)
target_include_directories(_jsbsim PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(_jsbsim PRIVATE libJSBSim)
set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${JSBSIM_TEST_PACKAGE_DIR})

if(WIN32)
  # Output directories for MSVC
  set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${JSBSIM_TEST_PACKAGE_DIR})
  set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${JSBSIM_TEST_PACKAGE_DIR})
  set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${JSBSIM_TEST_PACKAGE_DIR})
  set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${JSBSIM_TEST_PACKAGE_DIR})

  # Windows needs the DLL to be copied locally for unit tests to run.
  if(BUILD_SHARED_LIBS)
    add_custom_command(OUTPUT ${JSBSIM_TEST_PACKAGE_DIR}/JSBSim.dll
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:libJSBSim>
                        ${JSBSIM_TEST_PACKAGE_DIR})
    add_custom_target(CopyJSBSimDLL ALL DEPENDS ${JSBSIM_TEST_PACKAGE_DIR}/JSBSim.dll)
    add_dependencies(CopyJSBSimDLL libJSBSim)
  endif(BUILD_SHARED_LIBS)
endif(WIN32)

add_subdirectory(fpectl)

# Install the JSBSim Python module
if (INSTALL_JSBSIM_PYTHON_MODULE)
  execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  file(MAKE_DIRECTORY ${PYTHON_INSTALL_DIR}/jsbsim)
  install(DIRECTORY ${JSBSIM_TEST_PACKAGE_DIR} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
