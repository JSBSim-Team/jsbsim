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

install(TARGETS Propulsion LIBRARY DESTINATION lib
                           ARCHIVE DESTINATION lib)
install(FILES ${JSBSIM_MODELS_PROPULSION_HDR} DESTINATION include/JSBSim/models/propulsion)

