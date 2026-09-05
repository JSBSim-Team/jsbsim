
set(CMAKE_CXX_STANDARD 17)

set(UNIT_TESTS FGColumnVector3Test
               StringUtilitiesTest
               FGJSBBaseTest
               FGMatrix33Test
               FGQuaternionTest
               FGLocationTest
               FGGroundCallbackTest
               FGInitialConditionTest
               FGInertialTest
               FGPropertyValueTest
               FGTableTest
               FGRealValueTest
               FGParameterTest
               FGParameterValueTest
               FGConditionTest
               FGPropertyManagerTest
               FGAtmosphereTest
               FGAuxiliaryTest
               FGExternalReactionsTest
               FGMSISTest
               FGLogTest)


foreach(test ${UNIT_TESTS})
  cxxtest_add_test(${test}1 ${test}.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${test}.h)
  target_link_libraries(${test}1 libJSBSim)
  target_include_directories(${test}1 PUBLIC ${CXXTEST_INCLUDE_DIR})
  add_coverage(${test}1)
endforeach()

if(WIN32 AND BUILD_SHARED_LIBS)
  # Windows cannot locate the symbol gtd7 as it is not exported in the JSBSim
  # DLL. To keep NRLMSIS source files pristine, the option chosen is to
  # compile NRLMSIS alongside the unit test FGMSISTest1.
  set(MSIS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/models/atmosphere/MSIS)
  target_sources(FGMSISTest1 PUBLIC
                  ${MSIS_SOURCE_DIR}/nrlmsise-00.c
                  ${MSIS_SOURCE_DIR}/nrlmsise-00_data.c)

  # Windows needs the DLL to be copied locally for unit tests to run. Every
  # test target depends on the copy so that it is complete before any of them
  # links: vcpkg's applocal step reads the DLL next to the test it is deploying,
  # and a parallel build lets that happen while the copy is still writing.
  # The destination is the tests' output directory, with the per-configuration
  # subdirectory that multi-config generators add. It cannot be taken from a
  # test target, since the tests depend on this copy.
  if(CMAKE_CONFIGURATION_TYPES)
    set(UNIT_TESTS_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
  else()
    set(UNIT_TESTS_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()
  add_custom_target(copy_jsbsim_dll
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:libJSBSim> ${UNIT_TESTS_BIN_DIR}
                    DEPENDS libJSBSim)
  foreach(test ${UNIT_TESTS})
    add_dependencies(${test}1 copy_jsbsim_dll)
  endforeach()
endif(WIN32 AND BUILD_SHARED_LIBS)
