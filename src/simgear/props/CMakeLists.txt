set(JSBSIM_SIMGEAR_PROPS_SRC props.cxx)

set(JSBSIM_SIMGEAR_PROPS_HDR props.hxx)

add_library(Properties ${JSBSIM_SIMGEAR_PROPS_HDR} ${JSBSIM_SIMGEAR_PROPS_SRC})
set_target_properties (Properties PROPERTIES
                                  VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (Properties PROPERTIES
                                    SOVERSION ${LIBRARY_SOVERSION}
                                    FRAMEWORK ON)
endif()

install(TARGETS Properties LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib
                           RUNTIME DESTINATION lib
                           # For Mac
                           FRAMEWORK DESTINATION "/Library/Frameworks")

