# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
set_source_files_properties(jsbsim.pyx PROPERTIES CYTHON_IS_CXX TRUE)

# Build the Python module using Cython and the JSBSim library
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
compile_pyx(jsbsim JSBSIM_CXX jsbsim.pyx)

set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
configure_file(setup.py.in ${SETUP_PY})

execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findModuleFileName.py OUTPUT_VARIABLE PYTHON_MODULE_NAME)
set(JSBSIM_PYTHON_MODULE "${CMAKE_BINARY_DIR}/tests/${PYTHON_MODULE_NAME}")

# setup.py build_ext is called with --force because dependencies and time stamps
# are managed by CMake so we don't want setup.py to check them as well.
add_custom_command(OUTPUT ${JSBSIM_PYTHON_MODULE}
  DEPENDS ${SETUP_PY} ${JSBSIM_CXX} $<TARGET_FILE:libJSBSim>
  COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -b ${CMAKE_BINARY_DIR}/tests --force
  COMMENT "Building Python modules...")

add_custom_target(PythonJSBSim ALL DEPENDS ${JSBSIM_PYTHON_MODULE})
add_dependencies(PythonJSBSim libJSBSim)

# Install the JSBSim Python module
if (INSTALL_PYTHON_MODULE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  install(FILES ${JSBSIM_PYTHON_MODULE} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
