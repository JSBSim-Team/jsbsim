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
                                     FGMagnetometer.cpp
                                     FGAngles.cpp
                                     FGWaypoint.cpp
                                     FGDistributor.cpp)

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
                                     FGSensorOrientation.h
                                     FGAngles.h
                                     FGWaypoint.h
                                     FGDistributor.h)

add_library(FlightControl ${JSBSIM_MODELS_FLIGHT_CONTROL_HDR}
                          ${JSBSIM_MODELS_FLIGHT_CONTROL_SRC})
set_target_properties (FlightControl PROPERTIES
                                     VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (FlightControl PROPERTIES
                                       SOVERSION ${LIBRARY_SOVERSION}
                                       FRAMEWORK ON)
endif()

install(TARGETS FlightControl LIBRARY DESTINATION lib
                              ARCHIVE DESTINATION lib
                              RUNTIME DESTINATION lib
                              # For Mac
                              FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_MODELS_FLIGHT_CONTROL_HDR} DESTINATION include/JSBSim/models/flight_control)

