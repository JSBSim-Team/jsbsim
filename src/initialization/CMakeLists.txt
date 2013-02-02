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

install(TARGETS Init LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
install(FILES ${JSBSIM_INITIALISATION_HDR} DESTINATION include/JSBSim/initialization)

