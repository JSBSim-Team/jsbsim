# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
set_source_files_properties(jsbsim.pyx PROPERTIES CYTHON_IS_CXX TRUE)

# Build the Python module using Cython and the JSBSim library
cython_add_module(jsbsim jsbsim.pyx)
target_link_libraries(jsbsim libJSBSim)

# Replicate the Python files in the build dir.
# With CMake, the build tree can be separated from the source tree. For tests
# written in Python to be executed with 'make test', the sources must be
# collocated with the JSBSim Python module in the build tree.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${CMAKE_BINARY_DIR}
                                      FILES_MATCHING PATTERN "*.py")

# Declare the tests to CTest so that they can be executed with 'make test'
add_test(ResetOutputFiles ${PYTHON_EXECUTABLE} ResetOutputFiles.py
                                               ${CMAKE_SOURCE_DIR})
add_test(TestICOverride ${PYTHON_EXECUTABLE} TestICOverride.py ${CMAKE_SOURCE_DIR})
add_test(RunCheckCases ${PYTHON_EXECUTABLE} RunCheckCases.py ${CMAKE_SOURCE_DIR})
add_test(TestModelLoading ${PYTHON_EXECUTABLE} TestModelLoading.py ${CMAKE_SOURCE_DIR})
add_test(CheckFGBug1503 ${PYTHON_EXECUTABLE} CheckFGBug1503.py ${CMAKE_SOURCE_DIR})
add_test(TestGustReset ${PYTHON_EXECUTABLE} TestGustReset.py ${CMAKE_SOURCE_DIR})
add_test(TestPointMassInertia ${PYTHON_EXECUTABLE} TestPointMassInertia.py ${CMAKE_SOURCE_DIR})

# Install the JSBSim Python module
if (INSTALL_PYTHON_MODULE)
  set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
  configure_file(setup.py.in ${SETUP_PY})
  install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install)")
endif()

