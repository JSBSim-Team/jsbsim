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
            FGDistributor.cpp)

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
            FGDistributor.h)

add_full_path_name(FLIGHT_CONTROL_SRC "${SOURCES}")
add_full_path_name(FLIGHT_CONTROL_HDR "${HEADERS}")

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/flight_control)

