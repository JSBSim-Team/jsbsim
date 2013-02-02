set(JSBSIM_MODELS_ATMOSPHERE_SRC FGMSIS.cpp
                                 FGMSISData.cpp
                                 FGMars.cpp
                                 FGStandardAtmosphere.cpp
                                 FGWinds.cpp)

set(JSBSIM_MODELS_ATMOSPHERE_HDR FGMSIS.h
                                 FGMars.h
                                 FGStandardAtmosphere.h
                                 FGWinds.h)

add_library(Atmosphere ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_MODELS_ATMOSPHERE_SRC})

install(TARGETS Atmosphere LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib)
install(FILES ${JSBSIM_MODELS_ATMOSPHERE_HDR} DESTINATION include/JSBSim/models/atmosphere)

