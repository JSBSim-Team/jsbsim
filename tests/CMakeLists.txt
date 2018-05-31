# Import the Cython utilities for CMake
include(UseCython)

# Declare JSBSim as a C++ project
set_source_files_properties(jsbsim.pyx PROPERTIES CYTHON_IS_CXX TRUE)

# Build the Python module using Cython and the JSBSim library
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
compile_pyx(jsbsim JSBSIM_CXX jsbsim.pyx)

set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
configure_file(setup.py.in ${SETUP_PY})
add_custom_target(python_modules ALL DEPENDS ${SETUP_PY} ${JSBSIM_CXX}
  libJSBSim COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build_ext -i)

# Replicate the Python files in the build dir.
# With CMake, the build tree can be separated from the source tree. For tests
# written in Python to be executed by 'make test', the sources must be
# collocated with the JSBSim Python module in the build tree.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${CMAKE_BINARY_DIR}
                                      FILES_MATCHING PATTERN "*.py")

# Declare the tests to CTest so that they can be executed by 'make test'
set(PYTHON_TESTS ResetOutputFiles
                 TestICOverride
                 RunCheckCases
                 TestModelLoading
                 CheckFGBug1503
                 TestGustReset
                 TestPointMassInertia
                 CheckMomentsUpdate
                 TestFuelTanksInertia
                 TestInputSocket
                 TestInitialConditions
                 CheckScripts
                 CheckAircrafts
                 CheckOutputRate
                 TestAccelerometer
                 CheckDebugLvl
                 TestCosineGust
                 TestScriptOutput
                 CheckSimTimeReset
                 TestHoldDown
                 CheckTrim
                 TestChannelRate
                 TestWaypoint
                 TestSuspend
                 TestLGearSteer
                 TestAeroFuncOutput
                 TestKinematic
                 TestTurboProp
                 TestEngineIndexedProps
                 TestExternalReactions
                 TestTurbine
                 TestAeroFuncFrame
                 TestTemplateFunctions
                 fpectl
                 )

foreach(test ${PYTHON_TESTS})
  add_test(${test} ${PYTHON_EXECUTABLE} ${test}.py ${CMAKE_SOURCE_DIR})
endforeach()

# Install the JSBSim Python module
if (INSTALL_PYTHON_MODULE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findInstallDir.py OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/findModuleFileName.py OUTPUT_VARIABLE PYTHON_MODULE_NAME)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_MODULE_NAME} DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT pymodules)
endif()
