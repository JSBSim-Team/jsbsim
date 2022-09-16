set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp
            FGLinearization.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h
            FGLinearization.h)

add_library(Init OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Init PROPERTIES TARGET_DIRECTORY
                           ${CMAKE_CURRENT_SOURCE_DIR})
add_coverage(Init)

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization
                         COMPONENT devel)
