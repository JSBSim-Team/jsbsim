set(JSBSIM_MODELS_PROPULSION_SRC FGElectric.cpp
                                 FGEngine.cpp
                                 FGForce.cpp
                                 FGNozzle.cpp
                                 FGPiston.cpp
                                 FGPropeller.cpp
                                 FGRocket.cpp
                                 FGTank.cpp
                                 FGThruster.cpp
                                 FGTurbine.cpp
                                 FGTurboProp.cpp
                                 FGTransmission.cpp
                                 FGRotor.cpp)

set(JSBSIM_MODELS_PROPULSION_HDR FGElectric.h
                                 FGEngine.h
                                 FGForce.h
                                 FGNozzle.h
                                 FGPiston.h
                                 FGPropeller.h
                                 FGRocket.h
                                 FGTank.h
                                 FGThruster.h
                                 FGTurbine.h
                                 FGTurboProp.h
                                 FGTransmission.h
                                 FGRotor.h)

add_library(Propulsion ${JSBSIM_MODELS_PROPULSION_HDR}
                       ${JSBSIM_MODELS_PROPULSION_SRC})
set_target_properties (Propulsion PROPERTIES
                                  VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (Propulsion PROPERTIES
                                    SOVERSION ${LIBRARY_SOVERSION}
                                    FRAMEWORK ON)
endif()

install(TARGETS Propulsion LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib
                           RUNTIME DESTINATION lib
                           # For Mac
                           FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_MODELS_PROPULSION_HDR} DESTINATION include/JSBSim/models/propulsion)

