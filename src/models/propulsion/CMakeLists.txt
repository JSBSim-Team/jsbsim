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
            FGRotor.cpp
            FGBrushLessDCMotor.cpp)

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
            FGRotor.h
            FGBrushLessDCMotor.h)

add_library(Propulsion OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Propulsion PROPERTIES TARGET_DIRECTORY
                                 ${CMAKE_CURRENT_SOURCE_DIR})
add_coverage(Propulsion)

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/propulsion
                         COMPONENT devel)
