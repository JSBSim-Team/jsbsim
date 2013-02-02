set(JSBSIM_SIMGEAR_MAGVAR_SRC coremag.cxx)

set(JSBSIM_SIMGEAR_MAGVAR_HDR coremag.hxx)

add_library(coremag ${JSBSIM_SIMGEAR_MAGVAR_HDR} ${JSBSIM_SIMGEAR_MAGVAR_SRC})
set_target_properties (coremag PROPERTIES
                               VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (coremag PROPERTIES
                                 SOVERSION ${LIBRARY_SOVERSION}
                                 FRAMEWORK ON)
endif()

install(TARGETS coremag LIBRARY DESTINATION lib
                        ARCHIVE DESTINATION lib
                        RUNTIME DESTINATION lib
                        # For Mac
                        FRAMEWORK DESTINATION "/Library/Frameworks")

