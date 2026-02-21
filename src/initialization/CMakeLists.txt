set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp
            FGLinearization.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h
            FGLinearization.h)

add_library(Init OBJECT ${SOURCES})

if(CXXTEST_FOUND)
    add_coverage(Init)
endif(CXXTEST_FOUND)

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization
                         COMPONENT devel)
