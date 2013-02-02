set(JSBSIM_MODELS_ATMOSPHERE_SRC FGMSIS.cpp
                                 FGMSISData.cpp
                                 FGMars.cpp
                                 FGStandardAtmosphere.cpp
                                 FGWinds.cpp)

set(JSBSIM_MODELS_ATMOSPHERE_HDR FGMSIS.h
                                 FGMars.h
                                 FGStandardAtmosphere.h
                                 FGWinds.h)

add_library(Atmosphere ${JSBSIM_MODELS_ATMOSPHERE_HDR}
                       ${JSBSIM_MODELS_ATMOSPHERE_SRC})
set_target_properties (Atmosphere PROPERTIES
                                  VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (Atmosphere PROPERTIES
                                    SOVERSION ${LIBRARY_SOVERSION}
                                    FRAMEWORK ON)
endif()

install(TARGETS Atmosphere LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib
                           RUNTIME DESTINATION lib
                           # For Mac
                           FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_MODELS_ATMOSPHERE_HDR} DESTINATION include/JSBSim/models/atmosphere)

