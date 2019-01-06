set(SOURCES FGInitialCondition.cpp
            FGTrim.cpp
            FGTrimAxis.cpp)

set(HEADERS FGInitialCondition.h
            FGTrim.h
            FGTrimAxis.h)

add_library(Init OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Init PROPERTIES TARGET_DIRECTORY
                           ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION include/JSBSim/initialization
                         COMPONENT devel)
