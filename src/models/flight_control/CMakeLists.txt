set(JSBSIM_MODELS_FLIGHT_CONTROL_SRC FGDeadBand.cpp
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
                                     FGMagnetometer.cpp)

set(JSBSIM_MODELS_FLIGHT_CONTROL_HDR FGDeadBand.h
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
                                     FGSensorOrientation.h)

add_library(FlightControl ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_MODELS_FLIGHT_CONTROL_SRC})

install(TARGETS FlightControl LIBRARY DESTINATION lib
                              ARCHIVE DESTINATION lib)

