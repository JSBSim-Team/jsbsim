# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
set_source_files_properties(jsbsim.pyx PROPERTIES CYTHON_IS_CXX TRUE)

# Build the Python module using Cython and the JSBSim library
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
compile_pyx(jsbsim JSBSIM_CXX jsbsim.pyx)

set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
configure_file(setup.py.in ${SETUP_PY})
add_custom_target(PythonJSBSim ALL DEPENDS ${SETUP_PY} ${JSBSIM_CXX}
  libJSBSim COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -b ${CMAKE_BINARY_DIR}/tests
  COMMENT "Building Python modules...")

# Install the JSBSim Python module
if (INSTALL_PYTHON_MODULE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findModuleFileName.py OUTPUT_VARIABLE PYTHON_MODULE_NAME)
  install(FILES ${CMAKE_BINARY_DIR}/tests/${PYTHON_MODULE_NAME} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
