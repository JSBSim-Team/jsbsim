add_subdirectory(atmosphere)
add_subdirectory(propulsion)
add_subdirectory(flight_control)

set(JSBSIM_MODELS_SRC FGAerodynamics.cpp
                      FGAircraft.cpp
                      FGAtmosphere.cpp
                      FGAuxiliary.cpp
                      FGFCS.cpp
                      FGGroundReactions.cpp
                      FGInertial.cpp
                      FGLGear.cpp
                      FGMassBalance.cpp
                      FGModel.cpp
                      FGOutput.cpp
                      FGPropagate.cpp
                      FGPropulsion.cpp
                      FGInput.cpp
                      FGExternalReactions.cpp
                      FGExternalForce.cpp
                      FGBuoyantForces.cpp
                      FGGasCell.cpp
                      FGAccelerations.cpp)

set(JSBSIM_MODELS_HDR FGAerodynamics.h
                      FGAircraft.h
                      FGAtmosphere.h
                      FGAuxiliary.h
                      FGFCS.h
                      FGGroundReactions.h
                      FGInertial.h
                      FGLGear.h
                      FGMassBalance.h
                      FGModel.h
                      FGOutput.h
                      FGPropagate.h
                      FGPropulsion.h
                      FGInput.h
                      FGExternalReactions.h
                      FGExternalForce.h
                      FGBuoyantForces.h
                      FGGasCell.h
                      FGAccelerations.h
                      FGFCSChannel.h)

add_library(Models ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_MODELS_SRC})

install(TARGETS Models LIBRARY DESTINATION lib
                       ARCHIVE DESTINATION lib)
install(FILES ${JSBSIM_MODELS_HDR} DESTINATION include/JSBSim/models)

