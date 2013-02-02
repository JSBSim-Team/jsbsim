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

add_library(Models ${JSBSIM_MODELS_HDR} ${JSBSIM_MODELS_SRC})
set_target_properties (Models PROPERTIES
                              VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (Models PROPERTIES
                                SOVERSION ${LIBRARY_SOVERSION}
                                FRAMEWORK ON)
endif()

install(TARGETS Models LIBRARY DESTINATION lib
                       ARCHIVE DESTINATION lib
                       RUNTIME DESTINATION lib
                       # For Mac
                       FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_MODELS_HDR} DESTINATION include/JSBSim/models)

