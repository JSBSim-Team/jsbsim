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
                 TestActuator
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
                 TestDensityAltitude
                 TestPressureAltitude
                 fpectl
                 TestStdAtmosphere
                 TestMiscellaneous
                 TestGndReactions
                 TestIntegrators
                 TestSwitch
                 TestGain
                 TestDeadBand
                 TestFilter
                 TestFunctions
                 TestDistributor
                 )

foreach(test ${PYTHON_TESTS})
  add_test(${test} ${PYTHON_EXECUTABLE} ${test}.py ${CMAKE_SOURCE_DIR})
endforeach()
