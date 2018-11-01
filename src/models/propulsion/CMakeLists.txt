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

add_library(Propulsion OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/propulsion COMPONENT devel)
