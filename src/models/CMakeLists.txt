add_subdirectory(atmosphere)
add_subdirectory(propulsion)
add_subdirectory(flight_control)

set(SOURCES FGAerodynamics.cpp
            FGAircraft.cpp
            FGAtmosphere.cpp
            FGAuxiliary.cpp
            FGFCS.cpp
            FGSurface.cpp
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

set(HEADERS FGAerodynamics.h
            FGAircraft.h
            FGAtmosphere.h
            FGAuxiliary.h
            FGFCS.h
            FGSurface.h
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

add_full_path_name(MODELS_SRC "${SOURCES}")
add_full_path_name(MODELS_HDR "${HEADERS}")

propagate_source_files(MODELS ATMOSPHERE)
propagate_source_files(MODELS FLIGHT_CONTROL)
propagate_source_files(MODELS PROPULSION)

install(FILES ${HEADERS} DESTINATION include/JSBSim/models)

