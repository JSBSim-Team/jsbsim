set(JSBSIM_INITIALISATION_SRC FGInitialCondition.cpp
                              FGTrim.cpp
                              FGTrimAxis.cpp
                              FGSimplexTrim.cpp
                              FGTrimmer.cpp)

set(JSBSIM_INITIALISATION_HDR FGInitialCondition.h
                              FGTrim.h
                              FGTrimAxis.h
                              FGSimplexTrim.h
                              FGTrimmer.h)

add_library(Init ${JSBSIM_STATIC_LIBS_TYPE} ${JSBSIM_INITIALISATION_SRC})

install(TARGETS Init LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)

