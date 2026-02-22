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
  execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxy2PyDocStrings.py --pyxfile=${JSBSIM_PYX} --doxdir=${CMAKE_BINARY_DIR}/documentation
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  # Prepare the sphinx build files
  configure_file(${PROJECT_SOURCE_DIR}/doc/python/sphinx/conf.py.in ${CMAKE_BINARY_DIR}/documentation/conf.py)
  file(COPY ${PROJECT_SOURCE_DIR}/doc/python/sphinx/index.rst DESTINATION ${CMAKE_BINARY_DIR}/documentation)
endif(DOXYGEN_FOUND AND BUILD_DOCS)

# Build the Python module using Cython and the JSBSim library
compile_pyx(_jsbsim _JSBSIM_CXX ${JSBSIM_PYX})
file(RELATIVE_PATH JSBSIM_CXX ${CMAKE_CURRENT_BINARY_DIR} ${_JSBSIM_CXX})

# Build the package directory in the test folder
set(JSBSIM_TEST_DIR ${CMAKE_BINARY_DIR}/tests)
set(JSBSIM_TEST_PACKAGE_DIR ${JSBSIM_TEST_DIR}/jsbsim)
file(MAKE_DIRECTORY ${JSBSIM_TEST_PACKAGE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py ${JSBSIM_TEST_PACKAGE_DIR} COPYONLY)

# Copy license files to the package directory
install(FILES ${PROJECT_SOURCE_DIR}/src/GeographicLib/LICENSE.txt DESTINATION jsbsim
        RENAME GeographicLib-LICENSE.txt COMPONENT wheel)
install(FILES ${PROJECT_SOURCE_DIR}/src/simgear/xml/COPYING DESTINATION jsbsim
        RENAME libexpat-LICENSE.txt COMPONENT wheel)
install(FILES ${PROJECT_SOURCE_DIR}/COPYING DESTINATION jsbsim
        RENAME LICENSE.txt COMPONENT wheel)

# Duplicate data files (i.e. aircraft, engines, etc.)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/py.typed "")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/py.typed DESTINATION jsbsim COMPONENT wheel)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/aircraft DESTINATION jsbsim COMPONENT wheel)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/engine DESTINATION jsbsim COMPONENT wheel)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/systems DESTINATION jsbsim COMPONENT wheel)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts DESTINATION jsbsim COMPONENT wheel)

# Build the Python module
python3_add_library(_jsbsim MODULE ${JSBSIM_CXX})
target_include_directories(_jsbsim PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/python)
target_link_libraries(_jsbsim PRIVATE libJSBSim)
set_target_properties(_jsbsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${JSBSIM_TEST_PACKAGE_DIR})
install(TARGETS _jsbsim DESTINATION jsbsim COMPONENT wheel)
install(FILES ${PROJECT_SOURCE_DIR}/python/__init__.py DESTINATION jsbsim COMPONENT wheel)
install(FILES ${PROJECT_SOURCE_DIR}/python/_jsbsim.pyi DESTINATION jsbsim COMPONENT wheel)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/python/JSBSim.py DESTINATION jsbsim
        RENAME script.py COMPONENT wheel)

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

  if(SKBUILD)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    list(FILTER CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS INCLUDE REGEX "msvcp[0-9]+|vcruntime[0-9]+")

    if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
      message(FATAL_ERROR
        "\n"
        "----------------------------------------------------------------------\n"
        " FATAL ERROR: MSVC system runtime libraries not found!\n"
        " The Windows wheel build cannot proceed because required DLLs\n"
        " (msvcp140.dll, etc.) are missing from this build environment.\n"
        " \n"
        " Action: Ensure the 'MSVC C++ x64/x86 build tools' and the \n"
        " 'C++ Redistributable MSVC' components are installed via the \n"
        " Visual Studio Installer on this machine.\n"
        "----------------------------------------------------------------------\n"
      )
    endif()

    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION jsbsim COMPONENT wheel)

    message(STATUS "Windows runtimes added to wheel: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")
  endif(SKBUILD)
endif(WIN32)

if(NOT SKBUILD)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml.in pyproject.toml)
  add_subdirectory(fpectl)
endif(NOT SKBUILD)

# Install the JSBSim Python module
if (INSTALL_JSBSIM_PYTHON_MODULE)
  execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  file(MAKE_DIRECTORY ${PYTHON_INSTALL_DIR}/jsbsim)
  install(DIRECTORY ${JSBSIM_PACKAGE_DIR} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
