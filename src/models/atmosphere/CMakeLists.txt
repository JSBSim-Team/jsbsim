set(SOURCES FGMSIS.cpp
            FGStandardAtmosphere.cpp
            FGWinds.cpp
            MSIS/nrlmsise-00.c
            MSIS/nrlmsise-00_data.c)

set(HEADERS FGMSIS.h
            FGStandardAtmosphere.h
            FGWinds.h
            MSIS/nrlmsise-00.h)

add_library(Atmosphere OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Atmosphere PROPERTIES TARGET_DIRECTORY
                                 ${CMAKE_CURRENT_SOURCE_DIR})
add_coverage(Atmosphere)

install(FILES ${HEADERS} DESTINATION include/JSBSim/models/atmosphere
                         COMPONENT devel)
