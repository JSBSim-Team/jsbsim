set(SOURCES FGDeadBand.cpp
            FGFCSComponent.cpp
            FGFilter.cpp
            FGGain.cpp
            FGKinemat.cpp
            FGSummer.cpp
            FGSwitch.cpp
            FGFCSFunction.cpp
            FGSensor.cpp
            FGPID.cpp
            FGActuator.cpp
            FGAccelerometer.cpp
            FGGyro.cpp
            FGMagnetometer.cpp
            FGAngles.cpp
            FGWaypoint.cpp
            FGDistributor.cpp
            FGLinearActuator.cpp)

set(HEADERS FGDeadBand.h
            FGFCSComponent.h
            FGFilter.h
            FGGain.h
            FGKinemat.h
            FGSummer.h
            FGSwitch.h
            FGFCSFunction.h
            FGSensor.h
            FGPID.h
            FGActuator.h
            FGAccelerometer.h
            FGGyro.h
            FGMagnetometer.h
            FGSensorOrientation.h
            FGAngles.h
            FGWaypoint.h
            FGDistributor.h
            FGLinearActuator.h)

add_library(FlightControl OBJECT ${HEADERS} ${SOURCES})
set_target_properties(FlightControl PROPERTIES TARGET_DIRECTORY
                                    ${CMAKE_CURRENT_SOURCE_DIR})
add_coverage(FlightControl)

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/flight_control
                         COMPONENT devel)
