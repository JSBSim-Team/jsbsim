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
                 TestTurbulenceRandomSeed
                 TestCosineGust
                 TestTurbulenceGustStop
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
                 test_fpectl
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
                 TestMagnetometer
                 TestLinearization
                 TestLinearActuator
                 TestPlanet
                 TestLighterThanAir
                 TestUnusableFuel
                 TestSensorRandomSeed
                 TestPQRdot)

foreach(test ${PYTHON_TESTS})
  add_test(NAME ${test}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py)
endforeach()
