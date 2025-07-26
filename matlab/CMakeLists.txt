find_package(Matlab)

if(Matlab_FOUND)
  set(SOURCES JSBSim_SFunction.cpp
              JSBSimInterface.cpp)

  # Build the mex file.
  matlab_add_mex(NAME JSBSim_SFunction SRC ${SOURCES} LINK_TO libJSBSim)
  target_include_directories(JSBSim_SFunction PUBLIC ${PROJECT_SOURCE_DIR}/src
                             ${Matlab_ROOT_DIR}/simulink/include)
  set_target_properties(JSBSim_SFunction PROPERTIES CXX_STANDARD 17)

  # Generate the MATLAB compile script for the Windows installer
  if(MSVC)
    # List all the libraries that the S-Function must link to.
    set(WINDOWS_LINK_LIBRARIES "-l${CMAKE_PROJECT_NAME}")
    foreach(LINK_LIB ${JSBSIM_WINDOWS_LINK_LIBRARIES})
      string(APPEND WINDOWS_LINK_LIBRARIES " ${LINK_LIB}.lib")
    endforeach(LINK_LIB)

    # Convert the source files list to a string that can be used by the script.
    foreach(SOURCE_FILE ${SOURCES})
      string(APPEND SOURCE_FILES_LIST " ${SOURCE_FILE}")
    endforeach(SOURCE_FILE)

    if(NOT BUILD_SHARED_LIBS)
      set(JSBSIM_COMPFLAGS "/DJSBSIM_STATIC_LINK")
    endif(NOT BUILD_SHARED_LIBS)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/WindowsSimulinkCompile.m.in ${CMAKE_CURRENT_BINARY_DIR}/JSBSimSimulinkCompile.m)
  endif(MSVC)
endif(Matlab_FOUND)
