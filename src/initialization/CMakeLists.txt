set(JSBSIM_INITIALISATION_SRC FGInitialCondition.cpp
                              FGTrim.cpp
                              FGTrimAxis.cpp
                              FGSimplexTrim.cpp
                              FGTrimmer.cpp
                              FGLinearization.cpp)

set(JSBSIM_INITIALISATION_HDR FGInitialCondition.h
                              FGTrim.h
                              FGTrimAxis.h
                              FGSimplexTrim.h
                              FGTrimmer.h
                              FGLinearization.h)

add_library(Init ${JSBSIM_INITIALISATION_HDR} ${JSBSIM_INITIALISATION_SRC})
set_target_properties (Init PROPERTIES
                            VERSION ${LIBRARY_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties (Init PROPERTIES
                              SOVERSION ${LIBRARY_SOVERSION}
                              FRAMEWORK ON)
endif()

install(TARGETS Init LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib
                     RUNTIME DESTINATION lib
                     # For Mac
                     FRAMEWORK DESTINATION "/Library/Frameworks")
install(FILES ${JSBSIM_INITIALISATION_HDR} DESTINATION include/JSBSim/initialization)

