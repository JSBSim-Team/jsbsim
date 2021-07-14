find_package(Matlab)

if(Matlab_FOUND)
  matlab_add_mex(NAME JSBSim_SFunction SRC JSBSim_SFunction.cpp JSBSimInterface.cpp
                 LINK_TO libJSBSim)
  target_include_directories(JSBSim_SFunction PUBLIC ${CMAKE_SOURCE_DIR}/src
                             ${Matlab_ROOT_DIR}/simulink/include)
endif(Matlab_FOUND)
