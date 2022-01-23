find_package(Matlab)

if(Matlab_FOUND)
  matlab_add_mex(NAME JSBSim_SFunction SRC JSBSim_SFunction.cpp JSBSimInterface.cpp
                 LINK_TO libJSBSim)
  target_include_directories(JSBSim_SFunction PUBLIC ${CMAKE_SOURCE_DIR}/src
                             ${Matlab_ROOT_DIR}/simulink/include)
  set_target_properties(JSBSim_SFunction PROPERTIES CXX_STANDARD 14)

  if(MSVC)
    foreach(LINK_LIB ${JSBSIM_WINDOWS_LINK_LIBRARIES})
      string(APPEND WINDOWS_LINK_LIBRARIES " ${LINK_LIB}.lib")
    endforeach()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/WindowsSimulinkCompile.m.in ${CMAKE_CURRENT_BINARY_DIR}/JSBSimSimulinkCompile.m)
  endif(MSVC)

endif(Matlab_FOUND)
