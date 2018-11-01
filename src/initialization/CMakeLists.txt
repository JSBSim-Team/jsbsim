set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h)

add_library(Init OBJECT ${HEADERS} ${SOURCES})

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization COMPONENT devel)
