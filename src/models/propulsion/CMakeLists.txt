set(SOURCES FGElectric.cpp
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

set(HEADERS FGElectric.h
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

add_full_path_name(PROPULSION_SRC "${SOURCES}")
add_full_path_name(PROPULSION_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/propulsion)

